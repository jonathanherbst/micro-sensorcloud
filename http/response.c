#include "response.h"

#include <buffer/buffer_sequence.h>
#include "detail/algorithm.h"

HTTPError http_parseTransferEncoding(HTTPResponse* response);
HTTPError http_parseStandardBody(HTTPResponse* response, const void** unconsumed, const void** bodyData,
    size_t* bodyDataSize, const void* data, size_t dataSize);
HTTPError http_parseChunkedBody(HTTPResponse* response, const void** unconsumed, const void** bodyData,
    size_t* bodyDataSize, const void* data, size_t dataSize);

void ICACHE_FLASH_ATTR http_initResponse(HTTPResponse* response, Buffer headBuffer)
{
    response->headComplete = 0;
    response->bodyComplete = 0;
    response->dataLeft = 0;
    response->transferEncoding = httpEncoding_unknown;
    response->head = headBuffer;
    buffer_init(&response->parserBuffer, response->parserData, sizeof(response->parserData));
}

HTTPError ICACHE_FLASH_ATTR http_parseHead(HTTPResponse* response, const void** unconsumed, const void* data, size_t dataSize)
{
	static const char headerEnd[] = "\r\n\r\n";
	static const size_t headerEndSize = sizeof(headerEnd) - 1;

	size_t responseSize = buffer_size(&response->head);
	
	// compile possible part of eoh from response and current buffer
	size_t reUsedBytes = min(responseSize, headerEndSize - 1);
	char* reUsedBuffer = response->head.data + (responseSize - reUsedBytes);
	BufferSequence searchRange;
	bufferSequence_init(&searchRange, reUsedBuffer, reUsedBytes);
	BufferSequence newData;
	bufferSequence_init(&newData, data, dataSize);
	bufferSequence_append(&searchRange, &newData);
	
	// try to find eoh in the search range
	size_t headerData = dataSize;
	BufferSequence eoh;
	bufferSequence_search(&eoh, &searchRange, headerEnd, headerEndSize);
	if(eoh.data)
	{ // found the eoh
		// advance past the end of the header
		bufferSequence_advance(&eoh, headerEndSize);
        if(eoh.data)
        {
		    headerData = (const char*)eoh.data - (const char*)data;
		    *unconsumed = eoh.data;
        }
        else
        {
            headerData = dataSize;
            *unconsumed = (const char*)data + dataSize;
        }
        response->headComplete = 1;
	}
	else
	{ // didn't find the eoh
		// no unconsumed data
		*unconsumed = (const char*)data + dataSize;
	    return http_bufferError(buffer_write(&response->head, (const char*)data, headerData));
	}
	BufferError error = buffer_write(&response->head, (const char*)data, headerData);
    if(error != buffer_ok)
        return http_bufferError(error);
    HTTPError e = http_parseTransferEncoding(response);
    if(e != http_ok)
        return e;
    response->bodyComplete = response->transferEncoding == httpEncoding_standard && response->dataLeft == 0;
    return http_complete;
}

HTTPError ICACHE_FLASH_ATTR http_parseBody(HTTPResponse* response, const void** unconsumed, const void** bodyData, size_t* bodyDataSize,
    const void* data, size_t dataSize)
{
    if(response->bodyComplete)
        return http_complete;

    switch(response->transferEncoding)
    {
    case httpEncoding_standard:
        return http_parseStandardBody(response, unconsumed, bodyData, bodyDataSize, data, dataSize);
    case httpEncoding_chunked:
        return http_parseChunkedBody(response, unconsumed, bodyData, bodyDataSize, data, dataSize);
    default:
        break;
    }
    return http_unrecognizedEncoding;
}

HTTPError ICACHE_FLASH_ATTR http_parseTransferEncoding(HTTPResponse* response)
{
	static const char chunked[] = "chunked";
	static const size_t chunkedSize = sizeof(chunked) - 1;

	const char* header;
	size_t headerSize;
	HTTPError error = httpResponse_getHeader(&header, &headerSize, response, "Content-Length");
	if(error == http_ok)
	{ // standard encoding
		response->transferEncoding = httpEncoding_standard;
        size_t bodySize = memtoi(header, headerSize);
		response->dataLeft = bodySize;
		return http_ok;
	}

	error = httpResponse_getHeader(&header, &headerSize, response, "Transfer-Encoding");
	if(error == http_ok)
	{ // has a transfer-encoding header
		if(headerSize == chunkedSize && memcmp(header, chunked, headerSize) == 0)
		{ // chunked encoding
			response->dataLeft = 0;
			response->transferEncoding = httpEncoding_chunked;
			return http_ok;
		}
	}

	return http_unrecognizedEncoding;
}

HTTPError ICACHE_FLASH_ATTR http_parseStandardBody(HTTPResponse* response, const void** unconsumed, const void** bodyData,
    size_t* bodyDataSize, const void* data, size_t dataSize)
{
    size_t writeSize = min(response->dataLeft, dataSize);
    *bodyData = data;
    *bodyDataSize = writeSize;
    *unconsumed = (const char*)data + writeSize;
    response->dataLeft -= writeSize;
    if(response->dataLeft == 0)
    {
        response->bodyComplete = 1;
        return http_complete;
    }
	return http_ok;
}

HTTPError ICACHE_FLASH_ATTR http_parseChunkedBody(HTTPResponse* response, const void** unconsumed, const void** bodyData,
    size_t* bodyDataSize, const void* data, size_t dataSize)
{
	*unconsumed = data;
	while(dataSize > 0)
	{
		if(response->dataLeft == 0)
		{ // starting a new chunk
			void* endChunkHeader = memchr(*unconsumed, '\n', dataSize);
			if(endChunkHeader)
			{ // found the end of a chunk header
				size_t chunkHeaderSize = (char*)endChunkHeader - (const char*)*unconsumed + 1;
				BufferError e = buffer_write(&response->parserBuffer, (const char*)*unconsumed, chunkHeaderSize);
				if(e != buffer_ok)
					return http_bufferError(e);
				response->dataLeft = memhtoi(response->parserBuffer.getPtr, buffer_size(&response->parserBuffer) - 2) + 2;
				if(response->dataLeft == 2)
                {
                    response->bodyComplete = 1;
					return http_complete;
                }
				dataSize -= chunkHeaderSize;
				*unconsumed = (const char*)*unconsumed + chunkHeaderSize;
			}
			else
			{ // didn't find the end of the header
				return http_bufferError(buffer_write(&response->parserBuffer, (const char*)*unconsumed, dataSize));
			}
		}
		else
		{ // we already have a chunk
			if(response->dataLeft > 2)
			{ // we have some data to write
				size_t writeSize = min(response->dataLeft - 2, dataSize);
                *bodyData = *unconsumed;
                *bodyDataSize = writeSize;
                *unconsumed = (const char*)*unconsumed + writeSize;
                return http_ok;
			}

			if(response->dataLeft <= 2)
			{ // need to consume the eol
				size_t consumeSize = min(response->dataLeft, dataSize);
				dataSize -= consumeSize;
				response->dataLeft -= consumeSize;
				*unconsumed = (const char*)*unconsumed + consumeSize;
			}

			if(response->dataLeft == 0)
			{ // finished a chunk
				buffer_init(&response->parserBuffer, response->parserData, sizeof(response->parserData));
			}
		}
	}
	return http_ok;
}

HTTPError ICACHE_FLASH_ATTR httpResponse_getCode(HTTPResponseCode* code, const char** reason, size_t* reasonSize,
    const HTTPResponse* response)
{
	size_t bufferSize = buffer_size(&response->head);
	char* startCode = memchr(response->head.getPtr, ' ', bufferSize);
	if(!startCode)
		return http_bufferOverrun;
	++startCode;

	bufferSize -= startCode - response->head.getPtr;
	char* endCode = memchr(startCode, ' ', bufferSize);
	if(!endCode)
		return http_bufferOverrun;

	char* startReason = endCode + 1;
	bufferSize -= startReason - startCode;
	char* endReason = memchr(startReason, '\r', bufferSize);
	if(!endReason)
		return http_bufferOverrun;

	*code = (HTTPResponseCode)memtoi(startCode, endCode - startCode);
	*reason = startReason;
	*reasonSize = endReason - startReason;
	return http_ok;
}

HTTPError ICACHE_FLASH_ATTR httpResponse_getHeader(const char** buffer, size_t* size, const HTTPResponse* response, const char* name)
{
	size_t nameSize = strlen(name);
	size_t bufferSize = buffer_size(&response->head);
	char* headerStart = memchr(response->head.getPtr, '\n', bufferSize);
	if(!headerStart)
		return http_headerDoesntExist;
	++headerStart;
	bufferSize -= headerStart - response->head.getPtr;

	while(bufferSize > 0 && *headerStart != '\r')
	{
		char* headerNameEnd = memchr(headerStart, ':', bufferSize);
		if(!headerNameEnd)
			return http_headerDoesntExist;
		
		if(nameSize == headerNameEnd - headerStart &&
			memcmp(headerStart, name, nameSize) == 0)
		{
			if(bufferSize - nameSize < 1)
				return http_headerDoesntExist;
			
			char* startValue = headerNameEnd + 2;
			char* endValue = memchr(startValue, '\r', bufferSize - nameSize - 2);
			if(!endValue)
				return http_headerDoesntExist;
			*buffer = startValue;
			*size = endValue - startValue;
			return http_ok;
		}
		
		char* newHeaderStart = memchr(headerStart, '\n', bufferSize);
		if(!headerStart)
			return http_headerDoesntExist;
		++newHeaderStart;
		bufferSize -= newHeaderStart - headerStart;
		headerStart = newHeaderStart;
	}
	return http_headerDoesntExist;
}

