#include "xdr.h"

#ifndef __BYTE_ORDER__
#error Unable to resolve byte order
#endif

#if __BYTE_ORDER__ == __ORDER_PGP_ENDIAN__
#error PGP endian not supported
#endif

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
uint32_t ICACHE_FLASH_ATTR xdr_endianUint(uint32_t v)
{
    return (v << 24 & 0xFF000000) | (v << 8 & 0x00FF0000) | (v >> 8 & 0x0000FF00) | (v >> 24 & 0x000000FF);
}

int32_t ICACHE_FLASH_ATTR xdr_endianInt(int32_t v)
{
    return (v << 24 & 0xFF000000) | (v << 8 & 0x00FF0000) | (v >> 8 & 0x0000FF00) | (v >> 24 & 0x000000FF);
}

uint64_t ICACHE_FLASH_ATTR xdr_endianUhyper(uint64_t v)
{
    return (v << 56 & 0xFF00000000000000) | (v << 40 & 0x00FF000000000000) | (v << 24 & 0x0000FF0000000000) |
        (v << 8 & 0x000000FF00000000) | (v >> 8 & 0x00000000FF000000) | (v >> 24 & 0x0000000000FF0000) |
        (v >> 40 & 0x000000000000FF00) | (v >> 56 & 0x00000000000000FF);
}

int64_t ICACHE_FLASH_ATTR xdr_endianHyper(int64_t v)
{
    return (v << 56 & 0xFF00000000000000) | (v << 40 & 0x00FF000000000000) | (v << 24 & 0x0000FF0000000000) |
        (v << 8 & 0x000000FF00000000) | (v >> 8 & 0x00000000FF000000) | (v >> 24 & 0x0000000000FF0000) |
        (v >> 40 & 0x000000000000FF00) | (v >> 56 & 0x00000000000000FF);
}

float ICACHE_FLASH_ATTR xdr_endianFloat(float v)
{
    uint32_t data = xdr_endianUint(*(uint32_t*)&v);
    return *(float*)&data;
}

double ICACHE_FLASH_ATTR xdr_endianDouble(double v)
{
    uint64_t data = xdr_endianUhyper(*(uint64_t*)&v);
    return *(double*)&data;
}
#else
uint32_t ICACHE_FLASH_ATTR xdr_endianUint(uint32_t v)
{
    return v;
}

int32_t ICACHE_FLASH_ATTR xdr_endianInt(int32_t v)
{
    return v;
}

uint64_t ICACHE_FLASH_ATTR xdr_endianUhyper(uint64_t v)
{
    return v;
}

int64_t ICACHE_FLASH_ATTR xdr_endianHyper(int64_t v)
{
    return v;
}

float ICACHE_FLASH_ATTR xdr_endianFloat(float v)
{
    return v;
}

double ICACHE_FLASH_ATTR xdr_endianDouble(double v)
{
    return v;
}
#endif

size_t ICACHE_FLASH_ATTR xdr_lineSize(size_t size)
{
    size_t remainder = size % 4;
    return remainder == 0 ? size : size + 4 - remainder;
}

BufferError ICACHE_FLASH_ATTR xdr_readUInt(uint32_t* value, Buffer* buffer)
{
    BufferError e = buffer_read(buffer, (char*)value, 4);
    if(e != buffer_ok)
        return e;
    *value = xdr_endianUint(*value);
    return buffer_ok;
}

BufferError ICACHE_FLASH_ATTR xdr_readInt(int32_t* value, Buffer* buffer)
{
    BufferError e = buffer_read(buffer, (char*)value, 4);
    if(e != buffer_ok)
        return e;
    *value = xdr_endianInt(*value);
    return buffer_ok;
}

BufferError ICACHE_FLASH_ATTR xdr_readUHyper(uint64_t* value, Buffer* buffer)
{
    BufferError e = buffer_read(buffer, (char*)value, 8);
    if(e != buffer_ok)
        return e;
    *value = xdr_endianUhyper(*value);
    return buffer_ok;
}

BufferError ICACHE_FLASH_ATTR xdr_readHyper(int64_t* value, Buffer* buffer)
{
    BufferError e = buffer_read(buffer, (char*)value, 8);
    if(e != buffer_ok)
        return e;
    *value = xdr_endianHyper(*value);
    return buffer_ok;
}

BufferError ICACHE_FLASH_ATTR xdr_readFloat(float* value, Buffer* buffer)
{
    BufferError e = buffer_read(buffer, (char*)value, 4);
    if(e != buffer_ok)
        return e;
    *value = xdr_endianFloat(*value);
    return buffer_ok;
}

BufferError ICACHE_FLASH_ATTR xdr_readDouble(double* value, Buffer* buffer)
{
    BufferError e = buffer_read(buffer, (char*)value, 8);
    if(e != buffer_ok)
        return e;
    *value = xdr_endianDouble(*value);
    return buffer_ok;
}

BufferError ICACHE_FLASH_ATTR xdr_readString(char* value, size_t size, Buffer* buffer)
{
    size_t lineSize = xdr_lineSize(size);
    if(buffer_size(buffer) < lineSize)
        return buffer_overrun;
    if(value)
        memcpy(value, buffer->getPtr, size);
    return buffer_consume(buffer, lineSize);
}

BufferError ICACHE_FLASH_ATTR xdr_writeInt(Buffer* buffer, int32_t value)
{
    value = xdr_endianInt(value);
    return buffer_write(buffer, (char*)&value, 4);
}

BufferError ICACHE_FLASH_ATTR xdr_writeUInt(Buffer* buffer, uint32_t value)
{
    value = xdr_endianUint(value);
    return buffer_write(buffer, (char*)&value, 4);
}

BufferError ICACHE_FLASH_ATTR xdr_writeHyper(Buffer* buffer, int64_t value)
{
    value = xdr_endianHyper(value);
    return buffer_write(buffer, (char*)&value, 8);
}

BufferError ICACHE_FLASH_ATTR xdr_writeUHyper(Buffer* buffer, uint64_t value)
{
    value = xdr_endianUhyper(value);
    return buffer_write(buffer, (char*)&value, 8);
}

BufferError ICACHE_FLASH_ATTR xdr_writeFloat(Buffer* buffer, float value)
{
    value = xdr_endianFloat(value);
    return buffer_write(buffer, (char*)&value, 4);
}

BufferError ICACHE_FLASH_ATTR xdr_writeDouble(Buffer* buffer, double value)
{
    value = xdr_endianDouble(value);
    return buffer_write(buffer, (char*)&value, 8);
}

BufferError ICACHE_FLASH_ATTR xdr_writeString(Buffer* buffer, const char* value, size_t size)
{
    size_t lineSize = xdr_lineSize(size);
    if(buffer_bytesAvailable(buffer) < lineSize)
        return buffer_overrun;
    memcpy(buffer->putPtr, value, size);
    return buffer_commit(buffer, lineSize);
}
