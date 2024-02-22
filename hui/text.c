#include "hui.h"
#include "core.c"
#include "../hlib/hhashmap.h"

HHashMap hui_text_cache = {0};

u64 hash_str(str text) {
	u64 result = 0;
	for(usize i = 0; i < text.len; i++) {
		result = result*256 + ((u64)text.data[i]);
	}
	return result;
}

typedef struct {
	bool used;
	Pixels height;
	i64 last_frame; // TODO: Fix wrap
	Rectangle texture_rec;
	RenderTexture2D texture; // TODO: Remove
} HUITextCacheValue;

typedef struct {
	u64 hash;
	Pixels width;
	Pixels font_size;
	// TODO: Add font
} HUITextCacheKey;

#define HUI_TEXT_CACHE_TILE_WIDTH 2000
#define HUI_TEXT_CACHE_TILE_HEIGHT 500
#define HUI_TEXT_CACHE_WIDTH 5
#define HUI_TEXT_CACHE_HEIGHT 10
#define HUI_TEXT_CACHE_SIZE (HUI_TEXT_CACHE_WIDTH*HUI_TEXT_CACHE_HEIGHT)
HUITextCacheKey keys[HUI_TEXT_CACHE_SIZE] = {0};
HUITextCacheValue values[HUI_TEXT_CACHE_SIZE] = {0};
RenderTexture2D cache_texture;
bool cache_init = false;

u64 hash_key(HUITextCacheKey key) {
	return key.hash ^ *(u64*)&key.width ^ *(u64*)&key.font_size;
}

void init_cache() {
	cache_init = true;
	cache_texture = LoadRenderTexture(HUI_TEXT_CACHE_TILE_WIDTH*HUI_TEXT_CACHE_WIDTH, HUI_TEXT_CACHE_TILE_HEIGHT*HUI_TEXT_CACHE_HEIGHT);
	BeginTextureMode(cache_texture);
	ClearBackground(RED);
	EndTextureMode();
}

#define HUI_TEXT_CACHE_GIVE_UP 20
HUITextCacheValue* populate_cache(str text, u64 text_hash, Pixels width, Pixels font_size, u64 key_hash) {
	u64 index = key_hash % HUI_TEXT_CACHE_SIZE;
	i64 frame_num = hui_get_frame_num();
	usize safety;
	for(safety = 0; safety < HUI_TEXT_CACHE_GIVE_UP; safety++) {
		if (values[index].last_frame < hui_get_frame_num()-5) {
			values[index].used = false;
		}
		if (!values[index].used) break;
		index = (index+1) % HUI_TEXT_CACHE_SIZE;
	}

	int x_init = HUI_TEXT_CACHE_TILE_WIDTH*(index%(HUI_TEXT_CACHE_WIDTH));
	int y_init = HUI_TEXT_CACHE_TILE_HEIGHT*(index/(HUI_TEXT_CACHE_WIDTH));
	Pixels x = x_init;
	Pixels y = y_init;

	Font font = GetFontDefault();
	f32 scale_factor = font_size/(f32)font.baseSize;

	BeginTextureMode(cache_texture);
	BeginScissorMode(x_init, y_init, HUI_TEXT_CACHE_TILE_WIDTH, HUI_TEXT_CACHE_TILE_HEIGHT);
	ClearBackground((Color){0,0,0,0});
	int codepoint_bytes = 0;
	for (usize i = 0; i < text.len; i += codepoint_bytes) {
		int chr = GetCodepoint(&text.data[i], &codepoint_bytes);
		int index = GetGlyphIndex(font, chr);
		Pixels chr_width = font.glyphs[index].advanceX ? font.glyphs[index].advanceX : font.recs[index].width;
		chr_width *= scale_factor;
		if (x + chr_width > x_init + width) {
			x = x_init;
			y += font_size;
		}
		DrawTextCodepoint(font, chr, (Vector2){ .x = x, .y = y }, font_size, WHITE);
		x += chr_width;
		x += font_size*0.1;
	}
	EndScissorMode();
	EndTextureMode();

	values[index].used = true;
	values[index].last_frame = frame_num;
	values[index].height = y + font_size - y_init;
	values[index].texture_rec = (Rectangle){ .x = x_init, .y = -HUI_TEXT_CACHE_TILE_HEIGHT-y_init, .width = HUI_TEXT_CACHE_TILE_WIDTH, .height = -HUI_TEXT_CACHE_TILE_HEIGHT};
	keys[index].hash = text_hash;
	keys[index].width = width;
	keys[index].font_size = font_size;

	return &values[index];
}
HUITextCacheValue text_render_cached(str text, Pixels width, Pixels font_size) {
	if (!cache_init) {
		init_cache();
	}
	u64 text_hash = hash_str(text);
	HUITextCacheKey key = {
		.hash = text_hash,
		.width = width,
		.font_size = font_size,
	};
	u64 key_hash = hash_key(key);

	bool found = false;
	u64 index = key_hash % HUI_TEXT_CACHE_SIZE;
	usize safety;
	for(safety = 0; safety < HUI_TEXT_CACHE_GIVE_UP; safety++) {
		if (values[index].used && keys[index].hash == text_hash && keys[index].width == width && keys[index].font_size == font_size) {
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
		value = populate_cache(text, text_hash, width, font_size, key_hash);
	}
	value->last_frame = hui_get_frame_num();
	return *value;
}

typedef struct {
	str text;
	TextStyle style;
} HUITextData;

LayoutResult hui_text_layout(Element* element, void* data) {
	LayoutResult result = LAYOUT_OK;
	Layout* layout = &element->layout;
	HUITextData text_data = *(HUITextData*)data;
	str text = text_data.text;
	TextStyle style = text_data.style;

	if(layout->width == UNSET) {
		layout->width = MeasureText(TextFormat("%.*s", (int)text.len, text.data), style.font_size); // TODO: Handle different fonts
		if(layout->width > element->parent->layout.width) {
			layout->width = element->parent->layout.width;
		}
	}

	HUITextCacheValue cached_text = text_render_cached(text, layout->width, style.font_size);

	if(layout->height == UNSET) {
		layout->height = cached_text.height;
	}
	return result;
}

void hui_text_draw(Element* element, void* data) {
	HUITextData text_data = *(HUITextData*)data;
	str text = text_data.text;
	TextStyle style = text_data.style;

	HUITextCacheValue cached_text = text_render_cached(text, element->layout.width, style.font_size);
	DrawTextureRec(
		cache_texture.texture,
		cached_text.texture_rec,
		(Vector2){element->layout.x, element->layout.y},
		style.color
	);
}

void hui_text(str text, TextStyle style) {
	Element* element = push_element(sizeof(HUITextData));
	element->draw = hui_text_draw;
	element->compute_layout = hui_text_layout;
	*(HUITextData*)get_element_data(element) = (HUITextData){
		.text = text,
		.style = style
	};
}
