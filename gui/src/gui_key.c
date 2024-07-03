#include "gui.h"
/*
	Here is all the gui's keying mechanism
	TODO -- Maybe we should add a randomized seed on key generation
	TODO -- Maybe we should ditch djb2 for something more accurate
*/

guiKey gui_key_make(u64 val){
	guiKey res = (guiKey){val};
	return res;
}

guiKey gui_key_zero(void){
	return gui_key_make(0);
}

//guiKey gui_key_from_str(guiKey seed_key, char *string) {
guiKey gui_key_from_str(char *s) {
	guiKey res = gui_key_zero();
	if (s && strlen(s) != 0) {
		res = gui_key_make(djb2((u8*)s));
	}
	return res;
}

b32 gui_key_match(guiKey a, guiKey b) {
	return ((u64)a == (u64)b);
}

