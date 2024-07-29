#include "gui.h"

guiStatus gui_render_cmd_buf_clear(guiRenderCommandBuffer *cmd_buf){
	gsb_free(cmd_buf->commands);
	return GUI_GUD;
}

u32 gui_render_cmd_buf_count(guiRenderCommandBuffer *cmd_buf){
	return gsb_len(cmd_buf->commands);
}

guiStatus gui_render_cmd_buf_add(guiRenderCommandBuffer *cmd_buf, guiRenderCommand cmd){
	gsb_push(cmd_buf->commands, cmd);
	return GUI_GUD;
}

guiStatus gui_render_cmd_buf_add_quad(guiRenderCommandBuffer *cmd_buf, guiVec2 p0, guiVec2 dim, guiVec4 col, f32 softness, f32 corner_rad, f32 border_thickness){
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

//{{char_offset_x + bc.xoff,char_offset_y + (starting_y_offset - bc.yoff)},{char_offset_x+ bc.xoff+(bc.x1-bc.x0),char_offset_y+(bc.y1-bc.y0)+ (starting_y_offset - bc.yoff)},{bc.x0,bc.y0},{bc.x1,bc.y1},{0,1,1,1},0,0,0},
guiStatus gui_render_cmd_buf_add_codepoint(guiRenderCommandBuffer *cmd_buf, guiFontAtlas *atlas, u32 c, guiVec2 p0, guiVec2 dim, guiVec4 col){
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


guiStatus gui_draw_rect(guiRect r, guiVec4 color, guiBox *box) {
	guiState *state = gui_get_ui_state();
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (guiVec2){r.x0, r.y0}, (guiVec2){fabs(r.x1-r.x0), fabs(r.y1-r.y0)}, color,(box->flags & GUI_BOX_FLAG_ROUNDED_EDGES) ? 2:0, (box->flags & GUI_BOX_FLAG_ROUNDED_EDGES) ? 4:0, 0);
	if (box->flags & GUI_BOX_FLAG_DRAW_BORDER) {
		guiVec4 color_dim = gv4(color.x/2.f,color.y/2.f,color.z/2.f,1.f);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (guiVec2){r.x0, r.y0}, (guiVec2){fabs(r.x1-r.x0), fabs(r.y1-r.y0)}, color_dim,1, (box->flags & GUI_BOX_FLAG_ROUNDED_EDGES) ? 4:0,2);
	}
	if (box->flags & GUI_BOX_FLAG_DRAW_TEXT) {
		gui_draw_string_in_rect(box->str, r, box->text_scale, box->text_color);
	}
	if (box->flags & GUI_BOX_FLAG_DRAW_ICON) {
		gui_draw_icon_in_rect(box->icon_codepoint, r, box->text_scale, box->text_color);
	}
	return GUI_GUD;
}

void gui_render_hierarchy(guiBox *root) {
	// render current box
	if (root->flags & GUI_BOX_FLAG_DRAW_BACKGROUND) {
		gui_draw_rect(root->r, root->c, root);
	}

	// iterate through hierarchy
	for(guiBox *box = root->first; !gui_box_is_nil(box); box = box->next) {
		gui_render_hierarchy(box);
	}
}