#include "gui.h"


guiStatus gui_state_update(guiState *state){
	gui_input_process_events(&state->gis);
	return GUI_GUD;
}


guiStatus gui_state_init(guiState *state){
	memzero(state, sizeof(guiState));
	return gui_font_load_from_file(&state->atlas, "C:/windows/fonts/times.ttf");
}