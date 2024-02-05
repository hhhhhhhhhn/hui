#ifndef _HUI_LAYOUTS_H
#define _HUI_LAYOUTS_H
#include "hui.c"

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
	Element* element = push_element(sizeof(Pixels));
	element->compute_layout = hui_stack_layout;
	element->draw = hui_root_draw;
	*(Pixels*)get_element_data(element) = gap;
	start_adding_children();
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
	Element* element = push_element(sizeof(Pixels));
	element->compute_layout = hui_cluster_layout;
	element->draw = hui_cluster_draw;
	*(Pixels*)get_element_data(element) = padding;
	start_adding_children();
}

void hui_cluster_end() {
	stop_adding_children();
}

#endif
