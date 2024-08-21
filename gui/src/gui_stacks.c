#include "gui.h"

//-----------------------------------------------------------------------------
// Stack management MACROS
//-----------------------------------------------------------------------------
#define gui_stack_top_impl(state, name_upper, name_lower) return state->name_lower##_stack.top->v;

#define gui_stack_bottom_impl(state, name_upper, name_lower) return state->name_lower##_stack.bottom_val;

#define gui_stack_push_impl(state, name_upper, name_lower, type, new_value) \
gui##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {sll_stack_pop(state->name_lower##_stack.free);}\
else {node = gui_push_array(gui_get_build_arena(), gui##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
sll_stack_push(state->name_lower##_stack.top, node);\
if(node->next == &state->name_lower##_nil_stack_top)\
{\
state->name_lower##_stack.bottom_val = (new_value);\
}\
state->name_lower##_stack.auto_pop = 0;\
return old_value;

#define gui_stack_pop_impl(state, name_upper, name_lower) \
gui##name_upper##Node *popped = state->name_lower##_stack.top;\
if(popped != &state->name_lower##_nil_stack_top)\
{\
sll_stack_pop(state->name_lower##_stack.top);\
sll_stack_push(state->name_lower##_stack.free, popped);\
state->name_lower##_stack.auto_pop = 0;\
}\
return popped->v;\

#define gui_stack_set_next_impl(state, name_upper, name_lower, type, new_value) \
gui##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {sll_stack_pop(state->name_lower##_stack.free);}\
else {node = gui_push_array(gui_get_build_arena(), gui##name_upper##Node, 1);}\
type old_value = state->name_lower##_stack.top->v;\
node->v = new_value;\
sll_stack_push(state->name_lower##_stack.top, node);\
state->name_lower##_stack.auto_pop = 1;\
return old_value;
//-----------------------------------------------------------------------------



// This function should be huge and initialize ALL the UI stacks to empty
// TODO -- MAYBE this should be just some macro (maybe), declarations too?
void gui_init_stacks(guiState *state) {
	// -- parent stack initialization
	state->parent_nil_stack_top.v = gui_box_nil_id();
	state->parent_stack.top = &state->parent_nil_stack_top;
	state->parent_stack.bottom_val = gui_box_nil_id();
	state->parent_stack.free = 0;
	state->parent_stack.auto_pop = 0;
	// -- fixed_x stack initialization
	state->fixed_x_nil_stack_top.v = 0;
	state->fixed_x_stack.top = &state->fixed_x_nil_stack_top;
	state->fixed_x_stack.bottom_val = 0;
	state->fixed_x_stack.free = 0;
	state->fixed_x_stack.auto_pop = 0;
	// -- fixed_y stack initialization
	state->fixed_y_nil_stack_top.v = 0;
	state->fixed_y_stack.top = &state->fixed_y_nil_stack_top;
	state->fixed_y_stack.bottom_val = 0;
	state->fixed_y_stack.free = 0;
	state->fixed_y_stack.auto_pop = 0;
	// -- fixed_width stack initialization
	state->fixed_width_nil_stack_top.v = 0;
	state->fixed_width_stack.top = &state->fixed_width_nil_stack_top;
	state->fixed_width_stack.bottom_val = 0;
	state->fixed_width_stack.free = 0;
	state->fixed_width_stack.auto_pop = 0;
	// -- fixed_height stack initialization
	state->fixed_height_nil_stack_top.v = 0;
	state->fixed_height_stack.top = &state->fixed_height_nil_stack_top;
	state->fixed_height_stack.bottom_val = 0;
	state->fixed_height_stack.free = 0;
	state->fixed_height_stack.auto_pop = 0;
	// -- pref_width stack initialization
	state->pref_width_nil_stack_top.v = (guiSize){GUI_SIZEKIND_PIXELS,250.0f,1.0};
	state->pref_width_stack.top = &state->pref_width_nil_stack_top;
	state->pref_width_stack.bottom_val = state->pref_width_nil_stack_top.v;
	state->pref_width_stack.free = 0;
	state->pref_width_stack.auto_pop = 0;
	// -- pref_height stack initialization
	state->pref_height_nil_stack_top.v = (guiSize){GUI_SIZEKIND_PIXELS,30.0f,1.0};
	state->pref_height_stack.top = &state->pref_height_nil_stack_top;
	state->pref_height_stack.bottom_val = state->pref_height_nil_stack_top.v;
	state->pref_height_stack.free = 0;
	state->pref_height_stack.auto_pop = 0;
	// -- bg_color stack initialization
	state->bg_color_nil_stack_top.v = gv4(0,0,0,0);
	state->bg_color_stack.top = &state->bg_color_nil_stack_top;
	state->bg_color_stack.bottom_val = state->bg_color_nil_stack_top.v;
	state->bg_color_stack.free = 0;
	state->bg_color_stack.auto_pop = 0;
	// -- text_color stack initialization
	state->text_color_nil_stack_top.v = gv4(1,1,1,1);
	state->text_color_stack.top = &state->text_color_nil_stack_top;
	state->text_color_stack.bottom_val = state->text_color_nil_stack_top.v;
	state->text_color_stack.free = 0;
	state->text_color_stack.auto_pop = 0;
	// -- child_layout_axis stack initialization
	state->child_layout_axis_nil_stack_top.v = AXIS2_X;
	state->child_layout_axis_stack.top = &state->child_layout_axis_nil_stack_top;
	state->child_layout_axis_stack.bottom_val = state->child_layout_axis_nil_stack_top.v;
	state->child_layout_axis_stack.free = 0;
	state->child_layout_axis_stack.auto_pop = 0;
	// -- text_scale stack initialization
	state->text_scale_nil_stack_top.v = 1.0f;
	state->text_scale_stack.top = &state->text_scale_nil_stack_top;
	state->text_scale_stack.bottom_val = state->text_scale_nil_stack_top.v;
	state->text_scale_stack.free = 0;
	state->text_scale_stack.auto_pop = 0;
}

void gui_autopop_all_stacks() {
	guiState *state = gui_get_ui_state();
	if (state->parent_stack.auto_pop) { gui_pop_parent();state->parent_stack.auto_pop = 0; }
	if (state->fixed_x_stack.auto_pop) { gui_pop_fixed_x();state->fixed_x_stack.auto_pop = 0; }
	if (state->fixed_y_stack.auto_pop) { gui_pop_fixed_y();state->fixed_y_stack.auto_pop = 0; }
	if (state->fixed_width_stack.auto_pop) { gui_pop_fixed_width();state->fixed_width_stack.auto_pop = 0; }
	if (state->fixed_height_stack.auto_pop) { gui_pop_fixed_height();state->fixed_height_stack.auto_pop = 0; }
	if (state->pref_width_stack.auto_pop) { gui_pop_pref_width();state->pref_width_stack.auto_pop = 0; }
	if (state->pref_height_stack.auto_pop) { gui_pop_pref_height();state->pref_height_stack.auto_pop = 0; }
	if (state->bg_color_stack.auto_pop) { gui_pop_bg_color();state->bg_color_stack.auto_pop = 0; }
	if (state->text_color_stack.auto_pop) { gui_pop_text_color();state->text_color_stack.auto_pop = 0; }
	if (state->child_layout_axis_stack.auto_pop) { gui_pop_child_layout_axis();state->child_layout_axis_stack.auto_pop = 0; }
	if (state->text_scale_stack.auto_pop) { gui_pop_text_scale();state->text_scale_stack.auto_pop = 0; }
}

guiBox *gui_top_parent(void) { gui_stack_top_impl(gui_get_ui_state(), Parent, parent); }
guiBox *gui_set_next_parent(guiBox *box) { gui_stack_set_next_impl(gui_get_ui_state(), Parent, parent, guiBox*, box); }
guiBox *gui_push_parent(guiBox *box) { gui_stack_push_impl(gui_get_ui_state(), Parent, parent, guiBox*, box); }
guiBox *gui_pop_parent(void) { gui_stack_pop_impl(gui_get_ui_state(), Parent, parent); }

f32 gui_top_fixed_x(void) { gui_stack_top_impl(gui_get_ui_state(), FixedX, fixed_x); }
f32 gui_set_next_fixed_x(f32 v) { gui_stack_set_next_impl(gui_get_ui_state(), FixedX, fixed_x, f32, v); }
f32 gui_push_fixed_x(f32 v) { gui_stack_push_impl(gui_get_ui_state(), FixedX, fixed_x, f32, v); }
f32 gui_pop_fixed_x(void) { gui_stack_pop_impl(gui_get_ui_state(), FixedX, fixed_x); }

f32 gui_top_fixed_y(void) { gui_stack_top_impl(gui_get_ui_state(), FixedY, fixed_y); }
f32 gui_set_next_fixed_y(f32 v) { gui_stack_set_next_impl(gui_get_ui_state(), FixedY, fixed_y, f32, v); }
f32 gui_push_fixed_y(f32 v) { gui_stack_push_impl(gui_get_ui_state(), FixedY, fixed_y, f32, v); }
f32 gui_pop_fixed_y(void) { gui_stack_pop_impl(gui_get_ui_state(), FixedY, fixed_y); }

f32 gui_top_fixed_width(void) { gui_stack_top_impl(gui_get_ui_state(), FixedWidth, fixed_width); }
f32 gui_set_next_fixed_width(f32 v) { gui_stack_set_next_impl(gui_get_ui_state(), FixedWidth, fixed_width, f32, v); }
f32 gui_push_fixed_width(f32 v) { gui_stack_push_impl(gui_get_ui_state(), FixedWidth, fixed_width, f32, v); }
f32 gui_pop_fixed_width(void) { gui_stack_pop_impl(gui_get_ui_state(), FixedWidth, fixed_width); }

f32 gui_top_fixed_height(void) { gui_stack_top_impl(gui_get_ui_state(), FixedHeight, fixed_height); }
f32 gui_set_next_fixed_height(f32 v) { gui_stack_set_next_impl(gui_get_ui_state(), FixedHeight, fixed_height, f32, v); }
f32 gui_push_fixed_height(f32 v) { gui_stack_push_impl(gui_get_ui_state(), FixedHeight, fixed_height, f32, v); }
f32 gui_pop_fixed_height(void) { gui_stack_pop_impl(gui_get_ui_state(), FixedHeight, fixed_height); }

guiSize gui_top_pref_width(void) { gui_stack_top_impl(gui_get_ui_state(), PrefWidth, pref_width); }
guiSize gui_set_next_pref_width(guiSize v) { gui_stack_set_next_impl(gui_get_ui_state(), PrefWidth, pref_width, guiSize, v); }
guiSize gui_push_pref_width(guiSize v) { gui_stack_push_impl(gui_get_ui_state(), PrefWidth, pref_width, guiSize, v); }
guiSize gui_pop_pref_width(void) { gui_stack_pop_impl(gui_get_ui_state(), PrefWidth, pref_width); }

guiSize gui_top_pref_height(void) { gui_stack_top_impl(gui_get_ui_state(), PrefHeight, pref_height); }
guiSize gui_set_next_pref_height(guiSize v) { gui_stack_set_next_impl(gui_get_ui_state(), PrefHeight, pref_height, guiSize, v); }
guiSize gui_push_pref_height(guiSize v) { gui_stack_push_impl(gui_get_ui_state(), PrefHeight, pref_height, guiSize, v); }
guiSize gui_pop_pref_height(void) { gui_stack_pop_impl(gui_get_ui_state(), PrefHeight, pref_height); }

guiVec4 gui_top_bg_color(void) { gui_stack_top_impl(gui_get_ui_state(), BgColor, bg_color); }
guiVec4 gui_set_next_bg_color(guiVec4 v) { gui_stack_set_next_impl(gui_get_ui_state(), BgColor, bg_color, guiVec4, v); }
guiVec4 gui_push_bg_color(guiVec4 v) { gui_stack_push_impl(gui_get_ui_state(), BgColor, bg_color, guiVec4, v); }
guiVec4 gui_pop_bg_color(void) { gui_stack_pop_impl(gui_get_ui_state(), BgColor, bg_color); }

guiVec4 gui_top_text_color(void) { gui_stack_top_impl(gui_get_ui_state(), TextColor, text_color); }
guiVec4 gui_set_next_text_color(guiVec4 v) { gui_stack_set_next_impl(gui_get_ui_state(), TextColor, text_color, guiVec4, v); }
guiVec4 gui_push_text_color(guiVec4 v) { gui_stack_push_impl(gui_get_ui_state(), TextColor, text_color, guiVec4, v); }
guiVec4 gui_pop_text_color(void) { gui_stack_pop_impl(gui_get_ui_state(), TextColor, text_color); }

Axis2 gui_top_child_layout_axis(void) { gui_stack_top_impl(gui_get_ui_state(), ChildLayoutAxis, child_layout_axis); }
Axis2 gui_set_next_child_layout_axis(Axis2 v) { gui_stack_set_next_impl(gui_get_ui_state(), ChildLayoutAxis, child_layout_axis, Axis2, v); }
Axis2 gui_push_child_layout_axis(Axis2 v) { gui_stack_push_impl(gui_get_ui_state(), ChildLayoutAxis, child_layout_axis, Axis2, v); }
Axis2 gui_pop_child_layout_axis(void) { gui_stack_pop_impl(gui_get_ui_state(), ChildLayoutAxis, child_layout_axis); }

f32 gui_top_text_scale(void) { gui_stack_top_impl(gui_get_ui_state(), TextScale, text_scale); }
f32 gui_set_next_text_scale(f32 v) { gui_stack_set_next_impl(gui_get_ui_state(), TextScale, text_scale, f32, v); }
f32 gui_push_text_scale(f32 v) { gui_stack_push_impl(gui_get_ui_state(), TextScale, text_scale, f32, v); }
f32 gui_pop_text_scale(void) { gui_stack_pop_impl(gui_get_ui_state(), TextScale, text_scale); }

guiSize gui_push_pref_size(Axis2 axis, guiSize v) {
  guiSize result;
  switch(axis)
  {
    case AXIS2_X: {result = gui_push_pref_width(v);}break;
    case AXIS2_Y: {result = gui_push_pref_height(v);}break;
    default: break;
  }
  return result;
}

guiSize gui_pop_pref_size(Axis2 axis) {
  guiSize result;
  switch(axis)
  {
    case AXIS2_X: {result = gui_pop_pref_width();}break;
    case AXIS2_Y: {result = gui_pop_pref_height();}break;
    default: break;
  }
  return result;
}

guiSize gui_set_next_pref_size(Axis2 axis, guiSize v) {
  if (axis == AXIS2_X){
	return gui_set_next_pref_width(v);
  }
  return gui_set_next_pref_height(v);
}


void gui_push_rect(guiRect r) {
  guiVec2 size = {fabs(r.x1 - r.x0), fabs(r.y1 - r.y0)};
  gui_push_fixed_x(r.x0);
  gui_push_fixed_y(r.y0);
  gui_push_fixed_width(size.x);
  gui_push_fixed_height(size.y);
}

void gui_pop_rect(void) {
  gui_pop_fixed_x();
  gui_pop_fixed_y();
  gui_pop_fixed_width();
  gui_pop_fixed_height();
}

void gui_set_next_rect(guiRect r) {
  guiVec2 size = {fabs(r.x1 - r.x0), fabs(r.y1 - r.y0)};
  gui_set_next_fixed_x(r.x0);
  gui_set_next_fixed_y(r.y0);
  gui_set_next_fixed_width(size.x);
  gui_set_next_fixed_height(size.y);
}