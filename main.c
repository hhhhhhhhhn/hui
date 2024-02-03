#include <raylib.h>
#include "hlib/core.h"
#include "hlib/hstring.h"
#include "hlib/harena.h"
#include <time.h>

#define WIDTH 800
#define HEIGHT 600

typedef f32 Pixels;

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

HArena arena;

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

Element* parent; // At most, one of these is not NULL
Element* prev_sibling;

void hui_root_start() {
	root = harena_alloc(&arena, sizeof(Element));
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
	clock_t draw_start = clock();
	root->draw(root, root+1);
	clock_t draw_end = clock();
	harena_free(&arena); // TODO: Just reset
	arena = harena_new_with_cap(1024*4);

	printf("Layout: %f ms, Draw: %f ms\n", (double)(draw_start - layout_start) / CLOCKS_PER_SEC * 1000, (double)(draw_end - draw_start) / CLOCKS_PER_SEC * 1000);
}

void* get_element_data(Element* element) {
	return element + 1;
}

// DO NOT USE DIRECTLY, but rather push_element_as_parent and push_element_as_sibling
Element* push_element(usize data_size) {
	Element* element = harena_alloc(&arena, sizeof(Element) + data_size);
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
	return element;
}

Element* push_element_as_parent(usize data_size) {
	Element* element = push_element(data_size);
	parent = element;
	prev_sibling = NULL;
	return element;
}

Element* push_element_as_sibling(usize data_size) {
	Element* element = push_element(data_size);
	parent = NULL;
	prev_sibling = element;
	return element;
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
	Element* element = push_element_as_sibling(sizeof(Color));
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
	Element* element = push_element_as_sibling(sizeof(str));
	element->draw = hui_text_draw;
	element->compute_layout = hui_text_layout;
	*(str*)get_element_data(element) = text;
}

LayoutResult hui_stack_layout(Element* el, void* data) {
	Pixels gap = *(Pixels*)data;
	LayoutResult result = LAYOUT_OK;

	if (el->layout.width == UNSET) {
		el->layout.width = el->parent->layout.width;
		assert(el->layout.width != UNSET);
	}

	Pixels y = el->layout.y;
	Pixels x = el->layout.x;

	Element* child = el->first_child;
	while(child != NULL) {
		child->layout.width = el->layout.width;
		child->layout.y = y;
		child->layout.x = x;

		child->compute_layout(child, child+1);
		y += child->layout.height + gap;

		child = child->next_sibling;
	}
	if (el->layout.height == UNSET) {
		el->layout.height = y - el->layout.y - gap;
	}

	return result;
}

void hui_stack_start(Pixels gap) {
	Element* element = push_element_as_parent(sizeof(Pixels));
	element->compute_layout = hui_stack_layout;
	element->draw = hui_root_draw;
	*(Pixels*)get_element_data(element) = gap;
}


void hui_stack_end() {
	stop_adding_children();
}

typedef struct {
	Color  background_color;
	Pixels padding;
	Color  border_color;
	Pixels border_width;
} BoxStyle;

LayoutResult hui_box_layout(Element* el, void* data) {
	BoxStyle style = *(BoxStyle*)data;
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &el->layout;
	Pixels total_padding = style.padding + style.border_width;

	if (el->first_child == NULL || el->first_child->next_sibling != NULL) {
		panic("Box must have exactly one child");
	}

	bool width_was_set_by_parent = layout->width != UNSET;
	bool height_was_set_by_parent = layout->height != UNSET;

	if (width_was_set_by_parent) {
		el->first_child->layout.width = layout->width - 2*total_padding;
	} else {
		layout->width = el->parent->layout.width - 2*total_padding; // Temporary, until children's width is computed
	}
	if (height_was_set_by_parent) {
		el->first_child->layout.height = layout->height - 2*total_padding;
	} else {
		layout->height = el->parent->layout.height - 2*total_padding; // Temporary, until children's height is computed
	}

	if (layout->x != UNSET) {
		el->first_child->layout.x = layout->x + total_padding;
	} else {
		result |= LAYOUT_ASK_PARENT;
	}
	if (layout->y != UNSET) {
		el->first_child->layout.y = layout->y + total_padding;
	} else {
		result |= LAYOUT_ASK_PARENT;
	}

	el->first_child->compute_layout(el->first_child, el->first_child+1);

	if (!width_was_set_by_parent) {
		layout->width = el->first_child->layout.width + 2*total_padding;
	}
	if (!height_was_set_by_parent) {
		layout->height = el->first_child->layout.height + 2*total_padding;
	}

	return result;
}

void hui_box_draw(Element* el, void* data) {
	BoxStyle style = *(BoxStyle*)data;
	Layout* layout = &el->layout;
	assert(el->first_child);

	DrawRectangle(layout->x, layout->y, layout->width, layout->height, style.background_color);
	DrawRectangleLinesEx((Rectangle){.x = layout->x, .y = layout->y, .width = layout->width, .height = layout->height}, style.border_width, style.border_color);
	el->first_child->draw(el->first_child, el->first_child+1);
}

void hui_box_start(BoxStyle style) {
	Element* element = push_element_as_parent(sizeof(BoxStyle));
	element->compute_layout = hui_box_layout;
	element->draw = hui_box_draw;
	*(BoxStyle*)get_element_data(element) = style;
}

void hui_box_end() {
	stop_adding_children();
}

LayoutResult hui_center_layout(Element* el, void* data) {
	Layout* layout = &el->layout;
	Pixels padding = *(Pixels*)data;
	if (el->first_child == NULL || el->first_child->next_sibling != NULL) {
		panic("Box must have exactly one child");
	}

	bool width_was_set_by_parent = layout->width != UNSET;
	bool height_was_set_by_parent = layout->height != UNSET;
	Pixels padded_width;

	if (width_was_set_by_parent) {
		padded_width = layout->width;
	} else {
		padded_width = el->parent->layout.width;
	}
	layout->width = padded_width - 2*padding; // Remove padding, for children to use

	if (!height_was_set_by_parent) {
		layout->height = el->parent->layout.height; // Temporary, until children's height is computed
	}

	el->first_child->layout.y = layout->y;
	LayoutResult child_layout_result = el->first_child->compute_layout(el->first_child, el->first_child+1);

	layout->width = padded_width;

	if (!height_was_set_by_parent) {
		layout->height = el->first_child->layout.height;
	}

	el->first_child->layout.x = layout->x + (padded_width - el->first_child->layout.width)/2;

	if (child_layout_result & LAYOUT_ASK_PARENT) {
		el->first_child->compute_layout(el->first_child, el->first_child+1);
	}

	return LAYOUT_OK;
}

void hui_center_draw(Element* el, void* data) {
	(void) data;
	assert(el->first_child && !el->first_child->next_sibling);
	el->first_child->draw(el->first_child, el->first_child+1);
}

// Padding is only horizontal
void hui_center_start(Pixels padding) {
	Element* element = push_element_as_parent(sizeof(Pixels));
	element->compute_layout = hui_center_layout;
	element->draw = hui_center_draw;;
	*(Pixels*)get_element_data(element) = padding;
}

void hui_center_end() {
	stop_adding_children();
}

LayoutResult hui_cluster_layout(Element* el, void* data) {
	// How it should work:
	// - If width not set, simply put all the children in a row
	// - If width is set, then for each child
	//   - First, try intrinsic sizing
	//   - If it would go over the width adding it after the other elements in its row, then put it next row
	//   - If its size is bigger than the width, then set the child's width to the cluster width and relayout
	// - Keep track of the biggest height of every row, and add it + padding after every row
	// - Padding is only inside the cluster. There should be no padding between the top left element and the parent
	Layout* layout = &el->layout;
	Pixels padding = *(Pixels*)data;
	LayoutResult result = LAYOUT_OK;
	bool width_was_set_by_parent = layout->width != UNSET;
	bool height_was_set_by_parent = layout->height != UNSET;

	Pixels row_max_height = 0;
	Pixels max_width = 0;
	Pixels x = layout->x;
	Pixels y = layout->y;

	Pixels width_limit;
	if (width_was_set_by_parent) {
		width_limit = layout->width;
	} else {
		width_limit = el->parent->layout.width;
		layout->width = el->parent->layout.width; // Temporary, for the children
	}
	if (!height_was_set_by_parent) {
		layout->height = el->parent->layout.height; // Temporary, for the children
	}

	Element* child = el->first_child;
	while(child != NULL) {
		child->layout.x = x;
		child->layout.y = y;
		child->compute_layout(child, child+1);

		if (x + child->layout.width > layout->x + width_limit) {
			// Advance a row
			if (x - padding - layout->x > max_width) {
				max_width = x - padding - layout->x;
			}
			y += row_max_height + padding;
			x = layout->x;

			child->layout.y = y;
			child->layout.x = x;
			row_max_height = 0;

			if (child->layout.width > width_limit) {
				child->layout.width = width_limit;
				child->layout.height = UNSET;
			}
			child->compute_layout(child, child+1);
		}

		if (child->layout.height > row_max_height) {
			row_max_height = child->layout.height;
		}
		x += child->layout.width + padding;

		child = child->next_sibling;
	}

	if(!height_was_set_by_parent) {
		layout->height = y + row_max_height;
	} else {
		if(y + row_max_height > layout->height) {
			// asm("int3; nop");
		}
	}
	if(!width_was_set_by_parent) {
		if (x - padding - layout->x > max_width) {
			max_width = x - padding - layout->x;
		}
		layout->width = max_width;
	}

	if (!layout->x) {
		result |= LAYOUT_ASK_PARENT;
	}
	if (!layout->y) {
		result |= LAYOUT_ASK_PARENT;
	}
	return result;
}

void hui_cluster_draw(Element* el, void* data) {
	(void) data;
	Element* child = el->first_child;
	while(child != NULL) {
		child->draw(child, child+1);
		child = child->next_sibling;
	}
}

void hui_cluster_start(Pixels padding) {
	Element* element = push_element_as_parent(sizeof(Pixels));
	element->compute_layout = hui_cluster_layout;
	element->draw = hui_cluster_draw;
	*(Pixels*)get_element_data(element) = padding;
}

void hui_cluster_end() {
	stop_adding_children();
}

str lorem = STR_ARR(
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla lobortis purus a metus luctus molestie. Fusce magna dui, aliquet eget eros nec, iaculis luctus turpis. Phasellus commodo, nunc id euismod suscipit, nisl orci posuere sapien, nec convallis magna arcu nec urna. Fusce eleifend lacinia purus. Mauris sagittis ex ut bibendum dapibus. Phasellus et velit nunc. Vestibulum iaculis elementum auctor. Donec eu ultricies tortor. Donec dictum est ligula, quis tincidunt risus elementum viverra. Vestibulum eu sodales tortor. Aenean porttitor est in ex semper tincidunt. Donec pretium nec risus ut lacinia. Integer sollicitudin velit augue."
);

i32 main(void) {
	arena = harena_new_with_cap(1024*4);

	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);

	InitWindow(WIDTH, HEIGHT, "Example");
	SetTargetFPS(60);

	BoxStyle box_style = {
		.background_color = {.r = 200, .g = 200, .b = 200, .a = 255},
		.padding = 15,
		.border_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.border_width = 5,
	};
	while(!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(RAYWHITE);
			hui_root_start();
				hui_cluster_start(40);
					hui_cluster_start(20);
						hui_text(STR("First"));
						hui_text(STR("Uno"));
					hui_cluster_end();
					hui_cluster_start(20);
						hui_text(STR("Second"));
						hui_text(STR("Dos"));
					hui_cluster_end();
				hui_cluster_end();
				hui_block();
				hui_stack_start(50);
					hui_center_start(10);
						hui_box_start(box_style);
							hui_text(STR("This is a centered string"));
						hui_box_end();
					hui_center_end();
					hui_box_start(box_style);
						hui_stack_start(15);
							hui_block();
							hui_text(lorem);
							hui_box_start(box_style);
								hui_stack_start(30);
									hui_block();
									hui_text(STR("Woooooo"));
									hui_block();
									hui_text(STR("Woooooo"));
								hui_stack_end();
							hui_box_end();
						hui_stack_end();
					hui_box_end();
					hui_box_start(box_style);
						hui_block();
					hui_box_end();
					hui_text(STR("Hello, World!"));
				hui_stack_end();
				hui_block();
				hui_text(lorem);
			hui_root_end();

			int fps = GetFPS();
			DrawText(TextFormat("FPS: %d", fps), 10, 10, 20, GREEN);
		EndDrawing();
	}
	return 0;
}
