#include "gui.h"

guiRenderCommand *gui_render_cmd_buf_get_array(guiRenderCommandBuffer *cmd_buf) {
	guiRenderCommand *commands = gui_push_array(gui_get_build_arena(), guiRenderCommand, gui_render_cmd_buf_get_count(cmd_buf));
	u32 command_idx = 0;
	for (guiRenderCommandNode *rcn = cmd_buf->first; rcn != 0; rcn=rcn->next) {
		commands[command_idx] = rcn->rc;
		command_idx+=1;
	}
	return commands;
}

void gui_render_cmd_buf_clear(guiRenderCommandBuffer *cmd_buf){
	cmd_buf->first = 0;
	cmd_buf->last = 0;
}

u32 gui_render_cmd_buf_get_count(guiRenderCommandBuffer *cmd_buf){
	u32 count = 0;
	for (guiRenderCommandNode *rcn = cmd_buf->first; rcn != 0; rcn=rcn->next) {
		count+=1;
	}
	return count;
}

guiRenderCommand *gui_render_cmd_buf_add(guiRenderCommandBuffer *cmd_buf, guiRenderCommand cmd){
	guiRenderCommandNode *cmd_node = gui_push_array(gui_get_build_arena(), guiRenderCommandNode, 1);
	cmd_node->rc = cmd;
	sll_queue_push(cmd_buf->first, cmd_buf->last, cmd_node);
	return &(cmd_node->rc);
}

guiRenderCommand *gui_render_cmd_buf_add_quad(guiRenderCommandBuffer *cmd_buf, guiVec2 p0, guiVec2 dim, guiVec4 col, f32 softness, f32 corner_rad, f32 border_thickness){
	guiRenderCommand cmd = {0};
	cmd.pos0 = p0;
	cmd.pos1.x = p0.x + dim.x;
	cmd.pos1.y = p0.y + dim.y;
	cmd.color = col;
	cmd.edge_softness = softness;
	cmd.corner_radius = corner_rad;
	cmd.border_thickness = border_thickness;
	return gui_render_cmd_buf_add(cmd_buf, cmd);
}

guiRenderCommand *gui_render_cmd_buf_add_codepoint(guiRenderCommandBuffer *cmd_buf, guiFontAtlas *atlas, u32 c, guiVec2 p0, guiVec2 dim, guiVec4 col){
	guiRenderCommand cmd = {0};
	guiPackedChar bc = gui_font_atlas_get_codepoint(atlas, c);
	guiVec2 uv0 = {bc.x0,bc.y0};
	guiVec2 uv1 = {bc.x1,bc.y1};
	cmd.pos0 = p0;
	cmd.pos1.x = p0.x + dim.x;
	cmd.pos1.y = p0.y + dim.y;
	cmd.uv0 = uv0;
	cmd.uv1 = uv1;
	cmd.color = col;
	return gui_render_cmd_buf_add(cmd_buf, cmd);
}


guiRenderCommand *gui_render_cmd_buf_add_codepoint_testclip(guiRenderCommandBuffer *cmd_buf, guiFontAtlas *atlas, u32 c, guiVec2 p0, guiVec2 dim, guiVec4 col, guiRect clip_rect){
	guiRenderCommand cmd = {0};
	guiPackedChar bc = gui_font_atlas_get_codepoint(atlas, c);
	guiVec2 uv0 = {bc.x0,bc.y0};
	guiVec2 uv1 = {bc.x1,bc.y1};
	
	guiRect uvRect = (guiRect){bc.x0,bc.y0,bc.x1,bc.y1};
	guiRect posRect = (guiRect){p0.x, p0.y, p0.x+dim.x, p0.y+dim.y};
	guiRect oposRect = posRect;
	posRect = gui_intersect_rects(posRect, clip_rect);
	uvRect.x0 = uvRect.x0 + ((posRect.x0 - oposRect.x0) / (oposRect.x1 - oposRect.x0)) * (uvRect.x1 - uvRect.x0);
	uvRect.y0 = uvRect.y0 + ((posRect.y0 - oposRect.y0) / (oposRect.y1 - oposRect.y0)) * (uvRect.y1 - uvRect.y0);
	uvRect.x1 = uvRect.x1 - ((oposRect.x1 - posRect.x1) / (oposRect.x1 - oposRect.x0)) * (uvRect.x1 - uvRect.x0);
	uvRect.y1 = uvRect.y1 - ((oposRect.y1 - posRect.y1) / (oposRect.y1 - oposRect.y0)) * (uvRect.y1 - uvRect.y0);
	cmd.pos0 = posRect.p0;
	cmd.pos1 = posRect.p1;
	cmd.uv0 = uvRect.p0;
	cmd.uv1 = uvRect.p1;
	cmd.color = col;
	return gui_render_cmd_buf_add(cmd_buf, cmd);
}


void gui_draw_rect(guiRect r, guiVec4 color, guiBox *box) {
	guiState *state = gui_get_ui_state();

	// if a parent has FLAG_CLIP we clip all children to parent's rect
	guiRect clipped_rect = box->r;
	for(guiBox *p = box->parent; !gui_box_is_nil(p); p = p->parent) {
		if (p->flags & GUI_BOX_FLAG_CLIP) {
			clipped_rect = gui_intersect_rects(box->r,p->r);
			break;
		}
	}
	guiRect unclipped_rect = r;

	r = clipped_rect;

	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (guiVec2){r.x0, r.y0}, (guiVec2){fabs(r.x1-r.x0), fabs(r.y1-r.y0)}, color,(box->flags & GUI_BOX_FLAG_ROUNDED_EDGES) ? 2:0, (box->flags & GUI_BOX_FLAG_ROUNDED_EDGES) ? 4:0, 0);
	if (box->flags & GUI_BOX_FLAG_DRAW_BORDER) {
		guiVec4 color_dim = gv4(color.x/2.f,color.y/2.f,color.z/2.f,1.f);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (guiVec2){r.x0, r.y0}, (guiVec2){fabs(r.x1-r.x0), fabs(r.y1-r.y0)}, color_dim,1, (box->flags & GUI_BOX_FLAG_ROUNDED_EDGES) ? 4:0,2);
	}

	// TODO -- do correct clipping on text / icons
	r = unclipped_rect;
	//r = clipped_rect;

	if (box->flags & GUI_BOX_FLAG_DRAW_TEXT) {
		// we need unclipped rect for correct positioning (e.g middle-centered text), and clipped rect for clipping the characters
		gui_draw_string_in_rect(box->str, unclipped_rect, clipped_rect, box->text_scale, box->text_color);
	}
	if (box->flags & GUI_BOX_FLAG_DRAW_ICON) {
		gui_draw_icon_in_rect(box->icon_codepoint, unclipped_rect, clipped_rect, box->text_scale, box->text_color);
	}
}

void gui_render_hierarchy(guiBox *box) {

	// Visualize hot_t, active_t values to plug to renderer
	if (box->flags & GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION) {
		box->c.r += box->active_t/6.0f;
		if (box->flags & GUI_BOX_FLAG_DRAW_ICON) { box->text_color.r -= box->active_t/6.0f; box->text_color.g -= box->active_t/6.0f; box->text_color.b -= box->active_t/6.0f; }
	}
	if (box->flags & GUI_BOX_FLAG_DRAW_HOT_ANIMATION) {
		box->c.r += box->hot_t/6.0f;
		if (box->flags & GUI_BOX_FLAG_DRAW_ICON) { box->text_color.r -= box->hot_t/6.0f; box->text_color.g -= box->hot_t/6.0f; box->text_color.b -= box->hot_t/6.0f; }
	}


	// render current box
	if (box->flags & GUI_BOX_FLAG_DRAW_BACKGROUND) {
		gui_draw_rect(box->r, box->c, box);
	}

	// iterate through hierarchy
	for(guiBox *child = box->first; !gui_box_is_nil(child); child = child->next) {
		gui_render_hierarchy(child);
	}
}