#include "algorithm.h"

int memtoi(const void* buffer, size_t len)
{
	int v = 0;
	size_t i = 0;
	for(; i < len; ++i)
	{
		v = v * 10 + (*((const char*)buffer + i) - '0');
	}
	return v;
}

int memhtoi(const void* buffer, size_t len)
{
    int v = 0;
    size_t i = 0;
    const char* data = (const char*)buffer;
    for(; i < len; ++i)
    {
        v *= 16;
        if(data[i] >= '0' && data[i] <= '9')
            v += data[i] - '0';
        else if(data[i] >= 'a' && data[i] <= 'f')
            v += 10 + data[i] - 'a';
        else
            v += 10 + data[i] - 'A';
    }
    return v;
}

