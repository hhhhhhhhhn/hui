#include "hui.h"
#include "core.c"

LayoutResult hui_stack_layout(Element* el, void* data) {
	Pixels gap = *(Pixels*)data;
	LayoutResult result = LAYOUT_OK;

	if (is_unset(el->layout.width)) {
		el->layout.width = el->parent->layout.width;
		assert(!is_unset(el->layout.width));
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
	if (is_unset(el->layout.height)) {
		el->layout.height = y - el->layout.y - gap;
	}

	return result;
}

void hui_stack_start(Pixels gap) {
	Element* element = push_element(sizeof(Pixels));
	element->compute_layout = hui_stack_layout;
	element->draw = hui_root_draw;
	*(Pixels*)get_element_data(element) = gap;
	start_adding_children();
}

Margin margin_add(Margin a, Margin b) {
	return (Margin) {
		.left   = a.left + b.left,
		.right  = a.right + b.right,
		.top    = a.top + b.top,
		.bottom = a.bottom + b.bottom,
	};
}

void hui_stack_end() {
	stop_adding_children();
}
LayoutResult hui_box_layout(Element* el, void* data) {
	BoxStyle style = *(BoxStyle*)data;
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &el->layout;
	Margin total_padding = margin_add(style.padding, style.border);
	Pixels total_horizontal_padding = total_padding.left + total_padding.right;
	Pixels total_vertical_padding = total_padding.top + total_padding.bottom;

	if (el->first_child == NULL || el->first_child->next_sibling != NULL) {
		panic("Box must have exactly one child");
	}

	bool width_was_set_by_parent = !is_unset(layout->width);
	bool height_was_set_by_parent = !is_unset(layout->height);

	if (width_was_set_by_parent) {
		el->first_child->layout.width = layout->width - total_horizontal_padding;
	} else {
		layout->width = el->parent->layout.width - total_horizontal_padding; // Temporary, until children's width is computed
	}
	if (height_was_set_by_parent) {
		el->first_child->layout.height = layout->height - total_vertical_padding;
	} else {
		layout->height = el->parent->layout.height - total_vertical_padding; // Temporary, until children's height is computed
	}

	if (!is_unset(layout->x)) {
		el->first_child->layout.x = layout->x + total_padding.left;
	} else {
		result |= LAYOUT_ASK_PARENT;
	}
	if (!is_unset(layout->y)) {
		el->first_child->layout.y = layout->y + total_padding.top;
	} else {
		result |= LAYOUT_ASK_PARENT;
	}

	el->first_child->compute_layout(el->first_child, el->first_child+1);

	if (!width_was_set_by_parent) {
		layout->width = el->first_child->layout.width + total_horizontal_padding;
	}
	if (!height_was_set_by_parent) {
		layout->height = el->first_child->layout.height + total_vertical_padding;
	}

	return result;
}

void hui_box_draw(Element* el, void* data) {
	BoxStyle style = *(BoxStyle*)data;
	Layout* layout = &el->layout;
	assert(el->first_child);

	if (style.background_color.a != 0) {
		DrawRectangle(layout->x, layout->y, layout->width, layout->height, style.background_color);
	}
	if (style.border_color.a != 0) {
		// TODO: Handle different borders correctly
		DrawRectangleLinesEx((Rectangle){.x = layout->x, .y = layout->y, .width = layout->width, .height = layout->height}, style.border.top, style.border_color);
	}
	el->first_child->draw(el->first_child, el->first_child+1);
}

void hui_box_start(BoxStyle style) {
	Element* element = push_element(sizeof(BoxStyle));
	element->compute_layout = hui_box_layout;
	element->draw = hui_box_draw;
	*(BoxStyle*)get_element_data(element) = style;
	start_adding_children();
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

	bool width_was_set_by_parent = !is_unset(layout->width);
	bool height_was_set_by_parent = !is_unset(layout->height);
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
	Element* element = push_element(sizeof(Pixels));
	element->compute_layout = hui_center_layout;
	element->draw = hui_center_draw;;
	*(Pixels*)get_element_data(element) = padding;
	start_adding_children();
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
	bool width_was_set_by_parent = !is_unset(layout->width);
	bool height_was_set_by_parent = !is_unset(layout->height);

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
		layout->height = y + row_max_height - layout->y;
	} else {
		if(y + row_max_height - layout->y > layout->height) {
			// Oopsie daisy, too large. TODO: Handle this somehow
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
	Element* element = push_element(sizeof(Pixels));
	element->compute_layout = hui_cluster_layout;
	element->draw = hui_cluster_draw;
	*(Pixels*)get_element_data(element) = padding;
	start_adding_children();
}

void hui_cluster_end() {
	stop_adding_children();
}

LayoutResult hui_leftright_layout(Element* el, void* data) {
	Layout* layout = &el->layout;
	Pixels padding = *(Pixels*)data;
	LayoutResult result = LAYOUT_OK;

	if(!el->first_child || !el->first_child->next_sibling || el->first_child->next_sibling->next_sibling) {
		panic("Leftright must have exactly two children.");
	}
	Element* left = el->first_child;
	Element* right = left->next_sibling;

	if (is_unset(layout->width)) {
		layout->width = el->parent->layout.width;
	}

	Pixels x = layout->x;
	Pixels y = layout->y;

	left->layout.x = x;
	left->layout.y = y;
	left->compute_layout(left, left+1);
	x += left->layout.width + padding;

	if (left->layout.width > layout->width) {
		left->layout.width = layout->width;
		left->layout.height = UNSET;
		left->compute_layout(left, left+1);
	}

	bool wrapped;

	right->compute_layout(right, right+1);
	if (x + right->layout.width > layout->x + layout->width) { // Does not fit horizontally
		if(right->layout.width > layout->width) {
			right->layout.width = layout->width;
			right->layout.height = UNSET;
		}
		x = layout->x + layout->width - right->layout.width;
		y = layout->y + left->layout.height + padding;
		wrapped = true;
	}
	else { // Does fit horizontally
		x = layout->x + layout->width - right->layout.width;
		y = layout->y;
		wrapped = false;
	}

	right->layout.x = x;
	right->layout.y = y;
	right->compute_layout(right, right+1);

	if (wrapped) {
		layout->height = left->layout.height + right->layout.height + padding;
	} else {
		if(left->layout.height > right->layout.height) {
			layout->height = left->layout.height;
		} else {
			layout->height = right->layout.height;
		}
	}

	return result;
}

void hui_leftright_draw(Element* el, void* data) {
	(void) data;
	Element* left = el->first_child;
	left->draw(left, left+1);
	Element* right = left->next_sibling;
	right->draw(right, right+1);
}

void hui_leftright_start(Pixels padding) {
	Element* element = push_element(sizeof(Pixels));
	element->compute_layout = hui_leftright_layout;
	element->draw = hui_leftright_draw;
	*(Pixels*)get_element_data(element) = padding;
	start_adding_children();
}

void hui_leftright_end() {
	stop_adding_children();
}

LayoutResult hui_fixed_layout(Element* el, void* data) {
	Layout* layout = &el->layout;
	Pixels* size = (Pixels*)data;
	if (!el->first_child || el->first_child->next_sibling) {
		panic("hui_fixed must have exactly one child");
	}
	layout->width = size[0];
	layout->height = size[1];
	el->first_child->layout.x = layout->x;
	el->first_child->layout.y = layout->y;
	el->first_child->layout.width = size[0];
	el->first_child->layout.height = size[1];
	el->first_child->compute_layout(el->first_child, el->first_child+1);
	return LAYOUT_OK;
}

void hui_fixed_draw(Element* el, void* data) {
	(void) data;
	el->first_child->draw(el->first_child, el->first_child+1);
}

void hui_fixed_start(Pixels width, Pixels height) {
	Element* element = push_element(sizeof(Pixels)*2);
	element->compute_layout = hui_fixed_layout;
	element->draw = hui_fixed_draw;
	Pixels* data = get_element_data(element);
	data[0] = width;
	data[1] = height;
	start_adding_children();
}
void hui_fixed_end() {
	stop_adding_children();
}

LayoutResult hui_scroll_layout(Element* el, void* data) {
	Pixels* offset = *(Pixels**)data;
	Layout* layout = &el->layout;
	LayoutResult result = LAYOUT_OK;

	if(!el->first_child || el->first_child->next_sibling) {
		panic("hui_scroll must have exactly one child");
	}

	Element* child = el->first_child;
	child->layout.x = layout->x;
	child->layout.y = layout->y - *offset;

	child->compute_layout(child, child+1);

	if(is_unset(layout->width)) {
		layout->width = child->layout.width;
	}
	if(is_unset(layout->height)) {
		layout->height = child->layout.height;
	}
	return result;
}

// These variables are used so that only one scroll is handled with the wheel at a time
Pixels* last_scrolled_offset = NULL;
Pixels last_scrolled_prev_offset = UNSET;

void hui_scroll_draw(Element* el, void* data) {
	(void) data;
	BeginScissorMode(el->layout.x, el->layout.y, el->layout.width, el->layout.height);
		Element* child = el->first_child;
		child->draw(child, child+1);
	EndScissorMode();
}

void hui_scroll_handle(Element* el, void* data) {
	Pixels* offset = *(Pixels**)data;

	if (CheckCollisionPointRec(GetMousePosition(), el->layout)) {
		if(last_scrolled_offset != NULL && last_scrolled_offset != offset) {
			*last_scrolled_offset = last_scrolled_prev_offset;
		}

		last_scrolled_offset = offset;
		last_scrolled_prev_offset = *offset;

		Pixels dy = -GetMouseWheelMoveV().y * 1500 * frame_time;

		*offset += dy;
	}
	if (*offset > el->first_child->layout.height - el->layout.height) *offset = el->first_child->layout.height - el->layout.height;
	if (*offset < 0) *offset = 0;
}

void hui_scroll_start(Pixels* offset) {
	last_scrolled_offset = NULL;
	Element* element = push_element(sizeof(Pixels*));
	element->compute_layout = hui_scroll_layout;
	element->draw = hui_scroll_draw;
	*(Pixels**)get_element_data(element) = offset;
	push_handler(hui_scroll_handle, element);
	start_bounding_box(&element->layout);
	start_adding_children();
}

void hui_scroll_end() {
	stop_adding_children();
	end_bounding_box();
}
