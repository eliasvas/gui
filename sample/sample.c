
// Let's just make a Classic game OK?!

#include "gui.h"

void platform_init(u8 *font_atlas_data);
void platform_update();
vec2 platform_get_windim();
void platform_render(guiRenderCommand *rcommands, u32 command_count);
void platform_deinit();


void sample_push_event(guiInputEventNode e){
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

    // another UI test
    {

        gui_push_bg_color(v4(0.3,0.3,0.3,1.f));
        gui_set_next_fixed_x(100);
        gui_set_next_fixed_y(100);
        gui_set_next_fixed_width(600);
        gui_set_next_fixed_width(400);
        gui_set_next_child_layout_axis(AXIS2_X);
        gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PIXELS,400.f,0.f});
        gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PIXELS,300.f,0.f});
        guiSignal master_panel = gui_panel("master_panel");
        gui_push_parent(master_panel.box);
        {
            gui_set_next_bg_color(v4(0.6,0.6,0.6,1.f));
            gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,0.5f,0.f});
            gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f,0.f});
            guiSignal slave2_panel = gui_panel("slave2_panel");
            gui_push_parent(slave2_panel.box);
            {
                gui_set_next_child_layout_axis(AXIS2_Y);
                gui_set_next_bg_color(v4(0.8,0.8,0.8,1.f));
                gui_set_next_pref_width((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.f,0.f});
                gui_set_next_pref_height((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.f,0.f});
                guiSignal children_panel = gui_panel("children_panel");
                gui_push_parent(children_panel.box);
                {
                    gui_set_next_child_layout_axis(AXIS2_X);
                    gui_set_next_bg_color(v4(0.8,0.8,0.8,1.f));
                    gui_set_next_pref_width((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.f,0.f});
                    gui_set_next_pref_height((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.f,0.f});
                    guiSignal children_panel33 = gui_panel("children_panel33");
                    gui_push_parent(children_panel33.box);

                    gui_set_next_bg_color(v4(0.5,0.4,0.2,1.f));
                    gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_button("button1");

                    gui_set_next_bg_color(v4(0.2,0.6,0.2,1.f));
                    gui_spacer((guiSize){GUI_SIZEKIND_PIXELS, 5.f, 1.f});
                    gui_set_next_bg_color(v4(0.6,0.2,0.2,1.f));
                    gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_button("button7");
                    gui_pop_parent();


                    gui_set_next_child_layout_axis(AXIS2_X);
                    gui_set_next_bg_color(v4(0.8,0.8,0.8,1.f));
                    gui_set_next_pref_width((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.f,0.f});
                    gui_set_next_pref_height((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.f,0.f});
                    guiSignal children_panel44 = gui_panel("children_panel44");
                    gui_push_parent(children_panel44.box);

                    gui_set_next_bg_color(v4(0.2,0.2,0.6,1.f));
                    gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_button("button4");

                    gui_set_next_bg_color(v4(0.4,0.2,0.4,1.f));
                    gui_spacer((guiSize){GUI_SIZEKIND_PIXELS, 5.f, 1.f});
                    gui_set_next_bg_color(v4(0.6,0.2,0.2,1.f));
                    gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.f,0.f});
                    gui_button("button9");
                    gui_pop_parent();
                }
                gui_pop_parent();
            }
            gui_pop_parent();

            gui_set_next_child_layout_axis(AXIS2_Y);
            gui_set_next_bg_color(v4(0.4,0.4,0.4,1.f));
            gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,0.5f,0.f});
            gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f,0.f});
            guiSignal slave1_panel = gui_panel("slave1_panel");
            gui_push_parent(slave1_panel.box);
            {
                gui_set_next_bg_color(v4(0.3,0.7,0.4,1.f));
                gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f,0.f});
                gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f/4.f,0.f});
                gui_label("label1");

                gui_set_next_bg_color(v4(0.6,0.2,0.4,1.f));
                gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f,0.f});
                gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f/4.f,0.f});
                gui_button("button8");

                gui_set_next_bg_color(v4(0.3,0.4,0.9,1.f));
                gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f,0.f});
                gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f/4.f,0.f});
                gui_label("label2");

                gui_set_next_bg_color(v4(0.2,0.7,0.2,1.f));
                gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f,0.f});
                gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.f/4.f,0.f});
                gui_button("button5");
            }
            gui_pop_parent();
        }
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