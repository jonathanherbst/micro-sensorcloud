#ifndef DETAIL_ALGORITHM
#define DETAIL_ALGORITHM

#include <string.h>

#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))

#ifdef __cplusplus
extern "C" {
#endif

int memtoi(const void* buffer, size_t len);
int memhtoi(const void* buffer, size_t len);

#ifdef __cplusplus
}
#endif

#endif

