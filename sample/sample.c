
// Let's just make a Classic game OK?!

#include "gui.h"

void platform_init(u8 *font_atlas_data);
void platform_update();
vec2 platform_get_windim();
void platform_render(guiRenderCommand *rcommands, u32 command_count);
void platform_deinit();

guiSliderData *sd1;
guiSliderData *sd2;
guiSimpleWindowData *wdata;

void sample_init(){
    guiState *gui_state = gui_state_init();
    gui_set_ui_state(gui_state);

    sd1 = ALLOC(sizeof(guiSliderData));
    *sd1 = gui_slider_data_make(gui_scroll_point_make(3,0), 0);
    sd2 = ALLOC(sizeof(guiSliderData));
    *sd2 = gui_slider_data_make(gui_scroll_point_make(0,0), 0);
    wdata = ALLOC(sizeof(guiSimpleWindowData));
    wdata->dim = v2(400,300);
    wdata->pos = v2(100,100);
    wdata->active = 1;
    sprintf(wdata->name, "TestWindow");
}

void sample_update(){
    gui_get_ui_state()->win_dim.x = platform_get_windim().x;
    gui_get_ui_state()->win_dim.y = platform_get_windim().y;
    gui_build_begin();
    // TODO -- we need an ACTUAL dt, from the platform layer, there should be a CALL
    gui_state_update(1.0/60.f);

    if (gui_input_mb_pressed(GUI_MMB)){
        wdata->active = 1;
    }

    vec4 colors[15] = {
        v4(0.95f, 0.61f, 0.73f, 1.0f), // Light Pink
        v4(0.55f, 0.81f, 0.95f, 1.0f), // Sky Blue
        v4(0.68f, 0.85f, 0.90f, 1.0f), // Light Sky Blue
        v4(0.67f, 0.88f, 0.69f, 1.0f), // Light Green
        v4(1.00f, 0.78f, 0.49f, 1.0f), // Peach
        v4(0.98f, 0.93f, 0.36f, 1.0f), // Light Yellow
        v4(1.00f, 0.63f, 0.48f, 1.0f), // Coral
        v4(0.55f, 0.81f, 0.25f, 1.0f), // Light Slate Blue
        v4(0.85f, 0.44f, 0.84f, 1.0f), // Orchid
        v4(0.94f, 0.90f, 0.55f, 1.0f), // Light Goldenrod Yellow
        v4(0.80f, 0.52f, 0.25f, 1.0f), // Peru
        v4(0.70f, 0.13f, 0.13f, 1.0f), // Firebrick
        v4(0.56f, 0.93f, 0.56f, 1.0f), // Medium Sea Green
        v4(0.93f, 0.51f, 0.93f, 1.0f),  // Pale Violet Red
        v4(0.95f, 0.61f, 0.73f, 1.0f), // Light Goldenrod Yellow
    };

    if (wdata->active){
        gui_swindow_begin(wdata);


        gui_set_next_bg_color(v4(0.6,0.2,0.4,1.0));
        gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0,1.0});
        gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0/5.0,0.5});
        gui_slider("slider1", AXIS2_X, v2(0,400), sd1);

        for (u32 i = 0; i < 4; ++i) {
            char panel_name[128];
            sprintf(panel_name,"panel_abc%d", i);
            gui_set_next_child_layout_axis(AXIS2_X);
            gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0,1.0});
            gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0/5.0,1.0});
            guiSignal s = gui_panel(panel_name);
            gui_push_parent(s.box);
            for (u32 j = i; j < 5; ++j) {
                char button_name[128];
                sprintf(button_name, "b%d%d", i, j);
                gui_set_next_bg_color(colors[i*(j-1)]);
                gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0,0.0});
                gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0,1.0});
                gui_button(button_name);
            }
            gui_pop_parent();
        }

        gui_swindow_end(wdata);
    }

    gui_build_end();
}

void sample_push_input_event(guiInputEventNode e) {
    gui_input_push_event(e);
}

void mainLoop() {
    for (;;){
        sample_update();
        platform_update();
        platform_render(&gui_get_ui_state()->rcmd_buf.commands[0], gui_render_cmd_buf_count(&gui_get_ui_state()->rcmd_buf));
    }
}

int main(){
    sample_init();
    arena_test();
    ll_test();
    platform_init(gui_get_ui_state()->atlas.tex.data);

    // EMSCRIPTEN BULLSHIT
    #ifdef __EMSCRIPTEN__
        int fps = 0; // Use browser's requestAnimationFrame
        emscripten_set_main_loop_arg(mainLoop, NULL, fps, 1);
    #else
        while(1)
            mainLoop();
    #endif

    platform_deinit();
}