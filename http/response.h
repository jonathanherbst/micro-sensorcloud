#ifndef HTTP_PARSING
#define HTTP_PARSING

#include "error.h"

#include <buffer/buffer.h>

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	httpEncoding_unknown,
	httpEncoding_standard,
	httpEncoding_chunked
} HTTPTransferEncoding;

typedef struct
{
    Buffer head;

    uint8_t headComplete;
    uint8_t bodyComplete;
    HTTPTransferEncoding transferEncoding;
    char parserData[8 + 2];
    Buffer parserBuffer;
    size_t dataLeft;
} HTTPResponse;

typedef enum
{
	httpResponse_ok=200,
	httpResponse_created=201,

	httpResponse_badRequest=400,
	httpResponse_unauthorized=401,
	httpResponse_notFound=404,
} HTTPResponseCode;

/**
 * Initialize for parsing a new response.
 * @param[out]  response        Initialized HTTPResponse.
 * @param[in]   headBuffer      Buffer to store data for the response head.
 */
void http_initResponse(HTTPResponse* response, Buffer headBuffer);

/**
 * Write header data from a buffer to the specified request.
 * @param[in]   request     Request to write the buffer into.
 * @param[out]  unconsumed  A pointer to the buffer where the header ends.
 * @param[in]   buffer      Buffer to read from.
 * @param[in]   bufferSize  Size of the buffer in bytes.
 * @return http_ok if response parsing was successful, http_complete if response parsing complete, another error otherwise.
 */
HTTPError http_parseHead(HTTPResponse* response, const void** unconsumed, const void* data, size_t dataSize);

/**
 * Parse http response body data.
 * @param[in]   response        Response to parse.
 * @param[out]  unconsumed      A pointer to the buffer where parsing stopped.
 * @param[out]  bodyData        Buffer of body data.
 * @param[out]  bodyDataSize    Number of bytes of parsed body data.
 * @param[in]   data            Data to parse.
 * @param[in]   dataSize        Number of bytes to parse.
 * @return http_ok if parsing is successful, http_complete if the body is finished, another error otherwise.
 */
HTTPError http_parseBody(HTTPResponse* response, const void** unconsumed, const void** bodyData, size_t* bodyDataSize,
    const void* data, size_t dataSize);

/**
 * Get the code of the response.
 * @note Returns an error until the headers of the response have been received.
 * @param[out]  code        Response code of the response is populated here.
 * @param[out]  reason      A non terminated response code reason.
 * @param[out]  reasonSize  Number of characters in the reason.
 * @param[in]   response    Response with completed header.
 */
HTTPError httpResponse_getCode(HTTPResponseCode* code, const char** reason, size_t* reasonSize,
    const HTTPResponse* response);

/**
 * Get a header from the response.
 * @note Returns an error until the headers of the resonse have been received.
 * @param[out]  value       Value of the header.
 * @param[out]  size        Number of characters in the header value.
 * @param[in]   response    Response with completed header.
 * @param[in]   name        Name of the header to get.
 */
HTTPError httpResponse_getHeader(const char** value, size_t* size, const HTTPResponse* response, const char* name);


#ifdef __cplusplus
}
#endif

#endif
