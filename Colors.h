#pragma once

using color_t = unsigned int;

namespace colors
{
	constexpr color_t
		alpha = 0xFF000000,
		black = 0,
		transparent = black,
		visiable_transparent = alpha & transparent,
		gray = 0x808080,
		white = 0xFFFFFF,
		red = 0xFF0000,
		green = 0xFF00,
		blue = 0xFF,
		cyan = 0xFFFF,
		purple = 0xFF00FF,
		yellow = 0xFFFF00,
		orange = 0xFF8000,
		dark_orange = 0x804000,
		lime = 0x80FF00,
		dark_red = 0x800000,
		dark_green = 0x8000,
		dark_blue = 0x80,
		cyan_ish = ~dark_red,
		pink = ~dark_green,
		yellow_ish = ~dark_blue,
		slight_blue = 0x7070B0,
		blueish_gray = 0xA8A0B0,
		light3_4 = 0xC0C0C0,
		light = 0xDCDCDC,
		lighter = 0xE2E2E2,
		light_gray = 0xA0A0A0,
		cannot_c = 0xB060A0,
		dark_gray = 0x404040,
		sudayuzennutellarengi = 0x479111;
}
