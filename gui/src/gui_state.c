#include "gui.h"

guiState *ui_state;

guiState *gui_get_ui_state() {
	return ui_state;
}

void gui_set_ui_state(guiState *state) {
	ui_state = state;
}

// forward declared bc I don't want this function to be in the interface (gui.h)
b32 gui_input_mb_down(const guiInputState *gis, GUI_MOUSE_BUTTON button);
gui_style_default(guiStyle *style);

guiStatus gui_state_update(){
	guiState *state = gui_get_ui_state();

	// Update all event stuff
	gui_render_cmd_buf_clear(&state->rcmd_buf);
	gui_input_process_events(&state->gis);

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