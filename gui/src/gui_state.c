#include "gui.h"

// forward declared bc I don't want this function to be in the interface (gui.h)
b32 gui_input_mb_down(const guiInputState *gis, guiMouseButton button);

guiStatus gui_state_update(guiState *state){
	// At the end of every frame, if a box’s last_frame_touched_index < current_frame_index (where, on each frame, the frame index increments), then that box should be “pruned”.
	state->current_frame_index += 1; // This is used to prune unused boxs
	gui_render_cmd_buf_clear(&state->rcmd_buf);
	gui_input_process_events(&state->gis);
	if (gui_input_mb_down(&state->gis, GUI_LMB)){
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){100,100}, (vec2){200,200}, (vec4){1,1,1,1},3,2,0);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){100,100}, (vec2){200,200}, (vec4){1,0,0.5,1},3,2,3);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){400,400}, (vec2){100,100}, (vec4){1,0,0.5,1},0,5,0);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){400,400}, (vec2){100,100}, (vec4){1,1,1,1},0,0,2);
	}

	// SOME SAMPLE RENDERING COMMANDS
	f32 ts_x = 100.0f;
	f32 ts_y = 100.0f;
	guiBakedChar bc = gui_font_atlas_get_char(&state->atlas, 'D');
	// we always want to start from the first character's y offset, initial (first_y_off - bc.yoff) = 0
	f32 first_y_off = bc.yoff;
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){ts_x+bc.xoff, ts_y + (bc.yoff - first_y_off)}, (vec2){bc.x1-bc.x0,bc.y1-bc.y0}, (vec4){1,0,0,1},0,0,0);
	gui_render_cmd_buf_add_char(&state->rcmd_buf,&state->atlas, 'D', (vec2){ts_x+bc.xoff, ts_y + (bc.yoff - first_y_off)}, (vec2){bc.x1-bc.x0,bc.y1-bc.y0},(vec4){1,1,1,1});

	guiBakedChar bc1 = gui_font_atlas_get_char(&state->atlas, 'i');
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){ts_x+bc1.xoff + bc.xadvance, ts_y + (bc1.yoff - first_y_off)}, (vec2){bc1.x1-bc1.x0,bc1.y1-bc1.y0}, (vec4){0.9,0.9,0.2,1},0,0,0);
	gui_render_cmd_buf_add_char(&state->rcmd_buf,&state->atlas, 'i', (vec2){ts_x+bc1.xoff + bc.xadvance, ts_y + (bc1.yoff - first_y_off)}, (vec2){bc1.x1-bc1.x0,bc1.y1-bc1.y0},(vec4){0.1,0.1,1,1});

	guiBakedChar bc2 = gui_font_atlas_get_char(&state->atlas, 'e');
	gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){ts_x+bc2.xoff + bc.xadvance + bc1.xadvance, ts_y + (bc2.yoff - first_y_off)}, (vec2){bc2.x1-bc2.x0,bc2.y1-bc2.y0}, (vec4){0.2,0.5,1,1},0,0,0);
	gui_render_cmd_buf_add_char(&state->rcmd_buf,&state->atlas, 'e', (vec2){ts_x+bc2.xoff + bc.xadvance + bc1.xadvance, ts_y + (bc2.yoff - first_y_off)}, (vec2){bc2.x1-bc2.x0,bc2.y1-bc2.y0},(vec4){0.5,0.9,0.1,1});


	gui_button("Click me!");

	return GUI_GUD;
}


guiStatus gui_state_init(guiState *state){
	memzero(state, sizeof(guiState));
	return gui_font_load_from_file(&state->atlas, "C:/windows/fonts/times.ttf");
}