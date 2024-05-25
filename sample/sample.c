
// Let's just make a Classic game OK?!

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
    char debug_str[256] = {0};
    sprintf(debug_str, "ArenaSz: %dKB", 12);
	gui_draw_string_in_pos(&my_gui, debug_str, (vec2){0,0}, my_gui.style.base_text_color);
}

void sample_push_input_event(guiInputEvent e) {
    gui_input_push_event(&my_gui, e);
}

void hello_arena() {
    printf("Hello Arena!\n");
    ArenaTemp temp = arena_get_scratch(NULL);

    Arena *arena = arena_alloc();
    u8 arr[5560];
    //void* other_mem = push_array(arena, u8, 256);
   
    u8 *mem = arena_push_nz(arena, kilobytes(1));
    memcpy(mem, arr, 2560);
    mem = arena_push_nz(arena, gigabytes(0.9));
    printf("arena_current_pos=[%llud]", arena_current_pos(arena));
    ArenaTemp t = arena_begin_temp(arena);
    void *large_mem = arena_push_nz(arena, gigabytes(10));
    printf("\nafter [10GB] arena_current_pos=[%llud]", arena_current_pos(arena));
    printf("\nafter [10GB] temp_arena_current_pos=[%llud]", arena_current_pos(t.arena));
    arena_end_temp(&t);
    printf("\nafter [POP] arena_current_pos=[%llud]", arena_current_pos(arena));
    for (int i = 0; i < 9; ++i) {
        mem[i] = '0'+(9 - i);
    }
    printf("%s\n", &mem[0]);
    // mem = arena_push(arena, gigabytes(1));
    // mem = arena_push_nz(arena, gigabytes(5));
    arena_release(arena);
    arena_end_temp(&temp);
    printf("Bye Arena!\n");
}

int main(){
    sample_init();
    hello_arena();
    platform_init(my_gui.atlas.tex.data);
    for (;;){
        sample_update();
        platform_update();
        platform_render(&my_gui.rcmd_buf.commands[0], gui_render_cmd_buf_count(&my_gui.rcmd_buf));
    }
    platform_deinit();
}