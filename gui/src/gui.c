#include "gui.h"

guiRect make_gui_rect(float x, float y, float w, float h){
	return (guiRect){x,y,w,h};
}

extern vec2 dxb_get_win_dim();

void gui_build_begin(void) {
	guiState *state = gui_get_ui_state();

	// NOTE: build top level's root guiBox
	guiBox *root = gui_box_build_from_str(0, "zero");
	//guiBox *empty_spacer = gui_box_build_from_str(0, "");
	gui_push_parent(root);
    gui_get_ui_state()->root = root;

	// reset hot widget
	gui_get_ui_state()->hot_box_key = gui_key_zero();

	// reset active if currently active box no longer cached
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		guiKey active_key = gui_get_active_box_key(mk);
		guiBox *box = gui_box_lookup_from_key(0, active_key);
		b32 box_not_found = gui_box_is_nil(box);
		if (box_not_found) {
			gui_get_ui_state()->active_box_keys[mk] = gui_key_zero();
		}
	}

}

void gui_build_end(void) {
	if (gui_input_mb_pressed(GUI_RMB)) {
		print_gui_hierarchy();
	}
	guiState *state = gui_get_ui_state();
	gui_pop_parent();
}