
// IDEA: maybe a sample should be a little functional,
// we could make a simple color picker! maybe it can also process screenshots or
// its 'middle' panel can be like a camera seeing the below window (chrome page or something)

#include "gui.h"


void platform_init(u8 *font_atlas_data);
void platform_update();
void platform_render(guiRenderCommand *rcommands, u32 command_count);
void platform_deinit();


guiState my_gui;
sample_push_event(guiInputEvent e){
    gui_input_push_event(&my_gui, e);
}

void sample_init(){
    gui_state_init(&my_gui);
}

void sample_update(){
    gui_state_update(&my_gui);
}

void sample_push_input_event(guiInputEvent e) {
    gui_input_push_event(&my_gui, e);
}


int main(){
    sample_init();
    platform_init(my_gui.atlas.tex.data);
    for (;;){
        sample_update();
        platform_update();
        platform_render(&my_gui.rcmd_buf.commands[0], gui_render_cmd_buf_count(&my_gui.rcmd_buf));
    }
    platform_deinit();
}