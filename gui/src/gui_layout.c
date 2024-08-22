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
            f32 text_size;
            // TODO: icons and regular characters SHOULD be unified
            if (!(root->flags & GUI_BOX_FLAG_DRAW_ICON))
            {
                text_size = gui_font_get_string_dim(root->str, root->text_scale).raw[axis];
            }else {
                text_size = gui_font_get_icon_dim(root->icon_codepoint, root->text_scale).raw[axis];
            }
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
    guiBox *fixed_parent = gui_box_nil_id();
    switch(root->pref_size[axis].kind) {
        case GUI_SIZEKIND_PERCENT_OF_PARENT:
            fixed_parent = gui_box_nil_id();
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

void gui_layout_calc_solve_constraints(guiBox *root, Axis2 axis) {

    // fixup when we are NOT current layout axis
    if (axis != root->child_layout_axis && !(root->flags & (GUI_BOX_FLAG_OVERFLOW_X<<axis))) {
        f32 max_allowed_size = root->fixed_size.raw[axis];
        for (guiBox *child = root->first; !gui_box_is_nil(child); child = child->next) {
            if (!(child->flags & (GUI_BOX_FLAG_FIXED_X<<axis))) {
                f32 child_size = child->fixed_size.raw[axis];
                f32 fixup_needed = child_size - max_allowed_size;
                fixup_needed = maximum(0, minimum(fixup_needed, child_size));
                if (fixup_needed > 0) {
                    child->fixed_size.raw[axis] -= fixup_needed;
                }
            }
        }
    }

    // when we are along the layout axis, we have to check strictness and adjust accordingly
    if (axis == root->child_layout_axis && !(root->flags & (GUI_BOX_FLAG_OVERFLOW_X<<axis))) {
        f32 max_allowed_size = root->fixed_size.raw[axis];
        f32 total_size = 0;
        f32 total_weighed_size = 0;
        f32 child_count = root->child_count;
        for (guiBox *child = root->first; !gui_box_is_nil(child); child = child->next) {
            if (!(child->flags & (GUI_BOX_FLAG_FIXED_X<<axis))) {
                total_size += child->fixed_size.raw[axis];
                total_weighed_size += child->fixed_size.raw[axis] * (1.0f - child->pref_size[axis].strictness);
            }
        }
        f32 violation = total_size - max_allowed_size;

        if (violation > 0.0f) {
            f32 *child_fixup_array = gui_push_array(gui_get_build_arena(), f32, root->child_count);
            u32 child_idx = 0;
            for (guiBox *child = root->first; !gui_box_is_nil(child); child = child->next, ++child_idx) {
                if (!(child->flags & (GUI_BOX_FLAG_FIXED_X<<axis))) {
                    f32 child_weighed_size = child->fixed_size.raw[axis] * (1.0f - child->pref_size[axis].strictness);
                    child_weighed_size = maximum(0.0f, child_weighed_size);
                    child_fixup_array[child_idx] = child_weighed_size;
                }
            }

            child_idx = 0;
            for (guiBox *child = root->first; !gui_box_is_nil(child); child = child->next, ++child_idx) {
                if (!(child->flags & (GUI_BOX_FLAG_FIXED_X<<axis))) {
                    // this percentage will be applied to ALL child widgets
                    f32 fixup_needed = (violation / (f32)total_weighed_size);
                    fixup_needed = minimum(maximum(0.0f,fixup_needed),1.0f);
                    // if (fixup_needed > 0.0f) {
                    //     printf("violation : %f, total_weighed_size: %f\n", violation, total_weighed_size);
                    // }
                    child->fixed_size.raw[axis] -= fixup_needed * child_fixup_array[child_idx];
                }
            }
        }

    }

    // if root has OVERFLOW_X/Y, then its children's 'constraints' don't need to be solved
    // its mimicking solve_upward_dependent, just now, the OVERFLOW_X node HAS its fixed_size calculated
    if (root->flags & (GUI_BOX_FLAG_OVERFLOW_X<<axis)) {
        for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next) {
            if (child->pref_size[axis].kind == GUI_SIZEKIND_PERCENT_OF_PARENT) {
                child->fixed_size.raw[axis] = root->fixed_size.raw[axis] * child->pref_size[axis].value;
            }
        }
    }

    // do the same for all nodes and their children in hierarchy
    for(guiBox *child = root->first; !gui_box_is_nil(child); child = child->next) {
        gui_layout_calc_solve_constraints(child, axis);
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
        // HERE we view scroll (-=view_off)
        child->r.p0.raw[axis] = root->r.p0.raw[axis] + child->fixed_pos.raw[axis] - root->view_off.raw[axis];
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
    gui_layout_calc_solve_constraints(root,axis);
    gui_layout_calc_final_rects(root, axis);
}