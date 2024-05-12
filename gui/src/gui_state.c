#include "gui.h"

// forward declared bc I don't want this function to be in the interface (gui.h)
b32 gui_input_mb_down(const guiInputState *gis, guiMouseButton button);

guiStatus gui_state_update(guiState *state){
	// At the end of every frame, if a box’s last_frame_touched_index < current_frame_index (where, on each frame, the frame index increments), then that box should be “pruned”.
	state->current_frame_index += 1; // This is used to prune unused boxs
	gui_render_cmd_buf_clear(&state->rcmd_buf);
	gui_input_process_events(&state->gis);
	if (gui_input_mb_down(&state->gis, GUI_LMB)){
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){100,100}, (vec2){200,200}, (vec4){1,1,1,1},3,2,0);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){100,100}, (vec2){200,200}, (vec4){1,0,0.5,1},3,2,3);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){400,400}, (vec2){100,100}, (vec4){1,0,0.5,1},0,5,0);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){400,400}, (vec2){100,100}, (vec4){1,1,1,1},0,0,2);
	}



	// SOME SAMPLE RENDERING COMMANDS
	gui_draw_string_in_pos(state, "Die", (vec2){100,100});
	vec2 die_box = gui_font_get_string_dim(&state->atlas, "Die"); 
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){100,100}, (vec2){die_box.x,die_box.y}, (vec4){1,1,0,1},0,0,1);

	gui_button("Click me!");

	return GUI_GUD;
}


guiStatus gui_state_init(guiState *state){
	memzero(state, sizeof(guiState));
	return gui_font_load_from_file(&state->atlas, "C:/windows/fonts/times.ttf");
}