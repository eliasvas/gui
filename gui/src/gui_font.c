#include "gui.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../ext/stb/stb_truetype.h"
//TODO -- we need to do the base85 compressed ProggyClean in source thing

// TODO -- This field should be configurable
#define GUI_BASE_TEXT_SIZE 32
#define GUI_BASE_ICON_SIZE 32

// This function will load both Roboto and FontAwesome to one atlas, along with sizing info
guiStatus gui_font_load_default_font(guiFontAtlas *atlas){
	guiStatus status = GUI_GUD;
	atlas->tex.width = 1024;
	atlas->tex.height = 1024;
	atlas->tex.data = ALLOC(atlas->tex.width*atlas->tex.height*sizeof(u8));

	u8 *ttf_buffer = NULL;
	u32 byte_count=0;

	//gui_font_load_from_file(atlas, ascii_font_path);
	stbtt_pack_context pack_context;
	stbtt_PackBegin(&pack_context, atlas->tex.data, atlas->tex.width, atlas->tex.height, 0, 1, NULL);

	const char *ascii_font_path = "../fonts/roboto.ttf";
	ttf_buffer = gui_fu_read_all(ascii_font_path, &byte_count);
	assert(byte_count && "Couldn't read the ASCII ttf");
	stbtt_PackFontRange(&pack_context, ttf_buffer, 0, GUI_BASE_TEXT_SIZE, 32, 96, (stbtt_packedchar*)atlas->cdata);
	stbtt_GetScaledFontVMetrics(ttf_buffer, 0, GUI_BASE_TEXT_SIZE, &atlas->ascent, &atlas->descent, &atlas->line_gap);
	gui_fu_dealloc_all(ttf_buffer);



	const char *unicode_font_path = "../fonts/fa.ttf";
	ttf_buffer = gui_fu_read_all(unicode_font_path, &byte_count);
	assert(byte_count && "Couldn't read the unicode ttf");
	u32 start_uchar = FA_ICON_DOWN_OPEN;
	u32 char_count = FA_ICON_HEART_EMPTY - start_uchar;
	stbtt_PackFontRange(&pack_context, ttf_buffer, 0, GUI_BASE_ICON_SIZE, start_uchar, char_count, (stbtt_packedchar*)atlas->udata);
	atlas->base_unicode_codepoint = FA_ICON_DOWN_OPEN;
	// FIXME -- why are we doing getscaledfontvmetrics two times? its shadowed i think
	stbtt_GetScaledFontVMetrics(ttf_buffer, 0, GUI_BASE_TEXT_SIZE, &atlas->ascent, &atlas->descent, &atlas->line_gap);
	gui_fu_dealloc_all(ttf_buffer);

	stbtt_PackEnd(&pack_context);

	return status;
}

guiPackedChar gui_font_atlas_get_char(guiFontAtlas *atlas, char c){
	guiPackedChar bc = atlas->cdata[c-32];
	return bc;
}

// FIMXE: the whole font API is bullshit
// we should make a GlyphInfo struct and use a HashMap for searching unicode codepoints
guiPackedChar gui_font_atlas_get_codepoint(guiFontAtlas *atlas, u32 codepoint){
	if (codepoint < 256)
		return gui_font_atlas_get_char(atlas, codepoint);
	return atlas->udata[codepoint - atlas->base_unicode_codepoint];
}



// calculates width/height of a string in pixels
guiVec2 gui_font_get_string_dim(char *str, f32 scale) {
	guiState *state = gui_get_ui_state();
	if (!str) return gv2(0,0);

	f32 text_height = state->atlas.ascent - state->atlas.descent;
	f32 text_width = 0;

	for (u32 i = 0; i < strlen(str); ++i) {
		guiPackedChar b = gui_font_atlas_get_char(&state->atlas, str[i]);
		text_width += b.xadvance;
	}

	return gv2(text_width * scale, text_height * scale);
}

guiVec2 gui_font_get_icon_dim(u32 codepoint, f32 scale) {
	guiState *state = gui_get_ui_state();
	guiPackedChar b = gui_font_atlas_get_codepoint(&state->atlas, codepoint);
	f32 text_height = state->atlas.ascent - state->atlas.descent;
	guiVec2 dim = gv2(b.x1-b.x0, b.y1-b.y0);
	return gv2(dim.x * scale, text_height * scale);
}

// Shouldn't these functions be in gui_render?? hmmmmm, also why are they called gui_draw??
guiStatus gui_draw_string_in_pos(char *str, guiVec2 pos, f32 scale, guiVec4 color) {
	guiState *state = gui_get_ui_state();
	guiVec2 text_dim = gui_font_get_string_dim(str,scale);
	f32 text_x = pos.x;
	f32 text_y = pos.y;

	for (u32 i = 0; i < strlen(str); i+=1) {
		guiPackedChar b = gui_font_atlas_get_codepoint(&state->atlas, (u32)str[i]);
		f32 x0 = text_x + b.xoff * scale;
        f32 y0 = text_y + (b.yoff + state->atlas.ascent) * scale;
        f32 x1 = x0 + (b.x1 - b.x0) * scale;
        f32 y1 = y0 + (b.y1 - b.y0) * scale;
		gui_render_cmd_buf_add_codepoint(&state->rcmd_buf,&state->atlas, str[i], (guiVec2){x0,y0}, (guiVec2){x1-x0,y1-y0},color);
		text_x += b.xadvance * scale;
	}

	return GUI_GUD;
}

guiStatus gui_draw_string_in_rect(char *str, guiRect r, f32 scale, guiVec4 color) {
	guiVec2 text_dim = gui_font_get_string_dim(str,scale);
	f32 text_x = r.x0 + ((r.x1-r.x0) - text_dim.x) / 2.0f;
	f32 text_y = r.y0 + ((r.y1-r.y0) - text_dim.y) / 2.0f;

	return gui_draw_string_in_pos(str, gv2(text_x, text_y), scale, color);
}

guiStatus gui_draw_icon_in_rect(u32 codepoint, guiRect r, f32 scale, guiVec4 color) {
	guiVec2 text_dim = gui_font_get_icon_dim(codepoint,scale);
	f32 text_x = r.x0 + ((r.x1-r.x0) - text_dim.x) / 2.0f;
	f32 text_y = r.y0 + ((r.y1-r.y0) - text_dim.y) / 2.0f;

	return gui_draw_icon_in_pos(codepoint, gv2(text_x, text_y), scale, color);
}


guiStatus gui_draw_icon_in_pos(u32 codepoint, guiVec2 pos, f32 scale, guiVec4 color) {
	guiState *state = gui_get_ui_state();
	guiVec2 text_dim = gui_font_get_icon_dim(codepoint,scale);
	f32 text_x = pos.x;
	f32 text_y = pos.y;

	guiPackedChar b = gui_font_atlas_get_codepoint(&state->atlas, codepoint);
	f32 x0 = text_x + b.xoff * scale;
	f32 y0 = text_y + (b.yoff + state->atlas.ascent) * scale;
	f32 x1 = x0 + (b.x1 - b.x0) * scale;
	f32 y1 = y0 + (b.y1 - b.y0) * scale;
	gui_render_cmd_buf_add_codepoint(&state->rcmd_buf,&state->atlas, codepoint, (guiVec2){x0,y0}, (guiVec2){x1-x0,y1-y0},color);

	return GUI_GUD;
}

