
// Let's just make a Classic game OK?!

#include "gui.h"

void platform_init(u8 *font_atlas_data);
void platform_update();
vec2 platform_get_windim();
void platform_render(guiRenderCommand *rcommands, u32 command_count);
void platform_deinit();


sample_push_event(guiInputEventNode e){
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
    // TODO -- we need an ACTUAL dt, from the platform layer, there should be a CALL
    gui_state_update(1.f/60.f);

    //our test UI
    {
        static b32 box_pressed = 0;

        f32 pad_x = 10;
        f32 bhs_w = 100;
        f32 bhs_h = 50;
        f32 ww = platform_get_windim().x;
        f32 wh = platform_get_windim().y;
        rect base_button_rect = (rect){ww/2.f - bhs_w, wh/2.f - bhs_h, ww/2.f + bhs_w, wh/2.f + bhs_h};

        gui_set_next_child_layout_axis(AXIS2_X);
        gui_set_next_bg_color(v4(0.6,0.5,0.1,1.f));
        guiSignal s = gui_button("one");
        gui_push_parent(s.box);

        gui_set_next_bg_color(v4(0.4,0.5,0.9,1.f));
        gui_button("two");

        gui_set_next_bg_color(v4(0.2,0.9,0.3,1.f));
        gui_button("three");
        gui_pop_parent();
 
    }

    gui_build_end();
    gui_render_hierarchy(gui_get_ui_state()->root);
}

void sample_push_input_event(guiInputEventNode e) {
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