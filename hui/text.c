#include "hui.h"
#include "core.c"
#include "../hlib/hhashmap.h"

HHashMap hui_text_cache = {0};

u64 hash_str(str text) {
	u64 result = 0;
	for(usize i = 0; i < text.len; i++) {
		result ^= result*256 + ((u64)text.data[i]);
	}
	return result;
}

typedef struct {
	bool used;
	Pixels height;
	Pixels actual_width;
	Pixels next_glyph_x;
	Pixels next_glyph_y;
	i64 last_frame; // Used for cache invalidation.
	RenderTexture2D texture;
} HUITextCacheValue;

typedef struct {
	u64 hash;
	Pixels width;
	Pixels font_size;
	Pixels first_line_indent;
	// TODO: Add font
} HUITextCacheKey;

#define HUI_TEXT_CACHE_SIZE 256
HUITextCacheKey keys[HUI_TEXT_CACHE_SIZE] = {0};
HUITextCacheValue values[HUI_TEXT_CACHE_SIZE] = {0};

usize hui_get_text_cache_cap() {
	return HUI_TEXT_CACHE_SIZE;
}

usize hui_get_text_cache_used() {
	usize count = 0;
	for(usize i = 0; i < HUI_TEXT_CACHE_SIZE; i++) {
		if (values[i].used) count++;
	}
	return count;
}

u64 hash_key(HUITextCacheKey key) {
	// The font_size and width is not considered for the cache because it makes it so that,
	// upon resizing or changing the font size slightly, the same slot is used,
	// which has an already existing texture which most likely can fit the slightly changed result.
	// See the populate_cache for more details.
	return key.hash; // ^ *(u64*)&key.font_size ^*(u64*)&key.width;
}

#define HUI_TEXT_CACHE_GIVE_UP 20
HUITextCacheValue* populate_cache(str text, u64 text_hash, Pixels first_line_indent, Pixels width, Pixels font_size, u64 key_hash) {
	u64 index = key_hash % HUI_TEXT_CACHE_SIZE;
	i64 frame_num = hui_get_frame_num();
	usize safety;
	for(safety = 0; safety < HUI_TEXT_CACHE_GIVE_UP; safety++) {
		if (values[index].last_frame < hui_get_frame_num()-2) { // Two frame presistance
			values[index].used = false;
		}
		if (!values[index].used) break;
		index = (index+1) % HUI_TEXT_CACHE_SIZE;
	}
	Pixels x = first_line_indent;
	Pixels y = 0;

	// Assuming square glyphs, the height needed to fit al the characters in the given width
	Pixels tentative_height = text.len * font_size*font_size / width;

	tentative_height += font_size; // Extra line of margin, because the previous calculation assumes that all lines are filled.

	Font font = GetFontDefault();
	f32 scale_factor = font_size/(f32)font.baseSize;

	if (
			(values[index].texture.texture.width > width && values[index].texture.texture.width < 2*width)
			&& (values[index].texture.texture.height > tentative_height && values[index].texture.texture.height < 2*tentative_height)
		) {
		// There is already a texture and it is roughly the same size as we need, so we can reuse it.
		// Becuase we only use the text as a hash, this means that if the font size or the width changes a small bit,
		// we will likely reuse the same texture, saving the time of Unloading and Loading textures to the GPU.
	}
	else {
		if (values[index].texture.texture.width) {
			// There is already a texture, but it is too large or too small
			UnloadRenderTexture(values[index].texture);
		}
		values[index].texture = LoadRenderTexture(width*1.5, tentative_height*1.5); // Extra space is added, so it can be reused both it the text grows, or shrinks
	}

	BeginTextureMode(values[index].texture);
	BeginScissorMode(0, 0, width, tentative_height); // TODO: More than 100
	ClearBackground((Color){0,0,0,0});
	int codepoint_bytes = 0;
	for (usize i = 0; i < text.len; i += codepoint_bytes) {
		int chr = GetCodepoint(&text.data[i], &codepoint_bytes);
		int index = GetGlyphIndex(font, chr);
		Pixels chr_width = font.glyphs[index].advanceX ? font.glyphs[index].advanceX : font.recs[index].width;
		chr_width *= scale_factor;
		if (x + chr_width > width) {
			x = 0;
			y += font_size;
		}
		DrawTextCodepoint(font, chr, (Vector2){ .x = x, .y = y }, font_size, WHITE);
		x += chr_width;
		x += font_size*0.1;
	}
	EndScissorMode();
	EndTextureMode();

	Pixels height = y + font_size;
	values[index].used = true;
	values[index].last_frame = frame_num;
	values[index].height = height;
	values[index].next_glyph_x = x;
	values[index].next_glyph_y = y;
	if (y == 0) { // If we never wrapped
		values[index].actual_width = x;
	} else {
		values[index].actual_width = width;
	}
	keys[index].hash = text_hash;
	keys[index].first_line_indent = first_line_indent;
	keys[index].width = width;
	keys[index].font_size = font_size;

	return &values[index];
}

HUITextCacheValue text_render_cached(str text, Pixels first_line_indent, Pixels width, Pixels font_size) {
	u64 text_hash = hash_str(text);
	HUITextCacheKey key = {
		.hash = text_hash,
		.width = width,
		.font_size = font_size,
		.first_line_indent = first_line_indent,
	};
	u64 key_hash = hash_key(key);

	bool found = false;
	u64 index = key_hash % HUI_TEXT_CACHE_SIZE;
	i64 current_frame = hui_get_frame_num();
	for(usize safety = 0; safety < HUI_TEXT_CACHE_GIVE_UP; safety++) {
		if (values[index].used && values[index].last_frame < current_frame-100) {
			values[index].used = false;
			UnloadRenderTexture(values[index].texture);
			values[index].texture = (RenderTexture2D){0};
		}
		if (values[index].used && keys[index].hash == text_hash && keys[index].width == width && keys[index].font_size == font_size && keys[index].first_line_indent == first_line_indent) {
			found = true;
			break;
		}
		index = (index+1) % HUI_TEXT_CACHE_SIZE;
	}
	HUITextCacheValue* value;
	if (found) {
		value = &values[index];
	}
	else {
		value = populate_cache(text, text_hash, first_line_indent, width, font_size, key_hash);
	}
	value->last_frame = hui_get_frame_num();
	return *value;
}

typedef struct {
	str text;
	TextStyle style;
	Pixels first_line_indent;
} HUITextData;

LayoutResult hui_text_layout(Element* element, void* data) {
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &element->layout;
	HUITextData text_data = *(HUITextData*)data;
	str text = text_data.text;
	TextStyle style = text_data.style;

	Pixels width_limit;

	if(is_unset(layout->width)) {
		width_limit = element->parent->layout.width;
	} else {
		width_limit = layout->width;
	}

	HUITextCacheValue cached_text = text_render_cached(text, text_data.first_line_indent, width_limit, style.font_size);

	if (is_unset(layout->width)) {
		layout->width = cached_text.actual_width;
	}

	if(is_unset(layout->height)) {
		layout->height = cached_text.height;
	}
	return result;
}

void hui_text_draw(Element* element, void* data) {
	HUITextData text_data = *(HUITextData*)data;
	str text = text_data.text;
	TextStyle style = text_data.style;

	HUITextCacheValue cached_text = text_render_cached(text, text_data.first_line_indent, element->layout.width, style.font_size);
	DrawTextureRec(
		cached_text.texture.texture,
		(Rectangle){.x = 0, .y = cached_text.texture.texture.height - cached_text.height, .width = element->layout.width, .height = -cached_text.height},
		(Vector2){element->layout.x, element->layout.y},
		style.color
	);
}

void hui_text_ex(str text, TextStyle style, Pixels first_line_indent) {
	Element* element = push_element(sizeof(HUITextData));
	element->draw = hui_text_draw;
	element->compute_layout = hui_text_layout;
	*(HUITextData*)get_element_data(element) = (HUITextData){
		.text = text,
		.style = style,
		.first_line_indent = first_line_indent,
	};
}

void hui_text(str text, TextStyle style) {
	hui_text_ex(text, style, 0);
}

typedef struct {
	str text;
	TextStyle style;
	usize cursor;
} HUICursorTextData;

Pixels max(Pixels a, Pixels b) {
	return a > b ? a : b;
}

LayoutResult hui_cursor_text_layout(Element* element, void* data) {
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &element->layout;
	HUICursorTextData text_data = *(HUICursorTextData*)data;
	str text = text_data.text;
	TextStyle style = text_data.style;
	usize cursor = text_data.cursor;

	Pixels width_limit;

	if(is_unset(layout->width)) {
		width_limit = element->parent->layout.width;
	} else {
		width_limit = layout->width;
	}

	str before_cursor = str_slice(text, 0, cursor);
	str after_cursor = str_slice(text, cursor, text.len);

	HUITextCacheValue cached_before = text_render_cached(before_cursor, 0, width_limit, style.font_size);
	HUITextCacheValue cached_after = text_render_cached(after_cursor, cached_before.next_glyph_x, width_limit, style.font_size);

	if (is_unset(layout->width)) {
		layout->width = max(cached_before.actual_width, cached_after.actual_width);
	}

	if(is_unset(layout->height)) {
		layout->height = cached_before.next_glyph_y + cached_after.height;
	}
	return result;
}

void hui_cursor_text_draw(Element* element, void* data) {
	HUICursorTextData text_data = *(HUICursorTextData*)data;
	str text = text_data.text;
	TextStyle style = text_data.style;
	usize cursor = text_data.cursor;

	str before_cursor = str_slice(text, 0, cursor);
	str after_cursor = str_slice(text, cursor, text.len);

	HUITextCacheValue cached_before = text_render_cached(before_cursor, 0, element->layout.width, style.font_size);
	HUITextCacheValue cached_after = text_render_cached(after_cursor, cached_before.next_glyph_x, element->layout.width, style.font_size);
	DrawTextureRec(
		cached_before.texture.texture,
		(Rectangle){.x = 0, .y = cached_before.texture.texture.height - cached_before.height, .width = element->layout.width, .height = -cached_before.height},
		(Vector2){element->layout.x, element->layout.y},
		BLUE
	);
	DrawTextureRec(
		cached_after.texture.texture,
		(Rectangle){.x = 0, .y = cached_after.texture.texture.height - cached_after.height, .width = element->layout.width, .height = -cached_after.height},
		(Vector2){element->layout.x, element->layout.y + cached_before.next_glyph_y},
		RED
	);

	if (hui_get_frame_num() & 16) {
		DrawRectangle(element->layout.x + cached_before.next_glyph_x, element->layout.y + cached_before.next_glyph_y, style.font_size/8, style.font_size, GREEN);
	}
}

void hui_cursor_text(str text, TextStyle style, usize cursor) {
	Element* element = push_element(sizeof(HUITextData));
	element->draw = hui_cursor_text_draw;
	element->compute_layout = hui_cursor_text_layout;
	*(HUICursorTextData*)get_element_data(element) = (HUICursorTextData){
		.text = text,
		.style = style,
		.cursor = cursor
	};
}
