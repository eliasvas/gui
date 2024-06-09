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

