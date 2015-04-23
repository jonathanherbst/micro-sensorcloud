#ifndef HTTP_REQUEST
#define HTTP_REQUEST

#ifdef __cplusplus
extern "C" {
#endif

#include "error.h"
#include "response.h"

#include <net/driver.h>

#include <buffer/buffer.h>

#include <string.h>

typedef void(*HTTPRequestCallback)(void*, const void*, size_t, HTTPError);

struct HTTPRequestData
{
	Buffer head;
    Buffer body;
    HTTPResponse response;
	HTTPRequestCallback callback;
    void* userData;
    uint8_t headSent;
    HTTPError error;

    struct 
    {
        NetConnection driver;
        uint8_t connected;
        uint8_t secure;
        char hostname[256];
    } connection;
};
typedef struct HTTPRequestData HTTPRequest;

/**
 * Initialize an HTTPRequest.
 * @param[io]   request         Request to initialize.
 * @param[in]   method          HTTPMethod to use.
 * @param[in]   url             An http url to send the request for.
 * @param[in]   requestBuffer   A buffer to use for the request headers.
 * @param[in]   responseBuffer  A buffer to use for the response headers.
 * @param[in]   userData        User data to be passed in the callback.
 * @param[in]   callback        Function to be called when the request is complete.
 * @return 
 */
HTTPError http_initRequest(HTTPRequest* request, const char* method, const char* url, Buffer requestBuffer,
    Buffer responseBuffer, void* userData, HTTPRequestCallback callback);

/**
 * Add a header to the request.
 * @param[io]   request Request to add the header to.
 * @param[in]   name    The name of the header.
 * @param[in]   value   The value of the header.
 * @note Header uniqueness is not garanteed.
 */
HTTPError http_addRequestHeader(HTTPRequest* request, const char* name, const char* value);

/**
 * Start a request asynchronously.
 * @note Further calls to http_addRequestHeader will return an error.
 * @param[io]   request     Request to make.
 * @param[in]   body        A buffer containing the body of the request.
 * @param[in]   callback    Called when the request is complete or encounters an error.
 */
HTTPError http_asyncRequest(HTTPRequest* request, Buffer body);

/**
 * Get the code of the response.
 * @note Returns an error until the headers of the response have been received.
 * @param[out]  code        Response code of the response is populated here.
 * @param[out]  reason      A non terminated response code reason.
 * @param[out]  reasonSize  Number of characters in the reason.
 * @param[in]   response    Response with completed header.
 * @return http_ok if the code was found, http_bufferOverrun otherwise.
 */
static inline HTTPError http_getResponseCode(HTTPResponseCode* code, const char** reason, size_t* reasonSize,
    const HTTPRequest* request)
{
    return httpResponse_getCode(code, reason, reasonSize, &request->response);
}

/**
 * Get a header from the response.
 * @note Returns an error until the headers of the resonse have been received.
 * @param[out]  value       Value of the header.
 * @param[out]  size        Number of characters in the header value.
 * @param[in]   response    Response with completed header.
 * @param[in]   name        Name of the header to get.
 * @return http_ok if the header was found, http_headerDoesntExist otherwise.
 */
static inline HTTPError http_getResponseHeader(const char** value, size_t* size, const HTTPRequest* request,
    const char* name)
{
    return httpResponse_getHeader(value, size, &request->response, name);
}

#ifdef __cplusplus
}
#endif

#endif
