#ifndef GUI_H
#define GUI_H

//##########################
// BASE
//##########################

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

typedef struct guiRange2 guiRange2;
struct guiRange2 {
	s64 min;
	s64 max;
};

typedef union {
    struct{f32 x,y;};
    struct{f32 u,v;};
    struct{f32 r,g;};
    struct{f32 min,max;};
    f32 raw[2];
}guiVec2;
#define gv2(x,y) (guiVec2){x,y}

typedef union {
    struct{f32 x,y,z,w;};
    struct{f32 r,g,b,a;};
    f32 raw[4];
}guiVec4;
#define gv4(x,y,z,w) (guiVec4){x,y,z,w}

typedef union {
    struct{f32 x0,y0,x1,y1;};
    struct{guiVec2 p0,p1;};
}guiRect;

static b32 gui_point_inside_rect(guiVec2 p, guiRect r) {
	return ( (p.x <= r.x1) && (p.x >= r.x0) && (p.y >= r.y0) && (p.y <= r.y1) );
}

#define KB(val) ((val)*1024LL)
#define MB(val) ((KB(val))*1024LL)
#define GB(val) ((MB(val))*1024LL)
#define TB(val) ((GB(val))*1024LL)

#define PI 3.1415926535897f
#define align_pow2(val, align) (((val) + ((align) - 1)) & ~(((val) - (val)) + (align) - 1))
#define align2(val) align_pow2(val,2)
#define align4(val) align_pow2(val,4)
#define align8(val) align_pow2(val,8)
#define align16(val) align_pow2(val,16)
#define align32(val) align_pow2(val,32)
#define equalf(a, b, epsilon) (fabs(b - a) <= epsilon)
#define maximum(a, b) ((a) > (b) ? (a) : (b))
#define minimum(a, b) ((a) < (b) ? (a) : (b))
#define step(threshold, value) ((value) < (threshold) ? 0 : 1)
#define clamp(x, a, b)  (maximum(a, minimum(x, b)))
#define is_pow2(x) ((x & (x - 1)) == 0)
#define array_count(a) (sizeof(a) / sizeof((a)[0]))
#define each_enumv(type, it) type it = (type)0; it < type##_COUNT; it = (type)(it+1)
#define signof(x) ((x) > 0 ? 1 : ((x) < 0 ? -1 : 0))

#define ALLOC malloc
#define REALLOC realloc
#define FREE free

static guiRect gui_intersect_rects(guiRect a, guiRect b) {
	guiRect r = {0};
	r.p0.x = maximum(a.p0.x, b.p0.x);
	r.p0.y = maximum(a.p0.y, b.p0.y);
	r.p1.x = minimum(a.p1.x, b.p1.x);
	r.p1.y = minimum(a.p1.y, b.p1.y);

	// if no intersection at all, we return empty rect
    if (r.p0.x >= r.p1.x || r.p0.y >= r.p1.y) {
        r.p0.x = r.p0.y = r.p1.x = r.p1.y = 0;
    }

	return r;
}

#if defined(_WIN32)
	#include <windows.h>
#elif __EMSCRIPTEN__
	#include <emscripten.h>
	#include <sys/mman.h>
#else
	#include <sys/mman.h>
#endif

//##########################
// SIMPLE FILE I/O
//##########################
static u8 *gui_fu_read_all(const char *src_filepath, u32 *byte_count) {
	u8 *buffer = NULL;
	*byte_count = 0;
	FILE *file = fopen(src_filepath, "rb");
	if (file) {
		fseek(file,0,SEEK_END);
		u32 size = ftell(file);
		rewind(file);
		buffer = ALLOC(size+1);
		if (buffer) {
			fread(buffer, 1, size,file);
			buffer[size] = '\0'; // do we always need the null termination? i think not
			*byte_count = size;
		}
		fclose(file);
	}
	return buffer;
}
static b32 gui_fu_write_all(const char *dst_filepath, u8 *buffer, u32 byte_count) {
	b32 res = 0;
	FILE *file = fopen(dst_filepath, "wb");
	if (file){
		u32 wsize = fwrite(buffer, 1, byte_count, file);
		res = (wsize == byte_count);
		fclose(file);
	}
	return res;
}
static void gui_fu_dealloc_all(u8 *buffer) {
	FREE(buffer);
	buffer = NULL;
}

//##########################
//	ARENA ALLOCATOR (64-Bit)
//##########################

#if defined(_WIN32)
    #include <windows.h>
    #define GUI_M_RESERVE(bytes) VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_NOACCESS)
    #define GUI_M_COMMIT(reserved_ptr, bytes) VirtualAlloc(reserved_ptr, bytes, MEM_COMMIT, PAGE_READWRITE)
    #define GUI_M_ALLOC(bytes) VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
    #define GUI_M_RELEASE(base, bytes) VirtualFree(base, bytes, MEM_RELEASE)
    #define GUI_M_ZERO(p, s) (ZeroMemory(p, s))
#else
    #include <sys/mman.h>
    #include <string.h>
    #define GUI_M_RESERVE(bytes) mmap(NULL, bytes, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0)
    #define GUI_M_COMMIT(reserved_ptr, bytes) mmap(reserved_ptr, bytes, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define GUI_M_ALLOC(bytes) mmap(NULL, bytes, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0)
    #define GUI_M_RELEASE(base, bytes) munmap(base, bytes)
    #define GUI_M_ZERO(p, s) memset(p, 0, s)
#endif

#define GUI_M_ZERO_STRUCT(p)  GUI_M_ZERO((p), sizeof(*(p)))
#define GUI_M_ZERO_ARRAY(a)  GUI_M_ZERO((a), sizeof(a))


typedef struct guiArena guiArena;
struct guiArena {
	guiArena *curr;
	guiArena *prev;
	u64 alignment;
	u64 base_pos;
	u64 chunk_cap; // how many bytes current chunk can save
	u64 chunk_pos; // our offset inside that chunk
	u64 chunk_commit_pos; // how much memory in this chunk is CURRENTLY commited
	// for packing in 64-bit offset
	b32 growable;
	u32 trash;
};

typedef struct guiArenaTemp guiArenaTemp;
struct guiArenaTemp {
	guiArena *arena;
	u64 pos;
};

#define MGUI_ARENA_INITIAL_COMMIT_SIZE KB(4)
#define MGUI_ARENA_MAX_ALIGN 64
#define MGUI_ARENA_DEFAULT_RESERVE_SIZE MB(1)
#define MGUI_ARENA_COMMIT_BLOCK_SIZE MB(1)
#define MGUI_ARENA_INTERNAL_MIN_SIZE align_pow2(sizeof(guiArena), MGUI_ARENA_MAX_ALIGN)

static guiArena* gui_arena_alloc_reserve(u64 reserve_size) {
	guiArena *arena = NULL;
	if (reserve_size >= MGUI_ARENA_INITIAL_COMMIT_SIZE) {
		void *mem = GUI_M_RESERVE(reserve_size);
		if (GUI_M_COMMIT(mem, MGUI_ARENA_INITIAL_COMMIT_SIZE)) {
			arena = (guiArena*)mem;
			arena->curr = arena;
			arena->prev = 0;
			arena->alignment = sizeof(void*);
			arena->base_pos = 0;
			arena->growable = 1;
			arena->chunk_cap = reserve_size;
			arena->chunk_pos = MGUI_ARENA_INTERNAL_MIN_SIZE;
			arena->chunk_commit_pos = MGUI_ARENA_INITIAL_COMMIT_SIZE;
		}
	}
	assert(arena != NULL);
	return arena;
}

static guiArena* gui_arena_alloc() {
	return gui_arena_alloc_reserve(MGUI_ARENA_DEFAULT_RESERVE_SIZE);
}

static void gui_arena_release(guiArena *arena) {
	guiArena *curr = arena->curr;
	for(;curr != NULL;) {
        void *prev = curr->prev;
		GUI_M_RELEASE(curr, curr->chunk_cap);
        curr = prev;
	}
	//GUI_M_ZERO_STRUCT(arena);
}

static void* gui_arena_push_nz(guiArena *arena, u64 size) {
	void *res = NULL;
	guiArena *curr = arena->curr;

	// alloc a new chunk if not enough space
	if (arena->growable) {
		u64 next_chunk_pos = align_pow2(curr->chunk_pos, arena->alignment);
		if (next_chunk_pos + size > curr->chunk_cap) {
			u64 new_reserved_size = MGUI_ARENA_DEFAULT_RESERVE_SIZE;
			u64 least_size = MGUI_ARENA_INTERNAL_MIN_SIZE + size;
			if (new_reserved_size < least_size) {
				// because 4KB is recommended page size for most current Architectures
				new_reserved_size = align_pow2(least_size, KB(4));
			}
			void *mem = GUI_M_RESERVE(new_reserved_size);
			if (GUI_M_COMMIT(mem, MGUI_ARENA_INITIAL_COMMIT_SIZE)) {
				guiArena *new_chunk_arena = (guiArena*)mem;
				new_chunk_arena->curr = new_chunk_arena;
				new_chunk_arena->prev = curr;
				new_chunk_arena->base_pos = curr->base_pos + curr->chunk_cap;
				new_chunk_arena->chunk_cap = new_reserved_size;
				new_chunk_arena->chunk_pos = MGUI_ARENA_INTERNAL_MIN_SIZE;
				new_chunk_arena->chunk_commit_pos = MGUI_ARENA_INITIAL_COMMIT_SIZE;
				curr = new_chunk_arena;
				arena->curr = new_chunk_arena;
			}
		}
	}

	// assumming we have enough free space ( < chunk_cap)
	u64 result_pos = align_pow2(curr->chunk_pos, arena->alignment);
	u64 next_chunk_pos = result_pos + size;
	if (next_chunk_pos <= curr->chunk_cap) {
        // if we need memory for next_chunk_pos that isn't already commited, commit it
		if (next_chunk_pos > curr->chunk_commit_pos) {
			u64 next_commit_pos_aligned = align_pow2(next_chunk_pos, MGUI_ARENA_COMMIT_BLOCK_SIZE);
			u64 next_commit_pos = minimum(next_commit_pos_aligned,curr->chunk_cap);
			u64 commit_size = next_commit_pos - curr->chunk_commit_pos;
			if (GUI_M_COMMIT((u8*)curr + curr->chunk_commit_pos, commit_size)) {
				curr->chunk_commit_pos = next_commit_pos;
			}
		}
	}

	// if allocation succesful, return the pointer
	if (next_chunk_pos <= curr->chunk_commit_pos) {
        //unpoison the memory before returning it
		res = (u8*)curr + result_pos;
		curr->chunk_pos = next_chunk_pos;
	}

	return res;
}

static void* gui_arena_push(guiArena *arena, u64 size) {
	guiArena *curr = arena->curr;
	void *res = gui_arena_push_nz(curr, size);
	GUI_M_ZERO(res, size);
	return res;
}

static void gui_arena_align(guiArena *arena, u64 p)
{
	assert(is_pow2(p) && p < MGUI_ARENA_MAX_ALIGN);
	guiArena *curr = arena->curr;
	u64 current_chunk_pos = curr->chunk_pos;
	u64 current_chunk_pos_aligned = align_pow2(curr->chunk_pos, p);
	u64 needed_space = current_chunk_pos_aligned - current_chunk_pos;
	// This 'if' might not be needed
	if (needed_space > 0) {
		gui_arena_push(curr, needed_space);
	}
}

static u64 gui_arena_current_pos(guiArena *arena){
	guiArena *curr = arena->curr;
	u64 pos = curr->base_pos + curr->chunk_pos;
	return(pos);
}

static void gui_arena_pop_to_pos(guiArena *arena, u64 pos) {
	guiArena *curr = arena->curr;
	u64 total_pos = gui_arena_current_pos(curr);
	// release chunks that BEGIN after this pos
	if (pos < total_pos) {
		// We need at least MGUI_ARENA_INTERNAL_MIN_SIZE of allocation in our arena (for the guiArena* at least)
		u64 clamped_total_pos = maximum(pos, MGUI_ARENA_INTERNAL_MIN_SIZE);
		for(;clamped_total_pos < curr->base_pos;) {
			guiArena *prev = curr->prev;
			GUI_M_RELEASE(curr, curr->chunk_cap);
			curr = prev;
		}
        // arena's curr will become the last arena to have its memory released
		arena->curr = curr;

        // update arena's chunk_pos to only contain up to pop pos
		u64 chunk_pos = clamped_total_pos - curr->base_pos;
		u64 clamped_chunk_pos = maximum(chunk_pos, MGUI_ARENA_INTERNAL_MIN_SIZE);
		curr->chunk_pos = clamped_chunk_pos;
	}
}

static void gui_arena_pop_amount(guiArena *arena, u64 amount) {
	guiArena *curr = arena->curr;
	u64 total_pos = gui_arena_current_pos(curr);
	if (amount <= total_pos) {
		u64 new_pos = total_pos - amount;
		gui_arena_pop_to_pos(arena, new_pos);
	}
}

static void gui_arena_clear(guiArena *arena) {
  gui_arena_pop_to_pos(arena, 0);
}

static guiArenaTemp gui_arena_begin_temp(guiArena *arena) {
	u64 pos = gui_arena_current_pos(arena);
	guiArenaTemp t = {arena, pos};
	return t;
}

static void gui_arena_end_temp(guiArenaTemp *t) {
	gui_arena_pop_to_pos(t->arena, t->pos);
}

#define gui_push_array_nz(arena, type, count) (type *)gui_arena_push_nz((arena), sizeof(type)*(count))
#define gui_push_array(arena, type, count) (type *)gui_arena_push((arena), sizeof(type)*(count))

//##########################
// DATA STRUCTURES (mainly for use in conj. with guiArena)
//##########################

#define sll_stack_push_N(f,n,next) ((n)->next=(f), (f)=(n))
#define sll_stack_pop_N(f,next) ((f==0)?((f)=(f)):((f)=(f)->next))
#define sll_stack_push(f,n) sll_stack_push_N(f,n,next)
#define sll_stack_pop(f) sll_stack_pop_N(f,next)

#define sll_queue_push_N(f,l,n,next) (((f)==0)?((f)=(l)=(n)):((l)->next=(n),(l)=(n),(n)->next=0))
#define sll_queue_pop_N(f,l,next) (((f)==(l))?((f)=(l)=0):((f)=(f)->next))
#define sll_queue_push(f,l,n) sll_queue_push_N(f,l,n,next)
#define sll_queue_pop(f,l) sll_queue_pop_N(f,l,next)

#define check_zero(z,p) ((p) == 0 || (p) == z)
#define set_zero(z,p) ((p) = z)
#define dll_insert_NPZ(z,f,l,p,n,next,prev) (check_zero(z,f) ? ((f) = (l) = (n), set_zero(z,(n)->next), set_zero(z,(n)->prev)) : check_zero(z,p) ? ((n)->next = (f), (f)->prev = (n), (f) = (n), set_zero(z,(n)->prev)) : ((p)==(l)) ? ((l)->next = (n), (n)->prev = (l), (l) = (n), set_zero(z, (n)->next)) : (((!check_zero(z,p) && check_zero(z,(p)->next)) ? (0) : ((p)->next->prev = (n))), ((n)->next = (p)->next), ((p)->next = (n)), ((n)->prev = (p))))
#define dll_push_back_NPZ(z,f,l,n,next,prev) dll_insert_NPZ(z,f,l,l,n,next,prev)
#define dll_push_back_NP(f,l,n,next,prev) dll_push_back_NPZ(0,f,l,n,next,prev)
#define dll_push_back(f,l,n) dll_push_back_NP(f,l,n,next,prev)
#define dll_push_front_NPZ(z,f,l,n,next,prev) dll_insert_NPZ(z,l,f,f,n,prev,next)
#define dll_push_front_NP(f,l,n,next,prev) dll_push_front_NPZ(0,f,l,n,next,prev)
#define dll_push_front(f,l,n) dll_push_back_NP(l,f,n,prev,next)
#define dll_remove_NPZ(z,f,l,n,next,prev) (((n) == (f) ? (f) = (n)->next : (0)), ((n) == (l) ? (l) = (l)->prev : (0)), (check_zero(z,(n)->prev) ? (0) : ((n)->prev->next = (n)->next)), (check_zero(z,(n)->next) ? (0) : ((n)->next->prev = (n)->prev)))
#define dll_remove_NP(f,l,n,next,prev) dll_remove_NPZ(0,f,l,n,next,prev)
#define dll_remove(f,l,n) dll_remove_NP(f,l,n,next,prev)


//##########################
// GENERIC HASH FUNCTION
//##########################
static u64 djb2(u8 *str) {
	if (!str) return 0;
	u64 hash = 5381;
	int c;
	while ((c = *str++))
		hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
	return hash;
}

typedef enum {
	GUI_GUD,
	GUI_BAD
}guiStatus;

//##########################
// FONT
//##########################


typedef struct guiPackedChar guiPackedChar;

struct guiPackedChar {
	unsigned short x0,y0,x1,y1; // coordinates of bbox in bitmap
	f32 xoff,yoff,xadvance;
	f32 xoff2,yoff2;
};

//always TEX_FORMAT_R8
typedef struct guiFontAtlasTexture guiFontAtlasTexture;
struct guiFontAtlasTexture {
	u32 width;
	u32 height;
	u8 *data;
};

typedef struct guiFontAtlas guiFontAtlas;
struct guiFontAtlas {
	guiPackedChar cdata[96]; // ASCII 32..126 is 95 glyphs
	// TODO -- maybe not all our Unicode glyphs are packed one after another.. what next? we need a HashMap!
	guiPackedChar udata[200]; // 200 Unicode glyphs
	u32 unicode_base_address;
	guiFontAtlasTexture tex;
	f32 ascent, descent, line_gap;
	u32 base_unicode_codepoint;
};

guiStatus gui_font_load_default_font(guiArena *arena, guiFontAtlas *atlas);
guiPackedChar gui_font_atlas_get_codepoint(guiFontAtlas *atlas, u32 codepoint);

//##########################
// INPUT
//##########################

typedef enum GUI_MOUSE_BUTTON GUI_MOUSE_BUTTON;

enum GUI_MOUSE_BUTTON {
	GUI_LMB,
	GUI_MMB,
	GUI_RMB,
	//....
	GUI_MOUSE_BUTTON_COUNT,
};

typedef enum guiMouseButtonState guiMouseButtonState;
enum guiMouseButtonState {
	KEY_STATE_UP = 0x0, // 00
	KEY_STATE_PRESSED = 0x1, // 01
	KEY_STATE_RELEASED = 0x2, // 10
	KEY_STATE_DOWN = 0x3, // 11
	KEY_STATE_MASK = 0x3, // 11
};

typedef enum guiInputEventNodeType guiInputEventNodeType;
enum guiInputEventNodeType {
	GUI_INPUT_EVENT_TYPE_KEY_EVENT,
	GUI_INPUT_EVENT_TYPE_MOUSE_MOVE,
	GUI_INPUT_EVENT_TYPE_MOUSE_BUTTON_EVENT,
	GUI_INPUT_EVENT_TYPE_SCROLLWHEEL_EVENT,
};

typedef struct guiInputEventNode guiInputEventNode;
struct guiInputEventNode {
	u32 param0;
	u32 param1;
	guiInputEventNodeType type;
	guiInputEventNode *next;
};

typedef struct guiInputState guiInputState;
struct guiInputState {
	guiArena *event_arena;
	// global state used inside the GUI
	guiMouseButtonState mb[GUI_MOUSE_BUTTON_COUNT];
	s32 mouse_x, mouse_y;
	s32 scroll_y, prev_scroll_y;

	// per-frame event system (queue-like)
	guiInputEventNode *first;
	guiInputEventNode *last;
};

//##########################
// RENDERER
//##########################

typedef struct guiRenderCommand guiRenderCommand;
struct guiRenderCommand {
    guiVec2 pos0;
    guiVec2 pos1;
    guiVec2 uv0;
    guiVec2 uv1;
    guiVec4 color;
    float corner_radius;
    float edge_softness;
    float border_thickness;
};

typedef struct guiRenderCommandNode guiRenderCommandNode;
struct guiRenderCommandNode {
	guiRenderCommand rc;
	guiRenderCommandNode *next;
};
typedef struct guiRenderCommandBuffer guiRenderCommandBuffer;
struct guiRenderCommandBuffer {
	guiRenderCommandNode *first;
	guiRenderCommandNode *last;
};

void gui_render_cmd_buf_clear(guiRenderCommandBuffer *cmd_buf);
u32 gui_render_cmd_buf_get_count(guiRenderCommandBuffer *cmd_buf);
guiRenderCommand *gui_render_cmd_buf_get_array(guiRenderCommandBuffer *cmd_buf);
guiRenderCommand *gui_render_cmd_buf_add_quad(guiRenderCommandBuffer *cmd_buf, guiVec2 p0, guiVec2 dim, guiVec4 col, f32 softness, f32 corner_rad, f32 border_thickness);
guiRenderCommand *gui_render_cmd_buf_add_codepoint(guiRenderCommandBuffer *cmd_buf, guiFontAtlas *atlas, u32 c, guiVec2 p0, guiVec2 dim, guiVec4 col);
guiRenderCommand *gui_render_cmd_buf_add_codepoint_testclip(guiRenderCommandBuffer *cmd_buf, guiFontAtlas *atlas, u32 c, guiVec2 p0, guiVec2 dim, guiVec4 col, guiRect clip);

//##########################
// UI CORE STUFF
//##########################

typedef struct guiScrollPoint guiScrollPoint;

struct guiScrollPoint {
  u64 idx; // current index (between min/max)
  f32 off; // fractional offset
};

typedef struct guiSliderData guiSliderData;
struct guiSliderData {
	guiScrollPoint point;
	f32 value; // the actual value being calulated through the scroll point (updated each frame)
};


guiScrollPoint gui_scroll_point_make(u64 idx, f32 off);
void gui_scroll_point_target_idx(guiScrollPoint *p, s64 idx);
void gui_scroll_point_clamp_idx(guiScrollPoint *p, guiVec2 range);
guiSliderData gui_slider_data_make(guiScrollPoint sp, f32 px);

typedef enum guiSizeKind guiSizeKind;
enum guiSizeKind {
	GUI_SIZEKIND_NULL,
	GUI_SIZEKIND_PIXELS, // direct size in pixels
	GUI_SIZEKIND_TEXT_CONTENT, // axis' size determined by string dimensions
	GUI_SIZEKIND_PERCENT_OF_PARENT, // we want a percent of parent box's size in some axis
	GUI_SIZEKIND_CHILDREN_SUM, // size of given axis is sum of children sizes layed out in order
};

typedef struct guiSize guiSize;
struct guiSize {
	guiSizeKind kind;
	f32 value;
	f32 strictness;
};

typedef enum {
	AXIS2_X,
	AXIS2_Y,
	AXIS2_COUNT,
} Axis2;

typedef u32 guiKey;

typedef u32 guiBoxFlags;
enum {
	GUI_BOX_FLAG_CLICKABLE             = (1<<0),
	GUI_BOX_FLAG_DRAW_TEXT             = (1<<1),
	GUI_BOX_FLAG_DRAW_BORDER           = (1<<2),
	GUI_BOX_FLAG_DRAW_BACKGROUND       = (1<<3),
	GUI_BOX_FLAG_DRAW_HOT_ANIMATION    = (1<<4),
	GUI_BOX_FLAG_DRAW_ACTIVE_ANIMATION = (1<<5),
	GUI_BOX_FLAG_DRAW_ICON             = (1<<6),
	GUI_BOX_FLAG_FIXED_X               = (1<<7),
	GUI_BOX_FLAG_FIXED_Y               = (1<<8),
	GUI_BOX_FLAG_FIXED_WIDTH           = (1<<9),
	GUI_BOX_FLAG_FIXED_HEIGHT          = (1<<10),
	GUI_BOX_FLAG_ROUNDED_EDGES         = (1<<11),
	GUI_BOX_FLAG_OVERFLOW_X            = (1<<12),
	GUI_BOX_FLAG_OVERFLOW_Y            = (1<<13),
	// TODO -- add logic for flag disabled.. (probably disable layouting/signals/rendering)
	GUI_BOX_FLAG_DISABLED              = (1<<14),
	//-----------------------------------------------
	GUI_BOX_FLAG_HOVERING              = (1<<15),
	GUI_BOX_FLAG_CLIP                  = (1<<16),
	GUI_BOX_FLAG_SCROLL                = (1<<17),
};

#define GUI_BOX_MAX_STRING_SIZE 64

typedef struct guiBox guiBox;
struct guiBox {
	// gui box hierarchy (used to calculate new positions/animations/wtv each frame)
	guiBox *first; // first child
	guiBox *last; // last child
	guiBox *next; // next box of parent (meaning in the same 'level' on the n-ary tree)
	guiBox *prev; // prev box of parent (in the same 'level', as in, having the same parent with this)
	guiBox *parent; // parent ref

	// gui box hashing (used to iterate all cached boxs, e.g for pruning)
	// this part just represents a doubly linked list Node, as in dll_push_back(hash_first, hash_last, new_box);
	guiBox *hash_next;
	guiBox *hash_prev;

	// key+generation info
	guiKey key;
	u64 last_frame_touched_index; //if last_frame_touched_index < current_frame_index, we PRUNE from cache (box deleted)

	// per-frame info, provided by builder code (the main gui interface, e.g do_button(..))
	guiBoxFlags flags;
	char str[GUI_BOX_MAX_STRING_SIZE]; // box's string data??? (e.g a label)
	//guiSize semantic_size[AXIS2_COUNT];

	// computed every frame
	guiVec2 fixed_pos;
	guiVec2 fixed_size;
	guiSize pref_size[AXIS2_COUNT];
	Axis2 child_layout_axis;

	// f32 computed_rel_position[AXIS2_COUNT]; // position relative to parent
	// f32 computed_size[AXIS2_COUNT]; // computed size in pixels
	guiRect r; // final on-screen rectangular coordinates
	guiVec4 c; // color (not good design wise)
	guiVec4 text_color; // text color (not good design wise)
	f32 text_scale;

	u32 icon_codepoint; // in case this box represents an icon, this is its codepoint

	//persistent data (across box's lifetime)
	f32 hot_t;
	f32 active_t;
	u32 child_count;
	guiVec2 view_off;
	guiVec2 view_off_target;
};


//##########################
// UI stack stuff (for Style stacks and friends)
//##########################

void gui_autopop_all_stacks(void);

typedef struct guiParentNode guiParentNode; struct guiParentNode{guiParentNode *next; guiBox *v;};
typedef struct guiPrefWidthNode guiPrefWidthNode; struct guiPrefWidthNode {guiPrefWidthNode *next; guiSize v;};
typedef struct guiPrefHeightNode guiPrefHeightNode; struct guiPrefHeightNode {guiPrefHeightNode *next; guiSize v;};
typedef struct guiFixedXNode guiFixedXNode; struct guiFixedXNode {guiFixedXNode *next; f32 v;};
typedef struct guiFixedYNode guiFixedYNode; struct guiFixedYNode {guiFixedYNode *next; f32 v;};
typedef struct guiFixedWidthNode guiFixedWidthNode; struct guiFixedWidthNode {guiFixedWidthNode *next; f32 v;};
typedef struct guiFixedHeightNode guiFixedHeightNode; struct guiFixedHeightNode {guiFixedHeightNode *next; f32 v;};
typedef struct guiBgColorNode guiBgColorNode; struct guiBgColorNode {guiBgColorNode *next; guiVec4 v;};
typedef struct guiTextColorNode guiTextColorNode; struct guiTextColorNode {guiTextColorNode *next; guiVec4 v;};
typedef struct guiChildLayoutAxisNode guiChildLayoutAxisNode; struct guiChildLayoutAxisNode {guiChildLayoutAxisNode*next; Axis2 v;};
typedef struct guiTextScaleNode guiTextScaleNode; struct guiTextScaleNode {guiTextScaleNode *next; f32 v;};


guiKey gui_key_zero(void);
guiKey gui_key_from_str(char *s);
b32 gui_key_match(guiKey a, guiKey b);

guiBox *gui_box_nil_id();
b32 gui_box_is_nil(guiBox *box);
guiBox *gui_box_make(guiBoxFlags flags, char *str);
guiBox *gui_box_lookup_from_key(guiBoxFlags flags, guiKey key);
guiBox *gui_box_build_from_str(guiBoxFlags flags, char *str);
guiBox *gui_box_build_from_key(guiBoxFlags flags, guiKey key);

guiBox *gui_push_parent(guiBox *box);
guiBox *gui_set_next_parent(guiBox *box);
guiBox *gui_pop_parent(void);
guiBox *gui_top_parent(void);

f32 gui_push_fixed_x(f32 v);
f32 gui_set_next_fixed_x(f32 v);
f32 gui_pop_fixed_x(void);
f32 gui_top_fixed_x(void);

f32 gui_push_fixed_y(f32 v);
f32 gui_set_next_fixed_y(f32 v);
f32 gui_pop_fixed_y(void);
f32 gui_top_fixed_y(void);

f32 gui_push_fixed_width(f32 v);
f32 gui_set_next_fixed_width(f32 v);
f32 gui_pop_fixed_width(void);
f32 gui_top_fixed_width(void);

f32 gui_push_fixed_height(f32 v);
f32 gui_set_next_fixed_height(f32 v);
f32 gui_pop_fixed_height(void);
f32 gui_top_fixed_height(void);

guiSize gui_push_pref_width(guiSize v);
guiSize gui_set_next_pref_width(guiSize v);
guiSize gui_pop_pref_width(void);
guiSize gui_top_pref_width(void);

guiSize gui_push_pref_height(guiSize v);
guiSize gui_set_next_pref_height(guiSize v);
guiSize gui_pop_pref_height(void);
guiSize gui_top_pref_height(void);

guiVec4 gui_top_bg_color(void);
guiVec4 gui_set_next_bg_color(guiVec4 v);
guiVec4 gui_push_bg_color(guiVec4 v);
guiVec4 gui_pop_bg_color(void);

guiVec4 gui_top_text_color(void);
guiVec4 gui_set_next_text_color(guiVec4 v);
guiVec4 gui_push_text_color(guiVec4 v);
guiVec4 gui_pop_text_color(void);

f32 gui_top_text_scale(void);
f32 gui_set_next_text_scale(f32 v);
f32 gui_push_text_scale(f32 v);
f32 gui_pop_text_scale(void);

void gui_layout_root(guiBox *root, Axis2 axis);
Axis2 gui_top_child_layout_axis(void);
Axis2 gui_set_next_child_layout_axis(Axis2 v);
Axis2 gui_push_child_layout_axis(Axis2 v);
Axis2 gui_pop_child_layout_axis(void);

// pushes fixed widths heights (TODO -- i should probably add all the lower level stack functions in future)
void gui_push_rect(guiRect r);
void gui_set_next_rect(guiRect r);
void gui_pop_rect(void);

guiSize gui_push_pref_size(Axis2 axis, guiSize v);
guiSize gui_set_next_pref_size(Axis2 axis, guiSize v);
guiSize gui_pop_pref_size(Axis2 axis);

typedef u32 guiSignalFlags;
enum {
	GUI_SIGNAL_FLAG_LMB_PRESSED  = (1<<0),
	GUI_SIGNAL_FLAG_MMB_PRESSED  = (1<<2),
	GUI_SIGNAL_FLAG_RMB_PRESSED  = (1<<3),
	GUI_SIGNAL_FLAG_LMB_RELEASED = (1<<4),
	GUI_SIGNAL_FLAG_MMB_RELEASED = (1<<5),
	GUI_SIGNAL_FLAG_RMB_RELEASED = (1<<6),
	GUI_SIGNAL_FLAG_MOUSE_HOVER  = (1<<7),
	GUI_SIGNAL_FLAG_SCROLLED     = (1<<7),
	// TODO -- maybe we need one dragging for each mouse key
	GUI_SIGNAL_FLAG_DRAGGING     = (1<<8),
	// ...
};

typedef struct guiSignal guiSignal;
struct guiSignal {
	guiBox *box;
	guiVec2 mouse;
	guiVec2 drag_delta;
	guiSignalFlags flags;
};

guiSignal gui_get_signal_for_box(guiBox *box);

guiKey gui_get_hot_box_key();
guiKey gui_get_active_box_key(GUI_MOUSE_BUTTON b);


guiSignal gui_panel(char *str);
guiSignal gui_button(char *str);
guiSignal gui_checkbox(char *str, b32 *value);
guiSignal gui_spinner(char *str, Axis2 axis, guiVec2 val_range, guiSliderData *data);
guiSignal gui_slider(char *str, Axis2 axis, guiVec2 val_range, guiSliderData *data);
guiSignal gui_label(char *str);
guiSignal gui_clickable_region(char *str);
guiSignal gui_icon(char *str, u32 icon_codepoint);
guiSignal gui_spacer(guiSize size);


typedef struct guiScrollListRowBlock guiScrollListRowBlock;
struct guiScrollListRowBlock {
	u64 row_count; // how many 'rows' this block covers (vertically)
	u64 item_count; // how many items in this row (horizontally)
};
typedef struct guiScrollListRowBlockArray guiScrollListRowBlockArray;
struct guiScrollListRowBlockArray {
	guiScrollListRowBlock *blocks;
	u64 count;

};
typedef struct guiScrollListOptions guiScrollListOptions;
struct guiScrollListOptions {
	guiVec2 dim_px; // total dimension of scroll list (in pixels)
	f32 row_height_px; // row height of EACH row
	guiScrollListRowBlockArray row_blocks; // all the row blocks
	guiRange2 item_range; // for example [0,3] will produce 3 (0,1,2) visible item
};



guiScrollPoint gui_scroll_bar(Axis2 axis, guiScrollPoint sp, guiRange2 row_range, s64 num_of_visible_rows, b32 with_buttons);
guiSignal      gui_scroll_list_begin(guiScrollListOptions *opt, guiScrollPoint *sp);
void           gui_scroll_list_end();

typedef struct guiSimpleWindowData guiSimpleWindowData;
struct guiSimpleWindowData {
	guiVec2 pos,dim;
	char name[64];
	b32 active;
};
void gui_swindow_begin(guiSimpleWindowData *window, Axis2 layout_axis);
void gui_swindow_end(guiSimpleWindowData *window);


//##########################
// INTERFACE
//##########################

typedef struct guiBoxHashSlot guiBoxHashSlot;
struct guiBoxHashSlot
{
  guiBox *hash_first;
  guiBox *hash_last;
};

typedef struct {
	guiArena *arena;
	guiArena *build_arenas[2];

	u32 box_table_size;
	guiBoxHashSlot *box_table;

	guiBox *first_free_box;

	guiBox *root;

	guiRenderCommandBuffer rcmd_buf;
	guiFontAtlas atlas;
	guiInputState gis;
	u64 current_frame_index;

	guiVec2 win_dim;
	f32 dt;

	guiKey active_box_keys[GUI_MOUSE_BUTTON_COUNT];
	guiKey hot_box_key;
	f32 global_text_scale;

	guiVec2 drag_start_mp;

	// all the stacks! (there are a lot!)
	guiParentNode parent_nil_stack_top;
	struct { guiParentNode *top; guiBox * bottom_val; guiParentNode *free; b32 auto_pop; } parent_stack;
	guiFixedXNode fixed_x_nil_stack_top;
	struct { guiFixedXNode *top; f32 bottom_val; guiFixedXNode *free; b32 auto_pop; } fixed_x_stack;
	guiFixedYNode fixed_y_nil_stack_top;
	struct { guiFixedYNode *top; f32 bottom_val; guiFixedYNode *free; b32 auto_pop; } fixed_y_stack;
	guiFixedWidthNode fixed_width_nil_stack_top;
	struct { guiFixedWidthNode *top; f32 bottom_val; guiFixedWidthNode *free; b32 auto_pop; } fixed_width_stack;
	guiFixedHeightNode fixed_height_nil_stack_top;
	struct { guiFixedHeightNode *top; f32 bottom_val; guiFixedHeightNode *free; b32 auto_pop; } fixed_height_stack;
	guiPrefWidthNode pref_width_nil_stack_top;
	struct { guiPrefWidthNode *top; guiSize bottom_val; guiPrefWidthNode *free; b32 auto_pop; } pref_width_stack;
	guiPrefHeightNode pref_height_nil_stack_top;
	struct { guiPrefHeightNode *top; guiSize bottom_val; guiPrefHeightNode *free; b32 auto_pop; } pref_height_stack;
	guiBgColorNode bg_color_nil_stack_top;
	struct { guiBgColorNode *top; guiVec4 bottom_val; guiBgColorNode *free; b32 auto_pop; } bg_color_stack;
	guiTextColorNode text_color_nil_stack_top;
	struct { guiTextColorNode *top; guiVec4 bottom_val; guiTextColorNode *free; b32 auto_pop; } text_color_stack;
	guiChildLayoutAxisNode child_layout_axis_nil_stack_top;
	struct { guiChildLayoutAxisNode *top; Axis2 bottom_val; guiChildLayoutAxisNode *free; b32 auto_pop; } child_layout_axis_stack;
	guiTextScaleNode text_scale_nil_stack_top;
	struct { guiTextScaleNode *top; f32 bottom_val; guiTextScaleNode *free; b32 auto_pop; } text_scale_stack;
} guiState;
void gui_init_stacks(guiState *state);

guiStatus gui_state_update(f32 dt);
guiState *gui_state_init();


guiVec2 gui_drag_get_start_mp();
void gui_drag_set_mp(guiVec2 mp);
guiVec2 gui_drag_get_delta();
void gui_drag_set_current_mp();
guiStatus gui_input_push_event(guiInputEventNode e);
b32 gui_input_mb_down(GUI_MOUSE_BUTTON button);
b32 gui_input_mb_up(GUI_MOUSE_BUTTON button);
b32 gui_input_mb_pressed(GUI_MOUSE_BUTTON button);
b32 gui_input_mb_released(GUI_MOUSE_BUTTON button);
s32 gui_input_get_scroll_delta();
void gui_input_process_event_queue(void);

void gui_set_ui_state(guiState *state);
guiState * gui_get_ui_state();
guiArena *gui_get_build_arena();

guiVec2 gui_font_get_string_dim(char* str, f32 scale);
guiVec2 gui_font_get_icon_dim(u32 codepoint, f32 scale);
f32 gui_font_get_default_text_height(f32 scale);
guiStatus gui_draw_icon_in_pos(u32 codepoint, guiVec2 pos, f32 scale, guiVec4 color, guiRect clip_rect);
guiStatus gui_draw_icon_in_rect(u32 codepoint, guiRect r, guiRect clip_rect, f32 scale, guiVec4 color);
guiStatus gui_draw_string_in_rect(char *str, guiRect r, guiRect clip_rect, f32 scale, guiVec4 color);
void gui_draw_rect(guiRect r, guiVec4 color, guiBox *box);
void gui_render_hierarchy(guiBox *root);
void print_gui_hierarchy(void);

void gui_build_begin(void);
void gui_build_end(void);

#define FA_ICON_DOWN_OPEN 0xE800
#define FA_ICON_LEFT_OPEN 0xE801
#define FA_ICON_RIGHT_OPEN 0xE802
#define FA_ICON_UP_OPEN 0xE803
#define FA_ICON_RESIZE_FULL_ALT 0xE804
#define FA_ICON_RESIZE_SMALL 0xE805
#define FA_ICON_RESIZE_VERTICAL 0xE806
#define FA_ICON_RESIZE_HORIZONTAL 0xE807
#define FA_ICON_MOVE 0xE808
#define FA_ICON_STAR 0xE809
#define FA_ICON_STAR_EMPTY 0xE80A
#define FA_ICON_STAR_HALF 0xE80B
#define FA_ICON_STAR_HALF_ALT 0xE80C
#define FA_ICON_LOCK 0xE80D
#define FA_ICON_LOCK_OPEN 0xE80E
#define FA_ICON_LOCK_OPEN_ALT 0xE80F
#define FA_ICON_DOWN_BIG 0xE810
#define FA_ICON_LEFT_BIG 0xE811
#define FA_ICON_RIGHT_BIG 0xE812
#define FA_ICON_UP_BIG 0xE813
#define FA_ICON_COG_ALT 0xE814
#define FA_ICON_COG 0xE815
#define FA_ICON_SLIDERS 0xE816
#define FA_ICON_WRENCH 0xE817
#define FA_ICON_CHECK 0xE818
#define FA_ICON_CHECK_EMPTY 0xE819
#define FA_ICON_CIRCLE 0xE81A
#define FA_ICON_CIRCLE_EMPTY 0xE81B
#define FA_ICON_FOLDER 0xE81C
#define FA_ICON_FOLDER_OPEN 0xE81D
#define FA_ICON_FOLDER_EMPTY 0xE81E
#define FA_ICON_FOLDER_OPEN_EMPTY 0xE81F
#define FA_ICON_TH_LARGE 0xE820
#define FA_ICON_TH 0xE821
#define FA_ICON_TH_LIST 0xE822
#define FA_ICON_OK 0xE823
#define FA_ICON_OK_CIRCLED 0xE824
#define FA_ICON_OK_CIRCLED2 0xE825
#define FA_ICON_OK_SQUARED 0xE826
#define FA_ICON_CANCEL 0xE827
#define FA_ICON_CANCEL_CIRCLED 0xE828
#define FA_ICON_CANCEL_CIRCLED2 0xE829
#define FA_ICON_HEART 0xE82A
#define FA_ICON_HEART_EMPTY 0xE82B

#endif