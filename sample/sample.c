
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
        gui_push_bg_color(v4(0.3,0.3,0.3,1.f));

        gui_set_next_fixed_x(100);
        gui_set_next_fixed_y(100);
        gui_set_next_child_layout_axis(AXIS2_X);
        guiSignal s0 = gui_panel("master");
        gui_push_parent(s0.box);

        gui_spacer((guiSize){GUI_SIZEKIND_PIXELS, 10.f, 1.f});

        gui_set_next_child_layout_axis(AXIS2_Y);
        guiSignal s = gui_panel("panel");
        gui_push_parent(s.box);
        gui_spacer((guiSize){GUI_SIZEKIND_PIXELS, 5.f, 1.f});
        gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PIXELS, 40.f, 1.f});
        gui_set_next_bg_color(v4(0.1,0.1,0.1,1.f));
        gui_label("label");
        gui_set_next_bg_color(v4(0.4,0.5,0.9,1.f));
        gui_button("button1");
        gui_set_next_bg_color(v4(0.4,0.9,0.4,1.f));
        gui_button("button2");
        gui_set_next_bg_color(v4(0.8,0.6,0.1,1.f));
        gui_button("button3");
        gui_spacer((guiSize){GUI_SIZEKIND_PIXELS, 5.f, 1.f});
        gui_pop_parent();

        gui_spacer((guiSize){GUI_SIZEKIND_PIXELS, 10.f, 1.f});

        gui_pop_parent();
        gui_pop_bg_color();
 
    }

    gui_build_end();
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