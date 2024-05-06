#include "gui.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../ext/stb/stb_truetype.h"

//TODO -- we need to do the base85 compressed ProggyClean in source thing


guiStatus gui_font_load_from_file(guiFontAtlas *atlas, const char *filepath){
	atlas->tex.width = 1024;
	atlas->tex.height = 1024;
	atlas->tex.data = ALLOC(atlas->tex.width*atlas->tex.height*sizeof(u8));

	u8 *ttf_buffer = ALLOC(1<<20);
	fread(ttf_buffer, 1, 1<<20, fopen(filepath, "rb"));
	stbtt_BakeFontBitmap(ttf_buffer,0, 100.0, atlas->tex.data,1024,1024, 32,96, (stbtt_bakedchar *)atlas->cdata);
	FREE(ttf_buffer);
	return GUI_GUD;
}


guiBakedChar gui_font_atlas_get_char(guiFontAtlas *atlas, char c){
	guiBakedChar bc = atlas->cdata[c-32];
	return bc;
}