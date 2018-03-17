
#ifndef COMMON_LINUX_HTTP_UPLOAD_H__
#define COMMON_LINUX_HTTP_UPLOAD_H__

#include <map>
#include <string>

class HTTPUpload
{
public:
    // Sends the given set of parameters, along with the contents of
    // upload_file, as a multipart POST request to the given URL.
    // file_part_name contains the name of the file part of the request
    // (i.e. it corresponds to the name= attribute on an <input type="file">.
    // Parameter names must contain only printable ASCII characters,
    // and may not contain a quote (") character.
    // Only HTTP(S) URLs are currently supported.  Returns true on success.
    // If the request is successful and response_body is non-NULL,
    // the response body will be returned in response_body.
    // If response_code is non-NULL, it will be set to the HTTP response code
    // received (or 0 if the request failed before getting an HTTP response).
    // If the send fails, a description of the error will be
    // returned in error_description.
    static bool SendRequest(const std::string& url,
                            const std::map<std::string, std::string>& parameters,
                            const std::string& upload_file,
                            const std::string& file_part_name,
                            const std::string& proxy,
                            const std::string& proxy_user_pwd,
                            const std::string& ca_certificate_file,
                            std::string* response_body,
                            long* response_code,
                            std::string* error_description);

private:
    // Checks that the given list of parameters has only printable
    // ASCII characters in the parameter name, and does not contain
    // any quote (") characters.  Returns true if so.
    static bool CheckParameters(const std::map<std::string, std::string>&
                                parameters);

    // No instances of this class should be created.
    // Disallow all constructors, destructors, and operator=.
    HTTPUpload();
    explicit HTTPUpload(const HTTPUpload&);
    void operator=(const HTTPUpload&);
    ~HTTPUpload();
};

#endif  // COMMON_LINUX_HTTP_UPLOAD_H__
