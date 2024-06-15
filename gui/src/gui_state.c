#include "gui.h"

guiState *ui_state;

guiState *gui_get_ui_state() {
	return ui_state;
}

void gui_set_ui_state(guiState *state) {
	ui_state = state;
}

// forward declared bc I don't want this function to be in the interface (gui.h)
b32 gui_input_mb_down(const guiInputState *gis, guiMouseButton button);
gui_style_default(guiStyle *style);

guiStatus gui_state_update(){
	guiState *state = gui_get_ui_state();

	gui_render_cmd_buf_clear(&state->rcmd_buf);
	gui_input_process_events(&state->gis);
	if (gui_input_mb_down(&state->gis, GUI_LMB)){
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, v2(100,100), v2(200,200), v4(1,1,1,1),3,2,0);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, v2(100,100), v2(200,200), v4(1,0.f,0.5,1),3,2,3);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, v2(400,400), v2(100,100), v4(1,0.f,0.5,1),0,3,0);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, v2(400,400), v2(100,100), v4(1,1,1,1),0,0,2);
	}

	gui_push_bg_color(v4(0.4f,0.4f,0.9f,1.f));
	gui_push_rect((rect){0,0,200,100});
	gui_button("one");
	gui_push_bg_color(v4(1.0,0.5,0.1,1.f));
	gui_push_rect((rect){205,0,500,100});
	gui_button("two");
	gui_pop_bg_color();
	gui_pop_rect();
	gui_pop_rect();

	gui_push_rect((rect){505,0,785,100});
	gui_button("three");
	gui_pop_rect();
	gui_pop_bg_color();
	// guiBox* t = gui_box_build_from_str(0, "two"); //can this be a layout?
	// gui_push_parent(t);
	// gui_button("three");
	// gui_pop_parent();
	// gui_button("four");

	// At the end of every frame, if a box’s last_frame_touched_index < current_frame_index (where, on each frame, the frame index increments), then that box should be “pruned”.
	state->current_frame_index += 1; // This is used to prune unused boxes
	arena_clear(gui_get_build_arena());
	return GUI_GUD;
}

guiState *gui_state_init(){
	Arena *arena = arena_alloc();
	guiState *state = push_array(arena, guiState, 1);
	gui_style_default(&state->style);
	state->arena = arena;
	state->build_arenas[0] = arena_alloc();
	state->build_arenas[1] = arena_alloc();
	state->box_table_size = 4096;
	state->box_table = push_array(arena, guiBoxHashSlot, state->box_table_size);
	gui_font_load_from_file(&state->atlas, "C:/windows/fonts/times.ttf");
	gui_init_stacks(state);
	return state;
}

Arena *gui_get_build_arena() {
	return gui_get_ui_state()->build_arenas[gui_get_ui_state()->current_frame_index%2];
}