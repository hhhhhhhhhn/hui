#ifndef _HUI_CORE_C
#define _HUI_CORE_C

#include <raylib.h>
#include <raymath.h>
#include <time.h>
#include "../hlib/core.h"
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


Element* parent; // At most, one of these two is not NULL.
Element* prev_sibling;

Element* current_element() {
	if (parent) return parent;
	return prev_sibling;
}

ElementId hot_id = 0;
ElementId active_id = 0;

LayoutResult hui_root_layout(Element* el, void* data) {
	(void) data;
	if(!el->first_child || el->first_child->next_sibling) {
		panic("Root must have exactly one child");
	}
	Element* child = el->first_child;
	child->layout = el->layout;
	child->compute_layout(child, child+1);
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

i64 frame_num = 0;
i64 hui_get_frame_num() {
	return frame_num;
}

void hui_root_start() {
	root = harena_alloc(&element_arena, sizeof(Element));
	root->layout = (Layout) { .x = 0, .y = 0, .width = GetScreenWidth(), .height = GetScreenHeight() };
	root->parent = NULL;
	root->next_sibling = NULL;
	root->prev_sibling = NULL;
	root->first_child = NULL;
	root->compute_layout = hui_root_layout;
	root->draw = hui_root_draw;

	bounding_box_stack_len = 1;
	bounding_box_stack[0] = &root->layout;

	frame_num++;

	parent = root;
}

f32 frame_time;
void hui_root_end() {
	frame_time = GetFrameTime();

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

LayoutResult hui_nothing_layout(Element* element, void* data) {
	(void) data;
	if(element->layout.width == UNSET) {
		element->layout.width = 0;
	}
	if(element->layout.height == UNSET) {
		element->layout.height = 0;
	}
	return LAYOUT_OK;
}

void hui_nothing_draw(Element* element, void* data) {
	(void) data;
	(void) element;
}

void hui_nothing() {
	Element* element = push_element(0);
	element->draw = hui_nothing_draw;
	element->compute_layout = hui_nothing_layout;
}
#endif
