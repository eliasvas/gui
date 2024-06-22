#include "gui.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../ext/stb/stb_truetype.h"

// TODO -- This field should be configurable
#define GUI_TEXT_SIZE 128

//TODO -- we need to do the base85 compressed ProggyClean in source thing
//TODO -- we should probably do SDF fonts by default, I think stb_truetype has an API for this


guiStatus gui_font_load_from_file(guiFontAtlas *atlas, const char *filepath){
	guiStatus status = GUI_GUD;
	atlas->tex.width = 1024;
	atlas->tex.height = 1024;
	atlas->tex.data = ALLOC(atlas->tex.width*atlas->tex.height*sizeof(u8));

	u8 *ttf_buffer = ALLOC(1<<20);
	u32 byte_count = fread(ttf_buffer, 1, 1<<20, fopen(filepath, "rb"));
	stbtt_BakeFontBitmap(ttf_buffer,0, GUI_TEXT_SIZE, atlas->tex.data,1024,1024, 32,96, (stbtt_bakedchar *)atlas->cdata);

	if (ttf_buffer == NULL) {
		printf("Error allocating ttf's storage");
		status = GUI_BAD;
		return status;
	}else if (byte_count == 0) {
		printf("Couldn't read font file: [%s]", filepath);
	}

	FREE(ttf_buffer);
	return status;
}


guiBakedChar gui_font_atlas_get_char(guiFontAtlas *atlas, char c){
	guiBakedChar bc = atlas->cdata[c-32];
	return bc;
}


// calculates how much 'up' we need to go on y axis to draw a string correctly
f32 gui_font_get_string_y_to_add(guiFontAtlas *atlas, char *str) {
	assert(str);
	guiBakedChar bc = gui_font_atlas_get_char(atlas, (u8)str[0]);
	f32 first_yoff = bc.yoff;
	f32 yoff_up = 0;
	for (u32 i = 0; i < strlen(str);++i) {
		bc = gui_font_atlas_get_char(atlas, (u8)str[i]);
		f32 up_left_y = bc.yoff;
		yoff_up = minimum(yoff_up, up_left_y);
	}
	return yoff_up - first_yoff;
}

// calculates width/height of a string in pixels
vec2 gui_font_get_string_dim(guiFontAtlas *atlas, char *str) {
	assert(str);
	vec2 accum = {0};
	guiBakedChar bc = gui_font_atlas_get_char(atlas, (u8)str[0]);
	f32 yoff_up = 0;
	f32 yoff_down = 0;
	for (u32 i = 0; i < strlen(str);++i) {
		bc = gui_font_atlas_get_char(atlas, (u8)str[i]);
		accum.x += bc.xadvance;

		f32 up_left_y = bc.yoff;
		f32 down_right_y = bc.yoff + bc.y1 - bc.y0;
		yoff_up = minimum(yoff_up, up_left_y);
		yoff_down = maximum(yoff_down, down_right_y);
	}
	accum.x -= bc.xadvance;
	accum.x += bc.x1-bc.x0+bc.xoff;
	accum.y = yoff_down-yoff_up;
	return accum;
}

guiStatus gui_draw_string_in_pos(char *str, vec2 pos, vec4 color) {
	guiState *state = gui_get_ui_state();

	guiBakedChar bc = gui_font_atlas_get_char(&state->atlas, str[0]);
	f32 y_to_add = gui_font_get_string_y_to_add(&state->atlas, str);
	f32 first_y_off = bc.yoff;

	f32 xadvance = 0;
	for (u32 i = 0; i < strlen(str); ++i){
		guiBakedChar bc = gui_font_atlas_get_char(&state->atlas, str[i]);
		gui_render_cmd_buf_add_quad(&state->rcmd_buf, (vec2){pos.x+bc.xoff+xadvance, pos.y + (bc.yoff - first_y_off) - y_to_add}, (vec2){bc.x1-bc.x0,bc.y1-bc.y0}, v4(1,0,0,1),0,0,0);
		gui_render_cmd_buf_add_char(&state->rcmd_buf,&state->atlas, str[i], (vec2){pos.x+bc.xoff+xadvance, pos.y + (bc.yoff - first_y_off) - y_to_add}, (vec2){bc.x1-bc.x0,bc.y1-bc.y0},color);
		xadvance += bc.xadvance;

	}

	return GUI_GUD;
}

