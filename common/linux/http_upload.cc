
#include "linux/http_upload.h"
#include <assert.h>
#include <dlfcn.h>
#include "curl/curl.h"

namespace {

// Callback to get the response data from server.
static size_t WriteCallback(void* ptr, size_t size,
                            size_t nmemb, void* userp)
{
    if (!userp)
        return 0;

    std::string* response = reinterpret_cast<std::string*>(userp);
    size_t real_size = size * nmemb;
    response->append(reinterpret_cast<char*>(ptr), real_size);
    return real_size;
}

}  // namespace

static const char kUserAgent[] = "Breakpad/1.0 (Linux)";

// static
bool HTTPUpload::SendRequest(const std::string& url,
                             const std::map<std::string, std::string>& parameters,
                             const std::string& upload_file,
                             const std::string& file_part_name,
                             const std::string& proxy,
                             const std::string& proxy_user_pwd,
                             const std::string& ca_certificate_file,
                             std::string* response_body,
                             long* response_code,
                             std::string* error_description)
{
    if (response_code != NULL)
        *response_code = 0;

    if (!CheckParameters(parameters))
        return false;

    // We may have been linked statically; if curl_easy_init is in the
    // current binary, no need to search for a dynamic version.
    void* curl_lib = dlopen(NULL, RTLD_NOW);
    if (!curl_lib || dlsym(curl_lib, "curl_easy_init") == NULL)
    {
        dlerror();  // Clear dlerror before attempting to open libraries.
        dlclose(curl_lib);
        curl_lib = NULL;
    }
    if (!curl_lib)
    {
        curl_lib = dlopen("libcurl.so", RTLD_NOW);
    }
    if (!curl_lib)
    {
        if (error_description != NULL)
            *error_description = dlerror();
        curl_lib = dlopen("libcurl.so.4", RTLD_NOW);
    }
    if (!curl_lib)
    {
        // Debian gives libcurl a different name when it is built against GnuTLS
        // instead of OpenSSL.
        curl_lib = dlopen("libcurl-gnutls.so.4", RTLD_NOW);
    }
    if (!curl_lib)
    {
        curl_lib = dlopen("libcurl.so.3", RTLD_NOW);
    }
    if (!curl_lib)
    {
        return false;
    }

    CURL* (*curl_easy_init)(void);
    *(void**) (&curl_easy_init) = dlsym(curl_lib, "curl_easy_init");
    CURL* curl = (*curl_easy_init)();
    if (error_description != NULL)
        *error_description = "No Error";

    if (!curl)
    {
        dlclose(curl_lib);
        return false;
    }

    CURLcode err_code = CURLE_OK;
    CURLcode (*curl_easy_setopt)(CURL*, CURLoption, ...);
    *(void**) (&curl_easy_setopt) = dlsym(curl_lib, "curl_easy_setopt");
    (*curl_easy_setopt)(curl, CURLOPT_URL, url.c_str());
    (*curl_easy_setopt)(curl, CURLOPT_USERAGENT, kUserAgent);
    // Set proxy information if necessary.
    if (!proxy.empty())
        (*curl_easy_setopt)(curl, CURLOPT_PROXY, proxy.c_str());
    if (!proxy_user_pwd.empty())
        (*curl_easy_setopt)(curl, CURLOPT_PROXYUSERPWD, proxy_user_pwd.c_str());

    if (!ca_certificate_file.empty())
        (*curl_easy_setopt)(curl, CURLOPT_CAINFO, ca_certificate_file.c_str());

    struct curl_httppost* formpost = NULL;
    struct curl_httppost* lastptr = NULL;
    // Add form data.
    CURLFORMcode (*curl_formadd)(struct curl_httppost**, struct curl_httppost**,
                                 ...);
    *(void**) (&curl_formadd) = dlsym(curl_lib, "curl_formadd");
    std::map<std::string, std::string>::const_iterator iter = parameters.begin();
    for (; iter != parameters.end(); ++iter)
        (*curl_formadd)(&formpost, &lastptr,
                        CURLFORM_COPYNAME, iter->first.c_str(),
                        CURLFORM_COPYCONTENTS, iter->second.c_str(),
                        CURLFORM_END);

    // Add form file.
    (*curl_formadd)(&formpost, &lastptr,
                    CURLFORM_COPYNAME, file_part_name.c_str(),
                    CURLFORM_FILE, upload_file.c_str(),
                    CURLFORM_END);

    (*curl_easy_setopt)(curl, CURLOPT_HTTPPOST, formpost);

    // Disable 100-continue header.
    struct curl_slist* headerlist = NULL;
    char buf[] = "Expect:";
    struct curl_slist* (*curl_slist_append)(struct curl_slist*, const char*);
    *(void**) (&curl_slist_append) = dlsym(curl_lib, "curl_slist_append");
    headerlist = (*curl_slist_append)(headerlist, buf);
    (*curl_easy_setopt)(curl, CURLOPT_HTTPHEADER, headerlist);

    if (response_body != NULL)
    {
        (*curl_easy_setopt)(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        (*curl_easy_setopt)(curl, CURLOPT_WRITEDATA,
                            reinterpret_cast<void*>(response_body));
    }

    // Fail if 400+ is returned from the web server.
    (*curl_easy_setopt)(curl, CURLOPT_FAILONERROR, 1);

    CURLcode (*curl_easy_perform)(CURL*);
    *(void**) (&curl_easy_perform) = dlsym(curl_lib, "curl_easy_perform");
    err_code = (*curl_easy_perform)(curl);
    if (response_code != NULL)
    {
        CURLcode (*curl_easy_getinfo)(CURL*, CURLINFO, ...);
        *(void**) (&curl_easy_getinfo) = dlsym(curl_lib, "curl_easy_getinfo");
        (*curl_easy_getinfo)(curl, CURLINFO_RESPONSE_CODE, response_code);
    }
    const char* (*curl_easy_strerror)(CURLcode);
    *(void**) (&curl_easy_strerror) = dlsym(curl_lib, "curl_easy_strerror");
#ifndef NDEBUG
    if (err_code != CURLE_OK)
        fprintf(stderr, "Failed to send http request to %s, error: %s\n",
                url.c_str(),
                (*curl_easy_strerror)(err_code));
#endif
    if (error_description != NULL)
        *error_description = (*curl_easy_strerror)(err_code);

    void (*curl_easy_cleanup)(CURL*);
    *(void**) (&curl_easy_cleanup) = dlsym(curl_lib, "curl_easy_cleanup");
    (*curl_easy_cleanup)(curl);
    if (formpost != NULL)
    {
        void (*curl_formfree)(struct curl_httppost*);
        *(void**) (&curl_formfree) = dlsym(curl_lib, "curl_formfree");
        (*curl_formfree)(formpost);
    }
    if (headerlist != NULL)
    {
        void (*curl_slist_free_all)(struct curl_slist*);
        *(void**) (&curl_slist_free_all) = dlsym(curl_lib, "curl_slist_free_all");
        (*curl_slist_free_all)(headerlist);
    }
    dlclose(curl_lib);
    return err_code == CURLE_OK;
}

// static
bool HTTPUpload::CheckParameters(const std::map<std::string, std::string>&
                                 parameters)
{
    for (std::map<std::string, std::string>::const_iterator pos =
             parameters.begin();
         pos != parameters.end(); ++pos)
    {
        const std::string& str = pos->first;
        if (str.size() == 0)
            return false;  // disallow empty parameter names
        for (unsigned int i = 0; i < str.size(); ++i)
        {
            int c = str[i];
            if (c < 32 || c == '"' || c > 127)
            {
                return false;
            }
        }
    }
    return true;
}

