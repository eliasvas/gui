#include "gui.h"


guiStatus gui_state_update(guiState *state){
	gui_render_cmd_buf_clear(&state->rcmd_buf);
	gui_input_process_events(&state->gis);
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){100,100}, (vec2){200,200}, (vec4){1,1,1,1});
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){400,400}, (vec2){200,200}, (vec4){1,0,0.5,1});
	return GUI_GUD;
}


guiStatus gui_state_init(guiState *state){
	memzero(state, sizeof(guiState));
	return gui_font_load_from_file(&state->atlas, "C:/windows/fonts/times.ttf");
}