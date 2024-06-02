#include "gui.h"


b32 gui_key_is_null(guiKey k){
	return (k == 0);
}
guiKey gui_key_null(void){
	guiKey k = {0};
	return k;
}

guiKey gui_key_from_str(char *str) {
	return djb2(str);
}

b32 gui_key_equals(guiKey lk, guiKey rk) {
	return (lk==rk);
}

guiBox *gui_box_make(guiBoxFlags flags, char *str) {
	assert(strlen(str) < GUI_BOX_MAX_STRING_SIZE);
	guiBox *w = ALLOC(sizeof(guiBox));
	M_ZERO(w, sizeof(guiBox));
	w->flags = flags;
	memcpy(w->str, str, strlen(str));
	return w;
}

guiBox *gui_push_parent(guiBox *box) {
	return NULL;
}

guiBox *gui_pop_parent(void) {
	return NULL;
}


guiSignal gui_get_signal_for_box(guiBox *box) {
	guiSignal signal = {0};
	// lookup signal cache and find associated signal
	return signal;
}

b32 gui_button(char *str) {
	guiBox *w = gui_box_make( GUI_BOX_FLAG_CLICKABLE|
									GUI_BOX_FLAG_DRAW_BORDER|
									GUI_BOX_FLAG_DRAW_TEXT|
									GUI_BOX_FLAG_DRAW_BACKGROUND|
									GUI_BOX_FLAG_DRAW_HOT_ANIMATION|
									GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION,
									str);
	guiSignal signal = gui_get_signal_for_box(w);
	return (signal.flags & GUI_SIGNAL_FLAG_LMB_PRESSED) > 0;
}