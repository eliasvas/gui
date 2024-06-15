
// Let's just make a Classic game OK?!

#include "gui.h"

void platform_init(u8 *font_atlas_data);
void platform_update();
vec2 platform_get_windim();
void platform_render(guiRenderCommand *rcommands, u32 command_count);
void platform_deinit();


sample_push_event(guiInputEvent e){
    gui_input_push_event(e);
}

void sample_init(){
    guiState *gui_state = gui_state_init();
    gui_set_ui_state(gui_state);
}

void sample_update(){
    gui_get_ui_state()->win_dim.x = platform_get_windim().x;
    gui_get_ui_state()->win_dim.y = platform_get_windim().y;
    gui_build_begin();
    gui_state_update();
    // char debug_str[256] = {0};
    // sprintf(debug_str, "ArenaSz: %dKB", 12);
	// gui_draw_string_in_pos(debug_str, (vec2){0,0}, gui_get_ui_state()->style.base_text_color);
    gui_build_end();
}

void sample_push_input_event(guiInputEvent e) {
    gui_input_push_event(e);
}

int main(){
    sample_init();
    arena_test();
    ll_test();
    platform_init(gui_get_ui_state()->atlas.tex.data);
    for (;;){
        sample_update();
        platform_update();
        platform_render(&gui_get_ui_state()->rcmd_buf.commands[0], gui_render_cmd_buf_count(&gui_get_ui_state()->rcmd_buf));
    }
    platform_deinit();
}