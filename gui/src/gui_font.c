#include "gui.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "../ext/stb/stb_truetype.h"

//TODO -- we need to do the base85 compressed ProggyClean in source thing
//TODO -- we should probably do SDF fonts by default, I think stb_truetype has an API for this


guiStatus gui_font_load_from_file(guiFontAtlas *atlas, const char *filepath){
	guiStatus status = GUI_GUD;
	atlas->tex.width = 1024;
	atlas->tex.height = 1024;
	atlas->tex.data = ALLOC(atlas->tex.width*atlas->tex.height*sizeof(u8));

	u8 *ttf_buffer = ALLOC(1<<20);
	u32 byte_count = fread(ttf_buffer, 1, 1<<20, fopen(filepath, "rb"));
	stbtt_BakeFontBitmap(ttf_buffer,0, 100.0, atlas->tex.data,1024,1024, 32,96, (stbtt_bakedchar *)atlas->cdata);

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