#include "gui.h"

// TODO -- This input system FUCKING sucks, we need a new one that CONSUMES events!

b32 gui_input_mb_is_state(GUI_MOUSE_BUTTON button, guiMouseButtonState state){
	return ((gui_get_ui_state()->gis.mb[button] & KEY_STATE_MASK) == state);
}
b32 gui_input_mb_down(GUI_MOUSE_BUTTON button) {
	return gui_input_mb_is_state(button, KEY_STATE_DOWN);
}
b32 gui_input_mb_up(GUI_MOUSE_BUTTON button) {
	return gui_input_mb_is_state(button, KEY_STATE_UP);
}
b32 gui_input_mb_pressed(GUI_MOUSE_BUTTON button) {
	return gui_input_mb_is_state(button, KEY_STATE_PRESSED);
}
b32 gui_input_mb_released(GUI_MOUSE_BUTTON button) {
	return gui_input_mb_is_state(button, KEY_STATE_RELEASED);
}

void gui_input_process_event_queue(void){
	guiInputState *gis = &gui_get_ui_state()->gis;
	b32 mb_updated_this_frame[GUI_MOUSE_BUTTON_COUNT] = {0};
	GUI_MOUSE_BUTTON button;
	gis->prev_scroll_y = gis->scroll_y;
	s32 scroll_y;
	b32 is_down;
	for (guiInputEventNode *e = gis->first; e != 0; e=e->next) {
		switch(e->type){
			case GUI_INPUT_EVENT_TYPE_MOUSE_MOVE:
				gis->mouse_x = *((s32*)((void*)(&e->param0)));
				gis->mouse_y = *((s32*)((void*)(&e->param1)));
				break;
			case GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT:
				button = e->param0;
				is_down = e->param1;
				gis->mb[button] = (gis->mb[button] << 1) | (is_down & 1);
				mb_updated_this_frame[(u32)button] = 1;
				break;
			case GUI_INPUT_EVENT_TYPE_SCROLLWHEEL_EVENT:
				scroll_y = *((s32*)((void*)(&e->param0)));
				gis->prev_scroll_y = gis->scroll_y;
				gis->scroll_y += scroll_y;
				break;
			default:
				break;
		}
	}
	for (int i = 0; i < GUI_MOUSE_BUTTON_COUNT; ++i){
		GUI_MOUSE_BUTTON button = i;
		if (0 == mb_updated_this_frame[button]){
			guiMouseButtonState prev_state = gis->mb[i];
			gis->mb[button] = (gis->mb[button] << 1) | (prev_state & 1);
		}
	}
}

guiArena *gui_get_event_arena() {
	return gui_get_ui_state()->gis.event_arena;
}

void gui_input_clear_event_queue(void){
	gui_get_ui_state()->gis.first = 0;
	gui_get_ui_state()->gis.last= 0;
}

s32 gui_input_get_scroll_delta() {
	return gui_get_ui_state()->gis.scroll_y - gui_get_ui_state()->gis.prev_scroll_y;
}


guiStatus gui_input_push_event(guiInputEventNode e) {
	guiState *state = gui_get_ui_state();
	// We allocate the event in the current temp arena
	guiInputEventNode *node = gui_push_array(gui_get_event_arena(), guiInputEventNode, 1);
	*node = e; // TODO -- let's just make a M_SET(src, dest, size) and M_SET_STRUCT(src, dst)
	// We push it to our event queue
	sll_queue_push(state->gis.first, state->gis.last, node);
	return GUI_GUD;
}


guiVec2 gui_drag_get_start_mp() {
	return gui_get_ui_state()->drag_start_mp;
}

void gui_drag_set_current_mp() {
	guiVec2 mp = gv2(gui_get_ui_state()->gis.mouse_x, gui_get_ui_state()->gis.mouse_y);
	gui_drag_set_mp(mp);
}

void gui_drag_set_mp(guiVec2 mp) {
	gui_get_ui_state()->drag_start_mp = mp;
}

guiVec2 gui_drag_get_delta() {
	guiVec2 drag_pos = gui_get_ui_state()->drag_start_mp;
	guiVec2 mp = gv2(gui_get_ui_state()->gis.mouse_x, gui_get_ui_state()->gis.mouse_y);
	return gv2(mp.x - drag_pos.x, mp.y - drag_pos.y);
}