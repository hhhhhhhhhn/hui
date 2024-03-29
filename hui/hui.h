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

i64 hui_get_frame_num();
Element* current_element();
bool is_unset(Pixels value);
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
void hui_nothing();
void hui_block();

// Layouts
void hui_stack_start(Pixels gap);
void hui_stack_end();

typedef struct {
	Pixels left;
	Pixels right;
	Pixels top;
	Pixels bottom;
} Margin;

static inline Margin msymmetric(Pixels size) { return (Margin) {size, size, size, size}; }
static inline Margin mvertical(Pixels size) { return (Margin) {0, 0, size, size}; }
static inline Margin mhorizontal(Pixels size) { return (Margin) {size, size, 0, 0}; }
static inline Margin mnone() { return (Margin) {0, 0, 0, 0}; }

typedef struct {
	Color  background_color;
	Margin padding;
	Color  border_color;
	Margin border;
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

typedef struct {
	Color color;
	Pixels font_size;
} TextStyle;

void hui_text(str text, TextStyle style);
void hui_text_ex(str text, TextStyle style, Pixels first_line_indent);
void hui_cursor_text(str text, TextStyle style, usize cursor);

bool hui_button(ElementId id, str text, TextStyle style);
u64 hui_text_input(strb* builder, usize* cursor, TextStyle style);

usize hui_get_text_cache_used();
usize hui_get_text_cache_cap();
#endif
