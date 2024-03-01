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


usize* active_text_input_cursor = NULL;
usize* hot_text_input_cursor = NULL;

void hui_text_input_handle(Element* el, void* data) {
	(void) data;
	strb* builder = (strb*)el->id;

	Vector2 mouse = GetMousePosition();
	if (CheckCollisionPointRec(mouse, el->layout) && CheckCollisionPointRec(mouse, *el->bounding_box)) {
		hot_id = el->id;
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
			active_id = el->id;
			active_text_input_cursor = hot_text_input_cursor;
		}
	} else {
		if(hot_id == el->id) hot_id = 0;
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && active_id == el->id) active_id = 0;
	}
	if (active_id == el->id) {
		int key = GetCharPressed();
		usize* cursor = active_text_input_cursor;
		if (key > 0) {
			printf("============== %i, %lu =================\n", key, *cursor);
			if (key == KEY_BACKSPACE && builder->len > 0) {
				builder->len--;
			}
			if (key >= 32 && key <= 126) {
				assert(builder->cap && "Builder must me initted");
				strb_insert_char(builder, key, *cursor);
				(*cursor)++;
			}
		}
		if (IsKeyPressed(KEY_BACKSPACE) && builder->len > 0 && *cursor > 0) {
			strb_remove_char(builder, *cursor - 1);
			(*cursor)--;
		}
		else if (IsKeyPressed(KEY_LEFT) && *cursor > 0) {
			(*cursor)--;
		}
		else if (IsKeyPressed(KEY_RIGHT) && *cursor < builder->len) {
			(*cursor)++;
		}
	}
}

void hui_text_input(strb* builder, usize* cursor, TextStyle style) {
	BoxStyle box_style = {
		.background_color = {.r = 200, .g = 200, .b = 200, .a = 255},
		.padding = 15,
		.border_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.border_width = 5,
	};
	if (active_id == (u64)builder) {
		box_style.background_color = (Color){.r = 150, .g = 150, .b = 150, .a = 255};
		active_text_input_cursor = cursor;
	}
	if (hot_id == (u64)builder) {
		hot_text_input_cursor = cursor;
	}
	hui_box_start(box_style);
		Element* box = current_element();
		box->id = (u64)builder;
		push_handler(hui_text_input_handle, box);

		str view = str_from_strb(builder);
		hui_cursor_text(view, style, *cursor);
	hui_box_end();
}
