#include "gui.h"

//-----------------------------------------------------------------------------
// Stack management MACROS
//-----------------------------------------------------------------------------
#define gui_stack_top_impl(state, name_upper, name_lower) return state->name_lower##_stack.top->v;

#define gui_stack_bottom_impl(state, name_upper, name_lower) return state->name_lower##_stack.bottom_val;

#define gui_stack_push_impl(state, name_upper, name_lower, type, new_value) \
gui##name_upper##Node *node = state->name_lower##_stack.free;\
if(node != 0) {sll_stack_pop(state->name_lower##_stack.free);}\
else {node = push_array(gui_get_build_arena(), gui##name_upper##Node, 1);}\
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
else {node = push_array(gui_get_build_arena(), gui##name_upper##Node, 1);}\
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
	state->parent_nil_stack_top.v = &g_nil_box;
	state->parent_stack.top = &state->parent_nil_stack_top;
	state->parent_stack.bottom_val = &g_nil_box;
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
}

guiBox *gui_top_parent(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_top_impl(state, Parent, parent);
}
guiBox *gui_set_next_parent(guiBox *box) {
	guiState *state = gui_get_ui_state();
	gui_stack_set_next_impl(gui_get_ui_state(), Parent, parent, guiBox*, box);
}
guiBox *gui_push_parent(guiBox *box) {
	guiState *state = gui_get_ui_state();
	gui_stack_push_impl(gui_get_ui_state(), Parent, parent, guiBox*, box);
}
guiBox *gui_pop_parent(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_pop_impl(state, Parent, parent);
}

f32 gui_top_fixed_x(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_top_impl(state, FixedX, fixed_x);
}
f32 gui_set_next_fixed_x(f32 v) {
	guiState *state = gui_get_ui_state();
	gui_stack_set_next_impl(gui_get_ui_state(), FixedX, fixed_x, f32, v);
}
f32 gui_push_fixed_x(f32 v) {
	guiState *state = gui_get_ui_state();
	gui_stack_push_impl(gui_get_ui_state(), FixedX, fixed_x, f32, v);
}
f32 gui_pop_fixed_x(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_pop_impl(state, FixedX, fixed_x);
}

f32 gui_top_fixed_y(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_top_impl(state, FixedY, fixed_y);
}
f32 gui_set_next_fixed_y(f32 v) {
	guiState *state = gui_get_ui_state();
	gui_stack_set_next_impl(gui_get_ui_state(), FixedY, fixed_y, f32, v);
}
f32 gui_push_fixed_y(f32 v) {
	guiState *state = gui_get_ui_state();
	gui_stack_push_impl(gui_get_ui_state(), FixedY, fixed_y, f32, v);
}
f32 gui_pop_fixed_y(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_pop_impl(state, FixedY, fixed_y);
}

guiSize gui_top_pref_width(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_top_impl(state, PrefWidth, pref_width);
}
guiSize gui_set_next_pref_width(guiSize v) {
	guiState *state = gui_get_ui_state();
	gui_stack_set_next_impl(gui_get_ui_state(), PrefWidth, pref_width, guiSize, v);
}
guiSize gui_push_pref_width(guiSize v) {
	guiState *state = gui_get_ui_state();
	gui_stack_push_impl(gui_get_ui_state(), PrefWidth, pref_width, guiSize, v);
}
guiSize gui_pop_pref_width(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_pop_impl(state, PrefWidth, pref_width);
}

guiSize gui_top_pref_height(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_top_impl(state, PrefHeight, pref_height);
}
guiSize gui_set_next_pref_height(guiSize v) {
	guiState *state = gui_get_ui_state();
	gui_stack_set_next_impl(gui_get_ui_state(), PrefHeight, pref_height, guiSize, v);
}
guiSize gui_push_pref_height(guiSize v) {
	guiState *state = gui_get_ui_state();
	gui_stack_push_impl(gui_get_ui_state(), PrefHeight, pref_height, guiSize, v);
}
guiSize gui_pop_pref_height(void) {
	guiState *state = gui_get_ui_state();
	gui_stack_pop_impl(state, PrefHeight, pref_height);
}



guiSize gui_push_pref_size(Axis2 axis, guiSize v)
{
  guiSize result;
  switch(axis)
  {
    default: break;
    case AXIS2_X: {result = gui_push_pref_width(v);}break;
    case AXIS2_Y: {result = gui_push_pref_height(v);}break;
  }
  return result;
}

guiSize gui_pop_pref_size(Axis2 axis)
{
  guiSize result;
  switch(axis)
  {
    default: break;
    case AXIS2_X: {result = gui_pop_pref_width();}break;
    case AXIS2_Y: {result = gui_pop_pref_height();}break;
  }
  return result;
}

guiSize gui_set_next_pref_size(Axis2 axis, guiSize v)
{
  return (axis == AXIS2_X ? gui_set_next_pref_width : gui_set_next_pref_height)(v);
}


// TODO -- should these functions return whats pushed and popped?
void gui_push_rect(rect r)
{
  vec2 size = {abs(r.x1 - r.x0), abs(r.y1 - r.y0)};
  gui_push_fixed_x(r.x0);
  gui_push_fixed_y(r.y0);
  gui_push_pref_size(AXIS2_X, (guiSize){GUI_SIZEKIND_PIXELS,size.x,1.0f});
  gui_push_pref_size(AXIS2_Y, (guiSize){GUI_SIZEKIND_PIXELS,size.y,1.0f});
}

void gui_pop_rect(void)
{
  gui_pop_fixed_x();
  gui_pop_fixed_y();
  gui_pop_pref_size(AXIS2_X);
  gui_pop_pref_size(AXIS2_Y);
}

