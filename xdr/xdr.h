#ifndef XDR_XDR
#define XDR_XDR

#include <buffer/buffer.h>

#include <stdint.h>

BufferError xdr_readUInt(uint32_t* value, Buffer* buffer);

BufferError xdr_readInt(int32_t* value, Buffer* buffer);

BufferError xdr_readUHyper(uint64_t* value, Buffer* buffer);

BufferError xdr_readHyper(int64_t* value, Buffer* buffer);

BufferError xdr_readFloat(float* value, Buffer* buffer);

BufferError xdr_readDouble(double* value, Buffer* buffer);

BufferError xdr_readString(char* value, size_t size, Buffer* buffer);

BufferError xdr_writeInt(Buffer* buffer, int32_t value);

BufferError xdr_writeUInt(Buffer* buffer, uint32_t value);

BufferError xdr_writeHyper(Buffer* buffer, int64_t value);

BufferError xdr_writeUHyper(Buffer* buffer, uint64_t value);

BufferError xdr_writeFloat(Buffer* buffer, float value);

BufferError xdr_writeDouble(Buffer* buffer, double value);

BufferError xdr_writeString(Buffer* buffer, const char* value, size_t size);

#endif
