#include "buffer_sequence.h"

#define min(x, y) (((x) < (y)) ? (x) : (y))

void ICACHE_FLASH_ATTR bufferSequence_init(BufferSequence* b, const void* d, size_t s)
{
	b->data = d;
	b->length = s;
	b->next = NULL;
}

void ICACHE_FLASH_ATTR bufferSequence_append(BufferSequence* b, BufferSequence* e)
{
    while(b->next)
		b = b->next;
	b->next = e;
}

void ICACHE_FLASH_ATTR bufferSequence_advance(BufferSequence* b, size_t s)
{
	while(s)
	{
		if(s >= b->length)
		{
			if(b->next)
			{
				s -= b->length;
                *b = *b->next;
			}
			else
			{
				bufferSequence_init(b, NULL, 0);
                return;
			}
		}
		else
		{
			b->length -= s;
			b->data = b->data + s;
            return;
		}
	}
}

int ICACHE_FLASH_ATTR bufferSequence_compare(BufferSequence a, BufferSequence b, size_t s)
{
    while(s > 0)
    {
        size_t compareLength = min(a.length, b.length);
        compareLength = min(compareLength, s);
        int cmp = memcmp(a.data, b.data, compareLength);
        if(cmp != 0)
            return cmp;
        s -= compareLength;
        if(s == 0)
            return 0;
        a.length -= compareLength;
        b.length -= compareLength;
        if((a.length == 0 && !a.next) || (b.length == 0 && !b.next))
            // termination condition
            return a.length - b.length;
        if(a.length == 0)
            a = *a.next;
        if(b.length == 0)
            b = *b.next;
    }
    return 0;
}

void ICACHE_FLASH_ATTR bufferSequence_search(BufferSequence* dest, const BufferSequence* b, const void* d, size_t s)
{
    *dest = *b;
	if(s == 0)
		return;
	for(;;)
	{
		char* i = memchr(dest->data, *(const char*)d, dest->length);
		if(i)
		{
			dest->length -= (char*)i - (const char*)dest->data;
			dest->data = i;
            BufferSequence temp;
            bufferSequence_init(&temp, (char*)d, s);
			if(bufferSequence_compare(*dest, temp, s) == 0)
				return;
			--dest->length;
			dest->data = (const char*)dest->data + 1;
		}
		else
		{
			if(dest->next)
            {
                *dest = *dest->next;
            }
			else
            {
				bufferSequence_init(dest, NULL, 0);
                return;
            }
		}
	}
}
