#ifndef _HUI_CORE_C
#define _HUI_CORE_C

#include <raylib.h>
#include <time.h>
#include "../hlib/core.h"
#include "../hlib/hstring.h"
#include "../hlib/hvec.h"
#include "../hlib/harena.h"

#include "hui.h"

Element default_element = {
	.id = 0,
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

#define BOUNDING_BOX_STACK_CAP 10
Layout* bounding_box_stack[BOUNDING_BOX_STACK_CAP] = {0};
usize bounding_box_stack_len = 0;

Element* root;

Element* parent; // At most, one of these two is not NULL
Element* prev_sibling;

ElementId hot_id = 0;
ElementId active_id = 0;

LayoutResult hui_root_layout(Element* el, void* data) {
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

void hui_init() {
	element_arena = harena_new_with_cap(1024*4);
	functions_vec = hvec_new_with_cap(sizeof(Handler), 1024);
}

void hui_deinit() {
	if(element_arena.sarenas_used > 0) harena_free(&element_arena);
	if(functions_vec.data != NULL) hvec_free(&functions_vec);
}

void hui_root_start() {
	root = harena_alloc(&element_arena, sizeof(Element));
	root->layout = (Layout) { .x = 0, .y = 0, .width = GetScreenWidth(), .height = UNSET };
	root->parent = NULL;
	root->next_sibling = NULL;
	root->prev_sibling = NULL;
	root->first_child = NULL;
	root->compute_layout = hui_root_layout;
	root->draw = hui_root_draw;

	bounding_box_stack_len = 1;
	bounding_box_stack[0] = &root->layout;

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
	element->bounding_box = bounding_box_stack[bounding_box_stack_len-1];
	element->next_sibling = NULL;
	element->first_child = NULL;
	element->draw = NULL;
	element->compute_layout = NULL;
	element->id = 0;
	parent = NULL;
	prev_sibling = element;
	return element;
}

void start_bounding_box(Layout* layout) {
	bounding_box_stack_len++;
	assert(bounding_box_stack_len <= BOUNDING_BOX_STACK_CAP);
	bounding_box_stack[bounding_box_stack_len-1] = layout;
}

void end_bounding_box() {
	bounding_box_stack_len--;
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
