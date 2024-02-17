#ifndef _HUI_H
#define _HUI_H

#include "../hlib/core.h"
#include "../hlib/hstring.h"
#include <raylib.h>

typedef f32 Pixels;
typedef u64 ElementId;

static const Pixels UNSET = -15001;

typedef Rectangle Layout;
typedef u8 LayoutResult;

static const LayoutResult LAYOUT_OK = 0;
static const LayoutResult LAYOUT_ASK_PARENT = 1;
static const LayoutResult LAYOUT_ASK_CHILDREN = 2;
static const LayoutResult LAYOUT_ASK_ALL = LAYOUT_ASK_PARENT | LAYOUT_ASK_CHILDREN;

typedef struct Element {
	ElementId       id;
	struct Element* parent;
	struct Element* first_child;
	struct Element* next_sibling;
	struct Element* prev_sibling;
	Layout          layout;
	Layout*         bounding_box;
	LayoutResult    (*compute_layout)(struct Element*, void*);
	void            (*draw)(struct Element*, void*);
} Element;

void push_handler(void (*handler)(Element*, void*), Element* el);
void hui_init();
void hui_deinit();
void hui_root_start();
void hui_root_end();
void* get_element_data(Element* element);
Element* push_element(usize data_size);
void start_bounding_box(Layout* layout);
void end_bounding_box();
void start_adding_children();
void stop_adding_children();
void hui_block();
void hui_text(str text);

// Layouts
void hui_stack_start(Pixels gap);
void hui_stack_end();

typedef struct {
	Color  background_color;
	Pixels padding;
	Color  border_color;
	Pixels border_width;
} BoxStyle;

void hui_box_start(BoxStyle style);
void hui_box_end();
void hui_center_start(Pixels padding);
void hui_center_end();
void hui_cluster_start(Pixels padding);
void hui_cluster_end();
void hui_leftright_start(Pixels padding);
void hui_leftright_end();
void hui_fixed_start(Pixels width, Pixels height);
void hui_fixed_end();
void hui_scroll_start(Pixels* offset);
void hui_scroll_end();

bool hui_button(ElementId id, str text);
#endif
