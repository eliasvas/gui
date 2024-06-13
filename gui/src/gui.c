#include "gui.h"

guiRect make_gui_rect(float x, float y, float w, float h){
	return (guiRect){x,y,w,h};
}

extern vec2 dxb_get_win_dim();

void gui_build_begin(void) {
	guiState *state = gui_get_ui_state();
	gui_pop_parent();
	//gui_get_build_arena();

	// NOTE: build top level's root guiBox
	guiBox *root = gui_box_build_from_str(0, "zero");
	//guiBox *empty_spacer = gui_box_build_from_str(0, "");
	gui_push_parent(root);
    gui_get_ui_state()->root = root;
}

void gui_build_end(void) {
	if (gui_input_mb_pressed(&gui_get_ui_state()->gis, GUI_RMB)) {
		print_gui_hierarchy();
	}
	guiState *state = gui_get_ui_state();
}