#include <curl/curl.h>
#include <raylib.h>
#include "./hui/hui.h"
#include "./hlib/core.h"
#include "./hlib/hstring.h"

Pixels scroll = 0;

strb todos[1024] = {0};
usize todos_len = 0;
strb text = {0};

i32 main(void) {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
	InitWindow(800, 600, "Example");
	hui_init();

	text = strb_new();

	BoxStyle box_style = {
		.background_color = {.r = 200, .g = 200, .b = 200, .a = 255},
		.padding = 15,
		.border_color = {.r = 0, .g = 0, .b = 0, .a = 255},
		.border_width = 5,
	};
	(void) box_style;
	TextStyle text_style = {
		.color = BLACK,
		.font_size = 20,
	};
	while(!WindowShouldClose()) {
		BeginDrawing();
			ClearBackground(RAYWHITE);
			hui_root_start();
				hui_scroll_start(&scroll);
					hui_stack_start(0);
						hui_stack_start(0);
							hui_text_input(&text, &text.len, text_style);
							hui_leftright_start(0);
								hui_nothing();
								if (hui_button(1312, STR("Add"), text_style)) {
									todos[todos_len] = strb_from_str(str_from_strb(&text));
									todos_len++;
									text.len = 0;
								}
							hui_leftright_end();
						hui_stack_end();
						hui_stack_start(20);
							hui_nothing();
							for (usize i = 0; i < todos_len; i++) {
								hui_cluster_start(10);
									hui_box_start((BoxStyle){.padding = 20, .background_color = {0}, .border_color = {0}, .border_width = 0});
										hui_text(str_from_strb(&todos[i]), text_style);
									hui_box_end();
									if (hui_button(23273948 + i, STR("Done"), text_style)) {
										strb_free(&todos[i]);
										for (usize j = i; j < todos_len-1; j++) {
											todos[j] = todos[j+1];
										}
										todos_len--;
									}
								hui_cluster_end();
							}
						hui_stack_end();
					hui_stack_end();
				hui_scroll_end();
			hui_root_end();

			int fps = GetFPS();
			DrawText(TextFormat("FPS: %d", fps), 10, 10, 20, GREEN);
		EndDrawing();
	}

	for (usize i = 0; i < todos_len; i++) {
		strb_free(&todos[i]);
	}
	strb_free(&text);

	hui_deinit();
	return 0;
}
