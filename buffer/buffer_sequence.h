#ifndef BUFFER_BUFFER_SEQUENCE
#define BUFFER_BUFFER_SEQUENCE

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>

struct BufferSequenceData;
typedef struct BufferSequenceData
{
    const char* data;
    size_t length;

    struct BufferSequenceData* next;
} BufferSequence;

/**
 * Initialize a buffer sequence with the specified data.
 * @param[in]   b   The buffer sequence to initialize.
 * @param[in]   d   Data the buffer should represent.
 * @param[in]   s   Size of the data in bytes.
 */
void bufferSequence_init(BufferSequence* b, const void* d, size_t s);

/**
 * Append the e to b.
 * @param[in]   b   The buffer to be appended.
 * @param[in]   e   The buffer to append.
 */
void bufferSequence_append(BufferSequence* b, BufferSequence* e);

/**
 * Advance the buffer s bytes.
 * @param[in]   b   The buffer to advance.
 * @param[in]   s   The number of bytes to advance the buffer.
 */
void bufferSequence_advance(BufferSequence* b, size_t s);

/**
 * Compare two buffer sequences together.
 * @param[in]   a   First buffer to compare.
 * @param[in]   b   Second buffer to compare.
 * @param[in]   s   Number of bytes to compare.
 * @return 0 if a contains the same bytes in the same order as b, not 0 otherwise.
 */
int bufferSequence_compare(BufferSequence a, BufferSequence b, size_t s);

/**
 * Search for a sequence of bytes in a buffer.
 * @param[out]  dest    A buffer pointing to the sequence in b.
 * @param[in]   b       Buffer to search in.
 * @param[in]   d       Sequence to search for.
 * @param[in]   s       Size of the sequence in bytes.
 * @note If the sequence is not found dest points to a null buffer.
 */
void bufferSequence_search(BufferSequence* dest, const BufferSequence* b, const void* d, size_t s);

#ifdef __cplusplus
}
#endif

#endif
