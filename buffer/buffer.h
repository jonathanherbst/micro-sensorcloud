#ifndef HTTP_BUFFER
#define HTTP_BUFFER

#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * A buffer specifically designed for use with HTTP.
 */
typedef struct
{
    // Data in the buffer.
	char* data;
    // Number of bytes in the buffer.
	size_t length;

    // Write pointer.
	char* putPtr;
    // Read pointer.
	const char* getPtr;
} Buffer;

typedef enum
{
    buffer_ok,
    buffer_overrun
} BufferError;

/**
 * Initialize a buffer with the specified parameters.
 * @param[out]  b       Buffer to initialize.
 * @param[in]   data    Bytes to use as the underlying buffer.
 * @param[in]   length  Number of bytes to use.
 */
void buffer_init(Buffer* b, char* data, size_t length);

/**
 * Calculate the size of the buffer.
 * @param[in]   b   Buffer to calculate the size of.
 * @return Number of bytes in the buffer.
 */
size_t buffer_size(const Buffer* b);

/**
 * Calculate the number of bytes available to write in the buffer
 * @param[in]   b   Buffer to calculate for.
 * @return Number of bytes available in the buffer.
 */
size_t buffer_bytesAvailable(const Buffer* b);

/**
 * Write some data to the buffer.
 * @param[io]   b   Buffer to write data in.
 * @param[in]   d   Data to write to the buffer.
 * @param[in]   s   Number of bytes to write.
 * @return http_bufferOverrun if there wasn't enough available data in the buffer to write, http_ok otherwise.
 */
BufferError buffer_write(Buffer* b, const char* d, size_t s);

/**
 * Advance the write pointer.
 * @param[io]   b   Buffer to advance.
 * @param[in]   s   Number of bytes to advance the buffer.
 * @return http_bufferOverrun if s is greater than available data, http_ok otherwise.
 */
BufferError buffer_commit(Buffer* b, size_t s);

/**
 * Read data from the buffer.
 * @param[io]   b   Buffer to read from.
 * @param[out]  d   Data read from the buffer.
 * @param[in]   s   Number of bytes to read from the buffer.
 * @return http_bufferOverrun if s is greater than buffer size, http_ok otherwise.
 */
BufferError buffer_read(Buffer* b, char* d, size_t s);

/**
 * Advance the read pointer.
 * @param[io]   b   Buffer to advance.
 * @param[in]   s   Number of bytes to advance the buffer.
 * @return http_bufferOverrun if s is greater than buffer size, http_ok otherwise.
 */
BufferError buffer_consume(Buffer* b, size_t s);

#ifdef __cplusplus
}
#endif

#endif
