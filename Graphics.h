#pragma once

#include <numeric>

#include "Defines.h"

namespace graphics
{
	inline color_t rgb_color(uchar r, uchar g, uchar b)
	{
		static color_t color = colors::alpha;
		static auto shade_ptr = UCHAR_PTR(&color);
		shade_ptr[0] = b;
		shade_ptr[1] = g;
		shade_ptr[2] = r;
		return color;
	}

	inline color_t hsv_to_rgb(int h, uchar s, uchar v)
	{
		h = MODULO(h, 360);

		const uchar
			c = v * s / UINT8_MAX,
			x = c - static_cast<uchar>(c * fabs(fmodf(h * FLOAT_1DIV60, 2.0f) - 1.0f));

		static color_t color = colors::alpha;
		static auto shade_ptr = UCHAR_PTR(&color);
		shade_ptr[0] = v - c;
		shade_ptr[1] = shade_ptr[0];
		shade_ptr[2] = shade_ptr[0];

		const uchar i = (h / 60 + 1) % 6;

		shade_ptr[2 - (i >> 1)] += c;
		shade_ptr[i % 3] += x;

		return color;
	}

	struct surface
	{
		color_t* buffer, * end;
		point size;
		size_t buffer_size;

		surface() : buffer{ nullptr }, end{ nullptr }, size{ 0, 0 }, buffer_size{ 0 } { return; }

		surface(point size, bool reserve_buffer);
		~surface();
	};

	inline surface::surface(point size, bool reserve_buffer = true) : size{ size }
	{
		buffer_size = SIZE_MUL(size.x, size.y);
		if (reserve_buffer)
		{
			buffer = TYPE_MALLOC(color_t, buffer_size);
			end = buffer + buffer_size;
			return;
		}
		buffer = nullptr;
		end = nullptr;
	}

	inline surface::~surface()
	{
		if (buffer)
		{
			free(buffer);
			buffer = nullptr;
		}
	}

	inline void copy(surface& src, surface& dest) { memcpy(dest.buffer, src.buffer, src.buffer_size << 2); }
	inline void operator >>(surface& src, surface& dest) { memcpy(dest.buffer, src.buffer, src.buffer_size << 2); }
	inline void clear(surface& dest) { memset(dest.buffer, 0, dest.buffer_size << 2); }

	inline void fill(color_t color, surface& dest)
	{
		if (color == colors::black)
		{
			memset(dest.buffer, 0, dest.buffer_size << 2);
			return;
		}
		for (color_t* px = dest.buffer; px < dest.end; ++px)
			*px = color;
	}

	inline void operator >>(color_t color, surface& dest) { fill(color, dest); }

	inline void clamp_to_surface(point& p, surface& surf)
	{
		p.x = std::clamp(p.x, 0, surf.size.x);
		p.y = std::clamp(p.y, 0, surf.size.y);
	}
}

namespace data
{
	graphics::surface cache_surface = {};

	inline void init_cache_surface()
	{
		init_cache();
		cache_surface.buffer = cache_buffer;
	}
}

namespace graphics
{
	constexpr option_t
		F_AUTO = 0,
		F_RGB = 3,
		F_RGBA = 4,
		F_24B = F_RGB,
		F_32B = F_RGBA;

	surface* read_binary_into_surface(const char* file_path, option_t type = F_AUTO)
	{
		static uchar* src;
		static FILE* file;
		static point dim;

		if (fopen_s(&file, file_path, "rb") != SUCCESS)
			return nullptr;

		fread(&dim.x, 4, 2, file);

		if (SIZE_MUL(dim.x, dim.y) > data::cache_buffer_size)
		{
			fclose(file);
			return nullptr;
		}

		if (type == F_AUTO)
		{
			const char* str_end = file_path;
			while (*str_end != '\0') ++str_end;
			type = (str_end[-1] == '2' && str_end[-2] == '3') ? F_32B : F_24B;
		}

		auto dest = new surface(dim);

		for (auto px = dest->buffer; px < dest->end;)
		{
			fread(data::cache_shade, type, dim.x, file);
			src = data::cache_shade;
			for (color_t* x_end = px + dim.x; px < x_end; px++, src += type)
				*px = *reinterpret_cast<color_t*>(src);
		}

		fclose(file);
		return dest;
	}

	inline void reverse_colors(surface& surf)
	{
		for (auto px = surf.buffer; px < surf.end; px++)
			*px = ~*px;
	}

	inline void black_and_white(surface& surf)
	{
		for (auto px = surf.buffer; px < surf.end; px++)
			*px = ((UCHAR_PTR(px)[0] + UCHAR_PTR(px)[1] + UCHAR_PTR(px)[2]) >= 0x180) ? colors::white : colors::black;
	}

	void gray_scale(surface& surf)
	{
		for (auto px = UCHAR_PTR(surf.buffer); px < UCHAR_PTR(surf.end); px += 4)
		{
			uchar color = (px[0] + px[1] + px[2]) / 3;
			px[0] = color;
			px[1] = color;
			px[2] = color;
		}
	}

	void checkers(surface& surf, int size, color_t c1, color_t c2)
	{
		static point err;
		static color_t colors[2];
		static uchar yc, xc;

		colors[0] = c1;
		colors[1] = c2;

		err.y = 1;
		yc = 0;

		color_t color = c1;

		for (auto px = surf.buffer; px < surf.end; err.y++)
		{
			err.x = 1;
			xc = yc;

			for (auto x_end = px + surf.size.x; px < x_end; px++, err.x++)
			{
				*px = color;
				if (err.x == size)
				{
					xc ^= 1;
					color = colors[xc];
					err.x = 0;
				}
			}

			if (err.y == size)
			{
				yc ^= 1;
				color = colors[yc];
				err.y = 0;
			}
		}
	}

	struct three_2d_matrix
	{
		uchar
			x11, x12, x13,
			x21, x22, x23,
			x31, x32, x33;
	};

	struct three_3d_matrix
	{
		three_2d_matrix blue, green, red;
	};

	option_t complex_nine_convolution(surface& surf, three_3d_matrix& value)
	{
		if (surf.buffer_size > data::cache_buffer_size) return 1;
		if (data::cache_buffer == nullptr) return 2;

		data::cache_surface.end = data::cache_buffer + surf.buffer_size;

		surf >> data::cache_surface;

		static three_3d_matrix sums;

		// bottom-left
		sums.blue.x11 = value.blue.x22 + value.blue.x23 + value.blue.x32 + value.blue.x33;
		sums.green.x11 = value.green.x22 + value.green.x23 + value.green.x32 + value.green.x33;
		sums.red.x11 = value.red.x22 + value.red.x23 + value.red.x32 + value.red.x33;

		// bottom-right
		sums.blue.x13 = value.blue.x22 + value.blue.x21 + value.blue.x32 + value.blue.x31;
		sums.green.x13 = value.green.x22 + value.green.x21 + value.green.x32 + value.green.x31;
		sums.red.x13 = value.red.x22 + value.red.x21 + value.red.x32 + value.red.x31;

		// top-left
		sums.blue.x31 = value.blue.x22 + value.blue.x23 + value.blue.x12 + value.blue.x13;
		sums.green.x31 = value.green.x22 + value.green.x23 + value.green.x12 + value.green.x13;
		sums.red.x31 = value.red.x22 + value.red.x23 + value.red.x12 + value.red.x13;

		// top-right
		sums.blue.x33 = value.blue.x22 + value.blue.x21 + value.blue.x12 + value.blue.x11;
		sums.green.x33 = value.green.x22 + value.green.x21 + value.green.x12 + value.green.x11;
		sums.red.x33 = value.red.x22 + value.red.x21 + value.red.x12 + value.red.x11;

		// bottom
		sums.blue.x12 = sums.blue.x11 + value.blue.x21 + value.blue.x31;
		sums.green.x12 = sums.green.x11 + value.green.x21 + value.green.x31;
		sums.red.x12 = sums.red.x11 + value.red.x21 + value.red.x31;

		// left
		sums.blue.x21 = sums.blue.x11 + value.blue.x12 + value.blue.x13;
		sums.green.x21 = sums.green.x11 + value.green.x12 + value.green.x13;
		sums.red.x21 = sums.red.x11 + value.red.x12 + value.red.x13;

		// top
		sums.blue.x32 = sums.blue.x31 + value.blue.x21 + value.blue.x11;
		sums.green.x32 = sums.green.x31 + value.green.x21 + value.green.x11;
		sums.red.x32 = sums.red.x31 + value.red.x21 + value.red.x11;

		// right
		sums.blue.x23 = sums.blue.x13 + value.blue.x11 + value.blue.x12;
		sums.green.x23 = sums.green.x13 + value.green.x11 + value.green.x12;
		sums.red.x23 = sums.red.x13 + value.red.x11 + value.red.x12;

		// middle
		sums.blue.x22 = sums.blue.x12 + value.blue.x11 + value.blue.x12 + value.blue.x13;
		sums.green.x22 = sums.green.x12 + value.green.x11 + value.green.x12 + value.green.x13;
		sums.red.x22 = sums.red.x12 + value.red.x11 + value.red.x12 + value.red.x13;

		const int
			t0o = surf.size.x << 2, b0o = -t0o,
			tl0o = t0o - 4, tr0o = t0o + 4,
			bl0o = b0o - 4, br0o = b0o + 4,

			tdl0o = t0o - 8,

			t1o = t0o | 1, t2o = t0o | 2,
			tl1o = tl0o | 1, tl2o = tl0o | 2,
			tr1o = tr0o | 1, tr2o = tr0o | 2,

			b1o = b0o | 1, b2o = b0o | 2,
			bl1o = bl0o | 1, bl2o = bl0o | 2,
			br1o = br0o | 1, br2o = br0o | 2;

		// bottom-left
		auto dp = UCHAR_PTR(surf.buffer);
		dp[0] = sums.blue.x11 != 0 ? (
			dp[0] * value.blue.x22 +
			dp[4] * value.blue.x23 +
			dp[t0o] * value.blue.x32 +
			dp[tr0o] * value.blue.x33
			) / sums.blue.x11 : colors::black;
		dp[1] = sums.green.x11 != 0 ? (
			dp[1] * value.green.x22 +
			dp[5] * value.green.x23 +
			dp[t1o] * value.green.x32 +
			dp[tr1o] * value.green.x33
			) / sums.green.x11 : colors::black;
		dp[2] = sums.red.x11 != 0 ? (
			dp[2] * value.red.x22 +
			dp[6] * value.red.x23 +
			dp[t2o] * value.red.x32 +
			dp[tr2o] * value.red.x33
			) / sums.red.x11 : colors::black;

		// bottom-right
		dp += tl0o;
		dp[0] = sums.blue.x13 != 0 ? (
			dp[0] * value.blue.x22 +
			dp[-4] * value.blue.x21 +
			dp[t0o] * value.blue.x32 +
			dp[tl0o] * value.blue.x31
			) / sums.blue.x13 : colors::black;
		dp[1] = sums.green.x13 != 0 ? (
			dp[1] * value.green.x22 +
			dp[-3] * value.green.x21 +
			dp[t1o] * value.green.x32 +
			dp[tl1o] * value.green.x31
			) / sums.green.x13 : colors::black;
		dp[2] = sums.red.x13 != 0 ? (
			dp[2] * value.red.x22 +
			dp[-2] * value.red.x21 +
			dp[t2o] * value.red.x32 +
			dp[tl2o] * value.red.x31
			) / sums.red.x13 : colors::black;

		// top-right
		dp = UCHAR_PTR(surf.end - 1);
		dp[0] = sums.blue.x33 != 0 ? (
			dp[0] * value.blue.x22 +
			dp[-4] * value.blue.x21 +
			dp[b0o] * value.blue.x12 +
			dp[bl0o] * value.blue.x11
			) / sums.blue.x33 : colors::black;
		dp[1] = sums.green.x33 != 0 ? (
			dp[1] * value.green.x22 +
			dp[-3] * value.green.x21 +
			dp[b1o] * value.green.x12 +
			dp[bl1o] * value.green.x11
			) / sums.green.x33 : colors::black;
		dp[2] = sums.red.x33 != 0 ? (
			dp[2] * value.red.x22 +
			dp[-2] * value.red.x21 +
			dp[b2o] * value.red.x12 +
			dp[bl2o] * value.red.x11
			) / sums.red.x33 : colors::black;

		// top-left
		dp -= tl0o;
		dp[0] = sums.blue.x31 != 0 ? (
			dp[0] * value.blue.x22 +
			dp[4] * value.blue.x23 +
			dp[b0o] * value.blue.x12 +
			dp[br0o] * value.blue.x13
			) / sums.blue.x31 : colors::black;
		dp[1] = sums.green.x31 != 0 ? (
			dp[1] * value.green.x22 +
			dp[5] * value.green.x23 +
			dp[b1o] * value.green.x12 +
			dp[br1o] * value.green.x13
			) / sums.green.x31 : colors::black;
		dp[2] = sums.red.x31 != 0 ? (
			dp[2] * value.red.x22 +
			dp[6] * value.red.x23 +
			dp[b2o] * value.red.x12 +
			dp[br2o] * value.red.x13
			) / sums.red.x31 : colors::black;

		// bottom and top
		for (auto dpb = UCHAR_PTR(surf.buffer + 1), dpt = UCHAR_PTR(surf.end) - tl0o, end = dpb + tdl0o,
			spb = UCHAR_PTR(data::cache_buffer + 1), spt = UCHAR_PTR(data::cache_surface.end) - tl0o;
			dpb < end; dpb += 4, dpt += 4, spb += 4, spt += 4)
		{
			dpb[0] = sums.blue.x12 ? (
				spb[-4] * value.blue.x21 +
				spb[0] * value.blue.x22 +
				spb[4] * value.blue.x23 +
				spb[tl0o] * value.blue.x31 +
				spb[t0o] * value.blue.x32 +
				spb[tr0o] * value.blue.x33
				) / sums.blue.x12 : colors::black;
			dpb[1] = sums.green.x12 ? (
				spb[-3] * value.green.x21 +
				spb[1] * value.green.x22 +
				spb[5] * value.green.x23 +
				spb[tl1o] * value.green.x31 +
				spb[t1o] * value.green.x32 +
				spb[tr1o] * value.green.x33
				) / sums.green.x12 : colors::black;
			dpb[2] = sums.red.x12 ? (
				spb[-2] * value.red.x21 +
				spb[2] * value.red.x22 +
				spb[6] * value.red.x23 +
				spb[tl2o] * value.red.x31 +
				spb[t2o] * value.red.x32 +
				spb[tr2o] * value.red.x33
				) / sums.red.x12 : colors::black;

			dpt[0] = sums.blue.x32 ? (
				spt[-4] * value.blue.x21 +
				spt[0] * value.blue.x22 +
				spt[4] * value.blue.x23 +
				spt[bl0o] * value.blue.x11 +
				spt[b0o] * value.blue.x12 +
				spt[br0o] * value.blue.x13
				) / sums.blue.x32 : colors::black;
			dpt[1] = sums.green.x32 ? (
				spt[-3] * value.green.x21 +
				spt[1] * value.green.x22 +
				spt[5] * value.green.x23 +
				spt[bl1o] * value.green.x11 +
				spt[b1o] * value.green.x12 +
				spt[br1o] * value.green.x13
				) / sums.green.x32 : colors::black;
			dpt[2] = sums.red.x32 ? (
				spt[-2] * value.red.x21 +
				spt[2] * value.red.x22 +
				spt[6] * value.red.x23 +
				spt[bl2o] * value.red.x11 +
				spt[b2o] * value.red.x12 +
				spt[br2o] * value.red.x13
				) / sums.red.x32 : colors::black;
		}

		// left and right
		for (auto dpb = UCHAR_PTR(surf.buffer + surf.size.x), dpt = dpb + tl0o, end = UCHAR_PTR(surf.end - surf.size.x),
			spb = UCHAR_PTR(data::cache_buffer + surf.size.x), spt = spb + tl0o;
			dpb < end; dpb += t0o, dpt += t0o, spb += t0o, spt += t0o)
		{
			dpb[0] = sums.blue.x21 ? (
				spb[0] * value.blue.x22 +
				spb[4] * value.blue.x23 +
				spb[t0o] * value.blue.x32 +
				spb[tr0o] * value.blue.x33 +
				spb[b0o] * value.blue.x12 +
				spb[br0o] * value.blue.x13
				) / sums.blue.x21 : colors::black;
			dpb[1] = sums.green.x21 ? (
				spb[1] * value.green.x22 +
				spb[5] * value.green.x23 +
				spb[t1o] * value.green.x32 +
				spb[tr1o] * value.green.x33 +
				spb[b1o] * value.green.x12 +
				spb[br1o] * value.green.x13
				) / sums.green.x21 : colors::black;
			dpb[2] = sums.red.x21 ? (
				spb[2] * value.red.x22 +
				spb[6] * value.red.x23 +
				spb[t2o] * value.red.x32 +
				spb[tr2o] * value.red.x33 +
				spb[b2o] * value.red.x12 +
				spb[br2o] * value.red.x13
				) / sums.red.x21 : colors::black;

			dpb[0] = sums.blue.x23 ? (
				spb[0] * value.blue.x22 +
				spb[4] * value.blue.x23 +
				spb[t0o] * value.blue.x32 +
				spb[tl0o] * value.blue.x31 +
				spb[b0o] * value.blue.x12 +
				spb[bl0o] * value.blue.x11
				) / sums.blue.x23 : colors::black;
			dpb[1] = sums.green.x23 ? (
				spb[1] * value.green.x22 +
				spb[5] * value.green.x23 +
				spb[t1o] * value.green.x32 +
				spb[tl1o] * value.green.x31 +
				spb[b1o] * value.green.x12 +
				spb[bl1o] * value.green.x11
				) / sums.green.x23 : colors::black;
			dpb[2] = sums.red.x23 ? (
				spb[2] * value.red.x22 +
				spb[6] * value.red.x23 +
				spb[t2o] * value.red.x32 +
				spb[tl2o] * value.red.x31 +
				spb[b2o] * value.red.x12 +
				spb[bl2o] * value.red.x11
				) / sums.red.x23 : colors::black;
		}

		for (auto dp = UCHAR_PTR(surf.buffer + surf.size.x + 1), end = UCHAR_PTR(surf.end - surf.size.x),
			sp = UCHAR_PTR(data::cache_buffer + surf.size.x + 1); dp < end; dp += 8, sp += 8)
		{
			for (auto x_end = dp + tdl0o; dp < x_end; dp += 4, sp += 4)
			{
				dp[0] = sums.blue.x22 != 0 ? (
					sp[tl0o] * value.blue.x11 + sp[t0o] * value.blue.x12 + sp[tr0o] * value.blue.x13 +
					sp[-4] * value.blue.x21 + sp[0] * value.blue.x22 + sp[4] * value.blue.x23 +
					sp[bl0o] * value.blue.x31 + sp[b0o] * value.blue.x32 + sp[br0o] * value.blue.x33
					) / sums.blue.x22 : colors::black;
				dp[1] = sums.green.x22 != 0 ? (
					sp[tl1o] * value.green.x11 + sp[t1o] * value.green.x12 + sp[tr1o] * value.green.x13 +
					sp[-3] * value.green.x21 + sp[1] * value.green.x22 + sp[5] * value.green.x23 +
					sp[bl1o] * value.green.x31 + sp[b1o] * value.green.x32 + sp[br1o] * value.green.x33
					) / sums.green.x22 : colors::black;
				dp[2] = sums.red.x22 != 0 ? (
					sp[tl2o] * value.red.x11 + sp[t2o] * value.red.x12 + sp[tr2o] * value.red.x13 +
					sp[-2] * value.red.x21 + sp[2] * value.red.x22 + sp[6] * value.red.x23 +
					sp[bl2o] * value.red.x31 + sp[b2o] * value.red.x32 + sp[br2o] * value.red.x33
					) / sums.red.x22 : colors::black;
			}
		}

		return SUCCESS;
	}

	/*
	aka: complex_nine_convolution(surface,
	{{
		0, 1, 0
		1, 1, 1
		0, 1, 0
	}, {
		0, 1, 0
		1, 1, 1
		0, 1, 0
	}, {
		0, 1, 0
		1, 1, 1
		0, 1, 0
	}})
	*/
	option_t blur(surface& surf)
	{
		if (surf.buffer_size > data::cache_buffer_size) return 1;
		if (data::cache_buffer == nullptr) return 2;

		data::cache_surface.end = data::cache_buffer + surf.buffer_size;

		surf >> data::cache_surface;

		const int t0o = surf.size.x << 2, b0o = -t0o,
			tl0o = t0o - 4, tdl0o = t0o - 8,
			t1o = t0o | 1, t2o = t0o | 2,
			b1o = b0o | 1, b2o = b0o | 2;

		auto dp = UCHAR_PTR(surf.buffer);
		dp[0] = (dp[0] + dp[4] + dp[t0o]) / 3;
		dp[0] = (dp[1] + dp[5] + dp[t1o]) / 3;
		dp[0] = (dp[2] + dp[6] + dp[t2o]) / 3;

		dp += tl0o;
		dp[0] = (dp[0] + dp[-4] + dp[t0o]) / 3;
		dp[0] = (dp[1] + dp[-3] + dp[t1o]) / 3;
		dp[0] = (dp[2] + dp[-2] + dp[t2o]) / 3;

		dp = UCHAR_PTR(surf.end - 1);
		dp[0] = (dp[0] + dp[-4] + dp[b0o]) / 3;
		dp[0] = (dp[1] + dp[-3] + dp[b1o]) / 3;
		dp[0] = (dp[2] + dp[-2] + dp[b2o]) / 3;

		dp -= tl0o;
		dp[0] = (dp[0] + dp[4] + dp[b0o]) / 3;
		dp[0] = (dp[1] + dp[5] + dp[b1o]) / 3;
		dp[0] = (dp[2] + dp[6] + dp[b2o]) / 3;

		for (auto dpb = UCHAR_PTR(surf.buffer + 1), dpt = UCHAR_PTR(surf.end) - tl0o, end = dpb + tdl0o,
			spb = UCHAR_PTR(data::cache_buffer + 1), spt = UCHAR_PTR(data::cache_surface.end) - tl0o;
			dpb < end; dpb += 4, dpt += 4, spb += 4, spt += 4)
		{
			dpb[0] = (spb[0] + spb[4] + spb[-4] + spb[t0o]) >> 2;
			dpb[1] = (spb[1] + spb[5] + spb[-3] + spb[t1o]) >> 2;
			dpb[2] = (spb[2] + spb[6] + spb[-2] + spb[t2o]) >> 2;

			dpt[0] = (spt[0] + spt[4] + spt[-4] + spt[b0o]) >> 2;
			dpt[1] = (spt[1] + spt[5] + spt[-3] + spt[b1o]) >> 2;
			dpt[2] = (spt[2] + spt[6] + spt[-2] + spt[b2o]) >> 2;
		}

		for (auto dpb = UCHAR_PTR(surf.buffer + surf.size.x), dpt = dpb + tl0o, end = UCHAR_PTR(surf.end - surf.size.x),
			spb = UCHAR_PTR(data::cache_buffer + surf.size.x), spt = spb + tl0o;
			dpb < end; dpb += tl0o, dpt += tl0o, spb += tl0o, spt += tl0o)
		{
			dpb[0] = (spb[0] + spb[4] + spb[t0o] + spb[b0o]) >> 2;
			dpb[1] = (spb[1] + spb[5] + spb[t1o] + spb[b1o]) >> 2;
			dpb[2] = (spb[2] + spb[6] + spb[t2o] + spb[b2o]) >> 2;

			dpt[0] = (spt[0] + spt[-4] + spt[t0o] + spt[b0o]) >> 2;
			dpt[1] = (spt[1] + spt[-3] + spt[t1o] + spt[b1o]) >> 2;
			dpt[2] = (spt[2] + spt[-2] + spt[t2o] + spt[b2o]) >> 2;
		}

		for (auto dp = UCHAR_PTR(surf.buffer + surf.size.x + 1), end = UCHAR_PTR(surf.end - surf.size.x),
			sp = UCHAR_PTR(data::cache_buffer + surf.size.x + 1); dp < end; dp += 8, sp += 8)
		{
			for (auto x_end = dp + t2o; dp < x_end; dp += 4, sp += 4)
			{
				dp[0] = (sp[0] + sp[4] + sp[-4] + sp[t0o] + sp[b0o]) / 5;
				dp[1] = (sp[1] + sp[5] + sp[-3] + sp[t1o] + sp[b1o]) / 5;
				dp[2] = (sp[2] + sp[6] + sp[-2] + sp[t2o] + sp[b2o]) / 5;
			}
		}

		return SUCCESS;
	}

	option_t blur(surface& surf, int main_amount)
	{
		if (surf.buffer_size > data::cache_buffer_size) return 1;
		data::cache_surface.end = data::cache_buffer + surf.buffer_size;

		surf >> data::cache_surface;

		const int y_step = surf.size.x << 2, ny_step = -y_step,
			y_step1n = y_step - 4, y_step2n = y_step - 8,
			y_step1o = y_step | 1, y_step2o = y_step | 2,
			ny_step1o = ny_step | 1, ny_step2o = ny_step | 2;

		const int f1d = main_amount + 2, f2d = main_amount + 3, f3d = main_amount + 4;

		auto dp = UCHAR_PTR(surf.buffer);
		dp[0] = ((dp[0] * main_amount) + dp[4] + dp[y_step]) / f1d;
		dp[1] = ((dp[1] * main_amount) + dp[5] + dp[y_step1o]) / f1d;
		dp[2] = ((dp[2] * main_amount) + dp[6] + dp[y_step2o]) / f1d;

		dp += y_step1n;
		dp[0] = ((dp[0] * main_amount) + dp[-4] + dp[y_step]) / f1d;
		dp[1] = ((dp[1] * main_amount) + dp[-3] + dp[y_step1o]) / f1d;
		dp[2] = ((dp[2] * main_amount) + dp[-2] + dp[y_step2o]) / f1d;

		dp = UCHAR_PTR(surf.end - 1);
		dp[0] = ((dp[0] * main_amount) + dp[-4] + dp[ny_step]) / f1d;
		dp[1] = ((dp[1] * main_amount) + dp[-3] + dp[ny_step1o]) / f1d;
		dp[2] = ((dp[2] * main_amount) + dp[-2] + dp[ny_step2o]) / f1d;

		dp -= y_step1n;
		dp[0] = ((dp[0] * main_amount) + dp[4] + dp[ny_step]) / f1d;
		dp[1] = ((dp[1] * main_amount) + dp[5] + dp[ny_step1o]) / f1d;
		dp[2] = ((dp[2] * main_amount) + dp[6] + dp[ny_step2o]) / f1d;

		for (auto dpb = UCHAR_PTR(surf.buffer + 1), dpt = UCHAR_PTR(surf.end) - y_step1n, end = dpb + y_step2n,
			spb = UCHAR_PTR(data::cache_buffer + 1), spt = UCHAR_PTR(data::cache_surface.end) - y_step1n;
			dpb < end; dpb += 4, dpt += 4, spb += 4, spt += 4)
		{
			dpb[0] = ((spb[0] * main_amount) + spb[4] + spb[-4] + spb[y_step]) / f2d;
			dpb[1] = ((spb[1] * main_amount) + spb[5] + spb[-3] + spb[y_step1o]) / f2d;
			dpb[2] = ((spb[2] * main_amount) + spb[6] + spb[-2] + spb[y_step2o]) / f2d;

			dpt[0] = ((spt[0] * main_amount) + spt[4] + spt[-4] + spt[ny_step]) / f2d;
			dpt[1] = ((spt[1] * main_amount) + spt[5] + spt[-3] + spt[ny_step1o]) / f2d;
			dpt[2] = ((spt[2] * main_amount) + spt[6] + spt[-2] + spt[ny_step2o]) / f2d;
		}

		for (auto dpb = UCHAR_PTR(surf.buffer + surf.size.x), dpt = dpb + y_step1n, end = UCHAR_PTR(surf.end - surf.size.x),
			spb = UCHAR_PTR(data::cache_buffer + surf.size.x), spt = spb + y_step1n;
			dpb < end; dpb += y_step, dpt += y_step, spb += y_step, spt += y_step)
		{
			dpb[0] = ((spb[0] * main_amount) + spb[4] + spb[y_step] + spb[ny_step]) / f2d;
			dpb[1] = ((spb[1] * main_amount) + spb[5] + spb[y_step1o] + spb[ny_step1o]) / f2d;
			dpb[2] = ((spb[2] * main_amount) + spb[6] + spb[y_step2o] + spb[ny_step2o]) / f2d;

			dpt[0] = ((spt[0] * main_amount) + spt[-4] + spt[y_step] + spt[ny_step]) / f2d;
			dpt[1] = ((spt[1] * main_amount) + spt[-3] + spt[y_step1o] + spt[ny_step1o]) / f2d;
			dpt[2] = ((spt[2] * main_amount) + spt[-2] + spt[y_step2o] + spt[ny_step2o]) / f2d;
		}

		for (auto dp = UCHAR_PTR(surf.buffer + surf.size.x + 1), end = UCHAR_PTR(surf.end - surf.size.x),
			sp = UCHAR_PTR(data::cache_buffer + surf.size.x + 1); dp < end; dp += 8, sp += 8)
		{
			for (auto x_end = dp + y_step2n; dp < x_end; dp += 4, sp += 4)
			{
				dp[0] = ((sp[0] * main_amount) + sp[4] + sp[-4] + sp[y_step] + sp[ny_step]) / f3d;
				dp[1] = ((sp[1] * main_amount) + sp[5] + sp[-3] + sp[y_step1o] + sp[ny_step1o]) / f3d;
				dp[2] = ((sp[2] * main_amount) + sp[6] + sp[-2] + sp[y_step2o] + sp[ny_step2o]) / f3d;
			}
		}
	}

	void difference(surface& s1, surface& s2, surface& s_output)
	{
		for (uchar* px1 = UCHAR_PTR(s1.buffer),
			*px2 = UCHAR_PTR(s2.buffer),
			*px_o = UCHAR_PTR(s_output.buffer);
			px1 < UCHAR_PTR(s1.end); px1++, px2++, px_o++)
			*px_o = *px1 - *px2;
	}

	inline bool is_inside(point pos, point start, point end)
	{
		return (
			pos.x >= start.x
			&& pos.x < end.x
			&& pos.y >= start.y
			&& pos.y < end.y
			);
	}

	inline bool is_inside_size(point pos, point start, point size)
	{
		return (
			pos.x >= start.x
			&& pos.y >= start.y
			&& pos.x < start.x + size.x
			&& pos.y < start.y + size.y
			);
	}

	inline bool is_inside(point pos, point lim)
	{
		return (
			pos.x >= 0
			&& pos.y >= 0
			&& pos.x < lim.x
			&& pos.y < lim.y
			);
	}

	inline color_t* get_raw_pixel(point pos, surface& surf) { return &surf.buffer[pos.x + pos.y * surf.size.x]; }
	inline color_t get_raw_color(point pos, surface& surf) { return surf.buffer[pos.x + pos.y * surf.size.x]; }

	inline color_t* get_pixel(point pos, surface& surf)
	{
		return is_inside(pos, surf.size) ? get_raw_pixel(pos, surf) : nullptr;
	}

	inline color_t get_color(point pos, surface& surf)
	{
		return is_inside(pos, surf.size) ? get_raw_color(pos, surf) : colors::transparent;
	}

	inline void set_sure_pixel(point pos, color_t color, surface& surf)
	{
		*get_raw_pixel(pos, surf) = color;
	}

	inline void set_pixel(point pos, color_t color, surface& surf)
	{
		if (is_inside(pos, surf.size))
			*get_raw_pixel(pos, surf) = color;
	}

	inline void set_pixel(point pos, surface& surf)
	{
		if (is_inside(pos, surf.size))
		{
			color_t* c = get_raw_pixel(pos, surf);
			*c = *c;
		}
	}

	inline bool straighten_line(point start, point& end)
	{
		if (abs(end.y - start.y) > abs(end.x - start.x))
		{
			end.x = start.x;
			return true;
		}
		end.y = start.y;
		return false;
	}

	void resize_surface(surface& src, surface& dest)
	{
		if (src.size == dest.size) return src >> dest;

		auto [mx, my] = src.size / dest.size;
		auto [evx, evy] = src.size % dest.size;
		my = (my - 1) * src.size.x;

		int errx, erry = evy;
		const color_t* sp = src.buffer;

		for (color_t* dp = dest.buffer; dp < dest.end; sp += my, erry += evy)
		{
			errx = evx;
			for (const color_t* x_end = dp + dest.size.x; dp < x_end; ++dp, errx += evx, sp += mx)
			{
				*dp = *sp;
				if (errx >= dest.size.x)
				{
					++sp;
					errx -= dest.size.x;
				}
			}

			if (erry >= dest.size.y)
			{
				sp += src.size.x;
				erry -= dest.size.y;
			}
		}
	}

	constexpr option_t BLIT_SIGNATURE = 0B10;

	void _blit_cut_surface(surface& bs, surface& ss, point pos, uchar alpha, bool check)
	{
		point start = pos, size = ss.size;

		if (check)
		{
			size += pos;
			clamp_to_surface(start, bs);
			clamp_to_surface(size, bs);
			if (start ^= size) return;
			size -= start;
		}

		color_t* bs_p = get_raw_pixel(start, bs), * ss_p = get_raw_pixel(start - pos, ss);

		const int ysb = bs.size.x - size.x, yss = ss.size.x - size.x;

		color_t*& src = alpha ? ss_p : bs_p;
		color_t*& dest = alpha ? bs_p : ss_p;

		if (alpha & 0b1)
		{
			for (color_t* ss_end = ss_p + ss.size.x * size.y; ss_p < ss_end; ss_p += yss, bs_p += ysb)
				for (color_t* ss_x_end = ss_p + size.x * size.y; ss_p < ss_x_end; ++ss_p, ++bs_p)
					if (*src != colors::transparent) *dest = *src;
		}

		for (color_t* ss_end = ss_p + ss.size.x * size.y; ss_p < ss_end; ss_p += yss, bs_p += ysb)
			for (color_t* ss_x_end = ss_p + size.x * size.y; ss_p < ss_x_end; ++ss_p, ++bs_p)
				*dest = *src;
	}

	inline void blit_surface(surface& dest, surface& src, point pos, bool alpha = false, bool check = true)
	{
		_blit_cut_surface(dest, src, pos, alpha | BLIT_SIGNATURE, check);
	}

	inline void cut_surface(surface& dest, surface& src, point pos, bool alpha = false, bool check = true)
	{
		_blit_cut_surface(src, dest, pos, 0, check);
	}

	inline void _C1_alpha_set(color_t* px, uchar r, uchar g, uchar b, uchar a)
	{
		uchar* pxs = UCHAR_PTR(px);
		pxs[0] = slide_uchar(pxs[0], b, a);
		pxs[1] = slide_uchar(pxs[1], g, a);
		pxs[2] = slide_uchar(pxs[2], r, a);
	}

	namespace draw
	{
		void grid(int grid_size, point offset, color_t color, surface& surf, bool anti_aliasing = false)
		{
			offset.x = MODULO(offset.x, grid_size);
			offset.y = MODULO(offset.y, grid_size);

			int y_step = grid_size * surf.size.x;

			if (anti_aliasing)
			{
				uchar* shade = reinterpret_cast<uchar*>(&color);
				uchar cr = shade[2];
				uchar cg = shade[1];
				uchar cb = shade[0];

				color_t* spx = surf.buffer + offset.x;

				if (offset.x == 0)
				{
					for (color_t* px = spx; px < surf.end; px += surf.size.x)
					{
						px[0] = color;
						_C1_alpha_set(px + 1, cr, cg, cb, 0x20);
					}
					spx += grid_size;
				}

				for (color_t* x_end = surf.buffer + surf.size.x - grid_size; spx < x_end; spx += grid_size)
					for (color_t* px = spx; px < surf.end; px += surf.size.x)
					{
						_C1_alpha_set(px - 1, cr, cg, cb, 0x20);
						px[0] = color;
						_C1_alpha_set(px + 1, cr, cg, cb, 0x20);
					}

				if ((surf.size.x - offset.x) % grid_size == 1)
					for (color_t* px = spx; px < surf.end; px += surf.size.x)
					{
						_C1_alpha_set(px - 1, cr, cg, cb, 0x20);
						px[0] = color;
					}
				else
					for (color_t* px = spx; px < surf.end; px += surf.size.x)
					{
						_C1_alpha_set(px - 1, cr, cg, cb, 0x20);
						px[0] = color;
						_C1_alpha_set(px + 1, cr, cg, cb, 0x20);
					}

				spx = surf.buffer + offset.y * surf.size.x;

				if (offset.y == 0)
				{
					for (color_t* px = spx, *x_end = spx + surf.size.x; px < x_end; ++px)
					{
						px[0] = color;
						_C1_alpha_set(px + surf.size.x, cr, cg, cb, 0x20);
					}
					spx += y_step;
				}

				for (color_t* y_end = surf.end - y_step; spx < y_end; spx += y_step)
					for (color_t* px = spx, *x_end = spx + surf.size.x; px < x_end; ++px)
					{
						_C1_alpha_set(px - surf.size.x, cr, cg, cb, 0x20);
						px[0] = color;
						_C1_alpha_set(px + surf.size.x, cr, cg, cb, 0x20);
					}

				if ((surf.size.y - offset.y) % grid_size == 1)
					for (color_t* px = spx, *x_end = spx + surf.size.x; px < x_end; ++px)
					{
						_C1_alpha_set(px - surf.size.x, cr, cg, cb, 0x20);
						px[0] = color;
					}
				else
					for (color_t* px = spx, *x_end = spx + surf.size.x; px < x_end; ++px)
					{
						_C1_alpha_set(px - surf.size.x, cr, cg, cb, 0x20);
						px[0] = color;
						_C1_alpha_set(px + surf.size.x, cr, cg, cb, 0x20);
					}

				return;
			}

			for (color_t* spx = surf.buffer + offset.x, *x_end = surf.buffer + surf.size.x; spx < x_end; spx += grid_size)
				for (color_t* px = spx; px < surf.end; px += surf.size.x)
					*px = color;

			for (color_t* spx = surf.buffer + offset.y * surf.size.x; spx < surf.end; spx += y_step)
				for (color_t* px = spx, *x_end = spx + surf.size.x; px < x_end; ++px)
					*px = color;
		}

		void grid_dots(int grid_size, point offset, color_t color, surface& surf)
		{
			offset.x = MODULO(offset.x, grid_size);
			offset.y = MODULO(offset.y, grid_size);

			int y_step = surf.size.x * (grid_size - 1) + surf.size.x % grid_size,
				x_bias = surf.size.x - offset.x;
			for (color_t* px = get_raw_pixel(offset, surf); px < surf.end; px += y_step)
				for (color_t* x_end = px + x_bias; px < x_end; px += grid_size)
					*px = color;
		}

		void fill_rect(point start, point end, color_t color, surface& surf, uchar alpha = UINT8_MAX)
		{
			clamp_to_surface(start, surf);
			clamp_to_surface(end, surf);

			if (start ^= end) return;

			start | end;

			point size = end - start;
			uint* px = get_raw_pixel(start, surf);
			int y_step = surf.size.x - size.x;

			if (alpha == UINT8_MAX)
			{
				for (const color_t* y_end = px + size.y * surf.size.x; px < y_end; px += y_step)
					for (const color_t* x_end = px + size.x; px < x_end; ++px)
						*px = color;
				return;
			}

			y_step <<= 2;
			size <<= 2;

			auto px_ptr = UCHAR_PTR(px);
			auto color_ptr = UCHAR_PTR(&color);
			uchar b = color_ptr[0],
				g = color_ptr[1],
				r = color_ptr[2];

			for (const uchar* y_end = px_ptr + size.y * surf.size.x; px_ptr < y_end; px_ptr += y_step)
				for (const uchar* x_end = px_ptr + size.x; px_ptr < x_end; px_ptr += 4)
				{
					px_ptr[0] = slide_uchar(px_ptr[0], b, alpha);
					px_ptr[1] = slide_uchar(px_ptr[1], g, alpha);
					px_ptr[2] = slide_uchar(px_ptr[2], r, alpha);
				}
		}

		void _straight_line(int d1, int d2, int s, bool slope, color_t color, surface& surf, int dash = 0, int thickness = 1)
		{
			if (s < 0 || d1 == d2) return;

			if (thickness > 1)
			{
				int ts = thickness >> 1, te = ts + (thickness & 1);
				return slope
					? fill_rect({ s - te, d1 }, { s + ts, d2 }, color, surf)
					: fill_rect({ d1, s - te }, { d2, s + ts }, color, surf);
			}

			const point size = slope ? ~surf.size : surf.size;

			if (s >= size.y) return;
			if (d1 > d2) std::swap(d1, d2);

			int cache = d1;
			d1 = std::clamp(d1, 0, size.x - 1);
			d2 = std::clamp(d2, 0, size.x - 1);

			if (d1 == d2) return;

			point steps = slope ? point{ surf.size.x, 1 } : point{ 1, surf.size.x };

			color_t* px = surf.buffer + s * steps.y;
			color_t* const end = px + d2 * steps.x;
			px += d1 * steps.x;

			if (dash)
			{
				const int d_dash = dash << 1;
				cache = (d1 - cache) % d_dash;
				int index = 1;
				const int dash_step = dash * steps.x;

				if (cache >= dash)
					px += (d_dash - cache) * steps.x;
				else
					index = cache + 1;

				while (px < end)
				{
					*px = color;
					px += steps.x;
					if (index == dash)
					{
						index = 1;
						px += dash_step;
					}
					else ++index;
				}

				return;
			}

			while (px <= end)
			{
				*px = color;
				px += steps.x;
			}
		}

		inline void straight_line(point start, point end, color_t color, surface& surf, int dash = 0, int thickness = 1)
		{
			if (end.x == start.x)
				_straight_line(start.y, end.y, start.x, true, color, surf, dash, thickness);
			else if (end.y == start.y)
				_straight_line(start.x, end.x, start.y, false, color, surf, dash, thickness);
		}

		void line(point start, point end, color_t color, surface& surf)
		{
			point delta = end - start;

			if (delta.y == 0)
			{
				if (delta.x != 0) _straight_line(start.x, end.x, start.y, false, color, surf);
				return;
			}

			if (delta.x == 0)
			{
				if (delta.y != 0) _straight_line(start.y, end.y, start.x, true, color, surf);
				return;
			}

			bool slope = abs(delta.y) > abs(delta.x);

			if (slope)
			{
				std::swap(start.x, start.y);
				std::swap(end.x, end.y);
			}

			if (start.x > end.x)
			{
				std::swap(start.x, end.x);
				std::swap(start.y, end.y);
			}

			delta = end - start;
			int error = delta.x >> 1, abs_delta_y = abs(delta.y);
			point step = { 1, get_sign(delta.y) };
			delta.y = abs(delta.y);
			static color_t* px;

			if (slope)
			{
				// reverse get_raw_pixel(p, s);
				px = &surf.buffer[start.y + start.x * surf.size.x];
				step.x = surf.size.x;
			}
			else
			{
				px = get_raw_pixel(start, surf);
				step.y *= surf.size.x;
			}

			for (int i = start.x; i < end.x; ++i, px += step.x)
			{
				*px = color;
				error -= abs_delta_y;

				if (error < 0)
				{
					px += step.y;
					error += delta.x;
				}
			}
		}

		void circle(point center, int radius, color_t color, surface& surf)
		{
			point diff = { 0, radius };
			int d = 3 - (radius << 1);

			while (diff.y >= diff.x)
			{
				set_pixel({ center.x - diff.x, center.y - diff.y }, color, surf);
				set_pixel({ center.x - diff.x, center.y + diff.y }, color, surf);
				set_pixel({ center.x + diff.x, center.y - diff.y }, color, surf);
				set_pixel({ center.x + diff.x, center.y + diff.y }, color, surf);

				set_pixel({ center.x - diff.y, center.y - diff.x }, color, surf);
				set_pixel({ center.x - diff.y, center.y + diff.x }, color, surf);
				set_pixel({ center.x + diff.y, center.y - diff.x }, color, surf);
				set_pixel({ center.x + diff.y, center.y + diff.x }, color, surf);

				if (d < 0)
					d += (diff.x++ << 2) + 6;
				else
					d += 4 * (diff.x++ - diff.y--) + 10;
			}
		}

		void sure_circle(point center, int radius, color_t color, surface& surf)
		{
			point diff = { 0, radius };
			int d = 3 - (radius << 1);

			while (diff.y >= diff.x)
			{
				set_sure_pixel({ center.x - diff.x, center.y - diff.y }, color, surf);
				set_sure_pixel({ center.x - diff.x, center.y + diff.y }, color, surf);
				set_sure_pixel({ center.x + diff.x, center.y - diff.y }, color, surf);
				set_sure_pixel({ center.x + diff.x, center.y + diff.y }, color, surf);

				set_sure_pixel({ center.x - diff.y, center.y - diff.x }, color, surf);
				set_sure_pixel({ center.x - diff.y, center.y + diff.x }, color, surf);
				set_sure_pixel({ center.x + diff.y, center.y - diff.x }, color, surf);
				set_sure_pixel({ center.x + diff.y, center.y + diff.x }, color, surf);

				if (d < 0)
					d += (diff.x++ << 2) + 6;
				else
					d += 4 * (diff.x++ - diff.y--) + 10;
			}
		}

		inline void sure_basic_line_x(int xs, int xb, int y, color_t color, surface& surf)
		{
			color_t* px = surf.buffer + surf.size.x * y;
			color_t* const end = px + xb;
			px += xs;
			while (px < end)
			{
				*px = color;
				++px;
			}
		}

		inline void sure_basic_line_y(int ys, int yb, int x, color_t color, surface& surf)
		{
			color_t* px = surf.buffer + x; // 4
			color_t* const end = px + yb * surf.size.x; // 7
			px += ys * surf.size.x; // 7
			while (px < end)
			{
				*px = color;
				px += surf.size.x;
			}
		}

		void sure_fill_circle(point center, int radius, color_t color, surface& surf)
		{
			point diff = { 0, radius };
			int d = 3 - (radius << 1);

			while (diff.y >= diff.x)
			{
				sure_basic_line_x(center.x - diff.x, center.x + diff.x, center.y + diff.y, color, surf);
				sure_basic_line_x(center.x - diff.x, center.x + diff.x, center.y - diff.y, color, surf);
				sure_basic_line_x(center.x - diff.y, center.x + diff.y, center.y + diff.x, color, surf);
				sure_basic_line_x(center.x - diff.y, center.x + diff.y, center.y - diff.x, color, surf);

				if (d < 0)
					d += (diff.x++ << 2) + 6;
				else
					d += 4 * (diff.x++ - diff.y--) + 10;
			}
		}

		void fill_circle(point center, int radius, color_t color, surface& surf)
		{
			point diff = { 0, radius };
			int d = 3 - (radius << 1);

			while (diff.y >= diff.x)
			{
				_straight_line(center.x - diff.x, center.x + diff.x, center.y + diff.y, false, color, surf);
				_straight_line(center.x - diff.x, center.x + diff.x, center.y - diff.y, false, color, surf);
				_straight_line(center.x - diff.y, center.x + diff.y, center.y + diff.x, false, color, surf);
				_straight_line(center.x - diff.y, center.x + diff.y, center.y - diff.x, false, color, surf);

				if (d < 0)
					d += (diff.x++ << 2) + 6;
				else
					d += 4 * (diff.x++ - diff.y--) + 10;
			}
		}

		void circle(point center, int inner, int outer, color_t color, surface& surf)
		{
			int xo = outer, xi = inner, y = 0, erro = 1 - xo, erri = 1 - xi;

			while (xo >= y)
			{
				_straight_line(center.x - xo, center.x - xi, center.y - y, false, color, surf);
				_straight_line(center.x - xo, center.y - xi, center.x - y, true, color, surf);
				_straight_line(center.x - xo, center.x - xi, center.y + y, false, color, surf);
				_straight_line(center.x - xo, center.y - xi, center.x + y, true, color, surf);

				_straight_line(center.x + xi, center.x + xo, center.y - y, false, color, surf);
				_straight_line(center.x + xi, center.y + xo, center.x - y, true, color, surf);
				_straight_line(center.x + xi, center.x + xo, center.y + y, false, color, surf);
				_straight_line(center.x + xi, center.y + xo, center.x + y, true, color, surf);

				if (erro < 0)
					erro += (++y << 1) + 1;
				else
					erro += 2 * (++y - --xo + 1);

				if (y > inner)
					xi = y;
				else if (erri < 0)
					erri += (y << 1) + 1;
				else
					erri += 2 * (y - --xi + 1);
			}
		}

		inline void circle(point center, int radius, int thickness, bool inner, color_t color, surface& surf)
		{
			if (inner)
				circle(center, radius - thickness, radius, color, surf);
			else
				circle(center, radius, radius + thickness, color, surf);
		}

		inline void rect(point start, point end, color_t color, surface& surf, int dash = 0, int thickness = 1)
		{
			_straight_line(start.x, end.x, start.y, false, color, surf, dash, thickness);
			_straight_line(start.x, end.x, end.y, false, color, surf, dash, thickness);
			_straight_line(start.y, end.y, start.x, true, color, surf, dash, thickness);
			_straight_line(start.y, end.y, end.x, true, color, surf, dash, thickness);
		}

		inline void rect(point start, point end, bool b, bool t, bool l, bool r, color_t color, surface& surf, int dash = 0, int thickness = 1)
		{
			start | end;
			if (b) _straight_line(start.x, end.x, start.y, false, color, surf, dash, thickness);
			if (t) _straight_line(start.x, end.x, end.y, false, color, surf, dash, thickness);
			if (l) _straight_line(start.y, end.y, start.x, true, color, surf, dash, thickness);
			if (r) _straight_line(start.y, end.y, end.x, true, color, surf, dash, thickness);
		}

		inline void triangle(point a, point b, point c, color_t color, surface& surf)
		{
			line(a, b, color, surf);
			line(b, c, color, surf);
			line(c, a, color, surf);
		}
	}
}
