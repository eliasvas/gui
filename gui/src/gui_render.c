#include "gui.h"

guiStatus gui_render_cmd_buf_clear(guiRenderCommandBuffer *cmd_buf){
	sb_free(cmd_buf->commands);
	return GUI_GUD;
}

u32 gui_render_cmd_buf_count(guiRenderCommandBuffer *cmd_buf){
	return sb_len(cmd_buf->commands);
}

guiStatus gui_render_cmd_buf_add(guiRenderCommandBuffer *cmd_buf, guiRenderCommand cmd){
	sb_push(cmd_buf->commands, cmd);
	return GUI_GUD;
}

guiStatus gui_render_cmd_buf_add_quad(guiRenderCommandBuffer *cmd_buf, vec2 p0, vec2 dim, vec4 col, f32 softness, f32 corner_rad, f32 border_thickness){
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
guiStatus gui_render_cmd_buf_add_char(guiRenderCommandBuffer *cmd_buf, guiFontAtlas *atlas, char c, vec2 p0, vec2 dim, vec4 col){
	guiRenderCommand cmd = {0};
	guiBakedChar bc = gui_font_atlas_get_char(atlas, c);
	vec2 uv0 = {bc.x0,bc.y0};
	vec2 uv1 = {bc.x1,bc.y1};
	cmd.pos0 = p0;
	cmd.pos1.x = p0.x + dim.x;
	cmd.pos1.y = p0.y + dim.y;
	cmd.uv0 = uv0;
	cmd.uv1 = uv1;
	cmd.color = col;
	return gui_render_cmd_buf_add(cmd_buf, cmd);
}

guiStatus gui_draw_rect(rect r, vec4 color) {
	guiState *state = gui_get_ui_state();
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){r.x0, r.y0}, (vec2){abs(r.x1-r.x0), abs(r.y1-r.y0)}, color,2,4,0);
	return GUI_GUD;
}

void gui_render_hierarchy(void) {
	guiState *state = gui_get_ui_state();
	for(guiBox *box = state->root->first; !gui_box_is_nil(box); box = box->next)
	{
		if (box->flags & GUI_BOX_FLAG_DRAW_BACKGROUND) {
			gui_draw_rect(box->r, box->c);
		}
	}
}