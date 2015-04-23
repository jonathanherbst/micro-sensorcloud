#ifndef HTTP_ERROR
#define HTTP_ERROR

#include <buffer/buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
	http_ok,
	http_complete,
	http_bufferOverrun,
	http_invalidURL,
	http_headerDoesntExist,
	http_unrecognizedEncoding,
    http_unableToResolveHostname,
	http_error
} HTTPError;

static inline HTTPError http_bufferError(BufferError e)
{
    switch(e)
    {
    case buffer_overrun:
        return http_bufferOverrun;
    default:
        return http_ok;
    }
}

#ifdef __cplusplus
}
#endif

#endif
