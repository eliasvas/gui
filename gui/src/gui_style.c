#include "gui.h"

// TODO -- Right now, the style is CONSTANT (the struct) and global for all widgets, maybe we need to implement Style Stacks for more customizability


gui_style_default(guiStyle *style) {
	style->base_text_color = (vec4){1,1,1,1};
	style->base_background_color = (vec4){0.4,0.4,0.4,1.0};
}