#include "gui.h"


void gui_layout_calc_constant_sizes(guiBox *root, Axis2 axis) {
    guiState *state = gui_get_ui_state();
    // find the fixed size of the boxed
    f32 padding;
    switch(root->pref_size[axis].kind) {
        case GUI_SIZEKIND_PIXELS:
            root->fixed_size.raw[axis] = root->pref_size[axis].value;
            break;
        case GUI_SIZEKIND_TEXT_CONTENT:
            padding = root->pref_size[axis].value;
            f32 text_size = gui_font_get_string_dim(root->str).raw[axis];
            root->fixed_size.raw[axis] = padding + text_size;
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

void gui_layout_calc_upward_dependent_sizes(guiBox *root, Axis2 axis) {
    // try to find a parent with fixed size and get the percentage off of him
    guiBox *fixed_parent = &g_nil_box;
    switch(root->pref_size[axis].kind) {
        case GUI_SIZEKIND_PERCENT_OF_PARENT:
            fixed_parent = &g_nil_box;
            for(guiBox *box= root->parent; !gui_box_is_nil(box); box = box->parent)
            {
                if ( (box->flags & (GUI_BOX_FLAG_FIXED_WIDTH<<axis)) ||
                     box->pref_size[axis].kind == GUI_SIZEKIND_PIXELS ||
                     box->pref_size[axis].kind == GUI_SIZEKIND_TEXT_CONTENT||
                     box->pref_size[axis].kind == GUI_SIZEKIND_PERCENT_OF_PARENT)
                {
                    fixed_parent = box;
                    break;
                }
            }
            root->fixed_size.raw[axis] = fixed_parent->fixed_size.raw[axis] * root->pref_size[axis].value;
            break;
        default:
            break;
    }
    // loop through all the hierarchy
	for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next)
	{
        gui_layout_calc_upward_dependent_sizes(child, axis);
	}
}


void gui_layout_calc_downward_dependent_sizes(guiBox *root, Axis2 axis) {
    // loop through all the hierarchy
	for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next)
	{
        gui_layout_calc_downward_dependent_sizes(child, axis);
	}

    // add the size of all child boxes for ChildrenSum
    f32 sum = 0;
    switch(root->pref_size[axis].kind) {
        case GUI_SIZEKIND_CHILDREN_SUM:
            sum = 0;
            for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next)
            {
                if (!(child->flags & (GUI_BOX_FLAG_FIXED_X<<axis))) {
                    if (axis == root->child_layout_axis) {
                        sum += child->fixed_size.raw[axis];
                    } else {
                        sum = maximum(sum, child->fixed_size.raw[axis]);
                    }
                }
            }
            root->fixed_size.raw[axis] = sum;
            break;
        default:
            break;
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
    gui_layout_calc_upward_dependent_sizes(root,axis);
    gui_layout_calc_downward_dependent_sizes(root,axis);
    gui_layout_calc_final_rects(root, axis);
}