#ifndef _HUI_WIDGETS_H
#define _HUI_WIDGETS_H

#include "hui.c"
#include "hlib/hstring.h"
#include "layouts.c"
#include <raylib.h>

void hui_button_handle(Element* el, void* data) {
	printf("Hui button handle\n");
	(void) data;
	Rectangle rect = (Rectangle) {
		.x = el->layout.x,
		.y = el->layout.y,
		.width = el->layout.width,
		.height = el->layout.height,
	};
	if (CheckCollisionPointRec(GetMousePosition(), rect)) {
		hot_id = el->id;
	} else {
		if(hot_id == el->id) hot_id = 0;
	}
}

void hui_button(str text) {
	BoxStyle box_style = {
		.background_color = {.r = 200, .g = 200, .b = 200, .a = 255},
		.padding = 15,
		.border_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.border_width = 5,
	};
	if (hot_id == 1) {
		box_style.background_color = (Color){.r = 255, .g = 0, .b = 0, .a = 255};
	}
	hui_box_start(box_style);
		parent->id = 1;
		push_handler(hui_button_handle, parent);
		hui_text(text);
	hui_box_end();
}

#endif
