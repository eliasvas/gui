#include "gui.h"


b32 gui_box_is_nil(guiBox *box) {
	return (box == 0 || box == &g_nil_box);
}

// lookup a box in our box cache (box_table)
guiBox *gui_box_lookup_from_key(guiBoxFlags flags, guiKey key) {
	guiBox *res = &g_nil_box;
	if (!gui_key_match(key, gui_key_zero())) {
		u64 slot = key % gui_get_ui_state()->box_table_size;
		for (guiBox *box = gui_get_ui_state()->box_table[slot].hash_first; !gui_box_is_nil(box); box = box->hash_next) {
			if (gui_key_match(box->key, key)) {
				res = box;
				break;
			}
		}
	}
	return res;
}

guiBox *gui_box_build_from_key(guiBoxFlags flags, guiKey key) {
	guiState *state = gui_get_ui_state();
	guiBox *parent = gui_top_parent();

	//lookup in persistent guiBox cache for our box
	guiBox *box = gui_box_lookup_from_key(flags, key);
	b32 box_first_time = gui_box_is_nil(box);
	b32 box_is_spacer = gui_key_match(key,gui_key_zero());


	// Crash if we find a key conflict! (maybe we should just Let it Be though)
	if(!box_first_time && box->last_frame_touched_index == gui_get_ui_state()->current_frame_index)
	{
		box = &g_nil_box;
		key = gui_key_zero();
		box_first_time = 1;
		printf("Key [%s] has been detected twice, which means some box's hash to same ID!\n");
		printf("Exiting rn..\n");
		exit(1);
	}

	// if first frame, allocate the box, either from the free box list OR push a new box
	// TODO -- Dis way, our arena will always have allocated the MAX box count ahead of time, maybe not the best way?
	if (box_first_time) {
		printf("Inserting new key [%u]\n", (u32)key);
		box = !box_is_spacer? gui_get_ui_state()->first_free_box : 0;
		if (!gui_box_is_nil(box)) {
			sll_stack_pop(gui_get_ui_state()->first_free_box);
		}
		else {
			box = push_array_nz(box_is_spacer ? gui_get_build_arena() : gui_get_ui_state()->arena, guiBox, 1);
		}
		M_ZERO_STRUCT(box);
	}

	// zero out per-frame data for box (will be recalculated)
	{
		box->first = box->last = box->next = box->prev = box->parent = &g_nil_box;
		box->child_count = 0;
		box->flags = 0;
		box->last_frame_touched_index = gui_get_ui_state()->current_frame_index;
		M_ZERO_ARRAY(box->computed_rel_position);
		M_ZERO_ARRAY(box->computed_size);
	}

	// hook into persistent table (if first time && not spacer)
	if (box_first_time && !box_is_spacer) {
		u64 hash_slot = (u64)key % gui_get_ui_state()->box_table_size;
		dll_insert_NPZ(&g_nil_box, gui_get_ui_state()->box_table[hash_slot].hash_first, gui_get_ui_state()->box_table[hash_slot].hash_last, gui_get_ui_state()->box_table[hash_slot].hash_last, box, hash_next, hash_prev);
	}

	// hook into tree structure (no orphans allowed tho!)
	if (!gui_box_is_nil(parent)) {
		dll_push_back_NPZ(&g_nil_box, parent->first, parent->last, box, next, prev);
		parent->child_count += 1;
		box->parent = parent;
	}

	// fill the box's info stuff
	{
		box->key = key;
	}

	return box;
}

void print_gui_hierarchy(void) {
	printf("------------------\n");
	printf("Printing hierarchy..\n");
	guiBox *parent = gui_top_parent();
	printf("parent node: [%p]\n", parent);
	printf("g_nil_box: [%p]\n", &g_nil_box);
	for (guiBox *child= parent->first; !gui_box_is_nil(child); child = child->next){
		printf("node: [%p]\n", child);

	}
	printf("------------------\n");
}



guiBox *gui_box_build_from_str(guiBoxFlags flags, char *str) {
	guiBox *parent = gui_top_parent();
	guiKey key = gui_key_from_str(str);
	guiBox *box = gui_box_build_from_key(flags, key);
	if (flags & GUI_BOX_FLAG_DRAW_TEXT)
	{
		//printf("Text should be written on [%s] box! fix!", str);
	}
	return box;
}


guiSignal gui_get_signal_for_box(guiBox *box) {
	guiSignal signal = {0};
	// lookup signal cache and find associated signal
	return signal;
}

b32 gui_button(char *str) {

	guiBox *w = gui_box_build_from_str( GUI_BOX_FLAG_CLICKABLE|
									GUI_BOX_FLAG_DRAW_BORDER|
									GUI_BOX_FLAG_DRAW_TEXT|
									GUI_BOX_FLAG_DRAW_BACKGROUND|
									GUI_BOX_FLAG_DRAW_HOT_ANIMATION|
									GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION,
									str);
	guiSignal signal = gui_get_signal_for_box(w);
	return (signal.flags & GUI_SIGNAL_FLAG_LMB_PRESSED) > 0;
}