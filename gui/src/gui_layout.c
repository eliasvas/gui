#include "gui.h"


void gui_layout_calc_constant_sizes(guiBox *root, Axis2 axis) {
    guiState *state = gui_get_ui_state();
    // find the fixed size of the boxed
    switch(root->pref_size[axis].kind) {
        case GUI_SIZEKIND_PIXELS:
            root->fixed_size.raw[axis] = root->pref_size[axis].value;
            break;
        case GUI_SIZEKIND_TEXT_CONTENT:
            root->fixed_size.raw[axis] = root->pref_size[axis].value;
            break;
        default:
            break;
    }
    // loop through all the hierarchy
	for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next)
	{
        gui_layout_calc_constant_sizes(child, axis);
	}
}

void gui_layout_calc_final_rects(guiBox *root, Axis2 axis) {
    f32 layout_pos = 0;
    // do layouting for all children (only root's children)
	for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next) {
        if (!(child->flags & (GUI_BOX_FLAG_FIXED_X<<axis))) {
            child->fixed_pos.raw[axis] = layout_pos;
            // advance layout offset
            if (root->child_layout_axis == axis) {
                layout_pos += child->fixed_size.raw[axis];
            }
        }
        child->r.p0.raw[axis] = root->r.p0.raw[axis] + child->fixed_pos.raw[axis];
        child->r.p1.raw[axis] = child->r.p0.raw[axis] + child->fixed_size.raw[axis];
	}

    // do the same for all nodes and their children in hierarchy
    for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next) {
        gui_layout_calc_final_rects(child, axis);
	}
}

void gui_layout_root(guiBox *root, Axis2 axis)  {
    gui_layout_calc_constant_sizes(root, axis);
    gui_layout_calc_final_rects(root, axis);
}