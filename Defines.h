#pragma once

#define SUCCESS 0

#if _CONSOLE == 1
#include <iostream>
#endif // _CONSOLE

#include <fstream>
#include "Point.h"
#include "Colors.h"

#define FLOAT_1DIV60 0.016666666666666666f

#define MODULO(x, a) ((((x) % (a)) + (a)) % (a))
#define SIZE_MUL(a, b) static_cast<size_t>(a) * (b)
#define UCHAR_PTR(a) reinterpret_cast<uchar*>(a)
#define TYPE_MALLOC(T, size) static_cast<T*>(malloc((size) * sizeof(T)))

using uint = unsigned int;
using uchar = unsigned char;
using option_t = uchar;

inline uchar slide_uchar(uchar x1, uchar x2, uchar t)
{
	return x1 + (x2 - x1) * t / UINT8_MAX;
}

inline int get_sign(int x)
{
	return (x >> 31) | 1;
}

inline point get_sign(point x)
{
	return { get_sign(x.x), get_sign(x.y) };
}

namespace data
{
	constexpr size_t cache_buffer_size = 1 << 21;
	color_t* cache_buffer = nullptr;
	uchar* cache_shade = nullptr;

	inline void init_cache()
	{
		if (cache_buffer != nullptr) return;
		cache_buffer = TYPE_MALLOC(color_t, cache_buffer_size);
		cache_shade = UCHAR_PTR(cache_buffer);
	}

	inline void clean_up_cache()
	{
		free(cache_buffer);
		cache_buffer = nullptr;
		cache_shade = nullptr;
	}
}
