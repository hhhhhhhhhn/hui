#ifndef _HUI_H
#define _HUI_H

#include <raylib.h>
#include "hlib/core.h"
#include "hlib/hstring.h"
#include "hlib/hvec.h"
#include "hlib/harena.h"
#include <time.h>

#define WIDTH 800
#define HEIGHT 600


typedef f32 Pixels;
typedef u64 ElementId;

const Pixels UNSET = -15001;

typedef struct {
	Pixels x;
	Pixels y;
	Pixels width;
	Pixels height;
} Layout;

typedef u8 LayoutResult;
const LayoutResult LAYOUT_OK = 0;
const LayoutResult LAYOUT_ASK_PARENT = 1;
const LayoutResult LAYOUT_ASK_CHILDREN = 2;
const LayoutResult LAYOUT_ASK_ALL = LAYOUT_ASK_PARENT | LAYOUT_ASK_CHILDREN;

typedef struct Element {
	ElementId       id;
	struct Element* parent;
	struct Element* first_child;
	struct Element* next_sibling;
	struct Element* prev_sibling;
	Layout          layout;
	LayoutResult    (*compute_layout)(struct Element*, void*);
	void            (*draw)(struct Element*, void*);
} Element;

Element default_element = {
	.parent = NULL,
	.first_child = NULL,
	.next_sibling = NULL,
	.prev_sibling = NULL,
	.layout = { .x = UNSET, .y = UNSET, .width = UNSET, .height = UNSET },
	.compute_layout = NULL,
	.draw = NULL,
};

HArena element_arena = {0};
HVec functions_vec = {0};

Element* root;

LayoutResult hui_root_compute_layout(Element* el, void* data) {
	(void) data;
	Pixels y = el->layout.y;
	Pixels x = el->layout.x;

	Element* child = el->first_child;
	while(child != NULL) {
		child->layout.x = x;
		child->layout.y = y;
		child->compute_layout(child, child+1);
		y += child->layout.height;

		child = child->next_sibling;
	}
	el->layout.height = y - el->layout.y;

	return LAYOUT_OK;
}

void hui_root_draw(Element* el, void* data) {
	(void) data;
	Element* child = el->first_child;
	while(child != NULL) {
		child->draw(child, child+1);
		child = child->next_sibling;
	}
}

typedef struct {
	void (*handler)(Element*, void*);
	Element* element;
} Handler;

void push_handler(void (*handler)(Element*, void*), Element* el) {
	Handler handler_struct = {.handler = handler, .element = el};
	hvec_push(&functions_vec, &handler_struct);
}

Element* parent; // At most, one of these is not NULL
Element* prev_sibling;

ElementId hot_id = 0;
ElementId active_id = 0;

void hui_root_start() {
	if (element_arena.sarenas_used == 0) element_arena = harena_new_with_cap(1024*4);
	if (functions_vec.data == NULL) functions_vec = hvec_new_with_cap(sizeof(Handler), 1024);

	root = harena_alloc(&element_arena, sizeof(Element));
	root->layout = (Layout) { .x = 0, .y = 0, .width = GetScreenWidth(), .height = UNSET };
	root->parent = NULL;
	root->next_sibling = NULL;
	root->prev_sibling = NULL;
	root->first_child = NULL;
	root->compute_layout = hui_root_compute_layout;
	root->draw = hui_root_draw;

	parent = root;
}

void hui_root_end() {
	clock_t layout_start = clock();
	root->compute_layout(root, root+1);
	clock_t layout_end = clock();

	clock_t handle_start = clock();
	for(usize i = 0; i < functions_vec.len; i++) {
		Handler* handler = (Handler*)hvec_at(&functions_vec, i);
		handler->handler(handler->element, handler->element+1);
	}
	clock_t handle_end = clock();

	clock_t draw_start = clock();
	root->draw(root, root+1);
	clock_t draw_end = clock();

	harena_clear(&element_arena);
	hvec_clear(&functions_vec);

	printf("Layout: %f ms, Handle: %f ms, Draw: %f ms\n", (double)(layout_end - layout_start) / CLOCKS_PER_SEC * 1000, (double)(handle_end - handle_start) / CLOCKS_PER_SEC * 1000, (double)(draw_end - draw_start) / CLOCKS_PER_SEC * 1000);
}

void* get_element_data(Element* element) {
	return element + 1;
}

Element* push_element(usize data_size) {
	Element* element = harena_alloc(&element_arena, sizeof(Element) + data_size);
	if (parent) {
		element->parent = parent;
		parent->first_child = element;
		element->prev_sibling = NULL;
	}
	else if (prev_sibling) {
		element->prev_sibling = prev_sibling;
		prev_sibling->next_sibling = element;
		element->parent = prev_sibling->parent;
	}
	else {
		panic("No parent nor prev_sibling");
	}
	element->layout = (Layout) { .x = UNSET, .y = UNSET, .width = UNSET, .height = UNSET };
	element->next_sibling = NULL;
	element->first_child = NULL;
	element->draw = NULL;
	element->compute_layout = NULL;
	element->id = 0;
	parent = NULL;
	prev_sibling = element;
	return element;
}

void start_adding_children() {
	parent = prev_sibling;
	prev_sibling = NULL;
}

void stop_adding_children() {
	if (parent) {
		prev_sibling = parent;
		parent = NULL;
	}
	else if (prev_sibling) {
		prev_sibling = prev_sibling->parent;
		parent = NULL;
	}
}

LayoutResult hui_block_layout(Element* element, void* data) {
	(void) data;
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &element->layout;
	if(layout->width == UNSET) {
		if(element->parent->layout.width == UNSET) {
			result |= LAYOUT_ASK_PARENT;
		}
		else {
			layout->width = element->parent->layout.width * 0.5;
		}
	}
	if(layout->height == UNSET) {
		layout->height = 100;
	}
	return result;
}

void hui_block_draw(Element* element, void* data) {
	(void) data;
	Color* color = get_element_data(element);
	DrawRectangle(element->layout.x, element->layout.y, element->layout.width, element->layout.height, *color);
}

void hui_block() {
	Element* element = push_element(sizeof(Color));
	element->draw = hui_block_draw;
	element->compute_layout = hui_block_layout;
	Color* color = get_element_data(element);
	*color = RED;
}

LayoutResult hui_text_layout(Element* element, void* data) {
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &element->layout;
	str text = *(str*)data;

	Font font = GetFontDefault();
	f32 scale_factor = 20/(f32)font.baseSize;

	if(layout->width == UNSET) {
		layout->width = MeasureText(TextFormat("%.*s", (int)text.len, text.data), 20);
		if(layout->width > element->parent->layout.width) {
			layout->width = element->parent->layout.width;
		}
	}

	Pixels x_start = element->layout.x;
	Pixels x = x_start;
	Pixels y = element->layout.y;

	int codepoint_bytes = 0;
	for (usize i = 0; i < text.len; i += codepoint_bytes) {
		int chr = GetCodepoint(&text.data[i], &codepoint_bytes);
		int index = GetGlyphIndex(font, chr);
		Pixels chr_width = font.glyphs[index].advanceX ? font.glyphs[index].advanceX : font.recs[index].width;
		chr_width *= scale_factor;
		if (x + chr_width > element->layout.x + element->layout.width) {
			x = x_start;
			y += 20;
		}
		x += chr_width;
		x += 20*0.1;
	}

	if(layout->height == UNSET) {
		layout->height = y - element->layout.y + 20;
	}
	return result;
}

void hui_text_draw(Element* element, void* data) {
	str text = *(str*)data;

	Pixels x_start = element->layout.x;
	Pixels x = x_start;
	Pixels y = element->layout.y;

	Font font = GetFontDefault();
	f32 scale_factor = 20/(f32)font.baseSize;

	int codepoint_bytes = 0;
	for (usize i = 0; i < text.len; i += codepoint_bytes) {
		int chr = GetCodepoint(&text.data[i], &codepoint_bytes);
		int index = GetGlyphIndex(font, chr);

		Pixels chr_width = font.glyphs[index].advanceX ? font.glyphs[index].advanceX : font.recs[index].width;
		chr_width *= scale_factor;

		if (x + chr_width > element->layout.x + element->layout.width) {
			x = x_start;
			y += 20;
		}

		DrawTextCodepoint(font, chr, (Vector2){ .x = x, .y = y }, 20, BLACK);
		x += chr_width;
		x += 20*0.1;
	}
}

void hui_text(str text) {
	Element* element = push_element(sizeof(str));
	element->draw = hui_text_draw;
	element->compute_layout = hui_text_layout;
	*(str*)get_element_data(element) = text;
}

#endif
