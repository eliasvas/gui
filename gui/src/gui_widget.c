#include "gui.h"


typedef u32 guiKey;
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

guiWidget *gui_widget_make(guiWidgetFlags flags, char *str) {
	assert(strlen(str) < GUI_WIDGET_MAX_STRING_SIZE);
	guiWidget *w = ALLOC(sizeof(guiWidget));
	memzero(w, sizeof(guiWidget));
	w->flags = flags;
	memcpy(w->str, str, strlen(str));
	return w;
}

guiWidget *gui_push_parent(guiWidget *widget) {
	return NULL;
}

guiWidget *gui_pop_parent(void) {
	return NULL;
}


guiSignal gui_get_signal_for_widget(guiWidget *widget) {
	guiSignal signal = {0};
	// lookup signal cache and find associated signal
	return signal;
}

b32 gui_button(char *str) {
	guiWidget *w = gui_widget_make( GUI_WIDGET_FLAG_CLICKABLE|
									GUI_WIDGET_FLAG_DRAW_BORDER|
									GUI_WIDGET_FLAG_DRAW_TEXT|
									GUI_WIDGET_FLAG_DRAW_BACKGROUND|
									GUI_WIDGET_FLAG_DRAW_HOT_ANIMATION|
									GUI_WIDGET_FLAG_DRAW_ACTIVE_ANIMATION,
									str);
	guiSignal signal = gui_get_signal_for_widget(w);
	return (signal.flags & GUI_SIGNAL_FLAG_LMB_PRESSED) > 0;
}