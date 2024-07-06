#include "gui.h"
extern vec2 dxb_get_win_dim();

void gui_build_begin(void) {
	guiState *state = gui_get_ui_state();
	// FIXME: we init (overwrite) our per-frame stacks at the beginning of each frame,
	// this shouldn't happen, somehow at end of frame (gui_build_end) the stacks are not completely clear
	// INVESTIGATE
	gui_init_stacks(state);

	// FIXME: dragging currently kinda broken
	gui_drag_set_current_mp();

	// NOTE: build top level's root guiBox
	gui_set_next_child_layout_axis(AXIS2_Y);
	guiBox *root = gui_box_build_from_str(0, "ImRootPlsDontPutSameHashSomewhereElse");
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
	guiState *state = gui_get_ui_state();
	gui_pop_parent();

	// prune unused boxes
	for (u32 hash_slot = 0; hash_slot < state->box_table_size; hash_slot+=1) {
		for (guiBox *box = state->box_table[hash_slot].hash_first; !gui_box_is_nil(box); box = box->hash_next){
			if (box->last_frame_touched_index < state->current_frame_index) {
				dll_remove_NPZ(&g_nil_box, state->box_table[hash_slot].hash_first, state->box_table[hash_slot].hash_last,box,hash_next,hash_prev);
				sll_stack_push(state->first_free_box, box);
			}
		}
	}

	// do layout pass
	gui_layout_root(state->root, AXIS2_X);
	gui_layout_root(state->root, AXIS2_Y);

	// print hierarchy if need-be
	if (gui_input_mb_pressed(GUI_RMB)) {
		print_gui_hierarchy();
	}

	// render eveything
    gui_render_hierarchy(gui_get_ui_state()->root);

	// advance frame index + clear previous frame's arena
	state->current_frame_index += 1; // This is used to prune unused boxes
	arena_clear(gui_get_build_arena());
}