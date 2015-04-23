#include "request.h"

#include <buffer/buffer.h>
#include <net/driver.h>
#include <detail/algorithm.h>

#include <string.h>

const char httpVersion[] = "HTTP/1.1";
const size_t httpVersionSize = sizeof(httpVersion) - 1;

void http_connectCallback(void* request, NetError error);
void http_writeCallback(void* request, NetError error);
void http_receiveCallback(void* request, const void* buffer, size_t bufferSize, NetError error);
void http_disconnectCallback(void* connection, NetError error);

static const size_t http_eolSize = 2;

HTTPError ICACHE_FLASH_ATTR http_writeEol(Buffer* buffer)
{
    return http_bufferError(buffer_write(buffer, "\r\n", http_eolSize));
}

HTTPError ICACHE_FLASH_ATTR http_parseUrl(HTTPRequest* request, const char* url, const char** path)
{
	size_t urlLen = strlen(url);

	// get the protocol
	const char* endProto = strchr(url, ':');
	if(!endProto)
		return http_invalidURL;
	size_t protoLen = endProto - url;

	// get the domain
	if(protoLen + 3 > urlLen)
		return http_invalidURL;
	const char* startDomain = endProto + 3;
	const char* endDomain = strchr(startDomain, '/');
	if(!endDomain)
		return http_invalidURL;
	size_t domainLen = endDomain - startDomain;

	// setup the connection
	if(endProto - url == 5) // https
		request->connection.secure = 1;
	else
		request->connection.secure = 0;
	memcpy(request->connection.hostname, startDomain, domainLen);
	request->connection.hostname[domainLen] = '\0';
	*path = endDomain;
	return http_ok;
}

HTTPError ICACHE_FLASH_ATTR http_initRequest(HTTPRequest* request, const char* method, const char* url, Buffer requestBuffer,
    Buffer responseBuffer, void* userData, HTTPRequestCallback callback)
{
    // initialize the request variables
    request->head = requestBuffer;
    request->callback = callback;
    request->userData = userData;
    request->headSent = 0;
    
    http_initResponse(&request->response, responseBuffer);

    net_init(&request->connection.driver, request, http_connectCallback, http_receiveCallback, http_writeCallback,
        http_disconnectCallback);
    request->connection.connected = 0;
    request->connection.secure = 0;

    // parse the url
	const char* path;
	HTTPError r = http_parseUrl(request, url, &path);
	if(r != http_ok)
		return r;
    
    // write the request line to the head buffer
	size_t methodSize = strlen(method);
	size_t pathSize = strlen(path);
	size_t writeSize = methodSize + pathSize + httpVersionSize + 2 + http_eolSize;
	if(writeSize > buffer_bytesAvailable(&request->head))
		return http_bufferOverrun;
	char* writePtr = request->head.putPtr;
	memcpy(writePtr, method, methodSize); writePtr += methodSize;
	*writePtr++ = ' ';
	memcpy(writePtr, path, pathSize); writePtr += pathSize;
	*writePtr++ = ' ';
	memcpy(writePtr, httpVersion, httpVersionSize); writePtr += httpVersionSize;
    buffer_commit(&request->head, writeSize - http_eolSize);
	http_writeEol(&request->head);

    // write the host header to the head buffer
    return http_addRequestHeader(request, "Host", request->connection.hostname);
}

HTTPError ICACHE_FLASH_ATTR http_addRequestHeader(HTTPRequest* request, const char* name, const char* value)
{
	size_t nameSize = strlen(name);
	size_t valueSize = strlen(value);
	size_t writeSize = nameSize + valueSize + 2 + http_eolSize;
	if(writeSize > buffer_bytesAvailable(&request->head))
		return http_bufferOverrun;

	char* writePtr = request->head.putPtr;
	memcpy(writePtr, name, nameSize); writePtr += nameSize;
	*writePtr++ = ':';
	*writePtr++ = ' ';
	memcpy(writePtr, value, valueSize); writePtr += valueSize;
    buffer_commit(&request->head, writeSize - http_eolSize);
    http_writeEol(&request->head);
    return http_ok;
}

HTTPError ICACHE_FLASH_ATTR http_asyncRequest(HTTPRequest* request, Buffer requestBody)
{
    // finish off the headers
    char requestSize[11];
    ets_sprintf(requestSize, "%u", buffer_size(&requestBody));
    HTTPError e = http_addRequestHeader(request, "Content-Length", requestSize);
    if(e != http_ok)
        return e;
    e = http_writeEol(&request->head);
    if(e != http_ok)
        return e;
    request->body = requestBody;
    
    if(request->connection.secure)
        net_asyncSecureConnect(&request->connection.driver, request->connection.hostname);
    else
        net_asyncConnect(&request->connection.driver, request->connection.hostname);

    return http_ok;
}

HTTPError ICACHE_FLASH_ATTR http_netError(NetError e)
{
    switch(e)
    {
    case net_error:
        return http_error;
    default:
        return http_ok;
    }
}

void ICACHE_FLASH_ATTR http_connectCallback(void* request, NetError error)
{
	HTTPRequest* r = (HTTPRequest*)request;
	if(error != net_ok)
		r->callback(r->userData, NULL, 0, http_netError(error));

	r->connection.connected = 1;
    net_asyncWrite(&r->connection.driver, r->head.getPtr, buffer_size(&r->head));
}

void ICACHE_FLASH_ATTR http_writeCallback(void* request, NetError error)
{
    HTTPRequest* r = (HTTPRequest*)request;
	if(error != net_ok)
    {
        r->error = http_netError(error);
        net_asyncDisconnect(&r->connection.driver);
        return;
    }
    
    if(!r->headSent)
    { // finished sending the request head
        r->headSent = 1;
        // send the body
        net_asyncWrite(&r->connection.driver, r->body.getPtr, buffer_size(&r->body));
    }
}

void ICACHE_FLASH_ATTR http_receiveCallback(void* request, const void* buffer, size_t bufferSize, NetError error)
{

	HTTPRequest* r = (HTTPRequest*)request;
    if(r->response.bodyComplete)
        // we're done, just ignore the call
        return;

	if(error != net_ok)
	{
        r->error = http_netError(error);
        net_asyncDisconnect(&r->connection.driver);
		return;
	}
	
    const void* unconsumed = (const char*)buffer;
	if(!r->response.headComplete)
	{ // need more header data
        HTTPError e = http_parseHead(&r->response, &unconsumed, buffer, bufferSize);
        switch(e)
        {
        case http_complete: // finished parsing the header
            bufferSize -= (const char*)unconsumed - (const char*)buffer;
            if(r->response.bodyComplete)
            { // zero length body
                r->error = http_complete;
                net_asyncDisconnect(&r->connection.driver);
                return;
            }
            break;
        case http_ok: // not finished parsing the header
            return;
        default: // error parsing head
            r->error = e;
            net_asyncDisconnect(&r->connection.driver);
			return;
        };
	}

    while(bufferSize > 0)
    {
        const void* bodyData = NULL;
        size_t bodyDataSize = 0;
        HTTPError e = http_parseBody(&r->response, &unconsumed, &bodyData, &bodyDataSize, unconsumed, bufferSize);
        if(e != http_ok) // an error, or the body is complete
        {
            if(e == http_complete)
                r->callback(r->userData, bodyData, bodyDataSize, http_ok);
            r->error = e;
            net_asyncDisconnect(&r->connection.driver);
            return;
        }
        r->callback(r->userData, bodyData, bodyDataSize, http_ok);
        bufferSize -= (const char*)unconsumed - (const char*)buffer;
    }
}

void ICACHE_FLASH_ATTR http_disconnectCallback(void* request, NetError error)
{
    HTTPRequest* r = (HTTPRequest*)request;
    r->callback(r->userData, NULL, 0, r->error);
}

