#include "gui.h"
static guiBox g_nil_box = {
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box
};


guiBox *gui_box_nil_id() {
	return &g_nil_box;
}
b32 gui_box_is_nil(guiBox *box) {
	return (box == 0 || box == gui_box_nil_id());
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
			box = gui_push_array_nz(box_is_spacer ? gui_get_build_arena() : gui_get_ui_state()->arena, guiBox, 1);
		}
		GUI_M_ZERO_STRUCT(box);
	}

	// zero out per-frame data for box (will be recalculated)
	{
		box->first = box->last = box->next = box->prev = box->parent = &g_nil_box;
		box->child_count = 0;
		box->flags = 0;
		box->last_frame_touched_index = gui_get_ui_state()->current_frame_index;
		GUI_M_ZERO_ARRAY(box->pref_size);
		// GUI_M_ZERO_STRUCT(&box->child_layout_axis);
		// GUI_M_ZERO_STRUCT(&box->fixed_pos);
		// GUI_M_ZERO_STRUCT(&box->fixed_size);
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
	guiVec2 mp = gv2(gui_get_ui_state()->gis.mouse_x, gui_get_ui_state()->gis.mouse_y);

	// if a parent has FLAG_CLIP, we intersect its childrens rects to clip them
	guiRect r = box->r; for(guiBox *p = box->parent; !gui_box_is_nil(p); p = p->parent) {
		if (p->flags & GUI_BOX_FLAG_CLIP) {
			r = gui_intersect_rects(box->r,p->r);
			break;
		}
	}
	b32 mouse_inside_box = gui_point_inside_rect(mp, r);

	// preform scrolling via scroll wheel if widget in focus
	if (mouse_inside_box && (box->flags & GUI_BOX_FLAG_SCROLL)) {
		signal.flags |= GUI_SIGNAL_FLAG_SCROLLED;
	}

	// FIXME -- What the FUck? ////////
	if (!(box->flags & GUI_BOX_FLAG_CLICKABLE))return signal;
	///////////////////////////////////

	// if mouse inside box, the box is HOT
	if (mouse_inside_box && (box->flags & GUI_BOX_FLAG_CLICKABLE)) {
		gui_get_ui_state()->hot_box_key = box->key;
		box->flags |= GUI_BOX_FLAG_HOVERING;
	}
	// if mouse inside box AND mouse button pressed, box is ACTIVE, PRESS event
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		if (mouse_inside_box && gui_input_mb_pressed(mk)) {
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
		if (mouse_inside_box && gui_input_mb_released(mk) && gui_key_match(gui_get_active_box_key(mk), box->key)) {
			gui_get_ui_state()->hot_box_key = gui_key_zero();
			gui_get_ui_state()->active_box_keys[mk]= gui_key_zero();
			signal.flags |= (GUI_SIGNAL_FLAG_LMB_RELEASED << mk);
		}
	}
	// if mouse outside box AND mouse button released and box was ACTIVE, reset hot/active
	for (each_enumv(GUI_MOUSE_BUTTON, mk)) {
		if (!mouse_inside_box && gui_input_mb_released(mk) && gui_key_match(gui_get_active_box_key(mk), box->key)) {
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

guiSignal gui_clickable_region(char *str) {
	guiBox *w = gui_box_build_from_str( GUI_BOX_FLAG_CLICKABLE,
									str);
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
guiSignal gui_slider(char *str, Axis2 axis, guiVec2 val_range, guiSliderData *data) {
	guiSignal signal;
	guiVec4 parent_color = gui_top_bg_color();
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
			guiVec2 delta = gui_drag_get_delta();
			u64 new_idx = minimum(maximum(0,originalp.idx + delta.raw[axis]),val_count);
			gui_scroll_point_target_idx(&data->point, new_idx);
			data->value = data->point.idx+val_range.min;
			//gui_scroll_point_clamp_idx(&data->point, val_range);
		}
	}
	gui_pop_parent();
	return signal;
}

// TODO -- FIX this, when two widgets have equal values, we will have dupelicate IDs!!
// TODO right now we dont do anything for different axis
// we make a parent with GUI_SIZEKIND_SUM_OF_CHILDREN, and put SIZEKIND_TEXT_CONTEXT label inside!
guiSignal gui_spinner(char *str, Axis2 axis, guiVec2 val_range, guiSliderData *data) {
	s32 val = (s32)data->value;
	char val_str[64] = {0};
	sprintf(val_str, "%d", val);

	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.0,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_CHILDREN_SUM,1.0,1.0});
	//guiBox *parent = gui_box_build_from_str( GUI_BOX_FLAG_DRAW_BACKGROUND | GUI_BOX_FLAG_ROUNDED_EDGES ,str);
	guiBox *parent = gui_box_build_from_str( GUI_BOX_FLAG_DRAW_BACKGROUND | GUI_BOX_FLAG_ROUNDED_EDGES ,str);
	gui_push_parent(parent);


	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.0,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.0,1.0});
	char wname[64] = {0};
	sprintf(wname, "left_%s", str);
	guiSignal icon_left = gui_icon(wname, FA_ICON_LEFT_OPEN);
	if (icon_left.flags & GUI_SIGNAL_FLAG_LMB_RELEASED) {
		if(data->point.idx > 0) {
			guiScrollPoint originalp = data->point;
			f32 val_count = (val_range.max - val_range.min);
			guiVec2 delta = gv2(-1,0);
			u64 new_idx = minimum(maximum(0,originalp.idx + delta.raw[axis]),val_count);
			gui_scroll_point_target_idx(&data->point, new_idx);
			data->value = data->point.idx+val_range.min;

		}
	}


	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.0,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.0,1.0});
	guiBox *middle = gui_box_build_from_str( GUI_BOX_FLAG_CLICKABLE|
									GUI_BOX_FLAG_DRAW_BORDER|
									GUI_BOX_FLAG_DRAW_TEXT|
									GUI_BOX_FLAG_DRAW_BACKGROUND|
									GUI_BOX_FLAG_DRAW_HOT_ANIMATION|
									GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION,
									val_str);
	guiSignal signal = gui_get_signal_for_box(middle);

	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.0,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,5.0,1.0});
	sprintf(wname, "right_%s", str);
	guiSignal icon_right = gui_icon(wname, FA_ICON_RIGHT_OPEN);
	if (icon_right.flags & GUI_SIGNAL_FLAG_LMB_RELEASED) {
		if(data->point.idx < abs(val_range.y-val_range.x)) {
			guiScrollPoint originalp = data->point;
			f32 val_count = (val_range.max - val_range.min);
			guiVec2 delta = gv2(1,0);
			u64 new_idx = minimum(maximum(0,originalp.idx + delta.raw[axis]),val_count);
			gui_scroll_point_target_idx(&data->point, new_idx);
			data->value = data->point.idx+val_range.min;

		}
	}
	gui_pop_parent();
	return signal;
}

guiSignal gui_button(char *str) {
	guiBox *w = gui_box_build_from_str( GUI_BOX_FLAG_CLICKABLE|
									GUI_BOX_FLAG_DRAW_TEXT|
									GUI_BOX_FLAG_DRAW_BACKGROUND|
									GUI_BOX_FLAG_ROUNDED_EDGES|
									GUI_BOX_FLAG_DRAW_HOT_ANIMATION|
									GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION,
									str);
	guiSignal signal = gui_get_signal_for_box(w);
	if (signal.box->flags & GUI_BOX_FLAG_HOVERING) {
		w->flags |= GUI_BOX_FLAG_DRAW_BORDER;
	}
	return signal;
}
guiSignal gui_checkbox(char *str, b32 *value) {
	guiVec4 col = gui_top_bg_color();
	if (*value == 0) {
		col = gv4(col.x/4,col.y/4,col.z/4,col.w);
	}
	gui_set_next_bg_color(col);
	guiSignal sig = gui_button(str);
	if (sig.flags & GUI_SIGNAL_FLAG_LMB_RELEASED) {
		*value = 1 - *value;
	}
	return sig;
}

void gui_swindow_do_header(guiSimpleWindowData *window) {
	char wname[128];
	sprintf(wname, "header_panel_%s", window->name);
	gui_push_bg_color(gv4(0.2,0.2,0.2,1.0));
	gui_set_next_child_layout_axis(AXIS2_X);
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0f,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,0.0,1.0});
	guiSignal panel = gui_panel(wname);
	gui_push_parent(panel.box);
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0f,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,0.0,1.0});
	gui_set_next_text_color(gv4(1.0,0.6,0.6,1.0));
	gui_label(window->name);
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0f,0.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1.0,0.0});
	sprintf(wname, "clickable_reg%s", window->name);
	guiSignal clickable_region_sig = gui_clickable_region(wname);
	b32 is_clickable_region_active = gui_key_match(clickable_region_sig.box->key, gui_get_active_box_key(GUI_LMB));
	if (is_clickable_region_active) {
		guiVec2 md = gui_drag_get_delta();
		window->pos.x += md.x;
		window->pos.y += md.y;
	}
	sprintf(wname, "icon_header_panel_%s", window->name);
	gui_set_next_pref_width((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0f,1.0});
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,1.0,1.0});
	guiSignal icon = gui_icon(wname, FA_ICON_CANCEL_CIRCLED2);
	u32 codepoint = gui_key_match(icon.box->key, gui_get_active_box_key(GUI_LMB)) ? FA_ICON_CANCEL_CIRCLED2 : FA_ICON_CANCEL_CIRCLED;
	icon.box->icon_codepoint = codepoint;
	if (icon.flags & GUI_SIGNAL_FLAG_LMB_RELEASED) {
		window->active = 0;
	}
	gui_pop_parent();
	gui_pop_bg_color();
}

void gui_swindow_do_main_panel(guiSimpleWindowData *window, Axis2 layout_axis) {
	char main_panel_name[128];
	sprintf(main_panel_name, "main_panel_%s", window->name);
	gui_set_next_child_layout_axis(layout_axis);
	f32 header_height = gui_font_get_default_text_height(gui_top_text_scale());
	gui_set_next_fixed_width(window->dim.x);
	gui_set_next_fixed_height(window->dim.y - header_height);
	guiBox *main_panel_area = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND, main_panel_name);
	guiSignal main_panel_signal = gui_get_signal_for_box(main_panel_area);
	gui_push_parent(main_panel_area);
}

void gui_swindow_begin(guiSimpleWindowData *window, Axis2 layout_axis) {
	gui_push_bg_color(gv4(0.4,0.4,0.4,1.0));
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
	gui_swindow_do_main_panel(window, layout_axis);
	//----------------------------------------

}

void gui_swindow_end(guiSimpleWindowData *window) {
	// pop main panel
	gui_pop_parent();
	// pop master panel
	gui_pop_parent();
	// pop the 0.4 gs color
	gui_pop_bg_color();
}


guiScrollPoint gui_scroll_bar(Axis2 axis, guiScrollPoint sp, guiRange2 row_range, s64 num_of_visible_rows, b32 with_buttons) {
	gui_push_pref_width((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT,1,0});
	const char *scroll_name = "scroll_scroll";
	s64 row_range_dim = maximum(row_range.max - row_range.min, 1);
	char box_name[128];


	//main scroll area
	sprintf(box_name, "scroll_main_%s", scroll_name);
	gui_set_next_child_layout_axis(axis);
	guiBox *scroll_main_area = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND, box_name);
	guiSignal scroll_main_signal = gui_get_signal_for_box(scroll_main_area);
	gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT, 1,0});
	gui_push_parent(scroll_main_area);

	// left/up button
	guiSignal up_sig = {0};
	if (with_buttons){
		sprintf(box_name, "up_button_%s", scroll_name);
		gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT, 0,0});
		up_sig = gui_icon("box_name", (axis == AXIS2_Y) ? FA_ICON_UP_OPEN : FA_ICON_LEFT_OPEN);
	}

	//scrollbar area
	sprintf(box_name, "scroll_bar_%s", scroll_name);
	gui_set_next_child_layout_axis(axis);
	guiBox *scrollbar_area = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND, box_name);
	guiSignal scrollbar_signal = gui_get_signal_for_box(scrollbar_area);
	gui_push_parent(scrollbar_area);
	// space before scroll slider
	guiSignal before_sig = {0};
	if (row_range.min != row_range.max) {
		gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT, (sp.idx-row_range.min)/((f32)row_range_dim),0});
		sprintf(box_name, "scroll_bar_before_%s", scroll_name);
		guiBox * before_box = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND | GUI_BOX_FLAG_CLICKABLE , box_name);
		before_sig = gui_get_signal_for_box(before_box);
	}
	// scroll slider
	guiSignal slider_sig = {0};
	{
		gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT, (num_of_visible_rows)/((f32)row_range_dim),0});
		gui_set_next_bg_color(gv4(0.6,0.4,0.4,1));
		sprintf(box_name, "scroll_bar_slider_%s", scroll_name);
		slider_sig = gui_button(box_name);
		// don't show the label for this button
		slider_sig.box->flags &= ~(GUI_BOX_FLAG_DRAW_TEXT);
	}
	// space after scroll slider
	guiSignal after_sig = {0};
	if (row_range.min != row_range.max) {
		gui_set_next_pref_height((guiSize){GUI_SIZEKIND_PERCENT_OF_PARENT, 1.0f-(sp.idx-row_range.min)/((f32)row_range_dim),0});
		sprintf(box_name, "scroll_bar_after_%s", scroll_name);
		guiBox *after_box = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND | GUI_BOX_FLAG_CLICKABLE , box_name);
		after_sig = gui_get_signal_for_box(after_box);
	}
	// pop main scroll area
	gui_pop_parent();

	// right/down button
	guiSignal down_sig = {0};
	if (with_buttons){
		sprintf(box_name, "down_button_%s", scroll_name);
		gui_set_next_pref_height((guiSize){GUI_SIZEKIND_TEXT_CONTENT,0,0});
		down_sig = gui_icon(box_name, (axis == AXIS2_Y) ? FA_ICON_DOWN_OPEN : FA_ICON_RIGHT_OPEN);
	}

	// do dragging logic to update the scrollpoint if need be
	if (before_sig.flags & GUI_SIGNAL_FLAG_DRAGGING || (up_sig.flags & GUI_SIGNAL_FLAG_LMB_PRESSED && with_buttons)) {
		u64 new_idx = minimum(maximum(0,(s64)sp.idx-1),row_range_dim);
		gui_scroll_point_target_idx(&sp, new_idx);
	}
	if (after_sig.flags & GUI_SIGNAL_FLAG_DRAGGING || (down_sig.flags & GUI_SIGNAL_FLAG_LMB_PRESSED && with_buttons)) {
		u64 new_idx = minimum(maximum(0,(s64)sp.idx+1),row_range_dim);
		gui_scroll_point_target_idx(&sp, new_idx);
	}
	if (slider_sig.flags & GUI_SIGNAL_FLAG_DRAGGING) {
		guiVec2 delta = gui_drag_get_delta();
		u64 new_idx = minimum(maximum(0,(s64)sp.idx + signof(delta.raw[axis])),row_range_dim);
		gui_scroll_point_target_idx(&sp, new_idx);
		//sp.value = sp.point.idx+row_range.min;
	}

	gui_pop_pref_width();
	// scrollbar_area
	gui_pop_parent();
	return sp;
}

guiSignal gui_scroll_list_begin(guiScrollListOptions *opt, guiScrollPoint *sp) {
	guiSignal container_signal = {0};

	s64 num_of_visible_rows = (opt->dim_px.y/opt->row_height_px);
	guiRange2 scroll_row_idx_range = (guiRange2){.min = opt->item_range.min, .max = opt->item_range.max};
	s64 scroll_row_idx_range_dim = maximum(scroll_row_idx_range.max - scroll_row_idx_range.min, 1);

	//s64 scroller_width_px = gui_top_text_scale() * 20;
    s64 scroller_width_px = gui_font_get_icon_dim(FA_ICON_UP_BIG, gui_top_text_scale()).x;

	// main scroller area
	char msa_name[128];
	sprintf(msa_name, "msa_%s", "scroll_scroll");
	gui_set_next_child_layout_axis(AXIS2_Y);
	gui_set_next_fixed_width(opt->dim_px.x - scroller_width_px);
	gui_set_next_fixed_height(opt->dim_px.y);
	guiBox *main_scroll_area = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND | GUI_BOX_FLAG_CLIP | GUI_BOX_FLAG_SCROLL | GUI_BOX_FLAG_OVERFLOW_Y, msa_name);
	//guiBox *main_scroll_area = gui_box_build_from_str(GUI_BOX_FLAG_DRAW_BACKGROUND | GUI_BOX_FLAG_CLIP | GUI_BOX_FLAG_SCROLL, msa_name);
	guiSignal msa_signal = gui_get_signal_for_box(main_scroll_area);
	// if there was scrolling, update the scrollpoint
	if (msa_signal.flags & GUI_SIGNAL_FLAG_SCROLLED) {
		s32 scroll_delta = gui_input_get_scroll_delta();
		u64 new_idx = minimum(maximum(0,(s64)sp->idx-signof(scroll_delta)),scroll_row_idx_range_dim);
		gui_scroll_point_target_idx(sp, new_idx);
	}
	main_scroll_area->view_off.raw[AXIS2_Y] = main_scroll_area->view_off_target.raw[AXIS2_Y] = sp->idx * opt->row_height_px;

	gui_set_next_fixed_width(scroller_width_px);
	gui_set_next_fixed_height(opt->dim_px.y);
	*sp = gui_scroll_bar(AXIS2_Y, *sp, opt->item_range,num_of_visible_rows,1);

	gui_push_parent(main_scroll_area);
	gui_push_pref_height((guiSize){GUI_SIZEKIND_PIXELS, opt->row_height_px, 1});
	// ...


	return container_signal;
}

void gui_scroll_list_end() {

	gui_pop_pref_height();
	// pop main_scroll_area
	gui_pop_parent();
}
