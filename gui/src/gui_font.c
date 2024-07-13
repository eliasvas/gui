#include "gui.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../ext/stb/stb_truetype.h"
//TODO -- we need to do the base85 compressed ProggyClean in source thing

// TODO -- This field should be configurable
#define GUI_BASE_TEXT_SIZE 32

// This function will load both Roboto and FontAwesome to one atlas, along with sizing info
guiStatus gui_font_load_default_font(guiFontAtlas *atlas){
	guiStatus status = GUI_GUD;
	atlas->tex.width = 1024;
	atlas->tex.height = 1024;
	atlas->tex.data = ALLOC(atlas->tex.width*atlas->tex.height*sizeof(u8));

	const char *ascii_font_path = "../fonts/roboto.ttf";
	//gui_font_load_from_file(atlas, ascii_font_path);
	stbtt_pack_context pack_context;
	stbtt_PackBegin(&pack_context, atlas->tex.data, atlas->tex.width, atlas->tex.height, 0, 1, NULL);

	u8 *ttf_buffer = ALLOC(1<<20);
	u32 byte_count = fread(ttf_buffer, 1, 1<<20, fopen(ascii_font_path, "rb"));
	assert(byte_count);
	stbtt_PackFontRange(&pack_context, ttf_buffer, 0, GUI_BASE_TEXT_SIZE, 32, 96, (stbtt_packedchar*)atlas->cdata);
	stbtt_GetScaledFontVMetrics(ttf_buffer, 0, GUI_BASE_TEXT_SIZE, &atlas->ascent, &atlas->descent, &atlas->line_gap);



	const char *unicode_font_path = "../fonts/font_awesome.ttf";
	byte_count = fread(ttf_buffer, 1, 1<<20, fopen(unicode_font_path, "rb"));
	assert(byte_count);
	u32 char_count = 2;
	u32 start_uchar = FA_GLYPH_star;
	stbtt_PackFontRange(&pack_context, ttf_buffer, 0, GUI_BASE_TEXT_SIZE, start_uchar, char_count, (stbtt_packedchar*)atlas->udata);
	atlas->base_unicode_codepoint = FA_GLYPH_star;
	//assert(atlas->udata[0].xoff);

	stbtt_PackEnd(&pack_context);
	return status;
}

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

guiPackedChar gui_font_atlas_get_char(guiFontAtlas *atlas, char c){
	guiPackedChar bc = atlas->cdata[c-32];
	return bc;
}

// FIMXE: the whole font API is bullshit
// we should make a GlyphInfo struct and use a HashMap for searching
guiPackedChar gui_font_atlas_get_codepoint(guiFontAtlas *atlas, u32 codepoint){
	if (codepoint < 256)
		return gui_font_atlas_get_char(atlas, codepoint);
	return atlas->udata[codepoint - atlas->base_unicode_codepoint];
}



// calculates width/height of a string in pixels
vec2 gui_font_get_string_dim(char *str) {
	guiState *state = gui_get_ui_state();
	if (!str) return v2(0,0);

	f32 text_height = state->atlas.ascent - state->atlas.descent;
	f32 text_width = 0;

	for (u32 i = 0; i < strlen(str); ++i) {
		guiPackedChar b = gui_font_atlas_get_char(&state->atlas, str[i]);
		text_width += b.xadvance;
	}

	return v2(text_width, text_height);
}

vec2 gui_font_get_icon_dim(u32 codepoint) {
	guiState *state = gui_get_ui_state();
	guiPackedChar b = gui_font_atlas_get_codepoint(&state->atlas, codepoint);
	return v2(b.x1-b.x0, b.y1-b.y0);
}

// Shouldn't these 2 functions be in gui_render?? hmmmmm, also why are they called gui_draw??
// TODO -- we need a text color stack i think
guiStatus gui_draw_string_in_pos(char *str, vec2 pos, vec4 color) {
	guiState *state = gui_get_ui_state();
	vec2 text_dim = gui_font_get_string_dim(str);
	f32 text_x = pos.x;
	f32 text_y = pos.y;

	for (u32 i = 0; i < strlen(str); i+=1) {
		guiPackedChar b = gui_font_atlas_get_codepoint(&state->atlas, (char)str[i]);
		f32 x0 = text_x + b.xoff;
        f32 y0 = text_y + b.yoff + state->atlas.ascent;
        f32 x1 = x0 + (b.x1 - b.x0);
        f32 y1 = y0 + (b.y1 - b.y0);
		gui_render_cmd_buf_add_codepoint(&state->rcmd_buf,&state->atlas, str[i], (vec2){x0,y0}, (vec2){x1-x0,y1-y0},color);
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

guiStatus gui_draw_icon_in_pos(u32 codepoint, vec2 pos, vec4 color) {
	guiState *state = gui_get_ui_state();
	vec2 text_dim = gui_font_get_icon_dim(codepoint);
	f32 text_x = pos.x;
	f32 text_y = pos.y;

	guiPackedChar b = gui_font_atlas_get_codepoint(&state->atlas, codepoint);
	f32 x0 = text_x + b.xoff;
	f32 y0 = text_y + b.yoff + state->atlas.ascent;
	f32 x1 = x0 + (b.x1 - b.x0);
	f32 y1 = y0 + (b.y1 - b.y0);
	gui_render_cmd_buf_add_codepoint(&state->rcmd_buf,&state->atlas, codepoint, (vec2){x0,y0}, (vec2){x1-x0,y1-y0},color);

	return GUI_GUD;
}

