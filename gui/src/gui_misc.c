#include "gui.h"

guiScrollPoint gui_scroll_point_make(u64 idx, f32 off) {
	return (guiScrollPoint){idx,off};
}

void gui_scroll_point_target_idx(guiScrollPoint *p, s64 idx) {
	p->off = p->off + (p->idx+p->off-idx);
	p->idx = idx;
}
void gui_scroll_point_clamp_idx(guiScrollPoint *p, guiVec2 range) {
    f32 val_count = (range.max - range.min);
	if (p->idx > val_count){
		u64 clamped = range.max;
		gui_scroll_point_target_idx(p, clamped);
	}
}

guiSliderData gui_slider_data_make(guiScrollPoint sp, f32 px) {
	return (guiSliderData){sp,px};
}