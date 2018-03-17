
#ifndef COMMON_LINUX_LIBCURL_WRAPPER_H_
#define COMMON_LINUX_LIBCURL_WRAPPER_H_

#include <string>
#include <map>
#include "curl/curl.h"

class LibCurlWrapper
{
public:
    LibCurlWrapper();
    ~LibCurlWrapper();

    virtual bool Init();
    virtual bool SetProxy(const std::string& proxy_host,
                          const std::string& proxy_userpwd);
    virtual bool AddFile(const std::string& upload_file_path,
                         const std::string& basename);
    virtual bool SendRequest(const std::string& url,
                             const std::map<std::string, std::string>& parameters,
                             int* http_status_code,
                             std::string* http_header_data,
                             std::string* http_response_data);
private:
    // This function initializes class state corresponding to function
    // pointers into the CURL library.
    bool SetFunctionPointers();

    bool init_ok_;                 // Whether init succeeded
    void* curl_lib_;               // Pointer to result of dlopen() on
    // curl library
    std::string last_curl_error_;  // The text of the last error when
    // dealing
    // with CURL.

    CURL* curl_;                   // Pointer for handle for CURL calls.

    CURL* (*easy_init_)(void);

    // Stateful pointers for calling into curl_formadd()
    struct curl_httppost* formpost_;
    struct curl_httppost* lastptr_;
    struct curl_slist* headerlist_;

    // Function pointers into CURL library
    CURLcode (*easy_setopt_)(CURL*, CURLoption, ...);
    CURLFORMcode (*formadd_)(struct curl_httppost**,
                             struct curl_httppost**, ...);
    struct curl_slist* (*slist_append_)(struct curl_slist*, const char*);
    void (*slist_free_all_)(struct curl_slist*);
    CURLcode (*easy_perform_)(CURL*);
    const char* (*easy_strerror_)(CURLcode);
    void (*easy_cleanup_)(CURL*);
    CURLcode (*easy_getinfo_)(CURL*, CURLINFO info, ...);
    void (*formfree_)(struct curl_httppost*);

};

#endif  // COMMON_LINUX_LIBCURL_WRAPPER_H_
