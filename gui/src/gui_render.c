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

guiStatus gui_render_cmd_buf_add_quad(guiRenderCommandBuffer *cmd_buf, vec2 p0, vec2 dim, vec4 col){
	guiRenderCommand cmd = {0};
	cmd.pos0 = p0;
	cmd.pos1.x = p0.x + dim.x;
	cmd.pos1.y = p0.y + dim.y;
	cmd.color = col;
	return gui_render_cmd_buf_add(cmd_buf, cmd);
}