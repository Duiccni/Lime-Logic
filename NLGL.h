#pragma once

#include <Windows.h>
#include <list>

#pragma comment(lib, "Winmm.lib")

#include "Graphics.h"
#include "Colors.h"
#include "Font.h"

constexpr point screen_size = { 1920, 1080 }, half_screen_size = screen_size >> 1;
constexpr point screen_minus = screen_size - 1;
constexpr point screen_step = { 1, screen_size.x }, rev_screen_step = { screen_size.x, 1 };

#define POWER_C(p) ((p) ? colors::green : colors::red)
#define ALT_POWER_C(p) ((p) ? colors::orange : colors::slight_blue)
#define PTR_2U64(ptr) (reinterpret_cast<uint64_t>(ptr))
#define CHECK_LBGATE_TYPE (events::chosen_catagory == events::cExtra ?\
((objects::last_basic_gate->data&_GLOBAL_TYPE_SIGN) == (events::extra_chosen_object<<2))\
: ((objects::last_basic_gate->data&_TYPE_SIGN) == (events::chosen_object-events::oInput)<<2))

graphics::surface screen = graphics::surface(screen_size);

struct s_mouse
{
	POINT win_pos;
	point pos;
	point old_pos;
	point delta;
	int wheel;
	bool in_screen;
	bool left, middle, right;
	bool t_left, t_middle, t_right;
};

namespace data
{
	constexpr DWORD target_fps = 60, target_frame_time = 1000 / target_fps, h_target_fps = target_fps >> 1;
	DWORD tick = 0, delta_time = target_frame_time, performance = 0;
	int _int_tick = 0;
	bool running = true;

	s_mouse mouse = {};

	bool arrow_left = false, arrow_right = false, arrow_down = false, arrow_up = false;
	bool t_space = false;
	point arrows_vel = { 0, 0 };
}

constexpr point extra_size = { 16, 39 };

BITMAPINFO bitmap_info;

void update_mouse(HWND hwnd)
{
	GetCursorPos(&data::mouse.win_pos);
	ScreenToClient(hwnd, &data::mouse.win_pos);
	data::mouse.old_pos = data::mouse.pos;
	data::mouse.pos = { data::mouse.win_pos.x, screen.size.y - data::mouse.win_pos.y };
	data::mouse.delta = data::mouse.pos - data::mouse.old_pos;
	data::mouse.in_screen = graphics::is_inside(data::mouse.pos, screen_size);
}

void refresh_mouse()
{
	data::mouse.t_left = false;
	data::mouse.t_middle = false;
	data::mouse.t_right = false;
	data::mouse.wheel = 0;
}

void update(HDC hdc, BITMAPINFO* bitmap_info, DWORD start_time)
{
	StretchDIBits(
		hdc,
		0, 0, screen_size.x, screen_size.y,
		0, 0, screen_size.x, screen_size.y,
		screen.buffer, bitmap_info,
		DIB_RGB_COLORS, SRCCOPY
	);

	refresh_mouse();
	data::t_space = false;

	data::performance = timeGetTime() - start_time;
	if (data::performance < data::target_frame_time)
	{
		Sleep(data::target_frame_time - data::performance);
		data::delta_time = data::target_frame_time;
	}
	else data::delta_time = data::performance;
	++data::tick;
	data::_int_tick = data::tick;
}

// modulo for two values (point struct) including negative values
inline void point_MODULO(point& p, int x)
{
	p.x = MODULO(p.x, x);
	p.y = MODULO(p.y, x);
}

#define GRID_MAX_SIZE 128
#define GRID_MIN_SIZE 8
#define DEFAULT_CLOCK_TICK_PER_TICK 5

namespace v2
{
	namespace events
	{
		point world_start, world_end, screen_start;
		point world_low, world_high;
		bool steep;

		int object_amount = 7;
		enum ObjectId
		{
			oHand,
			oInt,
			oWire,
			oInput,
			oOutput,
			oNot,
			oAnd
		};
		ObjectId chosen_object = oHand;

		int extra_object_amount = 1;
		enum ExtraObjId
		{
			eoClock,
		};
		ExtraObjId extra_chosen_object = eoClock;

		int catagorie_amount = 2;
		enum CatagoryId
		{
			cBasic,
			cExtra,
		};
		CatagoryId chosen_catagory = cBasic;

		int is_catagory_chosen = 0;

		const char* object_strings[] = { "Hand", "Int", "Wire", "Input", "Output", "Not", "And" };
		const char* extra_object_strings[] = { "Clock" };
		const char* catagories_strings[] = { "Basic", "Extra" };
		int catagories_long[] = { 5, 5 };
		int objects_long[] = { 4, 3, 4, 5, 6, 3, 3 };
		int extra_objects_long[] = { 5 };
		
		namespace gui_info
		{
			int start_gap = 40, outside_gap = 15, inside_gap = 5, inside_gapd = inside_gap << 1, IDsO = gui_info::inside_gapd + gui_info::outside_gap;
		}

		void draw_gui()
		{
			point pos = { gui_info::start_gap, gui_info::outside_gap }, cache, start;
			int temp, length;

			static int Ysize = gui_info::inside_gapd + font::max_font_dim.y, secYstart = (gui_info::outside_gap << 1) + Ysize;

			graphics::draw::sure_circle({ 20, (is_catagory_chosen ? secYstart : gui_info::outside_gap)
				+ (Ysize >> 1) }, 7, colors::black, screen);

			int gui_obj_amount, * str_len_list, chosen;
			const char** str_list;

			if (chosen_catagory == cExtra)
			{
				gui_obj_amount = extra_object_amount;
				str_len_list = extra_objects_long;
				chosen = extra_chosen_object;
				str_list = extra_object_strings;
			}
			else
			{
				gui_obj_amount = object_amount;
				str_len_list = objects_long;
				chosen = chosen_object;
				str_list = object_strings;
			}

			for (int i = 0; i < gui_obj_amount; i++)
			{
				temp = pos.x;
				length = font::int_to_string(i + 1);
				pos.x += font::mfdim_xinc * (str_len_list[i] + length + 1) - 1;
				cache = pos + point{ gui_info::inside_gapd, gui_info::inside_gapd + font::max_font_dim.y };
				graphics::draw::fill_rect({ temp + 1, pos.y + 1 }, cache, i == chosen ? colors::slight_blue : colors::blueish_gray, screen);
				graphics::draw::rect({ temp, pos.y }, cache, colors::black, screen);
				start = { temp + gui_info::inside_gap, pos.y + gui_info::inside_gap };
				font::draw_string(start, data::string_buffer, colors::black, screen);
				start.x += length * font::mfdim_xinc;
				font::draw_string(start, ".", colors::black, screen);
				start.x += font::mfdim_xinc;
				font::draw_string(start, str_list[i], colors::black, screen);
				pos.x += gui_info::IDsO;
			}

			pos = { gui_info::start_gap, secYstart };

			for (int i = 0; i < catagorie_amount; i++)
			{
				temp = pos.x;
				length = font::int_to_string(i + 1);
				pos.x += font::mfdim_xinc * (catagories_long[i] + length + 1) - 1;
				cache = pos + point{ gui_info::inside_gapd, gui_info::inside_gapd + font::max_font_dim.y };
				graphics::draw::fill_rect({ temp + 1, pos.y + 1 }, cache, i == chosen_catagory ? colors::sudayuzennutellarengi : colors::blueish_gray, screen);
				graphics::draw::rect({ temp, pos.y }, cache, colors::black, screen);
				start = { temp + gui_info::inside_gap, pos.y + gui_info::inside_gap };
				font::draw_string(start, data::string_buffer, colors::black, screen);
				start.x += length * font::mfdim_xinc;
				font::draw_string(start, ".", colors::black, screen);
				start.x += font::mfdim_xinc;
				font::draw_string(start, catagories_strings[i], colors::black, screen);
				pos.x += gui_info::IDsO;
			}
		}
	}

	bool show_cursor = false;

	namespace grid_info
	{
		point old_pos, pos, mod, mod_nh, mod_ph, mouse_world, mouse_round;
		int old_size, size, h_size, q_size, qq_size, qh_size, q_qh_size, y_step, h_leak;
	}

	inline point round_grid(point s)
	{
		return s + grid_info::h_size - (s - grid_info::mod_ph) % grid_info::size;
	}

	inline point to_world(point s)
	{
		s -= grid_info::pos;
		return (s + grid_info::h_size * get_sign(s)) / grid_info::size;
	}

	inline point to_screen(point w)
	{
		return w * grid_info::size + grid_info::pos;
	}

	inline point rotation_to_vector(uchar rot)
	{
		point a = { (rot & 0b10) - 1, 0 };
		*(uint64_t*)&a.x <<= (rot & 1) << 5;
		return a;
	}

	inline void print_binary(uint64_t value, int size)
	{
		for (int i = size - 1; i >= 0; i--)
			std::cout << ((value >> i) & 1);
	}

	constexpr option_t
		_CLOCK_OBJ = 0b10000000,
		_EXTRA_OBJ_SIGN = _CLOCK_OBJ,
		_GLOBAL_TYPE_SIGN = 0b1100,
		_TYPE_SIGN = _EXTRA_OBJ_SIGN | _GLOBAL_TYPE_SIGN,
		_POWER = 0b10000,
		_SHIFT_POWER = 4,
		_POWER_OUT = _POWER,
		_ANTI_POWER = ~_POWER,
		_POWER_INP1 = 0b100000,
		_CLOCK_ON_OFF = _POWER_INP1,
		_CLOCK_ON = 0,
		_CLOCK_OFF = _POWER_INP1,
		_POWER_INP2 = 0b1000000,
		_GLOBAL_INPUT = 0b0000,
		_INPUT = _GLOBAL_INPUT,
		_OUTPUT = 0b0100,
		_NOT_GATE = 0b1000,
		_AND_GATE = 0b1100;

	void draw_not_gate(point pos, int data, color_t cm)
	{
		point rot_value = rotation_to_vector(data);
		point cache = (~rot_value) * grid_info::h_size;
		point p1 = pos + rot_value * grid_info::size, p2 = cache + pos;
		point low = p1 - cache, high = p2;
		low | high;

		if (graphics::is_inside(low, screen_size) && graphics::is_inside(high, screen_size))
		{
			graphics::draw::triangle(p1, p2, pos - cache, cm, screen); // 1 out of 3 is straight auto
			graphics::draw::circle(p1, grid_info::qh_size, POWER_C(data & _POWER_OUT), screen);
			graphics::draw::circle(pos, grid_info::qh_size, POWER_C(data & _POWER_INP1), screen);
		}
	}

	void draw_simple_gate(point pos, int data, color_t cm)
	{
		point rot_value = rotation_to_vector(data);
		point cache = rotation_to_vector(data + 1) * grid_info::size;
		point p1 = pos + rot_value * grid_info::size, p2 = pos + cache;
		point low = p1, high = p2;
		low | high;

		if (graphics::is_inside(low, screen_size) && graphics::is_inside(high, screen_size))
		{
			point cache2 = rot_value * grid_info::h_size;
			point p3 = p2 + cache2;

			graphics::draw::line(p2, p3, cm, screen);
			graphics::draw::line(p2, pos, cm, screen);
			graphics::draw::line(p3, p1, cm, screen);
			graphics::draw::line(pos, p1, cm, screen);
			graphics::draw::circle(p1, grid_info::qh_size, POWER_C(data & _POWER_OUT), screen);
			graphics::draw::circle(p2, grid_info::qh_size, POWER_C(data & _POWER_INP1), screen);
			graphics::draw::circle(pos, grid_info::qh_size, POWER_C(data & _POWER_INP2), screen);
		}
	}

	namespace objects
	{
		struct wire
		{
			int id;
			point world_low, world_high, screen_low, screen_high;
			bool slope, power; //, old_power;

			wire* low_wire, * high_wire;
			std::list<wire*> connected_wires;

			color_t* start_px, * end_px;
			bool out_of_screen;
		};

		struct basic_gate
		{
			int id;
			point pos;
			option_t data; // 0b<extra-object????><power-inp2><power-inp1><power(main-output)><IO:gate?><type><sign-rot><slope-rot>

			wire* common_wire;

			uint64_t info_ptr;
		};

		struct not_gate_info
		{
			point output;
			wire* output_wire;
		};

		not_gate_info* last_not_gi = nullptr;

		struct and_gate_info
		{
			point output;
			wire* output_wire;
			point input;
			wire* input_wire;
		};

		and_gate_info* last_and_gi = nullptr;

		int next_wire_id = 0, next_basic_gate_id = 0;
		option_t rotation = 0b00;
		wire* last_wire = nullptr;
		basic_gate* last_basic_gate = nullptr;
		std::list<wire*> wires;
		std::list<basic_gate*> basic_gates;

		int clock_tick_length = DEFAULT_CLOCK_TICK_PER_TICK;
		static constexpr int max_length = (DEFAULT_CLOCK_TICK_PER_TICK << 7) - DEFAULT_CLOCK_TICK_PER_TICK;

		inline void calc_wire_rendering_values(wire* w)
		{
			w->screen_low = to_screen(w->world_low);

			if (w->slope)
			{
				if (w->screen_low.x < 0 || w->screen_low.x >= screen_minus.x)
				{
					w->out_of_screen = true;
					return;
				}

				w->screen_low.y = std::clamp(w->screen_low.y, 0, screen_minus.y);
				w->screen_high.y = std::clamp(w->world_high.y * grid_info::size + grid_info::pos.y, 0, screen_minus.y);
				w->screen_high.x = w->screen_low.x;

				if (w->screen_low.y == w->screen_high.y)
				{
					w->out_of_screen = true;
					return;
				}

				w->start_px = &screen.buffer[w->screen_low.y * screen_size.x + w->screen_low.x];
				w->end_px = &screen.buffer[w->screen_high.y * screen_size.x + w->screen_low.x];
			}
			else
			{
				if (w->screen_low.y < 0 || w->screen_low.y >= screen_minus.y)
				{
					w->out_of_screen = true;
					return;
				}

				w->screen_low.x = std::clamp(w->screen_low.x, 0, screen_minus.x);
				w->screen_high.x = std::clamp(w->world_high.x * grid_info::size + grid_info::pos.x, 0, screen_minus.x);
				w->screen_high.y = w->screen_low.y;

				if (w->screen_low.x == w->screen_high.x)
				{
					w->out_of_screen = true;
					return;
				}

				int tmp = w->screen_low.y * screen_size.x;
				w->start_px = &screen.buffer[w->screen_low.x + tmp];
				w->end_px = &screen.buffer[w->screen_high.x + tmp];
			}

			w->out_of_screen = false;
		}

		namespace tick
		{
			/*
			inline void replace_wires_old_powers()
			{
				for (objects::wire* w : objects::wires)
					w->old_power = w->power;
			}
			*/

			// anti-loop
			std::list<int> already_paint;
			bool power_temp;

			DWORD tick_counter = 0;

			void _Thread_spread_wire_power(wire* w)
			{
				if (std::find(already_paint.begin(), already_paint.end(), w->id) != already_paint.end())
					return;

				w->power = power_temp;
				already_paint.push_back(w->id);

				// std::cout << " - " << w->id << '\n';

				if (w->low_wire)
					_Thread_spread_wire_power(w->low_wire);
				if (w->high_wire)
					_Thread_spread_wire_power(w->high_wire);

				for (wire* cw : w->connected_wires)
					_Thread_spread_wire_power(cw);
			}

			void spread_wire_power(wire* w)
			{
				power_temp = w->power;

				// std::cout << power_temp << '\n';

				already_paint.clear();

				_Thread_spread_wire_power(w);
			}

			// Includes Clock
			void spread_inputs_powers()
			{
				for (objects::basic_gate* i : objects::basic_gates)
				{
					// if ((i->data & (_GLOBAL_TYPE_SIGN | _CLOCK_ON_OFF)) == (_INPUT | _CLOCK_ON) && i->common_wire)
					// which equals to (cuz _CLOCK_ON is actualy Zero 0) ->
					if ((i->data & (_GLOBAL_TYPE_SIGN | _CLOCK_ON_OFF)) == _INPUT && i->common_wire)
					{
						power_temp = (i->data >> _SHIFT_POWER) & 1;
						already_paint.clear();
						_Thread_spread_wire_power(i->common_wire);
					}
				}
			}

			void get_wire_to_outputs()
			{
				for (objects::basic_gate* i : objects::basic_gates)
					if ((i->data & _TYPE_SIGN) == _OUTPUT && i->common_wire)
						i->data = (i->common_wire->power << _SHIFT_POWER) | (i->data & _ANTI_POWER);
			}

			void run_gates()
			{
				for (objects::basic_gate* b : objects::basic_gates)
				{
					if (b->common_wire)
					{
						switch (b->data & _TYPE_SIGN)
						{
						case _NOT_GATE:
							last_not_gi = reinterpret_cast<not_gate_info*>(b->info_ptr);
							if (last_not_gi->output_wire)
							{
								power_temp = !b->common_wire->power;
								already_paint.clear();
								_Thread_spread_wire_power(last_not_gi->output_wire);
							}
							break;
						case _AND_GATE:
							last_and_gi = reinterpret_cast<and_gate_info*>(b->info_ptr);
							if (last_and_gi->input_wire && last_and_gi->output_wire)
							{
								power_temp = (b->common_wire->power & last_and_gi->input_wire->power);
								_Thread_spread_wire_power(last_and_gi->output_wire);
								already_paint.clear();
							}
							break;
						}
					}
				}
			}

			void tick()
			{
				++tick_counter;

				if (tick_counter % clock_tick_length == 0)
				{
					for (objects::basic_gate* b : objects::basic_gates)
					{
						if ((b->data & _TYPE_SIGN) == _CLOCK_OBJ && (b->data & _CLOCK_ON_OFF) == _CLOCK_ON)
							b->data ^= _POWER;
					}
				}

				spread_inputs_powers();
				run_gates();
				get_wire_to_outputs();
			}
		}

		wire* get_wire(point world)
		{
			for (objects::wire* w : objects::wires)
				if (w->world_low.x <= world.x && w->world_high.x >= world.x
					&& w->world_low.y <= world.y && w->world_high.y >= world.y) return w;

			return nullptr;
		}

		basic_gate* get_basic_gate(point world)
		{
			for (objects::basic_gate* b : objects::basic_gates)
				if (b->pos == world) return b;

			return nullptr;
		}

		void set_basic_wire_connections(wire* w)
		{
			for (objects::basic_gate* b : objects::basic_gates)
			{
				if (b->pos == w->world_low || b->pos == w->world_high)
				{
					b->common_wire = w;
					continue;
				}

				switch (b->data & _TYPE_SIGN)
				{
				case _NOT_GATE:
					last_not_gi = reinterpret_cast<not_gate_info*>(b->info_ptr);
					if (last_not_gi->output == w->world_low || last_not_gi->output == w->world_high)
						last_not_gi->output_wire = w;
					continue;
				case _AND_GATE:
					last_and_gi = reinterpret_cast<and_gate_info*>(b->info_ptr);
					if (last_and_gi->output == w->world_low || last_and_gi->output == w->world_high)
					{
						last_and_gi->output_wire = w;
						continue;
					}
					if (last_and_gi->input == w->world_low || last_and_gi->input == w->world_high)
						last_and_gi->input_wire = w;
					continue;
				}
			}
		}

		inline void create_event_wire()
		{
			using namespace events;

			last_wire = new wire;
			last_wire->id = next_wire_id++;
			last_wire->slope = steep;
			last_wire->power = false;

			if (steep)
			{
				if (world_start.y > world_end.y)
				{
					last_wire->world_high = world_start;
					last_wire->world_low = { world_start.x, world_end.y };
				}
				else
				{
					last_wire->world_low = world_start;
					last_wire->world_high = { world_start.x, world_end.y };
				}
			}
			else
			{
				if (world_start.x > world_end.x)
				{
					last_wire->world_high = world_start;
					last_wire->world_low = { world_end.x, world_start.y };
				}
				else
				{
					last_wire->world_low = world_start;
					last_wire->world_high = { world_end.x, world_start.y };
				}
			}

			last_wire->low_wire = get_wire(last_wire->world_low);
			if (last_wire->low_wire)
				last_wire->low_wire->connected_wires.push_back(last_wire);

			last_wire->high_wire = get_wire(last_wire->world_high);
			if (last_wire->high_wire)
				last_wire->high_wire->connected_wires.push_back(last_wire);

			set_basic_wire_connections(last_wire);

			calc_wire_rendering_values(last_wire);

			wires.push_back(last_wire);
		}

		inline void delete_event_wire()
		{
			last_wire = get_wire(grid_info::mouse_world);

			if (last_wire == nullptr) return;

			wires.remove(last_wire);

			if (last_wire->low_wire)
				last_wire->low_wire->connected_wires.remove(last_wire);

			if (last_wire->high_wire)
				last_wire->high_wire->connected_wires.remove(last_wire);

			for (objects::wire* w : last_wire->connected_wires)
			{
				if (w->low_wire == last_wire)
				{
					w->low_wire = nullptr;
					if (w->high_wire == last_wire)
						w->high_wire = nullptr;
				}
				else w->high_wire = nullptr;
			}

			for (objects::basic_gate* b : objects::basic_gates)
			{
				if (b->common_wire == last_wire)
				{
					b->common_wire = nullptr;
					continue;
				}

				switch (b->data & _TYPE_SIGN)
				{
				case _NOT_GATE:
					last_not_gi = reinterpret_cast<not_gate_info*>(b->info_ptr);
					if (last_not_gi->output_wire == last_wire)
						last_not_gi->output_wire = nullptr;
					continue;
				case _AND_GATE:
					last_and_gi = reinterpret_cast<and_gate_info*>(b->info_ptr);
					if (last_and_gi->output_wire == last_wire)
					{
						last_and_gi->output_wire = nullptr;
						continue;
					}
					if (last_and_gi->input_wire == last_wire)
						last_and_gi->input_wire = nullptr;
					continue;
				}
			}

			delete last_wire;
		}

		bool delete_racism = true;

		inline void delete_event_basic_gate()
		{
			last_basic_gate = get_basic_gate(grid_info::mouse_world);

			if (last_basic_gate == nullptr || (delete_racism && !CHECK_LBGATE_TYPE)) return;

			basic_gates.remove(last_basic_gate);

			delete last_basic_gate;
		}

		inline void draw_wire(wire* w)
		{
			if (w->out_of_screen) return;

			if (w->low_wire) graphics::draw::circle(w->screen_low, grid_info::qq_size, colors::black, screen);
			if (w->high_wire) graphics::draw::circle(w->screen_high, grid_info::qq_size, colors::black, screen);

			color_t c = POWER_C(w->power);

			// if (grid_info::debug)

			point step = w->slope ? rev_screen_step : screen_step;

			for (color_t* px = w->start_px; px < w->end_px; px += step.x)
				*px = px[step.y] = c;
		}

		inline void draw_basic_gate(basic_gate* b)
		{
			point pos = to_screen(b->pos);

			switch (b->data & _TYPE_SIGN)
			{
			case _INPUT:
				graphics::draw::fill_rect(pos - grid_info::q_size + 1, pos + grid_info::q_size, POWER_C(b->data & _POWER), screen);
				graphics::draw::rect(pos - grid_info::q_size, pos + grid_info::q_size, colors::black, screen);
				break;
			case _OUTPUT:
				graphics::draw::fill_circle(pos, grid_info::q_size, POWER_C(b->data & _POWER), screen);
				graphics::draw::circle(pos, grid_info::q_size, colors::black, screen);
				break;
			case _NOT_GATE:
				draw_not_gate(pos, b->data, colors::black);
				break;
			case _AND_GATE:
				draw_simple_gate(pos, b->data, colors::black);
				break;
			case _CLOCK_OBJ:
				graphics::draw::fill_circle(pos, grid_info::q_qh_size, ALT_POWER_C(b->data & _POWER), screen);
				graphics::draw::circle(pos, grid_info::q_qh_size, colors::black, screen);
				break;
			default:
				break;
			}
		}
	}

	namespace grid_info
	{
		inline void _update_size_values()
		{
			h_size = size >> 1;
			h_leak = size & 1;
			q_size = h_size >> 1;
			qq_size = q_size >> 1;
			q_qh_size = q_size + qq_size;
			qh_size = (q_size + qq_size) >> 1;
			y_step = size * screen_size.x;
		}

		inline void init(int grid_size)
		{
			size = grid_size;
			_update_size_values();
			mod_nh = -grid_info::h_size;
			mod_ph = grid_info::h_size;
		}

		inline void _update_mod()
		{
			mod.x = MODULO(pos.x, size);
			mod.y = MODULO(pos.y, size);
			mod_nh = grid_info::mod - grid_info::h_size;
			mod_ph = grid_info::mod + grid_info::h_size;
		}

		inline void calc_wires_rendering_values()
		{
			for (objects::wire* w : objects::wires)
				objects::calc_wire_rendering_values(w);
		}

		int auto_tick_simulation = 0;
		bool action = true, debug = false, smooth_mouse = false, dark_mode = false;
		inline void update()
		{
			if (action == false) return;

			// --------- Proc ---------
			if (data::mouse.wheel)
			{
				if (data::mouse.wheel & 0b10)
				{
					if (size > GRID_MIN_SIZE)
						size -= 2;
				}
				else if (size < GRID_MAX_SIZE)
					size += 2;

				pos -= data::mouse.pos;
				pos *= size;
				pos /= old_size;
				pos += data::mouse.pos;
			}

			if (smooth_mouse)
				pos += (data::mouse.pos - mouse_round) / 8;

			// --- Mouse Next Tick Skip Point ---

			if (data::mouse.middle)
				pos += data::mouse.delta; // (Uses Mouse-values Updates Pos)

			// --------- Calc --------- (Uses pos Updates pos-values)
			if (old_size != size)
			{
				_update_size_values();

				_update_mod();
				calc_wires_rendering_values();
				events::screen_start = to_screen(events::world_start);

				old_pos = pos;
				old_size = size;
			}
			else if (old_pos != pos)
			{
				_update_mod();
				calc_wires_rendering_values();
				events::screen_start = to_screen(events::world_start);

				old_pos = pos;
			}

			mouse_world = to_world(data::mouse.pos); // (Uses pos and pos-values Updates Mouse-values)
			mouse_round = to_screen(mouse_world); // = round_grid(data::mouse.pos)

			if (data::mouse.t_left)
			{
				using namespace events;

				if (data::mouse.left)
				{
					// std::cout << "[Mouse] Left Down\n";

					world_start = mouse_world;
					screen_start = to_screen(world_start);

					if ((events::chosen_object >= events::oInput && chosen_object <= events::oAnd) || is_catagory_chosen)
					{
						using namespace objects;

						// std::cout << "\n[Create] $" << object_strings[events::chosen_object] << " - Step I\n";

						if (get_basic_gate(mouse_world))
						{
							// std::cout << "[Create] $" << object_strings[events::chosen_object] << " - Fail\n\n";
							return;
						}

						// std::cout << "[Create] $" << object_strings[events::chosen_object] << " - Step II\n\n";

						last_basic_gate = new basic_gate;
						last_basic_gate->id = next_basic_gate_id++;
						last_basic_gate->common_wire = get_wire(mouse_world);

						if (chosen_catagory)
						{
							last_basic_gate->data = ((extra_chosen_object) << 2) | rotation | _EXTRA_OBJ_SIGN;
							std::cout << "sex :)))";
						}
						else
						{
							last_basic_gate->data = ((chosen_object - events::oInput) << 2) | rotation;

							if (chosen_object == events::oNot)
							{
								last_not_gi = new not_gate_info;
								last_basic_gate->info_ptr = PTR_2U64(last_not_gi);
								last_not_gi->output = mouse_world + rotation_to_vector(last_basic_gate->data);
								last_not_gi->output_wire = get_wire(last_not_gi->output);
							}
							else if (chosen_object == events::oAnd)
							{
								last_and_gi = new and_gate_info;
								last_basic_gate->info_ptr = PTR_2U64(last_and_gi);

								last_and_gi->input = mouse_world + rotation_to_vector(last_basic_gate->data + 1);
								last_and_gi->input_wire = get_wire(last_and_gi->input);

								last_and_gi->output = mouse_world + rotation_to_vector(last_basic_gate->data);
								last_and_gi->output_wire = get_wire(last_and_gi->output);
							}
						}

						last_basic_gate->pos = mouse_world;

						basic_gates.push_back(last_basic_gate);
					}
				}
				else
				{
					// std::cout << "[Mouse] Left Up\n";
					// std::cout << "[Chosen] $" << object_strings[events::chosen_object] << "\n\n";

					switch (chosen_object)
					{
					case oInt:
					{
						using namespace objects;

						last_basic_gate = get_basic_gate(mouse_world);
						if (last_basic_gate)
						{
							if ((last_basic_gate->data & _TYPE_SIGN) == _INPUT)
								last_basic_gate->data ^= _POWER;
							else if ((last_basic_gate->data & _TYPE_SIGN) == _CLOCK_OBJ)
								last_basic_gate->data ^= _CLOCK_ON_OFF;
						}
					}
					break;
					/*
					case v2::events::oHand:
						objects::last_wire = objects::get_wire(grid_info::mouse_world);

						if (objects::last_wire)
						{
							using namespace objects;

							std::cout << last_wire->id << ":\n  " << last_wire->world_low << " : " << last_wire->world_high << "\n  "
								<< (last_wire->low_wire ? last_wire->low_wire->id : -1) << ' '
								<< (last_wire->high_wire ? last_wire->high_wire->id : -1) << "\n  ";

							for (objects::wire* w : last_wire->connected_wires)
								std::cout << w->id << ", ";

							std::cout << std::endl;
						}

						objects::last_basic_gate = objects::get_basic_gate(grid_info::mouse_world);

						if (objects::last_basic_gate)
						{
							using namespace objects;

							std::cout << last_basic_gate->id << ":\n  ";
							print_binary(last_basic_gate->data, 7);
							std::cout << ":\n  com " << last_basic_gate->pos << ' '
								<< (last_basic_gate->common_wire ? last_basic_gate->common_wire->id : -1);

							switch (last_basic_gate->data & _TYPE_SIGN)
							{
							case _NOT_GATE:
								last_not_gi = reinterpret_cast<not_gate_info*>(last_basic_gate->info_ptr);
								std::cout << "\n  out " << last_not_gi->output << ' '
									<< (last_not_gi->output_wire ? last_not_gi->output_wire->id : -1) << std::endl;
								break;
							case _AND_GATE:
								last_and_gi = reinterpret_cast<and_gate_info*>(last_basic_gate->info_ptr);
								std::cout << "\n  out " << last_and_gi->output << ' '
									<< (last_and_gi->output_wire ? last_and_gi->output_wire->id : -1) << "\n  inp2 "
									<< last_and_gi->input << ' ' << (last_and_gi->input_wire ? last_and_gi->input_wire->id : -1) << std::endl;
								break;
							}
						}
						break;
					*/
					case v2::events::oWire:
						world_end = mouse_world;

						if (world_start == world_end)
							return;

						world_high = mouse_world;
						world_low = world_start;
						world_low | world_high;

						steep = (world_high.x - world_low.x) < (world_high.y - world_low.y);

						objects::create_event_wire();

						break;
					default:
						break;
					}
				}
			}

			if (data::mouse.t_right && data::mouse.right == false)
			{
				// std::cout << "[Mouse] Right Up\n";

				if (events::chosen_catagory)
					objects::delete_event_basic_gate();
				else
				{
					switch (events::chosen_object)
					{
					case v2::events::oHand:
						objects::last_wire = objects::get_wire(grid_info::mouse_world);
						if (objects::last_wire != nullptr) objects::tick::spread_wire_power(objects::last_wire);
						break;
					case v2::events::oWire:
						objects::delete_event_wire();
						break;
					default:
						if (events::chosen_object >= events::oInput && events::chosen_object <= events::oAnd)
							objects::delete_event_basic_gate();
						break;
					}
				}
			}

			if (data::t_space || (auto_tick_simulation && data::tick % auto_tick_simulation == 0))
			{
				objects::tick::tick();
			}
		}

		inline void draw()
		{
			for (objects::wire* w : objects::wires)
				objects::draw_wire(w);

			for (objects::basic_gate* b : objects::basic_gates)
				objects::draw_basic_gate(b);

			if (events::chosen_catagory)
			{
				switch (events::extra_chosen_object)
				{
				case events::eoClock:
					graphics::draw::circle(mouse_round, q_qh_size, colors::slight_blue, screen);
					break;
				default:
					break;
				}
			}
			else
			{
				switch (events::chosen_object)
				{
				case events::oWire:
					if (data::mouse.left && data::mouse.t_left == false && mouse_round != events::screen_start)
					{
						point mouse_pos = mouse_round;
						graphics::straighten_line(events::screen_start, mouse_pos);
						graphics::draw::straight_line(events::screen_start, mouse_pos, colors::slight_blue, screen);
					}
					break;
				case events::oInput:
					graphics::draw::rect(mouse_round - q_size, mouse_round + q_size, colors::slight_blue, screen);
					break;
				case events::oOutput:
					graphics::draw::circle(mouse_round, q_size, colors::slight_blue, screen);
					break;
				case events::oNot:
					// Performance 6/10 (Sucks)
					draw_not_gate(mouse_round, objects::rotation, colors::slight_blue);
					break;
				case events::oAnd:
					// Performance 6/10 (Sucks) O(n^2^2^2^(n! + n/n!^(e^(-n!))))
					draw_simple_gate(mouse_round, objects::rotation, colors::slight_blue);
					break;
				default:
					break;
				}
			}
		}
	};

	inline int draw_point_string(point pos, point val, color_t bracket_c, color_t value_c, graphics::surface& surf)
	{
		font::draw_string(pos, "{", bracket_c, screen);
		pos.x += font::mfdim_xinc;
		int temp = font::int_to_string(val.x), ret = temp;
		font::draw_string(pos, data::string_buffer, value_c, surf);
		pos.x += font::mfdim_xinc * temp;
		font::draw_string(pos, ", ", bracket_c, screen);
		pos.x += font::mfdim_xincd;
		temp = font::int_to_string(val.y);
		font::draw_string(pos, data::string_buffer, value_c, surf);
		pos.x += font::mfdim_xinc * temp;
		font::draw_string(pos, "}", bracket_c, screen);

		return ret + temp + 4;
	}

	// draws dot grid
	void draw_grid(int size, point pos, color_t c, graphics::surface& surf)
	{
		point_MODULO(pos, size);
		int fit_x = surf.size.x - pos.x, y_step = size * surf.size.x;

		for (color_t* y = graphics::get_raw_pixel(pos, surf); y < surf.end; y += y_step)
			for (color_t* x = y, *end = y + fit_x; x < end; x += size)
				*x = c;
	}

	// Cuts 1px from every side
	inline void draw_grid_blur()
	{
		point mod = grid_info::mod;
		if (mod.x == 0) mod.x = grid_info::size;
		if (mod.y == 0) mod.y = grid_info::size;
		int fit_x = screen.size.x - mod.x - 1;

		for (color_t* y = graphics::get_raw_pixel(mod, screen),
			*y_end = screen.end - screen.size.x;
			y < y_end; y += grid_info::y_step)
			for (color_t* x = y, *x_end = y + fit_x; x < x_end; x += grid_info::size)
			{
				x[-1] = 0xC0C0C0;
				x[0] = 0;
				x[1] = 0xC0C0C0;
				x[screen.size.x] = 0xC0C0C0;
				x[-screen.size.x] = 0xC0C0C0;
			}
	}

	inline void draw_grid()
	{
		int fit_x = screen.size.x - grid_info::mod.x;

		for (color_t* y = graphics::get_raw_pixel(grid_info::mod, screen); y < screen.end; y += grid_info::y_step)
			for (color_t* x = y, *x_end = y + fit_x; x < x_end; x += grid_info::size)
				x[0] = 0;
	}

	inline void draw_cursor(color_t mc, graphics::surface& surf)
	{
		graphics::draw::_straight_line(grid_info::mouse_round.x - grid_info::h_size, grid_info::mouse_round.x + grid_info::h_size, grid_info::mouse_round.y, false, mc, screen);
		graphics::draw::_straight_line(grid_info::mouse_round.y - grid_info::h_size, grid_info::mouse_round.y + grid_info::h_size, grid_info::mouse_round.x, true, mc, screen);
	}
}
