#define _CONSOLE 1
#define _FONT_ANTI_ALIASING 1

#include "NLGL.h"

LRESULT CALLBACK window_proc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param)
{
	switch (u_msg)
	{
	case WM_CLOSE:
		data::running = false;
		return 0;
	case WM_DESTROY:
		data::running = false;
		return 0;
	case WM_LBUTTONDOWN:
		data::mouse.t_left = true;
		data::mouse.left = true;
		return 0;
	case WM_LBUTTONUP:
		data::mouse.t_left = true;
		data::mouse.left = false;
		return 0;
	case WM_MBUTTONDOWN:
		data::mouse.t_middle = true;
		data::mouse.middle = true;
		return 0;
	case WM_MBUTTONUP:
		data::mouse.t_middle = true;
		data::mouse.middle = false;
		return 0;
	case WM_RBUTTONDOWN:
		data::mouse.t_right = true;
		data::mouse.right = true;
		return 0;
	case WM_RBUTTONUP:
		data::mouse.t_right = true;
		data::mouse.right = false;
		return 0;
	case WM_KEYDOWN:
		switch (w_param)
		{
		case 'C':
			v2::show_cursor ^= true;
			ShowCursor(v2::show_cursor);
			return 0;
		case 'X':
			v2::block_font ^= true;
			return 0;
		case 'D':
			v2::grid_info::debug ^= true;
			return 0;
		case 'S':
			v2::grid_info::smooth_mouse ^= true;
			return 0;
		case 'M':
			v2::grid_info::dark_mode ^= true;
			return 0;
		case 'L':
			v2::objects::clock_tick_length = (v2::objects::clock_tick_length << 1) % v2::objects::max_length;
			return 0;
		case 'T':
		{
			using namespace v2::grid_info;

			auto_tick_simulation = (auto_tick_simulation == 0 ? data::target_fps :
				(auto_tick_simulation <= 9 ? (auto_tick_simulation - 1) : (auto_tick_simulation >> 1)));

			return 0;
		}
		case 'V':
			v2::objects::delete_racism ^= true;
			return 0;
		}

		if (v2::grid_info::action)
		{
			switch (w_param)
			{
			case 'R':
				v2::objects::rotation = (v2::objects::rotation + 3) % 4;
				return 0;
			case VK_LEFT:
				data::arrow_left = true;
				if (v2::events::is_catagory_chosen)
					v2::events::chosen_catagory = (v2::events::CatagoryId)MODULO(v2::events::chosen_catagory - 1, v2::events::catagorie_amount);
				else
					v2::events::chosen_object = (v2::events::ObjectId)MODULO(v2::events::chosen_object - 1, v2::events::object_amount);
				return 0;
			case VK_RIGHT:
				data::arrow_right = true;
				if (v2::events::is_catagory_chosen)
					v2::events::chosen_catagory = (v2::events::CatagoryId)((v2::events::chosen_catagory + 1) % v2::events::catagorie_amount);
				else
					v2::events::chosen_object = (v2::events::ObjectId)((v2::events::chosen_object + 1) % v2::events::object_amount);
				return 0;
			case VK_DOWN:
				data::arrow_down = true;
				v2::events::is_catagory_chosen = MODULO(v2::events::is_catagory_chosen - 1, 2);
				return 0;
			case VK_UP:
				data::arrow_up = true;
				v2::events::is_catagory_chosen = ((v2::events::is_catagory_chosen + 1) & 1);
				return 0;
			default:
				if (w_param > '0' && w_param <= '0' + v2::events::object_amount)
					v2::events::chosen_object = (v2::events::ObjectId)(w_param - '1');
				return 0;
			}
		}
	case WM_KEYUP:
		switch (w_param)
		{
		case VK_LEFT:
			data::arrow_left = false;
			return 0;
		case VK_RIGHT:
			data::arrow_right = false;
			return 0;
		case VK_DOWN:
			data::arrow_down = false;
			return 0;
		case VK_UP:
			data::arrow_up = false;
			return 0;
		case VK_SPACE:
			data::t_space = true;
			return 0;
		default:
			return 0;
		};
	case WM_MOUSEWHEEL:
		if (v2::grid_info::action)
		{
			data::mouse.wheel = 1 - ((w_param >> 23) & 2);
		}
		return 0;
	default:
		return DefWindowProcW(hwnd, u_msg, w_param, l_param);
	}
}

int WINAPI wWinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPWSTR lpCmdLine,
	int nShowCmd
)
{
	data::init_cache_surface();
	if (font::init())
	{
		MessageBoxW(nullptr, L"'font.bin' file cant found!", L"Error", MB_OK);
		return 0;
	}

	WNDCLASSW wc = {};
	wc.lpfnWndProc = window_proc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"ICE v1.3.2 - GHMO v6.3-improved";

	if (!RegisterClassW(&wc))
	{
		MessageBoxW(nullptr, L"Failed to register Window Class!", L"Error", MB_OK);
		return 0;
	}

	HWND window = CreateWindowExW(
		0,
		wc.lpszClassName,
		wc.lpszClassName,
		WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX,
		CW_USEDEFAULT, CW_USEDEFAULT,
		screen_size.x + extra_size.x, screen_size.y + extra_size.y,
		nullptr, nullptr, hInstance, nullptr
	);

	if (window == nullptr)
	{
		UnregisterClassW(wc.lpszClassName, hInstance);

		MessageBoxW(nullptr, L"Failed to create Window!", L"Error", MB_OK);
		return 0;
	}

	bitmap_info.bmiHeader.biSize = sizeof(bitmap_info.bmiHeader);
	bitmap_info.bmiHeader.biWidth = screen_size.x;
	bitmap_info.bmiHeader.biHeight = screen_size.y;
	bitmap_info.bmiHeader.biPlanes = 1;
	bitmap_info.bmiHeader.biBitCount = 32;
	bitmap_info.bmiHeader.biCompression = BI_RGB;

	HDC hdc = GetDC(window);

	ShowWindow(window, nShowCmd);
	ShowCursor(FALSE);
	UpdateWindow(window);

	FILE* fp = nullptr;
	if constexpr (_CONSOLE)
	{
		AllocConsole();
		freopen_s(&fp, "CONOUT$", "w", stdout);
	}

	MSG msg = {};
	DWORD start_time;

	v2::grid_info::init(30);

	while (data::running)
	{
		start_time = timeGetTime();
		while (PeekMessageW(&msg, window, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessageW(&msg);
		}

		update_mouse(window);

		v2::grid_info::update();

		colors::white >> screen;

		graphics::draw::grid(v2::grid_info::size, v2::grid_info::pos, 0xF0F0F0, screen);

		graphics::draw::straight_line({ 0, v2::grid_info::pos.y },
			{ screen_size.x, v2::grid_info::pos.y }, 0xD0D0D0, screen);
		graphics::draw::straight_line({ v2::grid_info::pos.x, 0 },
			{ v2::grid_info::pos.x, screen_size.y }, 0xD0D0D0, screen);

		v2::draw_grid();

		v2::grid_info::draw();

		if (v2::events::chosen_object <= v2::events::oWire && v2::events::chosen_catagory == 0)
			v2::draw_cursor(colors::black, screen);

		v2::events::draw_gui();

		{
			int gap = 8, y_step = font::max_font_dim.y + gap, temp;
			point pos = { gap, screen_size.y - y_step };
			font::draw_string(pos, "ICE v1.3.2", colors::dark_green, screen);

			// Delta-Time
			pos.y -= y_step;
			font::draw_string(pos, "Performance:", colors::black, screen);
			pos.x += font::mfdim_xinc * 13;
			temp = font::int_to_string(data::delta_time);
			font::draw_string(pos, data::string_buffer, colors::dark_blue, screen);
			pos.x += temp * font::mfdim_xinc;
			font::unsafe_draw_char('/', pos, colors::black, screen);
			pos.x += font::mfdim_xinc;
			font::int_to_string(data::target_frame_time);
			font::draw_string(pos, data::string_buffer, colors::dark_blue, screen);

			// Tick
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Tick:", colors::black, screen);
			pos.x += font::mfdim_xinc * 6;
			font::int_to_string(data::_int_tick);
			font::draw_string(pos, data::string_buffer, colors::dark_blue, screen);

			// Block Font & Show Cursor
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Block Font {X}", colors::black, screen);
			pos.y -= y_step;
			font::draw_string(pos, v2::show_cursor ? "Hide Cursor {C}" : "Show Cursor {C}", colors::black, screen);

			// Camera Pos
			pos.y -= y_step;
			font::draw_string(pos, "Pos:", colors::black, screen);
			pos.x += font::mfdim_xinc * 5;
			v2::draw_point_string(pos, -v2::grid_info::pos, colors::black, colors::blue, screen);

			// Mouse Pos
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Mouse:", colors::black, screen);
			pos.x += font::mfdim_xinc * 7;
			temp = font::int_to_string(v2::grid_info::size);
			font::draw_string(pos, data::string_buffer, colors::dark_blue, screen);
			pos.x += temp * font::mfdim_xinc;
			font::unsafe_draw_char('*', pos, colors::black, screen);
			pos.x += font::mfdim_xinc;
			v2::draw_point_string(pos, v2::grid_info::mouse_world, colors::black, colors::blue, screen);

			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Rotation {R}:", colors::black, screen);
			pos.x += font::mfdim_xinc * 14;
			font::draw_string(pos, "0b", colors::black, screen);
			pos.x += font::mfdim_xincd;
			font::int_to_string(v2::objects::rotation, 2, 2);
			font::draw_string(pos, data::string_buffer, colors::dark_blue, screen);

			// Debug
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Debug {D}:", colors::dark_orange, screen);
			pos.x += font::mfdim_xinc * 11;
			font::bool_to_string(v2::grid_info::debug);
			font::draw_string(pos, data::string_buffer, colors::dark_green, screen);

			// Debug
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Smooth Mouse {S}:", 0x00B080, screen);
			pos.x += font::mfdim_xinc * 18;
			font::bool_to_string(v2::grid_info::smooth_mouse);
			font::draw_string(pos, data::string_buffer, 0xA00E50, screen);

			// Auto Simulate
			pos.y -= y_step << 1;
			pos.x = gap;
			font::draw_string(pos, "Tick amount:", colors::black, screen);
			pos.x += font::mfdim_xinc * 13;
			temp = font::int_to_string(v2::objects::tick::tick_counter);
			font::draw_string(pos, data::string_buffer, colors::red, screen);
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "{Space} for Tick.", colors::dark_gray, screen);

			pos.y -= y_step;
			font::draw_string(pos, "Auto Tick Simulation {T}...", colors::dark_gray, screen);
			pos.y -= y_step;
			font::draw_string(pos, "once in every '", colors::dark_gray, screen);
			pos.x += font::mfdim_xinc * 15;
			temp = font::int_to_string(v2::grid_info::auto_tick_simulation);
			font::draw_string(pos, data::string_buffer, colors::red, screen);
			pos.x += temp * font::mfdim_xinc;
			font::draw_string(pos, "' tick.", colors::dark_gray, screen);

			// Delete Racism
			pos.y -= y_step << 1;
			pos.x = gap;
			font::draw_string(pos, "Type Delete {V}:", colors::black, screen);
			pos.x += font::mfdim_xinc * 17;
			font::bool_to_string(v2::objects::delete_racism);
			font::draw_string(pos, data::string_buffer, colors::red, screen);

			// Dark Mode
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Dark Mode {M}:", colors::dark_gray, screen);
			pos.x += font::mfdim_xinc * 15;
			font::bool_to_string(v2::grid_info::dark_mode);
			font::draw_string(pos, data::string_buffer, colors::red, screen);

			// Clock Tick Length
			pos.y -= y_step;
			pos.x = gap;
			font::draw_string(pos, "Clock Speed {L}:", colors::black, screen);
			pos.x += font::mfdim_xinc * 17;
			font::int_to_string(v2::objects::clock_tick_length);
			font::draw_string(pos, data::string_buffer, colors::red, screen);
		}

		if (v2::grid_info::dark_mode)
			graphics::reverse_colors(screen);

		update(hdc, &bitmap_info, start_time);
	}

	ReleaseDC(window, hdc);
	DestroyWindow(window);
	UnregisterClassW(wc.lpszClassName, hInstance);

	if constexpr (_CONSOLE)
	{
		if (fp) fclose(fp);
		FreeConsole();
	}

	free(screen.buffer);
	screen.buffer = nullptr;
	data::clean_up_cache();
	font::clean_up();

	return 0;
}