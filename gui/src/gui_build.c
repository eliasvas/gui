#include "gui.h"
extern guiVec2 dxb_get_win_dim();

void gui_build_begin(void) {
	guiState *state = gui_get_ui_state();
	// We init all stacks here because they are STRICTLY per-frame data structures
	gui_init_stacks(state);


	// NOTE: build top level's root guiBox
	gui_set_next_child_layout_axis(AXIS2_Y);
	gui_push_text_scale(gui_get_ui_state()->global_text_scale);
	guiBox *root = gui_box_build_from_str(0, "ImRootPlsDontPutSameHashSomewhereElse");
	gui_push_parent(root);
    gui_get_ui_state()->root = root;

	// reset hot if box pruned
	{
		guiKey hot_key = gui_get_hot_box_key();
		guiBox *box = gui_box_lookup_from_key(0, hot_key);
		b32 box_not_found = gui_box_is_nil(box);
		if (box_not_found) {
			gui_get_ui_state()->hot_box_key = gui_key_zero();
		}
	}

	// reset active if box pruned
	b32 active_exists = 0;
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		guiKey active_key = gui_get_active_box_key(mk);
		guiBox *box = gui_box_lookup_from_key(0, active_key);
		b32 box_not_found = gui_box_is_nil(box);
		if (box_not_found) {
			gui_get_ui_state()->active_box_keys[mk] = gui_key_zero();
		}else {
			active_exists = 1;
		}
	}
	// reset hot if there is no active
	if (!active_exists) {
		gui_get_ui_state()->hot_box_key = gui_key_zero();
	}

}

void gui_build_end(void) {
	guiState *state = gui_get_ui_state();
	gui_pop_text_scale();
	gui_pop_parent();

	// prune unused boxes
	for (u32 hash_slot = 0; hash_slot < state->box_table_size; hash_slot+=1) {
		for (guiBox *box = state->box_table[hash_slot].hash_first; !gui_box_is_nil(box); box = box->hash_next){
			if (box->last_frame_touched_index < state->current_frame_index) {
				dll_remove_NPZ(gui_box_nil_id(), state->box_table[hash_slot].hash_first, state->box_table[hash_slot].hash_last,box,hash_next,hash_prev);
				sll_stack_push(state->first_free_box, box);
			}
		}
	}

	// do layout pass
	gui_layout_root(state->root, AXIS2_X);
	gui_layout_root(state->root, AXIS2_Y);

	// print hierarchy if need-be
	// if (gui_input_mb_pressed(GUI_RMB)) {
	// 	print_gui_hierarchy();
	// }

	gui_drag_set_current_mp();

	// do animations
	for (u32 hash_slot = 0; hash_slot < state->box_table_size; hash_slot+=1) {
		for (guiBox *box = state->box_table[hash_slot].hash_first; !gui_box_is_nil(box); box = box->hash_next){
			// TODO -- do some logarithmic curve here, this is not very responsive!
			f32 trans_rate = 10 * state->dt;

			b32 is_box_hot = gui_key_match(box->key,gui_get_hot_box_key());
			b32 is_box_active = gui_key_match(box->key,gui_get_active_box_key(GUI_LMB));

			box->hot_t += trans_rate * (is_box_hot - box->hot_t);
			box->active_t += trans_rate * (is_box_active - box->active_t);
		}
	}

	// render eveything
    gui_render_hierarchy(gui_get_ui_state()->root);

	// clear previous frame's arena + advance frame_index
	gui_arena_clear(gui_get_build_arena());
	state->current_frame_index += 1;
}