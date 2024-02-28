#include "hui.h"
#include "../hlib/hstring.h"
#include "core.c"
#include <raylib.h>

ElementId button_clicked = 0;

void hui_button_handle(Element* el, void* data) {
	(void) data;
	Vector2 mouse = GetMousePosition();
	if (CheckCollisionPointRec(mouse, el->layout) && CheckCollisionPointRec(mouse, *el->bounding_box)) {
		hot_id = el->id;
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			active_id = el->id;
		}
	} else {
		if(hot_id == el->id) hot_id = 0;
	}
	if (button_clicked == el->id) button_clicked = 0;
	if (active_id == el->id && IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
		if(hot_id == el->id) {
			button_clicked = el->id;
		}
		active_id = 0;
	}
}

bool hui_button(ElementId id, str text, TextStyle style) {
	BoxStyle box_style = {
		.background_color = {.r = 200, .g = 200, .b = 200, .a = 255},
		.padding = 15,
		.border_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.border_width = 5,
	};
	if (active_id == id) {
		box_style.background_color = (Color){.r = 100, .g = 0, .b = 0, .a = 255};
	}
	else if (hot_id == id) {
		box_style.background_color = (Color){.r = 255, .g = 0, .b = 0, .a = 255};
	}
	hui_box_start(box_style);
		Element* box = current_element();
		box->id = id;
		push_handler(hui_button_handle, box);
		hui_text(text, style);
	hui_box_end();

	return button_clicked == id;
}

void hui_text_input_handle(Element* el, void* data) {
	(void) data;
	strb* builder = (strb*)el->id;

	Vector2 mouse = GetMousePosition();
	if (CheckCollisionPointRec(mouse, el->layout) && CheckCollisionPointRec(mouse, *el->bounding_box)) {
		hot_id = el->id;
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			active_id = el->id;
		}
	} else {
		if(hot_id == el->id) hot_id = 0;
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && active_id == el->id) active_id = 0;
	}
	if (active_id == el->id) {
		int key = GetCharPressed();
		if (key > 0) {
			printf("============== %i =================\n", key);
			if (key == KEY_BACKSPACE && builder->len > 0) {
				builder->len--;
			}
			if (key >= 32 && key <= 126) {
				assert(builder->cap && "Builder must me initted");
				strb_push_char(builder, key);
			}
		}
		if (IsKeyPressed(KEY_BACKSPACE) && builder->len > 0) {
			builder->len--;
		}
	}
}

void hui_text_input(strb* builder, TextStyle style) {
	BoxStyle box_style = {
		.background_color = {.r = 200, .g = 200, .b = 200, .a = 255},
		.padding = 15,
		.border_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.border_width = 5,
	};
	if (active_id == (u64)builder) {
		box_style.background_color = (Color){.r = 150, .g = 150, .b = 150, .a = 255};
	}
	hui_box_start(box_style);
		Element* box = current_element();
		box->id = (u64)builder;
		push_handler(hui_text_input_handle, box);

		str view;
		if (active_id == (u64)builder) {
			// Selected, put underscore on the end
			assert(builder->cap && "Builder must me initted");
			if(hui_get_frame_num() & 16) {
				strb_push_char(builder, '_');
			} else {
				strb_push_char(builder, ' ');
			}
			builder->len--;
			view = str_from_strb(builder);
			view.len++; // Include the underscore
		}
		else {
			view = str_from_strb(builder);
		}
		hui_text(view, style);
	hui_box_end();
}
