#include "gui.h"

// forward declared bc I don't want this function to be in the interface (gui.h)
b32 gui_input_mb_down(const guiInputState *gis, guiMouseButton button);

guiStatus gui_state_update(guiState *state){
	gui_render_cmd_buf_clear(&state->rcmd_buf);
	gui_input_process_events(&state->gis);
	if (gui_input_mb_down(&state->gis, GUI_LMB)){
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){100,100}, (vec2){200,200}, (vec4){1,1,1,1});
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){400,400}, (vec2){200,200}, (vec4){1,0,0.5,1});
	}
	
	// SOME SAMPLE RENDERING COMMANDS
	guiBakedChar bc = gui_font_atlas_get_char(&state->atlas, 'D');
	guiBakedChar bc1 = gui_font_atlas_get_char(&state->atlas, 'i');
	guiBakedChar bc2 = gui_font_atlas_get_char(&state->atlas, 'e');
	//printf("[%i %i] [%i %i] %f %f\n",bc1.x0,bc1.y0,bc1.x1,bc1.y1, bc1.xoff, bc1.yoff);
	f32 char_offset_x = 100.0f;
	f32 char_offset_y = 100.0f;
	f32 starting_y_offset= bc.yoff;
	guiRenderCommand rc[] = {
		{{char_offset_x + bc.xoff,char_offset_y + (starting_y_offset - bc.yoff)},{char_offset_x+ bc.xoff+(bc.x1-bc.x0),char_offset_y+(bc.y1-bc.y0)+ (starting_y_offset - bc.yoff)},{0,0},{0,0},{0,0,1,1}},
		{{char_offset_x + bc.xoff,char_offset_y + (starting_y_offset - bc.yoff)},{char_offset_x+ bc.xoff+(bc.x1-bc.x0),char_offset_y+(bc.y1-bc.y0)+ (starting_y_offset - bc.yoff)},{bc.x0,bc.y0},{bc.x1,bc.y1},{0,1,1,1}},

		{{char_offset_x + bc1.xoff + bc.xadvance,char_offset_y + (bc1.yoff-starting_y_offset)},{char_offset_x + bc1.xoff +bc.xadvance+(bc1.x1-bc1.x0),char_offset_y+ (bc1.yoff-starting_y_offset)+(bc1.y1-bc1.y0)},{0,0},{0,0},{0,0,0,1}},
		{{char_offset_x + bc1.xoff + bc.xadvance,char_offset_y + (bc1.yoff-starting_y_offset)},{char_offset_x + bc1.xoff +bc.xadvance+(bc1.x1-bc1.x0),char_offset_y+ (bc1.yoff-starting_y_offset)+(bc1.y1-bc1.y0)},{bc1.x0,bc1.y0},{bc1.x1,bc1.y1},{1,1,0,1}},
		
		{{char_offset_x + bc2.xoff + bc.xadvance+ bc1.xadvance,char_offset_y + (bc2.yoff-starting_y_offset)},{char_offset_x + bc2.xoff +bc.xadvance+ bc1.xadvance+(bc2.x1-bc2.x0),char_offset_y+ (bc2.yoff-starting_y_offset)+(bc2.y1-bc2.y0)},{0,0},{0,0},{1,0.4,0,1}},
		{{char_offset_x + bc2.xoff + bc.xadvance+ bc1.xadvance,char_offset_y + (bc2.yoff-starting_y_offset)},{char_offset_x + bc2.xoff +bc.xadvance+ bc1.xadvance+(bc2.x1-bc2.x0),char_offset_y+ (bc2.yoff-starting_y_offset)+(bc2.y1-bc2.y0)},{bc2.x0,bc2.y0},{bc2.x1,bc2.y1},{0,1,1,1}},
	};
	for (int i = 0; i < array_count(rc);++i){
		gui_render_cmd_buf_add(&state->rcmd_buf, rc[i]);
	}
	return GUI_GUD;
}


guiStatus gui_state_init(guiState *state){
	memzero(state, sizeof(guiState));
	return gui_font_load_from_file(&state->atlas, "C:/windows/fonts/times.ttf");
}