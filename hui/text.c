#include "hui.h"
#include "core.c"
#include "../hlib/hhashmap.h"

HHashMap hui_text_cache;

u64 hash_str(str text) {
	u64 result = 0;
	for(usize i = 0; i < text.len; i++) {
		result = result*256 + ((u64)text.data[i]);
	}
	return result;
}

typedef struct {
	RenderTexture2D texture;
	Pixels height;
} HUITextCacheValue;

typedef struct {
	u64 hash;
	Pixels width;
	Pixels font_size;
	// TODO: Add font and font size
} HUITextCacheKey;

HUITextCacheValue text_render(str text, Pixels width, Pixels font_size) {
	RenderTexture2D texture = LoadRenderTexture(width, 1000); // TODO: Dynamic height
	Pixels x = 0;
	Pixels y = 0;

	Font font = GetFontDefault();
	f32 scale_factor = font_size/(f32)font.baseSize;

	BeginTextureMode(texture);
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
	EndTextureMode();

	return (HUITextCacheValue) { .texture = texture, .height = y + font_size};
}

HUITextCacheValue text_render_cached(str text, Pixels width, Pixels font_size) {
	if(!hui_text_cache.cap) {
		hui_text_cache = hhashmap_new_with_cap(sizeof(HUITextCacheKey), sizeof(HUITextCacheValue), HKEYTYPE_DIRECT, 1024);
	}
	u64 hash = hash_str(text);
	HUITextCacheKey key = {
		.hash = hash,
		.width = width,
		.font_size = font_size,
	};
	HUITextCacheValue* value_ptr = hhashmap_get(&hui_text_cache, &key);
	HUITextCacheValue value;
	if (value_ptr) {
		value = *value_ptr;
	}
	else {
		HUITextCacheValue value = text_render(text, width, font_size);
		value = text_render(text, width, font_size);
		hhashmap_set(&hui_text_cache, &key, &value);
	}
	return value;
}

LayoutResult hui_text_layout(Element* element, void* data) {
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &element->layout;
	str text = *(str*)data;

	if(layout->width == UNSET) {
		layout->width = MeasureText(TextFormat("%.*s", (int)text.len, text.data), 20); // TODO: Handle different fonts
		if(layout->width > element->parent->layout.width) {
			layout->width = element->parent->layout.width;
		}
	}

	HUITextCacheValue cached_text = text_render_cached(text, layout->width, 20);

	if(layout->height == UNSET) {
		layout->height = cached_text.height;
	}
	return result;
}

void hui_text_draw(Element* element, void* data) {
	str text = *(str*)data;
	HUITextCacheValue cached_text = text_render_cached(text, element->layout.width, 20);
	DrawTextureRec(
		cached_text.texture.texture,
		(Rectangle){ .x = 0, .y = 0, .width = cached_text.texture.texture.width, .height = -cached_text.texture.texture.height },
		(Vector2){element->layout.x, element->layout.y},
		BLACK
	);
}

void hui_text(str text) {
	Element* element = push_element(sizeof(str));
	element->draw = hui_text_draw;
	element->compute_layout = hui_text_layout;
	*(str*)get_element_data(element) = text;
}
