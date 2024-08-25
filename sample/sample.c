#include "gui.h"

void platform_init(u8 *font_atlas_data);
void platform_update();
guiVec2 platform_get_windim();
void platform_render(guiRenderCommand *rcommands, u32 command_count);
void platform_deinit();

guiSimpleWindowData *wdata;

void sample_init(){
    guiState *gui_state = gui_state_init();
    gui_set_ui_state(gui_state);

    guiSimpleWindowData d = {
        .active = 1,
        .dim = gv2(400,300),
        .pos = gv2(100,100),
        .name = "SimpleWindow",
    };
    wdata = ALLOC(sizeof(guiSimpleWindowData));
    memcpy(wdata, &d, sizeof(d));
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

    guiVec4 colors[15] = {
        gv4(0.95f, 0.61f, 0.73f, 1.0f), // Light Pink
        gv4(0.55f, 0.81f, 0.95f, 1.0f), // Sky Blue
        gv4(0.68f, 0.85f, 0.90f, 1.0f), // Light Sky Blue
        gv4(0.67f, 0.88f, 0.69f, 1.0f), // Light Green
        gv4(1.00f, 0.78f, 0.49f, 1.0f), // Peach
        gv4(0.98f, 0.93f, 0.36f, 1.0f), // Light Yellow
        gv4(1.00f, 0.63f, 0.48f, 1.0f), // Coral
        gv4(0.55f, 0.81f, 0.25f, 1.0f), // Light Slate Blue
        gv4(0.85f, 0.44f, 0.84f, 1.0f), // Orchid
        gv4(0.94f, 0.90f, 0.55f, 1.0f), // Light Goldenrod Yellow
        gv4(0.80f, 0.52f, 0.25f, 1.0f), // Peru
        gv4(0.70f, 0.13f, 0.13f, 1.0f), // Firebrick
        gv4(0.56f, 0.93f, 0.56f, 1.0f), // Medium Sea Green
        gv4(0.93f, 0.51f, 0.93f, 1.0f),  // Pale Violet Red
        gv4(0.95f, 0.61f, 0.73f, 1.0f), // Light Goldenrod Yellow
    };

    if (wdata->active){
        gui_swindow_begin(wdata, AXIS2_X);

        f32 header_height = gui_font_get_default_text_height(gui_top_text_scale());
        static guiScrollPoint sp = {0};
        guiScrollListRowBlock sb[10];
        for (u32 i = 0; i < 10; i+=1){ sb[i] = (guiScrollListRowBlock){.item_count = 1, .row_count = 1}; }
        guiScrollListOptions scroll_opt = {
            .dim_px = gv2(wdata->dim.x,wdata->dim.y - header_height),
            .item_range = (guiRange2){0,10},
            .row_blocks = (guiScrollListRowBlockArray){.blocks = sb, .count = array_count(sb)},
            .row_height_px = 50,
        };
        gui_scroll_list_begin(&scroll_opt, &sp);

        // ------------------------------------
        const char *random_words [] = {"hello","there","general","kenobi","I am","General","grievous","nice","to","meet","you"};
        char button_name[128];
        gui_push_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1,1});
        for (u32 i = 0; i < array_count(random_words);i+=1) {
            sprintf(button_name, random_words[i]);
            gui_set_next_bg_color(colors[i]);
            gui_button(button_name);
        }
        // ------------------------------------

        gui_scroll_list_end();
        gui_swindow_end(wdata);
    }
    gui_build_end();

    //wdata->dim.y = 310 + 30 * sin(gui_get_ui_state()->current_frame_index / 30.0);
}

void sample_push_input_event(guiInputEventNode e) {
    gui_input_push_event(e);
}

void mainLoop() {
    for (;;){
        sample_update();
        platform_update();

        platform_render(gui_render_cmd_buf_get_array(&gui_get_ui_state()->rcmd_buf), gui_render_cmd_buf_get_count(&gui_get_ui_state()->rcmd_buf));
    }
}

int main(){
    sample_init();
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