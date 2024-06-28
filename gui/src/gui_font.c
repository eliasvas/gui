#include "gui.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../ext/stb/stb_truetype.h"

// TODO -- This field should be configurable
#define GUI_BASE_TEXT_SIZE 32

//TODO -- we need to do the base85 compressed ProggyClean in source thing
//TODO -- we should probably do SDF fonts by default, I think stb_truetype has an API for this


guiStatus gui_font_load_from_file(guiFontAtlas *atlas, const char *filepath){
	guiStatus status = GUI_GUD;
	atlas->tex.width = 1024;
	atlas->tex.height = 1024;
	atlas->tex.data = ALLOC(atlas->tex.width*atlas->tex.height*sizeof(u8));

	u8 *ttf_buffer = ALLOC(1<<20);
	u32 byte_count = fread(ttf_buffer, 1, 1<<20, fopen(filepath, "rb"));
	stbtt_BakeFontBitmap(ttf_buffer,0, GUI_BASE_TEXT_SIZE, atlas->tex.data,1024,1024, 32,96, (stbtt_bakedchar *)atlas->cdata);

	if (ttf_buffer == NULL) {
		printf("Error allocating ttf's storage");
		status = GUI_BAD;
		return status;
	}else if (byte_count == 0) {
		printf("Couldn't read font file: [%s]", filepath);
	}
	stbtt_GetScaledFontVMetrics(ttf_buffer, 0, GUI_BASE_TEXT_SIZE, &atlas->ascent, &atlas->descent, &atlas->line_gap);

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
vec2 gui_font_get_string_dim(char *str) {
	guiState *state = gui_get_ui_state();
	if (!str) return v2(0,0);
	
	f32 text_height = state->atlas.ascent - state->atlas.descent;
	f32 text_width = 0;

	for (u32 i = 0; i < strlen(str); ++i) {
		guiBakedChar b = gui_font_atlas_get_char(&state->atlas, str[i]);
		text_width += b.xadvance;
	}

	return v2(text_width, text_height);
}

// Shouldn't these 2 functions be in gui_render?? hmmmmm, also why are they called gui_draw??
// TODO -- we need a text color stack i think
guiStatus gui_draw_string_in_pos(char *str, vec2 pos, vec4 color) {
	guiState *state = gui_get_ui_state();
	vec2 text_dim = gui_font_get_string_dim(str);
	f32 text_x = pos.x;
	f32 text_y = pos.y;
	
	for (u32 i = 0; i < strlen(str); i+=1) {
		guiBakedChar b = gui_font_atlas_get_char(&state->atlas, str[i]);
		f32 x0 = text_x + b.xoff;
        f32 y0 = text_y + b.yoff + state->atlas.ascent;
        f32 x1 = x0 + (b.x1 - b.x0);
        f32 y1 = y0 + (b.y1 - b.y0);
		gui_render_cmd_buf_add_char(&state->rcmd_buf,&state->atlas, str[i], (vec2){x0,y0}, (vec2){x1-x0,y1-y0},color);
		text_x += b.xadvance;
	}

	return GUI_GUD;
}

guiStatus gui_draw_string_in_rect(char *str, rect r, vec4 color) {
	guiState *state = gui_get_ui_state();
	vec2 text_dim = gui_font_get_string_dim(str);
	f32 text_x = r.x0 + ((r.x1-r.x0) - text_dim.x) / 2.0f;
	f32 text_y = r.y0 + ((r.y1-r.y0) - text_dim.y) / 2.0f;

	return gui_draw_string_in_pos(str, v2(text_x, text_y), color);
}

