#include "hui/hui.h"
#include "hlib/core.h"
#include "hlib/hstring.h"

str lorem = STR_ARR(
	"Lorem ipsum dolor sit amet, consectetur adipiscing elit. Nulla lobortis purus a metus luctus molestie. Fusce magna dui, aliquet eget eros nec, iaculis luctus turpis. Phasellus commodo, nunc id euismod suscipit, nisl orci posuere sapien, nec convallis magna arcu nec urna. Fusce eleifend lacinia purus. Mauris sagittis ex ut bibendum dapibus. Phasellus et velit nunc. Vestibulum iaculis elementum auctor. Donec eu ultricies tortor. Donec dictum est ligula, quis tincidunt risus elementum viverra. Vestibulum eu sodales tortor. Aenean porttitor est in ex semper tincidunt. Donec pretium nec risus ut lacinia. Integer sollicitudin velit augue."
);

Pixels scroll = 0;
Pixels inner_scroll = 0;
i32 counter = 0;
i32 main(void) {
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_RESIZABLE);
	SetTargetFPS(60);
	InitWindow(800, 600, "Example");
	hui_init();

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
				hui_scroll_start(&scroll);
					hui_stack_start(0);
						hui_fixed_start(800, 200);
							hui_scroll_start(&inner_scroll);
								hui_stack_start(20);
									hui_text(lorem);
									hui_text(lorem);
									hui_text(lorem);
									hui_button(1231, STR("Should not work outside scroll"));
								hui_stack_end();
							hui_scroll_end();
						hui_fixed_end();
						hui_leftright_start(20);
							if(hui_button(6, STR("LEFT"))) {
								scroll++;
							};
							if (hui_button(5, STR("RIGHT"))) {
								scroll--;
							};
						hui_leftright_end();
						hui_cluster_start(40);
							hui_cluster_start(20);
								hui_text(STR("Second"));
								hui_text(STR("Dos"));
								if(hui_button(1, STR("Add 1"))) {
									counter++;
								}
								if(hui_button(2, STR("Substract 1"))) {
									counter--;
								}
								char buf[32];
								snprintf(buf, sizeof(buf), "Counter: %d", counter);
								hui_text(str_from_cstr(buf));
								hui_cluster_start(20);
									for (i32 i = 0; i < counter; i++) {
										hui_box_start(box_style);
											hui_text(STR("Element"));
										hui_box_end();
									}
								hui_cluster_end();
							hui_cluster_end();
						hui_cluster_end();
						hui_block();
						hui_stack_start(10);
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
					hui_stack_end();
				hui_scroll_end();
			hui_root_end();

			int fps = GetFPS();
			DrawText(TextFormat("FPS: %d", fps), 10, 10, 20, GREEN);
		EndDrawing();
	}
	hui_deinit();
	return 0;
}
