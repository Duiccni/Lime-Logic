#pragma once

#include "Graphics.h"

const char* ascii_text = R"(!"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\]^_`abcdefghijklmnopqrstuvwxyz{|}~)";

namespace data
{
	char* string_buffer;
	constexpr int string_buffer_size = 0x100;
}

#define COPY_CHAR_SURF(src, dest) memcpy(dest.buffer, src. buffer, src.buffer_size)
#define ASCII_TAB 9
#define HEX_BASE 0x10
#define DECIMAL_BASE 10

#ifndef _FONT_ANTI_ALIASING
#define _FONT_ANTI_ALIASING 0
#endif // _FONT_ANTI_ALIASING

namespace v2
{
	bool block_font = false;
}

namespace font
{
	const char* font_path = "font.bin";

	constexpr size_t buffer_size = 9091;
	constexpr int chars_size = 94;
	const double ln10 = log(10), ln16 = log(16);

	constexpr point max_font_dim = { 14, 19 };
	constexpr int font_size_hy = max_font_dim.y >> 1,
		mfdim_xinc = max_font_dim.x + 1,
		mfdim_xincd = mfdim_xinc << 1,
		mfdim_xincq = mfdim_xincd << 1;

	struct character_surface
	{
		uchar* buffer, * end;
		point size;
		char y_bias;
		size_t buffer_size;
	};

	using ccsr = const character_surface&;

	character_surface* characters;
	character_surface* end_characters;
	uchar* file_buffer;

	inline void bool_to_string(bool value)
	{
		strcpy_s(data::string_buffer, 6, value ? "True" : "False");
	}

	inline void lowercase_bool_to_string(bool value)
	{
		strcpy_s(data::string_buffer, 6, value ? "true" : "false");
	}

	inline char int_or_hex(char c)
	{
		static constexpr char hex_offset = 'a' - 10;
		return c <= 9 ? c + '0' : c + hex_offset;
	}

	int int_to_string(int value, int base = DECIMAL_BASE, int fill_zeros = 0, bool force_fill = true)
	{
		if ((value | fill_zeros) == 0)
		{
			data::string_buffer[0] = '0';
			data::string_buffer[1] = '\0';
			return 1;
		}

		bool sign = value >> 31;
		if (sign)
		{
			value = -value;
			data::string_buffer[0] = '-';
		}

		int length;
		if (fill_zeros && force_fill)
			length = fill_zeros;
		else
		{
			double logbase = (base == DECIMAL_BASE ? ln10 : (base == HEX_BASE ? ln16 : log(base)));
			length = static_cast<int>(log(value) / logbase) + 1;
			if (fill_zeros != 0 && fill_zeros > length)
				length = fill_zeros;
		}
		length += sign;

		char* addr = data::string_buffer + length;

		*addr = '\0';
		while (value > 0)
		{
			*--addr = int_or_hex(value % base);
			value /= base;
		}

		char* end = data::string_buffer + sign;
		while (addr > end)
			*--addr = '0';

		return length;
	}

	void reverse_int_to_string(int value, int base = DECIMAL_BASE)
	{
		if (value == 0)
		{
			data::string_buffer[0] = '0';
			data::string_buffer[1] = '\0';
			return;
		}

		bool sign = value >> 31;
		if (sign) value = -value;

		char* addr = data::string_buffer;

		while (value > 0)
		{
			*addr++ = int_or_hex(value % base);
			value /= base;
		}

		if (sign) *addr++ = '-';
		*addr = '\0';
	}

	void characer_slight_blur(ccsr surf)
	{
		auto cache_buffer_end = data::cache_shade + surf.buffer_size;
		memcpy(data::cache_shade, surf.buffer, surf.buffer_size);

		const int y_step = surf.size.x, ny_step = -y_step,
			y_step1n = y_step - 1, y_step2n = y_step1n - 1;

		// Bottom & Top Middle
		for (auto dpb = surf.buffer + 1,
			dpt = surf.end - y_step1n,
			end = dpb + y_step2n,
			spb = data::cache_shade + 1,
			spt = cache_buffer_end - y_step1n;
			dpb < end; ++dpb, ++dpt, ++spb, ++spt)
		{
			dpb[0] = ((spb[0] << 3) + spb[1] + spb[-1] + spb[y_step]) / 11;
			dpt[0] = ((spt[0] << 3) + spt[1] + spt[-1] + spt[ny_step]) / 11;
		}

		// Left & Right Middle
		for (auto dpb = surf.buffer + surf.size.x,
			dpt = dpb + y_step1n,
			end = surf.end - surf.size.x,
			spb = data::cache_shade + surf.size.x,
			spt = spb + y_step1n;
			dpb < end; dpb += y_step, dpt += y_step, spb += y_step, spt += y_step)
		{
			dpb[0] = ((spb[0] << 3) + spb[1] + spb[y_step] + spb[ny_step]) / 11;
			dpt[0] = ((spt[0] << 3) + spt[-1] + spt[y_step] + spt[ny_step]) / 11;
		}

		// Middle
		for (auto dp = surf.buffer + surf.size.x + 1,
			end = surf.end - surf.size.x,
			sp = data::cache_shade + surf.size.x + 1;
			dp < end; dp += 2, sp += 2)
			for (auto x_end = dp + y_step2n; dp < x_end; ++dp, ++sp)
				dp[0] = ((sp[0] << 3) + sp[1] + sp[-1] + sp[y_step] + sp[ny_step]) / 12;
	}

	bool init()
	{
		std::ifstream file(font_path, std::ios::binary);
		if (file.is_open() == false)
			return true;
		file_buffer = UCHAR_PTR(malloc(buffer_size));
		file.read(reinterpret_cast<char*>(file_buffer), buffer_size);
		file.close();

		data::string_buffer = static_cast<char*>(malloc(data::string_buffer_size));

		characters = TYPE_MALLOC(character_surface, chars_size);
		end_characters = characters + chars_size;

		uchar* _byte = file_buffer;
		for (character_surface* current = characters; current < end_characters; ++current)
		{
			int rsx = *_byte;
			current->size.x = rsx + 2;
			current->size.y = *++_byte + 2;
			current->y_bias = *++_byte;
			current->buffer_size = SIZE_MUL(current->size.x, current->size.y);

			current->buffer = UCHAR_PTR(calloc(current->buffer_size, 1));
			current->end = current->buffer + current->buffer_size;

			++_byte;
			for (uchar* px = current->buffer + current->size.x + 1, *end = current->end - current->size.x; px < end; px += 2)
				for (uchar* x_end = px + rsx; px < x_end; ++px, ++_byte)
					*px = *_byte * UCHAR_MAX;

			if constexpr (_FONT_ANTI_ALIASING == 1)
				characer_slight_blur(*current);
		}

		return false;
	}

	void clean_up()
	{
		free(file_buffer);
		file_buffer = nullptr;
		free(data::string_buffer);
		data::string_buffer = nullptr;
		for (character_surface* current = characters; current < end_characters; ++current)
		{
			free(current->buffer);
			current->buffer = nullptr;
		}
		free(characters);
		characters = nullptr;
	}

	void unsafe_draw_char(char c, point pos, color_t color, graphics::surface& dest)
	{
		if (c <= ' ') return;

		character_surface* char_surf = characters + c - 33;

		if (v2::block_font)
		{
			point end = pos + font::max_font_dim;
			if (color == colors::black) color = colors::cannot_c;
			graphics::draw::fill_rect({ pos.x + char_surf->size.x, pos.y }, end, color, dest);
			graphics::draw::fill_rect({ pos.x, pos.y + char_surf->size.y }, { pos.x + char_surf->size.x, end.y }, color, dest);
			graphics::draw::fill_rect(pos, pos + char_surf->size, colors::black, dest);
			return;
		}

		auto color_ptr = UCHAR_PTR(&color);
		uchar b = color_ptr[0],
			g = color_ptr[1],
			r = color_ptr[2];

		pos.y += char_surf->y_bias;
		uchar* dp = UCHAR_PTR(graphics::get_raw_pixel(pos, dest));

		int y_step = (dest.size.x - char_surf->size.x) << 2;

		for (uchar* sp = char_surf->buffer; sp < char_surf->end; dp += y_step)
			for (uchar* x_end = sp + char_surf->size.x; sp < x_end; ++sp, dp += 4)
			{
				dp[0] = slide_uchar(dp[0], b, *sp);
				dp[1] = slide_uchar(dp[1], g, *sp);
				dp[2] = slide_uchar(dp[2], r, *sp);
			}
	}

	void draw_string(point pos, const char* str, color_t color, graphics::surface& dest)
	{
		if (pos.y < 0 || pos.y >= dest.size.y) return;

		if (pos.x < 0)
		{
			int start_bias = (max_font_dim.x - pos.x) / mfdim_xinc;
			pos.x += mfdim_xinc * start_bias;
			str += start_bias;
		}

		for (int x_lim = dest.size.x - mfdim_xinc; *str != '\0'; ++str)
		{
			if (pos.x >= x_lim) return;
			unsafe_draw_char(*str, pos, color, dest);
			if (*str == ASCII_TAB) pos.x += mfdim_xincq;
			else pos.x += mfdim_xinc;
		}
	}

	inline void reverse_unsafe_draw_string(point pos, const char* str, color_t color, graphics::surface& dest)
	{
		while (*str != '\0')
		{
			unsafe_draw_char(*str, pos, color, dest);
			pos.x -= mfdim_xinc;
			++str;
		}
	}
}
