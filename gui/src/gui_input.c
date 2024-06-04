#include "gui.h"

b32 gui_input_mb_is_state(const guiInputState *gis, guiMouseButton button, guiMouseButtonState state){
	return ((gis->mb[button] & KEY_STATE_MASK) == state);
}
b32 gui_input_mb_down(const guiInputState *gis, guiMouseButton button) {
	return gui_input_mb_is_state(gis, button, KEY_STATE_DOWN);
}
b32 gui_input_mb_up(const guiInputState *gis, guiMouseButton button) {
	return gui_input_mb_is_state(gis, button, KEY_STATE_UP);
}
b32 gui_input_mb_pressed(const guiInputState *gis, guiMouseButton button) {
	return gui_input_mb_is_state(gis, button, KEY_STATE_PRESSED);
}
b32 gui_input_mb_released(const guiInputState *gis, guiMouseButton button) {
	return gui_input_mb_is_state(gis, button, KEY_STATE_RELEASED);
}

void gui_input_process_events(guiInputState *gis){
	b32 mb_updated_this_frame [GUI_MOUSE_BUTTON_COUNT] = {0};
	for (int i = 0; i < sb_len(gis->events); ++i){
		guiInputEvent event = gis->events[i];
		switch(event.type){
			case GUI_INPUT_EVENT_TYPE_MOUSE_MOVE:
				gis->mouse_x = *((s32*)((void*)(&event.param0)));
				gis->mouse_y = *((s32*)((void*)(&event.param1)));
				break;
			case GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT:
				guiMouseButton button = event.param0;
				b32 is_down = event.param1;
				gis->mb[button] = (gis->mb[button] << 1) | (is_down & 1);
				mb_updated_this_frame[(u32)button] = 1;
				break;
			default:
				break;
		}
	}
	sb_free(gis->events);
	for (int i = 0; i < GUI_MOUSE_BUTTON_COUNT; ++i){
		guiMouseButton button = i;
		if (0 == mb_updated_this_frame[button]){
			guiMouseButtonState prev_state = gis->mb[i];
			gis->mb[button] = (gis->mb[button] << 1) | (prev_state & 1);
		}
	}
}


guiStatus gui_input_push_event(guiInputEvent e) {
	guiState *state = gui_get_ui_state();

	sb_push(state->gis.events, e);
	//gui_input_process_events(&state->gis);
	return GUI_GUD;
}