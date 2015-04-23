#include "buffer.h"

void ICACHE_FLASH_ATTR buffer_init(Buffer* b, char* data, size_t length)
{
	b->data = data;
   	b->length = length;
	b->putPtr = data;
	b->getPtr = data;
}

size_t ICACHE_FLASH_ATTR buffer_size(const Buffer* b)
{
	return b->putPtr - b->getPtr;
}

size_t ICACHE_FLASH_ATTR buffer_bytesAvailable(const Buffer* b)
{
	return (b->data + b->length) - b->putPtr;
}

BufferError ICACHE_FLASH_ATTR buffer_write(Buffer* b, const char* d, size_t s)
{
	if(buffer_bytesAvailable(b) < s)
		return buffer_overrun;
	memcpy(b->putPtr, d, s);
	return buffer_commit(b, s);
}

BufferError ICACHE_FLASH_ATTR buffer_commit(Buffer* b, size_t s)
{
	if(buffer_bytesAvailable(b) < s)
		return buffer_overrun;
	b->putPtr += s;
	return buffer_ok;
}

BufferError ICACHE_FLASH_ATTR buffer_read(Buffer* b, char* d, size_t s)
{
	if(buffer_size(b) < s)
		return buffer_overrun;
	memcpy(d, b->getPtr, s);
	return buffer_consume(b, s);
}

BufferError ICACHE_FLASH_ATTR buffer_consume(Buffer* b, size_t s)
{
	if(buffer_size(b) < s)
		return buffer_overrun;
	b->getPtr += s;
	return buffer_ok;
}

