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
		printf("Key [%u] has been detected twice, which means some box's hash to same ID!\n", key);
		printf("Exiting rn..\n");
		exit(1);
	}

	// if first frame, allocate the box, either from the free box list OR push a new box
	// TODO -- Dis way, our arena will always have allocated the MAX box count ahead of time, maybe not the best way?
	if (box_first_time) {
		//printf("Inserting new key [%u]\n", (u32)key);
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
		// M_ZERO_ARRAY(box->pref_size);
		// M_ZERO_STRUCT(&box->child_layout_axis);
		// M_ZERO_STRUCT(&box->fixed_pos);
		// M_ZERO_STRUCT(&box->fixed_size);
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
		box->flags |= flags;
		box->child_layout_axis = gui_top_child_layout_axis();
		// We are doing all layouting here, we should probably just traverse the hierarchy like Ryan says
		if (state->fixed_x_stack.top != &state->fixed_x_nil_stack_top) {
			box->fixed_pos.raw[AXIS2_X] = state->fixed_x_stack.top->v;
			box->flags |= GUI_BOX_FLAG_FIXED_X;
		}
		if (state->fixed_y_stack.top != &state->fixed_y_nil_stack_top) {
			box->fixed_pos.raw[AXIS2_Y] = state->fixed_y_stack.top->v;
			box->flags |= GUI_BOX_FLAG_FIXED_Y;
		}
		if (state->fixed_width_stack.top != &state->fixed_width_nil_stack_top) {
			box->fixed_size.raw[AXIS2_X] = state->fixed_width_stack.top->v;
			box->flags |= GUI_BOX_FLAG_FIXED_WIDTH;
		}
		if (state->fixed_height_stack.top != &state->fixed_height_nil_stack_top) {
			box->fixed_size.raw[AXIS2_Y] = state->fixed_height_stack.top->v;
			box->flags |= GUI_BOX_FLAG_FIXED_HEIGHT;
		}
		box->pref_size[AXIS2_X] = gui_top_pref_width();
		box->pref_size[AXIS2_Y] = gui_top_pref_height();
		//box->fixed_size = (vec2){gui_top_fixed_width(), gui_top_fixed_height()};
		//box->r = (rect){box->fixed_pos.x, box->fixed_pos.y, box->fixed_pos.x + box->fixed_size.x, box->fixed_pos.y + box->fixed_size.y};
		box->c = gui_top_bg_color();
		box->c.r += box->active_t/3.0f;
	}

	// calculate hot_t and active_t for our box
	{
		//f32 trans_rate = 1.f - pow(2, (-50.f * state->dt));
		f32 trans_rate = 20 * state->dt;

		b32 is_box_hot = gui_key_match(box->key,gui_get_hot_box_key());
		b32 is_box_active = gui_key_match(box->key,gui_get_active_box_key(GUI_LMB));
		box->hot_t += trans_rate * (is_box_hot - box->hot_t);
		box->active_t += trans_rate * (is_box_active - box->active_t);
	}


	// pop all stacks (meaning, EAT all stack.set_next(..))
	gui_autopop_all_stacks();

	return box;
}

void print_gui_box_hierarchy(guiBox *box, u32 depth) {
	if (gui_box_is_nil(box))return;

	if (depth == 0) {
		printf("[%u:%u]\n",box->key, depth);
	}else {
		for (u32 i = 1; i < depth; ++i) {
			printf("\t");
		}
		printf("+----[%u:%u]\n", box->key, depth);
	}
	for (guiBox* child = box->first; !gui_box_is_nil(child); child = child->next) {
		print_gui_box_hierarchy(child, depth+1);
	}
}

void print_gui_hierarchy(void) {
	printf("------------------\n");
	printf("Printing hierarchy..\n");
	printf("g_nil_box=[%p]\n", &g_nil_box);
	printf("zero=[%u]\n", gui_key_from_str("zero"));
	printf("one=[%u]\n", gui_key_from_str("one"));
	printf("two=[%u]\n", gui_key_from_str("two"));
	printf("three=[%u]\n", gui_key_from_str("three"));
	printf("four=[%u]\n", gui_key_from_str("four"));
	guiBox *root = gui_get_ui_state()->root;

	print_gui_box_hierarchy(root, 0);
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

guiKey gui_get_hot_box_key() {
	return gui_get_ui_state()->hot_box_key;
}

guiKey gui_get_active_box_key(GUI_MOUSE_BUTTON b){
	return gui_get_ui_state()->active_box_keys[b];
}

guiSignal gui_get_signal_for_box(guiBox *box) {
	guiSignal signal = {0};
	rect r = box->r;
	vec2 mp = v2(gui_get_ui_state()->gis.mouse_x, gui_get_ui_state()->gis.mouse_y);
	// if mouse inside box, the box is HOT
	if (point_inside_rect(mp, r)) {
		gui_get_ui_state()->hot_box_key = box->key;
	}
	// if mouse inside box AND mouse button pressed, box is ACTIVE, PRESS event
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		if (point_inside_rect(mp, r) && gui_input_mb_pressed(mk)) {
			gui_get_ui_state()->active_box_keys[mk] = box->key;
			// TODO -- This is pretty crappy logic, fix someday
			signal.flags |= (GUI_SIGNAL_FLAG_LMB_PRESSED << mk);
		}
	}
	// if mouse inside box AND mouse button released and box was ACTIVE, reset hot/active RELEASE signal 
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		if (point_inside_rect(mp, r) && gui_input_mb_released(mk) && gui_key_match(gui_get_active_box_key(mk), box->key)) {
			gui_get_ui_state()->hot_box_key = gui_key_zero();
			gui_get_ui_state()->active_box_keys[mk]= gui_key_zero();
			signal.flags |= (GUI_SIGNAL_FLAG_LMB_RELEASED << mk);
		}
	}
	// if mouse outside box AND mouse button released and box was ACTIVE, reset hot/active
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		if (!point_inside_rect(mp, r) && gui_input_mb_released(mk) && gui_key_match(gui_get_active_box_key(mk), box->key)) {
			gui_get_ui_state()->hot_box_key = gui_key_zero();
			gui_get_ui_state()->active_box_keys[mk] = gui_key_zero();
		}
	}
	return signal;
}

guiSignal gui_button(char *str) {
	guiBox *w = gui_box_build_from_str( GUI_BOX_FLAG_CLICKABLE|
									GUI_BOX_FLAG_DRAW_BORDER|
									GUI_BOX_FLAG_DRAW_TEXT|
									GUI_BOX_FLAG_DRAW_BACKGROUND|
									GUI_BOX_FLAG_DRAW_HOT_ANIMATION|
									GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION,
									str);
	guiSignal signal = gui_get_signal_for_box(w);
	//return (signal.flags & GUI_SIGNAL_FLAG_LMB_PRESSED) > 0;
	return signal;
}