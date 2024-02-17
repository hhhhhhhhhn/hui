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

bool hui_button(ElementId id, str text) {
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
		parent->id = id;
		push_handler(hui_button_handle, parent);
		hui_text(text);
	hui_box_end();

	return button_clicked == id;
}
