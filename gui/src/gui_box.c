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
		M_ZERO_ARRAY(box->pref_size);
		// M_ZERO_STRUCT(&box->child_layout_axis);
		// M_ZERO_STRUCT(&box->fixed_pos);
		// M_ZERO_STRUCT(&box->fixed_size);
	}

	// hook into persistent table (if first time && not spacer)
	if (box_first_time && !box_is_spacer) {
		u64 hash_slot = (u64)key % gui_get_ui_state()->box_table_size;
		dll_insert_NPZ(&g_nil_box, gui_get_ui_state()->box_table[hash_slot].hash_first, gui_get_ui_state()->box_table[hash_slot].hash_last, gui_get_ui_state()->box_table[hash_slot].hash_last, box, hash_next, hash_prev);
	}

	// hook into tree structure
	if (!gui_box_is_nil(parent)) {
		dll_push_back_NPZ(&g_nil_box, parent->first, parent->last, box, next, prev);
		parent->child_count += 1;
		box->parent = parent;
	}

	// fill the box's info stuff
	{
		box->key = key;
		box->flags |= flags;
		// TODO (FIX) -- this is to reset the draw border each frame (its set when widget active)
		box->flags &= ~(GUI_BOX_FLAG_DRAW_BORDER);
		// -----
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
		// FIXED_WIDTH/HEIGHT have NO pref size (GUI_SIZEKIND_NULL) so their fixed_size will stay the same
		if (state->fixed_width_stack.top != &state->fixed_width_nil_stack_top) {
			box->fixed_size.raw[AXIS2_X] = state->fixed_width_stack.top->v;
			box->flags |= GUI_BOX_FLAG_FIXED_WIDTH;
		}else {
			box->pref_size[AXIS2_X] = gui_top_pref_width();
		}
		if (state->fixed_height_stack.top != &state->fixed_height_nil_stack_top) {
			box->fixed_size.raw[AXIS2_Y] = state->fixed_height_stack.top->v;
			box->flags |= GUI_BOX_FLAG_FIXED_HEIGHT;
		}else {
			box->pref_size[AXIS2_Y] = gui_top_pref_height();
		}
		box->c = gui_top_bg_color();
		box->text_color = gui_top_text_color();
		box->text_scale = gui_top_text_scale();
		if (box->flags & GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION) {
			box->c.r += box->active_t/3.0f;
			if (box->flags & GUI_BOX_FLAG_DRAW_ICON) {
				box->text_color.r -= box->active_t/3.0f;
				box->text_color.g -= box->active_t/3.0f;
				box->text_color.b -= box->active_t/3.0f;
			}
		}
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
	if (str){
		strcpy(box->str, str);
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
	signal.box = box;
	rect r = box->r;
	vec2 mp = v2(gui_get_ui_state()->gis.mouse_x, gui_get_ui_state()->gis.mouse_y);

	if (!(box->flags & GUI_BOX_FLAG_CLICKABLE))return signal;

	// if mouse inside box, the box is HOT
	if (point_inside_rect(mp, r)) {
		gui_get_ui_state()->hot_box_key = box->key;
		if (!(box->flags & GUI_BOX_FLAG_DRAW_ICON)) {
			box->flags |= GUI_BOX_FLAG_DRAW_BORDER;
		}
	}
	// if mouse inside box AND mouse button pressed, box is ACTIVE, PRESS event
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		if (point_inside_rect(mp, r) && gui_input_mb_pressed(mk)) {
			gui_get_ui_state()->active_box_keys[mk] = box->key;
			// TODO -- This is pretty crappy logic, fix someday
			signal.flags |= (GUI_SIGNAL_FLAG_LMB_PRESSED << mk);
			gui_drag_set_current_mp();
		}
	}
	// if current box is active, set is as dragging
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		if (gui_key_match(gui_get_active_box_key(mk), box->key)) {
			signal.flags |= (GUI_SIGNAL_FLAG_DRAGGING);
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

guiSignal gui_spacer(guiSize size) {
	guiBox *parent = gui_top_parent();
	gui_set_next_pref_size(parent->child_layout_axis, size);
	guiBox *w = gui_box_build_from_str(0, NULL);
	guiSignal signal = gui_get_signal_for_box(w);
	return signal;
}

// TODO -- maybe panels can be non drawable?? just to get parents and stuff
guiSignal gui_panel(char *str) {
	guiBox *w = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND
	//|GUI_BOX_FLAG_ROUNDED_EDGES
	,str);
	guiSignal signal = gui_get_signal_for_box(w);
	signal.box = w;
	return signal;
}

guiSignal gui_icon(char *str, u32 icon_codepoint) {
	guiBox *w = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_ICON | GUI_BOX_FLAG_DRAW_BACKGROUND | GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION | GUI_BOX_FLAG_CLICKABLE, str);
	w->icon_codepoint = icon_codepoint;
	guiSignal signal = gui_get_signal_for_box(w);
	return signal;
}

guiSignal gui_label(char *str) {
	// TODO -- probably, labels should be sized by text content!
	guiBox *w = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_TEXT|
									GUI_BOX_FLAG_DRAW_BACKGROUND,
									str);
	guiSignal signal = gui_get_signal_for_box(w);
	return signal;
}

// TODO -- maybe an axis can be provided and do slider in that axis
guiSignal gui_slider(char *str, Axis2 axis, vec2 val_range, guiSliderData *data) {
	guiSignal signal;
	vec4 parent_color = gui_top_bg_color();
	char slider_text[256];
	sprintf(slider_text, "slider_%s", str);
	gui_set_next_child_layout_axis(axis);
	guiBox *container = gui_box_build_from_str( GUI_BOX_FLAG_DRAW_BORDER|GUI_BOX_FLAG_DRAW_TEXT| GUI_BOX_FLAG_DRAW_BACKGROUND| GUI_BOX_FLAG_ROUNDED_EDGES, str);
	guiSignal csignal = gui_get_signal_for_box(container);
	gui_push_parent(container);
	{
		guiScrollPoint currentp = data->point;
		f32 val_count = (val_range.max - val_range.min);
		f32 percent = currentp.idx / val_count;
		// TODO -- fix this logic, we mult by 0.8 because 0.2 is used by slider widget
		gui_spacer((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT, percent * (1.0 - 0.1 * (axis+1)), 0.0f});

		if (axis == AXIS2_X) {
			gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,0.1,0.f});
			gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0f,0.f});
		}else {
			gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,0.2,0.f});
			gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0f,0.f});
		}

		parent_color.b += 0.2;
		gui_set_next_bg_color(parent_color);
		guiBox *slider = gui_box_build_from_str( GUI_BOX_FLAG_CLICKABLE | GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION | GUI_BOX_FLAG_DRAW_HOT_ANIMATION | GUI_BOX_FLAG_DRAW_BACKGROUND| GUI_BOX_FLAG_ROUNDED_EDGES, slider_text);
		//gui_spacer((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0 - percent * (1.0 * (axis+1)), 0.0f});
		signal = gui_get_signal_for_box(slider);
		if (signal.flags & GUI_SIGNAL_FLAG_DRAGGING)
		{
			guiScrollPoint originalp = data->point;
			vec2 delta = gui_drag_get_delta();
			u64 new_idx = minimum(maximum(0,originalp.idx + delta.raw[axis]),val_count);
			gui_scroll_point_target_idx(&data->point, new_idx);
			data->value = data->point.idx+val_range.min;
			//gui_scroll_point_clamp_idx(&data->point, val_range);
		}
	}
	gui_pop_parent();
	return signal;
}
guiSignal gui_button(char *str) {
	guiBox *w = gui_box_build_from_str( GUI_BOX_FLAG_CLICKABLE|
									GUI_BOX_FLAG_DRAW_BORDER|
									GUI_BOX_FLAG_DRAW_TEXT|
									GUI_BOX_FLAG_DRAW_BACKGROUND|
									GUI_BOX_FLAG_ROUNDED_EDGES|
									GUI_BOX_FLAG_DRAW_HOT_ANIMATION|
									GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION,
									str);
	guiSignal signal = gui_get_signal_for_box(w);
	return signal;
}

void gui_swindow_do_header(guiSimpleWindowData *window) {
	char panel_name[128];
	sprintf(panel_name, "header_panel_%s", window->name);
	//gui_set_next_child_layout_axis(AXIS2_Y);
	gui_set_next_bg_color(v4(0.3,0.3,0.3,1.0));
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0f,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0,1.0});
	guiSignal panel = gui_panel(panel_name);
	gui_push_parent(panel.box);
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0f,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0,1.0});
	gui_label(window->name);
	gui_spacer((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0, 0.0});
	char icon_name[128];
	sprintf(icon_name, "icon_header_panel_%s", window->name);
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0f,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0,1.0});
	guiSignal icon = gui_icon(icon_name, FA_ICON_CANCEL_CIRCLED2);
	u32 codepoint = gui_key_match(icon.box->key, gui_get_active_box_key(GUI_LMB)) ? FA_ICON_CANCEL_CIRCLED2 : FA_ICON_CANCEL_CIRCLED;
	icon.box->icon_codepoint = codepoint;
	if (icon.flags & GUI_SIGNAL_FLAG_LMB_RELEASED) {
		window->active = 0;
	}
	gui_pop_parent();
}

void gui_swindow_do_main_panel(guiSimpleWindowData *window) {
	char main_panel_name[128];
	sprintf(main_panel_name, "main_panel_%s", window->name);
	gui_set_next_child_layout_axis(AXIS2_Y);
	gui_set_next_bg_color(v4(0.4,0.4,0.4,1.0));
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0f,0.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0,0.0});
	guiSignal panel = gui_panel(main_panel_name);
	gui_push_parent(panel.box);
}

void gui_swindow_begin(guiSimpleWindowData *window) {
	gui_set_next_bg_color(v4(0.2,0.2,0.2,1.0));
	gui_set_next_fixed_x(window->pos.x);
	gui_set_next_fixed_y(window->pos.y);
	gui_set_next_fixed_width(window->dim.x);
	gui_set_next_fixed_height(window->dim.y);
	gui_set_next_child_layout_axis(AXIS2_Y);
	char main_window_area_name[128];
	sprintf(main_window_area_name, "main_area_%s", window->name);
	guiSignal master_panel = gui_panel(main_window_area_name);
	gui_push_parent(master_panel.box);
	gui_swindow_do_header(window);
	//will do the main panel AND set it as parent for next calls
	gui_swindow_do_main_panel(window);
}

void gui_swindow_end(guiSimpleWindowData *window) {
	// pop the main_window_area panel
	gui_pop_parent();
	// pop the main_panel panel
	gui_pop_parent();
}

