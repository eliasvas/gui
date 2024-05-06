#ifndef GUI_H
#define GUI_H

//-----------------------------------------------------------------------------
// BASE
//-----------------------------------------------------------------------------

#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stddef.h>

typedef uint8_t   u8;
typedef int8_t    s8;
typedef uint16_t  u16;
typedef int16_t   s16;
typedef uint32_t  u32;
typedef int32_t   s32;
typedef uint64_t  u64;
typedef int64_t   s64;
typedef float     f32;
typedef double    f64;
typedef int32_t   b32;
typedef char      b8;

typedef union {
    struct{f32 x,y;};
    struct{f32 u,v;};
    struct{f32 r,g;};
    f32 raw[2];
}vec2;
typedef union {
    struct{f32 x,y,z,w;};
    struct{f32 r,g,b,a;};
    f32 raw[4];
}vec4;

#define kilobytes(val) ((val)*1024LL)
#define megabytes(val) ((kilobytes(val))*1024LL)
#define gigabytes(val) ((megabytes(val))*1024LL)
#define terabytes(val) ((gigabytes(val))*1024LL)

#define PI 3.1415926535897f
#define align_pow2(val, align) (((val) + ((align) - 1)) & ~(((val) - (val)) + (align) - 1))
#define align4(val) (((val) + 3) & ~3)
#define align8(val) (((val) + 7) & ~7)
#define align16(val) (((val) + 15) & ~15)
#define equalf(a, b, epsilon) (fabs(b - a) <= epsilon)
#define maximum(a, b) ((a) > (b) ? (a) : (b))
#define minimum(a, b) ((a) < (b) ? (a) : (b))
#define step(threshold, value) ((value) < (threshold) ? 0 : 1)
#define clamp(x, a, b)  (maximum(a, minimum(x, b)))
#define array_count(a) (sizeof(a) / sizeof((a)[0]))
#define memzero(p,s) (memset(p,0,s))

#define ALLOC malloc
#define REALLOC realloc
#define FREE free

//-----------------------------------------------------------------------------
// STRETCHY BUFFER IMPLEMENTATION
//-----------------------------------------------------------------------------

typedef struct
{
    u32 len;
    u32 cap;
    char buf[0];
}sbHdr;
#define sb__hdr(b) ((sbHdr*)((char*)b - offsetof(sbHdr, buf)))
#define sb_len(b) ((b) ? sb__hdr(b)->len : 0)
#define sb_cap(b) ((b) ? sb__hdr(b)->cap : 0)
#define sb_end(b) ((b) + sb_len(b))
#define sb_fit(b, n) ((n) <= sb_cap(b) ? 0 : (*((void**)&(b)) = sb__grow((b), (n), sizeof(*(b)))))
#define sb_push(b, ...) (sb_fit((b), 1 + sb_len(b)), (b)[sb__hdr(b)->len++] = (__VA_ARGS__))
#define sb_free(b) ((b) ? (FREE(sb__hdr(b)), (b) = NULL) : 0)
static void *sb__grow(const void *buf, u32 new_len, u32 element_size)
{
   u32 new_cap = maximum(16, maximum(1 + 2*sb_cap(buf), new_len));
   assert(new_len <= new_cap);
   u32 new_size = offsetof(sbHdr, buf) + new_cap * element_size;
   sbHdr *new_hdr;
   if(buf)
   {
       new_hdr = (sbHdr*)REALLOC(sb__hdr(buf), new_size);
   }
   else
   {
       new_hdr = (sbHdr*)ALLOC(new_size);
       new_hdr->len = 0;
   }
   new_hdr->cap = new_cap;
   return new_hdr->buf;// + offsetof(sbHdr, buf);
}
/* example usage of stretchy buffer
{
	int *arr = NULL;
	sb_push(arr, 1);
	sb_push(arr, 2);
	sb_push(arr, 3);
	for (int i = 0; i < 3; ++i)
	{
		int x = arr[i];
		assert(x == i+1);
	}
	sb_free(arr);
}
*/

typedef enum {
	GUI_GUD,
	GUI_BAD
}guiStatus;

//-----------------------------------------------------------------------------
// FONT
//-----------------------------------------------------------------------------
typedef struct
{
	unsigned short x0,y0,x1,y1; // coordinates of bbox in bitmap
	f32 xoff,yoff,xadvance;
} guiBakedChar;

//always TEX_FORMAT_R8
typedef struct {
	u32 width;
	u32 height;
	u8 *data;
} guiFontAtlasTexture;

typedef struct {
	guiBakedChar cdata[96]; // ASCII 32..126 is 95 glyphs
	guiFontAtlasTexture tex;
} guiFontAtlas;

guiStatus gui_font_load_from_file(guiFontAtlas *atlas, const char *filepath);

guiBakedChar gui_font_atlas_get_char(guiFontAtlas *atlas, char c);

//-----------------------------------------------------------------------------
// INPUT
//-----------------------------------------------------------------------------

typedef enum {
	GUI_LMB,
	GUI_MMB,
	GUI_RMB,
	//....
	GUI_MOUSE_BUTTON_COUNT,
}guiMouseButton;

typedef enum {
	KEY_STATE_UP = 0x0, // 00
	KEY_STATE_PRESSED = 0x1, // 01
	KEY_STATE_RELEASED = 0x2, // 10
	KEY_STATE_DOWN = 0x3, // 11
	KEY_STATE_MASK = 0x3, // 11
}guiMouseButtonState;

typedef enum {
	GUI_INPUT_EVENT_TYPE_KEY_EVENT,
	GUI_INPUT_EVENT_TYPE_MOUSE_MOVE,
	GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT,
}guiInputEventType;

typedef struct {
	u32 param0;
	u32 param1;
	guiInputEventType type;
} guiInputEvent;

typedef struct {
	guiMouseButtonState mb[GUI_MOUSE_BUTTON_COUNT];
	s32 mouse_x, mouse_y;
	guiInputEvent *events;
} guiInputState;

//-----------------------------------------------------------------------------
// RENDERER
//-----------------------------------------------------------------------------

typedef struct {
    vec2 pos0;
    vec2 pos1;
    vec2 uv0; //uv's = v2(0) for quad rendering
    vec2 uv1;
    vec4 color;
}guiRenderCommand;

typedef struct {
	guiRenderCommand *commands;
}guiRenderCommandBuffer;
guiStatus gui_render_cmd_buf_clear(guiRenderCommandBuffer *cmd_buf);
u32 gui_render_cmd_buf_count(guiRenderCommandBuffer *cmd_buf);
guiStatus gui_render_cmd_buf_add(guiRenderCommandBuffer *cmd_buf, guiRenderCommand cmd);
guiStatus gui_render_cmd_buf_add_quad(guiRenderCommandBuffer *cmd_buf, vec2 p0, vec2 dim, vec4 col);

//-----------------------------------------------------------------------------
// INTERFACE
//-----------------------------------------------------------------------------

typedef struct {
	guiRenderCommandBuffer rcmd_buf;
	guiFontAtlas atlas;
	guiInputState gis;
} guiState;

/*
	You should use this function to push all input events to the
	gui library, you should pass all your input events through here
*/
guiStatus gui_input_push_event(guiState *state, guiInputEvent e);

/*
	Once every frame you should call gui_state_update which
	will update input and the internal caching stuff for the gui
*/
guiStatus gui_state_update(guiState *state);


/*
	Should be called before anything else, will initialize all needed
	state for the gui library to function properly
*/
guiStatus gui_state_init(guiState *state);

typedef struct {
	float x,y,w,h;
} guiRect;
guiRect make_gui_rect(float x, float y, float w, float h);

#endif