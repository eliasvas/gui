#include "gui.h"

// Forward declared because I dont wanna expose these to the main API (gui.h)
void gui_input_process_event_queue(void);
void gui_input_clear_event_queue(void);

guiState *ui_state;

guiState *gui_get_ui_state() {
	return ui_state;
}

void gui_set_ui_state(guiState *state) {
	ui_state = state;
}

guiStatus gui_state_update(f32 dt){
	guiState *state = gui_get_ui_state();
	state->dt = dt;


	// Update all event stuff
	gui_render_cmd_buf_clear(&state->rcmd_buf);
	// We process all events and set our state (guiInputState)
	gui_input_process_event_queue();
	gui_input_clear_event_queue();

	return GUI_GUD;
}

guiState *gui_state_init(){
	guiArena *arena = gui_arena_alloc();
	guiState *state = gui_push_array(arena, guiState, 1);
	state->arena = arena;
	state->build_arenas[0] = gui_arena_alloc();
	state->build_arenas[1] = gui_arena_alloc();
	// TODO -- maybe gis should have its own init?
	state->gis.event_arena = gui_arena_alloc();
	state->box_table_size = 4096;
	state->box_table = gui_push_array(arena, guiBoxHashSlot, state->box_table_size);

	gui_font_load_default_font(state->build_arenas[0], &state->atlas);

	gui_init_stacks(state);

	state->global_text_scale = 1.0f;
	return state;
}


guiArena *gui_get_build_arena() {
	return gui_get_ui_state()->build_arenas[gui_get_ui_state()->current_frame_index%2];
}