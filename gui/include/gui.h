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
#define v2(x,y) (vec2){x,y}

typedef union {
    struct{f32 x,y,z,w;};
    struct{f32 r,g,b,a;};
    f32 raw[4];
}vec4;
#define v4(x,y,z,w) (vec4){x,y,z,w}

typedef struct {f32 x0,y0,x1,y1;} rect;

static b32 point_inside_rect(vec2 p, rect r) {
	return ( (p.x <= r.x1) && (p.x >= r.x0) && (p.y >= r.y0) && (p.y <= r.y1) );
}

#define kilobytes(val) ((val)*1024LL)
#define megabytes(val) ((kilobytes(val))*1024LL)
#define gigabytes(val) ((megabytes(val))*1024LL)
#define terabytes(val) ((gigabytes(val))*1024LL)

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

#define ALLOC malloc
#define REALLOC realloc
#define FREE free

#ifdef __GNUC__
	#define thread_loc __thread
#elif __STDC_VERSION__ >= 201112L
	#define thread_loc _Thread_local
#elif defined(_MSC_VER)
	#define thread_loc __declspec(thread)
#else
	# error Cannot define thread_loc
#endif

//-----------------------------------------------------------------------------
// SIMPLE FILE I/O
//-----------------------------------------------------------------------------
static u8 *read_entire_file(const char *src_filepath, u32 *byte_count) {
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
static b32 write_buffer_to_file(const char *dst_filepath, u8 *buffer, u32 byte_count) {
	b32 res = 0;
	FILE *file = fopen(dst_filepath, "wb");
	if (file){
		u32 wsize = fwrite(buffer, 1, byte_count, file);
		res = (wsize == byte_count);
		fclose(file);
	}
	return res;
}
static void free_entire_file(u8 *buffer) {
	FREE(buffer);
	buffer = NULL;
}

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

//-----------------------------------------------------------------------------
//	ARENA ALLOCATOR (64-Bit)
//-----------------------------------------------------------------------------
#if defined(_WIN32)
	#include <windows.h>
	#define M_RESERVE(bytes) VirtualAlloc(NULL, bytes, MEM_RESERVE, PAGE_NOACCESS)
	#define M_COMMIT(reserved_ptr, bytes) VirtualAlloc(reserved_ptr, bytes, MEM_COMMIT, PAGE_READWRITE)
	#define M_ALLOC(bytes) VirtualAlloc(NULL, bytes, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE)
	#define M_RELEASE(base, bytes) VirtualFree(base, bytes, MEM_RESERVE | MEM_RELEASE)
	#define M_ZERO(p,s) (ZeroMemory(p,s))
#else
	#define M_ZERO(p,s) memset(p,0,s)
	#error "Implement virtual memory stuff for this target (probably just mmap)"
#endif
#define M_ZERO_STRUCT(p)  M_ZERO((p), sizeof(*(p)))
#define M_ZERO_ARRAY(a)  M_ZERO((a), sizeof(a))


typedef struct Arena Arena;
struct Arena {
	Arena *curr;
	Arena *prev;
	u64 alignment;
	u64 base_pos;
	u64 chunk_cap; // how many bytes current chunk can save
	u64 chunk_pos; // our offset inside that chunk
	u64 chunk_commit_pos; // how much memory in this chunk is CURRENTLY commited
	// for packing in 64-bit offset
	b32 growable;
	u32 trash;
};
typedef struct ArenaTemp ArenaTemp;
struct ArenaTemp {
	Arena *arena;
	u64 pos;
};
#define M_ARENA_INITIAL_COMMIT kilobytes(4)
#define M_ARENA_MAX_ALIGN 64
#define M_ARENA_DEFAULT_RESERVE_SIZE gigabytes(1)
#define M_ARENA_COMMIT_BLOCK_SIZE megabytes(64)
#define M_ARENA_INTERNAL_MIN_SIZE align_pow2(sizeof(Arena), M_ARENA_MAX_ALIGN)

// ref: https://git.mr4th.com/mr4th-public/mr4th/src/branch/main/src/base/base_big_functions.c
static Arena* arena_alloc_reserve(u64 reserve_size) {
	Arena *arena = NULL;
	if (reserve_size >= M_ARENA_INITIAL_COMMIT) {
		void *mem = M_RESERVE(reserve_size);
		if (M_COMMIT(mem, M_ARENA_INITIAL_COMMIT)) {
			arena = (Arena*)mem;
			arena->curr = arena;
			arena->prev = 0;
			arena->alignment = sizeof(void*);
			arena->base_pos = 0;
			arena->growable = 1;
			arena->chunk_cap = reserve_size;
			arena->chunk_pos = M_ARENA_INTERNAL_MIN_SIZE;
			arena->chunk_commit_pos = M_ARENA_INITIAL_COMMIT;
		}
	}
	assert(arena != NULL);
	return arena;
}
static Arena* arena_alloc() {
	return arena_alloc_reserve(M_ARENA_DEFAULT_RESERVE_SIZE);
}
static void arena_release(Arena *arena) {
	//M_RELEASE(arena, arena->chunk_cap);
	//Arena *curr = arena->curr;
	for(Arena *curr = arena->curr; curr != NULL; curr = curr->prev) {
		M_RELEASE(curr, curr->chunk_cap);
	}
	M_ZERO_STRUCT(arena);
}

static void* arena_push_nz(Arena *arena, u64 size) {
	void *res = NULL;
	Arena *curr = arena->curr;

	// alloc a new chunk if not enough space
	if (arena->growable) {
		u64 next_chunk_pos = align_pow2(curr->chunk_pos, arena->alignment);
		if (next_chunk_pos + size > curr->chunk_cap) {
			u64 new_reserved_size = M_ARENA_DEFAULT_RESERVE_SIZE;
			u64 least_size = M_ARENA_INTERNAL_MIN_SIZE + size;
			if (new_reserved_size < least_size) {
				// because 4KB is recommended page size for most currect Architectures
				new_reserved_size = align_pow2(least_size, kilobytes(4));
			}
			void *mem = M_RESERVE(new_reserved_size);
			if (M_COMMIT(mem, M_ARENA_INITIAL_COMMIT)) {
				Arena *new_chunk_arena = (Arena*)mem;
				new_chunk_arena->curr = new_chunk_arena;
				new_chunk_arena->prev = curr;
				new_chunk_arena->base_pos = curr->base_pos + curr->chunk_cap;
				new_chunk_arena->chunk_cap = new_reserved_size;
				new_chunk_arena->chunk_pos = M_ARENA_INTERNAL_MIN_SIZE;
				new_chunk_arena->chunk_commit_pos = M_ARENA_INITIAL_COMMIT;
				curr = new_chunk_arena;
				arena->curr = new_chunk_arena;
			}
		}
	}

	// assumming we have enough free space ( < chunk_cap)
	u64 result_pos = align_pow2(curr->chunk_pos, arena->alignment);
	u64 next_chunk_pos = result_pos + size;
	if (next_chunk_pos <= curr->chunk_cap) {
		if (next_chunk_pos > curr->chunk_commit_pos) {
			u64 next_commit_pos_aligned = align_pow2(next_chunk_pos, M_ARENA_COMMIT_BLOCK_SIZE);
			u64 next_commit_pos = minimum(next_commit_pos_aligned,curr->chunk_cap);
			u64 commit_size = next_commit_pos - curr->chunk_commit_pos;
			if (M_COMMIT((u8*)curr + curr->chunk_commit_pos, commit_size)) {
				curr->chunk_commit_pos = next_commit_pos;
			}
		}
	}

	// if allocation successful, return the pointer
	if (next_chunk_pos <= curr->chunk_commit_pos) {
		res = (u8*)curr + result_pos;
		curr->chunk_pos = next_chunk_pos;
	}

	return res;
}
static void* arena_push(Arena *arena, u64 size) {
	Arena *curr = arena->curr;
	void *res = arena_push_nz(curr, size);
	M_ZERO(res, size);
	return res;
}

static void arena_align(Arena *arena, u64 p)
{
	assert(is_pow2(p) && p < M_ARENA_MAX_ALIGN);
	Arena *curr = arena->curr;
	u64 current_chunk_pos = curr->chunk_pos;
	u64 current_chunk_pos_aligned = align_pow2(curr->chunk_pos, p);
	u64 needed_space = current_chunk_pos_aligned - current_chunk_pos;
	// This 'if' might not be needed
	if (needed_space > 0) {
		arena_push(curr, needed_space);
	}
}
static u64 arena_current_pos(Arena *arena){
	Arena *curr = arena->curr;
	u64 pos = curr->base_pos + curr->chunk_pos;
	return(pos);
}

static void* arena_pop_to_pos(Arena *arena, u64 pos) {
	Arena *curr = arena->curr;
	u64 total_pos = arena_current_pos(curr);
	// release chunks that BEGIN after this pos
	if (pos < total_pos) {
		// We need at least M_ARENA_INTERNAL_MIN_SIZE of allocation in our arena (for the Arena* at least)
		u64 clamped_total_pos = maximum(pos, M_ARENA_INTERNAL_MIN_SIZE);
		for(;clamped_total_pos < pos;) {
			Arena *prev = curr->prev;
			M_RELEASE(curr, curr->chunk_cap);
			curr = prev;
		}
		arena->curr = curr;
		u64 chunk_pos = clamped_total_pos - curr->base_pos;
		u64 clamped_chunk_pos = maximum(chunk_pos, M_ARENA_INTERNAL_MIN_SIZE);
		curr->chunk_pos = clamped_chunk_pos;
	}
	return NULL;
}
static void* arena_pop_amount(Arena *arena, u64 amount) {
	Arena *curr = arena->curr;
	u64 total_pos = arena_current_pos(curr);
	if (amount <= total_pos) {
		u64 new_pos = total_pos - amount;
		arena_pop_to_pos(arena, new_pos);
	}
}

static void arena_clear(Arena *arena) {
  arena_pop_to_pos(arena, 0);
}

static ArenaTemp arena_begin_temp(Arena *arena) {
	u64 pos = arena_current_pos(arena);
	ArenaTemp t = {arena, pos};
	return t;
}
static void arena_end_temp(ArenaTemp *t) {
	arena_pop_to_pos(t->arena, t->pos);
}
#define push_array_nz(arena, type, count) (type *)arena_push_nz((arena), sizeof(type)*(count))
#define push_array(arena, type, count) (type *)arena_push((arena), sizeof(type)*(count))

static thread_loc Arena *m__scratch_pool[2] = {0};
static ArenaTemp arena_get_scratch(Arena *conflict) {

	// init the scratch pool at the first time
	if (m__scratch_pool[0] == 0) {
		Arena **scratch_slot = m__scratch_pool;
		for (u32 i = 0; i < 2; i+=1, scratch_slot+=1) {
			*scratch_slot = arena_alloc();
		}
	}

	// return the non conflicting arena from pool
	ArenaTemp res = {0};
	Arena **scratch_slot = m__scratch_pool;
	for (u32 i = 0; i < 2; i+=1, scratch_slot+=1) {
		if (*scratch_slot == conflict){
			continue;
		}
		res = arena_begin_temp(*scratch_slot);
	}

	return res;
}

static void arena_test() { printf("------Arena test!-----\n"); printf("---------------------\n"); ArenaTemp temp = arena_get_scratch(NULL); Arena *arena = arena_alloc(); u8 arr[5560]; u8 *mem = arena_push_nz(arena, kilobytes(1)); memcpy(mem, arr, 2560); mem = arena_push(arena, gigabytes(0.1)); printf("arena_current_pos=[%llud]", arena_current_pos(arena)); ArenaTemp t = arena_begin_temp(arena); void *large_mem = arena_push(arena, gigabytes(1)); printf("\nafter [10GB] arena_current_pos=[%llud]", arena_current_pos(arena)); printf("\nafter [10GB] temp_arena_current_pos=[%llud]", arena_current_pos(t.arena)); arena_end_temp(&t); printf("\nafter [POP] arena_current_pos=[%llud]\n", arena_current_pos(arena)); for (int i = 0; i < 9; ++i) { mem[i] = '0'+(9 - i); } printf("%s\n", &mem[0]); arena_release(arena); arena_end_temp(&temp); printf("---------------------\n"); }
/*
// WHY we need to pass conflict arena in arena_get_scratch(..);
void* bar(Arena *arena){
    // this should be arena_get_scratch(arena) to get the other scratch arena, and not foo's
    ArenaTemp temp = arena_get_scratch(0);
    // we allocate memory on 'arena' allocator which is the same as foo's
    u8 *mem = arena_push(arena, kilobytes(1));
    memcpy(mem, "Hello bar\n", strlen("Hello bar\n"));
    // some BS allocation we need to do with temp
    void *bs_allocation = arena_push(temp.arena, megabytes(2));
    // This will free temp, but ALSO, because temp == arena, arena will be freed and our data (Hello bar) will be invalid
    arena_end_temp(&temp);
    return mem;
}
void foo(){
    ArenaTemp temp = arena_get_scratch(0);
    void *mem_old = bar(temp.arena);

    u8 *mem = arena_push(temp.arena, kilobytes(1));
    memcpy(mem, "Hello foo\n", strlen("Hello foo\n"));

    // Because our bar() function popped temp allocator, 'Hello foo' will OVERWRITE 'Hello bar'
    // And it will be the output of this printf, thats why we need to pass conflict arenas in bar
    printf("%s", mem_old);

    arena_end_temp(&temp);
}
*/

//-----------------------------------------------------------------------------
// DATA STRUCTURES (mainly for use in conj. with Arena)
//-----------------------------------------------------------------------------

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

typedef struct TestNode TestNode;
struct TestNode {TestNode*next;TestNode*prev;int data;};
static void print_ll(TestNode *firstn) {printf("{");for(TestNode*n=firstn;n!=NULL;n=n->next){printf("%i,",n->data);}printf("}\n");}
static void ll_test() { ArenaTemp temp = arena_get_scratch(NULL); printf("--Linked-List test!--\n"); printf("---------------------\n"); TestNode *ll_first = NULL; printf("SLL_STACK_PUSH: "); for (int i = 0; i < 10; ++i) { TestNode *n = push_array(temp.arena, TestNode, 1); n->data = i; sll_stack_push(ll_first, n); } print_ll(ll_first); printf("SLL_STACK_POP3E_I7:  "); sll_stack_pop(ll_first); sll_stack_pop(ll_first); sll_stack_pop(ll_first); TestNode *g0 = push_array(temp.arena, TestNode, 1); g0->data = 7; sll_stack_push(ll_first,g0); print_ll(ll_first); ll_first = NULL; TestNode *ll_last = NULL; printf("SLL_QUEUE_PUSH: "); for (int i = 0; i < 10; ++i) { TestNode *n = push_array(temp.arena, TestNode, 1); n->data = i; sll_queue_push(ll_first,ll_last, n); } print_ll(ll_first); printf("SLL_QUEUE_POP3E_I7:  "); sll_queue_pop(ll_first,ll_last); sll_queue_pop(ll_first,ll_last); sll_queue_pop(ll_first,ll_last); TestNode *g1 = push_array(temp.arena, TestNode, 1); g1->data = 7; sll_queue_push(ll_first,ll_last, g1); print_ll(ll_first); ll_first = NULL; ll_last = NULL; printf("DLL_PUSH_BACK:  "); for (int i = 0; i < 10; ++i) { TestNode *n = push_array(temp.arena, TestNode, 1); n->data = i; dll_push_back(ll_first,ll_last, n); } print_ll(ll_first); printf("DLL_REMOVE_ODD: "); for (TestNode *n = ll_first;n != NULL;n=n->next){ if (n->data % 2 == 0) { dll_remove(ll_first,ll_last,n); } } print_ll(ll_first); ll_first = NULL; ll_last = NULL; printf("DLL_PUSH_FRONT: "); for (int i = 0; i < 10; ++i) { TestNode *n = push_array(temp.arena, TestNode, 1); n->data = i; dll_push_front(ll_first,ll_last, n); } print_ll(ll_first); printf("DLL_REMOVE_ODD: "); for (TestNode *n = ll_first;n != NULL;n=n->next){ if (n->data % 2 == 0) { dll_remove(ll_first,ll_last,n); } } print_ll(ll_first); printf("---------------------\n"); arena_end_temp(&temp); }



//-----------------------------------------------------------------------------
// GENERIC HASH FUNCTION
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// STYLE
//-----------------------------------------------------------------------------

typedef struct {
	vec4 base_text_color;
	vec4 base_background_color;
}guiStyle;


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
}GUI_MOUSE_BUTTON;

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
}guiInputEventNodeType;

typedef struct guiInputEventNode guiInputEventNode;
struct guiInputEventNode {
	u32 param0;
	u32 param1;
	guiInputEventNodeType type;
	guiInputEventNode *next;
};

typedef struct {
	Arena *event_arena;
	// global state used inside the GUI
	guiMouseButtonState mb[GUI_MOUSE_BUTTON_COUNT];
	s32 mouse_x, mouse_y;

	// per-frame event system (queue-like)
	guiInputEventNode *first;
	guiInputEventNode *last;
} guiInputState;

//-----------------------------------------------------------------------------
// RENDERER
//-----------------------------------------------------------------------------

typedef struct {
    vec2 pos0;
    vec2 pos1;
    vec2 uv0;
    vec2 uv1;
    vec4 color;
    float corner_radius;
    float edge_softness;
    float border_thickness;
}guiRenderCommand;

typedef struct {
	guiRenderCommand *commands;
}guiRenderCommandBuffer;
guiStatus gui_render_cmd_buf_clear(guiRenderCommandBuffer *cmd_buf);
u32 gui_render_cmd_buf_count(guiRenderCommandBuffer *cmd_buf);
guiStatus gui_render_cmd_buf_add(guiRenderCommandBuffer *cmd_buf, guiRenderCommand cmd);
guiStatus gui_render_cmd_buf_add_quad(guiRenderCommandBuffer *cmd_buf, vec2 p0, vec2 dim, vec4 col, f32 softness, f32 corner_rad, f32 border_thickness);
guiStatus gui_render_cmd_buf_add_char(guiRenderCommandBuffer *cmd_buf, guiFontAtlas *atlas, char c, vec2 p0, vec2 dim, vec4 col);
//-----------------------------------------------------------------------------
// UI CORE STUFF
//-----------------------------------------------------------------------------
typedef enum {
	GUI_SIZEKIND_NULL,
	GUI_SIZEKIND_PIXELS, // direct size in pixels
	GUI_SIZEKIND_TEXT_CONTENT, // axis' size determined by string dimensions
	GUI_SIZEKIND_TEXT_PERCENT_OF_PARENT, // we want a percent of parent box's size in some axis
	GUI_SIZEKIND_CHILDREN_SUM, // size of given axis is sum of children sizes layed out in order
} guiSizeKind;

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
	vec2 fixed_pos;
	vec2 fixed_size;
	guiSize pref_size[AXIS2_COUNT];
	Axis2 child_layout_axis;

	// f32 computed_rel_position[AXIS2_COUNT]; // position relative to parent
	// f32 computed_size[AXIS2_COUNT]; // computed size in pixels
	rect r; // final on-screen rectangular coordinates
	vec4 c; // color (not good design wise)

	//persistent data (across box's lifetime)
	f32 hot_t;
	f32 active_t;
	u32 child_count;
};
static guiBox g_nil_box = {
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box,
	&g_nil_box
};

//-----------------------------------------------------------------------------
// UI stack stuff (for Style stacks and friends)
//-----------------------------------------------------------------------------
void gui_autopop_all_stacks(void);

typedef struct guiParentNode guiParentNode; struct guiParentNode{guiParentNode *next; guiBox *v;};
typedef struct guiPrefWidthNode guiPrefWidthNode; struct guiPrefWidthNode {guiPrefWidthNode *next; guiSize v;};
typedef struct guiPrefHeightNode guiPrefHeightNode; struct guiPrefHeightNode {guiPrefHeightNode *next; guiSize v;};
typedef struct guiFixedXNode guiFixedXNode; struct guiFixedXNode {guiFixedXNode *next; f32 v;};
typedef struct guiFixedYNode guiFixedYNode; struct guiFixedYNode {guiFixedYNode *next; f32 v;};
typedef struct guiFixedWidthNode guiFixedWidthNode; struct guiFixedWidthNode {guiFixedWidthNode *next; f32 v;};
typedef struct guiFixedHeightNode guiFixedHeightNode; struct guiFixedHeightNode {guiFixedHeightNode *next; f32 v;};
typedef struct guiBgColorNode guiBgColorNode; struct guiBgColorNode {guiBgColorNode *next; vec4 v;};


guiKey gui_key_zero(void);
guiKey gui_key_from_str(char *s);
b32 gui_key_match(guiKey a, guiKey b);

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

vec4 gui_top_bg_color(void);
vec4 gui_set_next_bg_color(vec4 v);
vec4 gui_push_bg_color(vec4 v);
vec4 gui_pop_bg_color(void);

// pushes fixed widths heights (TODO -- i should probably add all the lower level stack functions in future)
void gui_push_rect(rect r);
void gui_set_next_rect(rect r);
void gui_pop_rect(void);

typedef u32 guiSignalFlags;
enum {
	GUI_SIGNAL_FLAG_LMB_PRESSED  = (1<<0),
	GUI_SIGNAL_FLAG_MMB_PRESSED  = (1<<2),
	GUI_SIGNAL_FLAG_RMB_PRESSED  = (1<<3),
	GUI_SIGNAL_FLAG_LMB_RELEASED = (1<<4),
	GUI_SIGNAL_FLAG_MMB_RELEASED = (1<<5),
	GUI_SIGNAL_FLAG_RMB_RELEASED = (1<<6),
	GUI_SIGNAL_FLAG_MOUSE_HOVER  = (1<<7),
	// ...
};

typedef struct guiSignal guiSignal;
struct guiSignal {
	guiBox *box;
	vec2 mouse;
	vec2 drag_delta;
	guiSignalFlags flags;
};

guiSignal gui_get_signal_for_box(guiBox *box);

guiKey gui_get_hot_box_key();
guiKey gui_get_active_box_key(GUI_MOUSE_BUTTON b);


guiSignal gui_button(char *str);


//-----------------------------------------------------------------------------
// INTERFACE
//-----------------------------------------------------------------------------

typedef struct guiBoxHashSlot guiBoxHashSlot;
struct guiBoxHashSlot
{
  guiBox *hash_first;
  guiBox *hash_last;
};

typedef struct {
	Arena *arena;
	Arena *build_arenas[2];

	u32 box_table_size;
	guiBoxHashSlot *box_table;

	guiBox *first_free_box;

	guiBox *root;

	guiRenderCommandBuffer rcmd_buf;
	guiFontAtlas atlas;
	guiInputState gis;
	guiStyle style;
	u64 current_frame_index;

	vec2 win_dim;
	f32 dt;

	guiKey active_box_keys[GUI_MOUSE_BUTTON_COUNT];
	guiKey hot_box_key;

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
	struct { guiBgColorNode *top; vec4 bottom_val; guiBgColorNode *free; b32 auto_pop; } bg_color_stack;
} guiState;

guiStatus gui_input_push_event(guiInputEventNode e);
guiStatus gui_state_update(f32 dt);
guiState *gui_state_init();

void gui_set_ui_state(guiState *state);
guiState * gui_get_ui_state();
Arena *gui_get_build_arena();

vec2 gui_font_get_string_dim(guiFontAtlas *atlas, char* str);
f32 gui_font_get_string_y_to_add(guiFontAtlas *atlas, char* str);
guiStatus gui_draw_string_in_pos(char *str, vec2 pos, vec4 color);
guiStatus gui_draw_rect(rect r, vec4 color);
void gui_render_hierarchy(void);

#endif