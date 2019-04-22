#ifndef RC_NOSTDLIB
#include <stdlib.h>
#endif//RC_NOSTDLIB

// TODO:
// - debugging
// 		- add __LINE__, __FILE__ (and __func__, where possible)
// 		- keep `purged` ptrs for later review
// - where do I want to choose whether ptrs are freed when dec to 0?
// - add hash fn
// - add key array (delete by swapping in last key) to allow looping?
// 		- (this is for general hashmap, is it needed here?)
// - ensure you can't track NULL ptrs

struct Rc;
typedef struct Rc Rc;

struct RcInfo;
typedef struct RcInfo RcInfo;

// suffix 'n' for breaking allocations to el_size/num_els
// suffix 'c' for referring to the refcount


// allocate some memory and start keeping track of references to it
// returns a pointer to the start of the allocated memory
void *rc_new  (Rc *rc, size_t alloc_size, size_t init_refs);
void *rc_new_n(Rc *rc, size_t el_size, size_t el_n, size_t init_refs);

// start refcounting an already existing ptr
// returns ptr
// if ptr is already being refcounted, this acts as rc_inc_c (with init_refs as count)
void *rc_add  (Rc *rc, void *ptr, size_t alloc_size, size_t init_refs);
void *rc_add_n(Rc *rc, void *ptr, size_t el_size, size_t el_n, size_t init_refs);
// stop tracking a ptr
void *rc_remove(Rc *rc, void *ptr);

// increment or decrement the count for a given reference
// returns ptr
void *rc_inc  (Rc *rc, void *ptr);
void *rc_inc_c(Rc *rc, void *ptr, size_t count);
void *rc_dec  (Rc *rc, void *ptr);
void *rc_dec_c(Rc *rc, void *ptr, size_t count);
// decrement the count, and free the memory if the new count hits 0
/* void *rc_dec_del(Rc *rc, void *ptr); */
/* void *rc_dec_c_del(Rc *rc, void *ptr, size_t count); */

// duplicate a given piece of memory that is already tracked by rc
// returns a pointer to the new memory block
void *rc_dup(Rc *rc, void *ptr);

// count of references to a particular ptr
size_t rc_count(Rc *rc, void *ptr);

// frees and removes all pointers with a refcount of 0
// returns number removed
size_t rc_purge(Rc *rc);

// sets the size of an alloc
size_t rc_resize(Rc *rc, void *ptr, size_t new_size);
// sets the count of an alloc
size_t rc_recount(Rc *rc, void *ptr, size_t new_count);

// returns a pointer to the reference-count information on ptr.
// This can be read from or written to
// returns NULL if ptr is not being refcounted
RcInfo *rc_info(Rc *rc, void *ptr);

// uses stdlib allocation (mainly for transition from normal malloc/free code)
/* void *rc_stdmalloc(size_t size); */
/* void *rc_stdcalloc(size_t size); */
/* void *rc_stdrealloc(void *ptr, size_t new_size); */
/* void *rc_stdmallocn(size_t el_size, size_t n); */
/* void *rc_stdcallocn(size_t el_size, size_t n); */
/* void *rc_stdreallocn(void *ptr, size_t el_size, size_t new_n); */
/* void  rc_stdfree(void *ptr); */

// force a free, regardless of number of references
int rc_free(void *ptr);
// force a free, but only if number of refs is below the given max
int rc_free_c(void *ptr, size_t max_refs);

#if defined(REFCOUNT_IMPLEMENTATION) || defined(REFCOUNT_TEST)

#define RC_INVALID (~((size_t)0))

struct RcInfo {
	size_t refcount; // is size_t excessive?
	size_t el_n;
	size_t el_size;

#ifdef RC_DEBUG
#ifdef  RC_DEBUG_FUNC
	char *func;
#endif//RC_DEBUG_FUNC
	char *file;
	size_t line_n;
#endif//RC_DEBUG
};


#define MAP_INVALID_VAL { RC_INVALID, RC_INVALID, RC_INVALID }
#define MAP_TYPES(T) T(RcPtrInfoMap, rc__map, void *, RcInfo)
#include "hash.h"

#define Rc_Test_Len 8
struct Rc {
	RcPtrInfoMap ptr_infos;

	// functions for custom allocators?
};

RcInfo *rc_info(Rc *rc, void *ptr) {
	return (rc && ptr)
		? rc__map_ptr(&rc->ptr_infos, ptr)
		: 0;
}

size_t rc_count(Rc *rc, void *ptr) {   
	RcInfo *info = rc_info(rc, ptr);
	return (info)
		? info->refcount
		: 0;
}


void *rc_add_n(Rc *rc, void *ptr, size_t el_size, size_t el_n, size_t init_refs) {
	if (! rc || ! ptr) { return 0; }

	RcInfo info = {0};
	info.refcount = init_refs;
	info.el_n = el_n;
	info.el_size = el_size;

	int insert_result = rc__map_insert(&rc->ptr_infos, ptr, info);
	switch(insert_result) {
		default: return 0;
		case 0:  return ptr;
		case 1:  return rc_inc_c(rc, ptr, init_refs);
	}
}

void *rc_add(Rc *rc, void *ptr, size_t alloc_size, size_t init_refs)
{   return rc_add_n(rc, ptr, alloc_size, 1, init_refs);   }

void *rc_remove(Rc *rc, void *ptr)
{   rc__map_remove(&rc->ptr_infos, ptr); return ptr;   }


// TODO (api): custom allocators
void *rc_new_n(Rc *rc, size_t el_size, size_t el_n, size_t init_refs) {
	void *ptr = malloc(el_size * el_n);
	return (ptr)
		? rc_add_n(rc, ptr, el_size, el_n, init_refs)
		: ptr;
}

// i.e. 1 block of the full size
void *rc_new(Rc *rc, size_t size, size_t init_refs)
{   return rc_new_n(rc, size, 1, init_refs);   }


void *rc_inc_c(Rc *rc, void *ptr, size_t c) {
	RcInfo *info = rc_info(rc, ptr);
	if (info) {   info->refcount += c; return ptr;   }
	else return 0;
}
void *rc_inc(Rc *rc, void *ptr) {
	RcInfo *info = rc_info(rc, ptr);
	if (info) {   ++info->refcount; return ptr;   }
	else return 0;
}

void *rc_dec_c(Rc *rc, void *ptr, size_t c) {
	RcInfo *info = rc_info(rc, ptr);
	if (info) {
		if (info->refcount >= c) {   info->refcount -= c;   }
		else                     {   info->refcount  = 0;   } // TODO (api): is this the right behaviour? should it cause error?
		return ptr;
	}
	else return 0;
}
void *rc_dec(Rc *rc, void *ptr) {
	RcInfo *info = rc_info(rc, ptr);
	if (info) {
		if (info->refcount > 0) {   --info->refcount;   }
		return ptr;
	}
	else return 0;
}

// clear all that have a refcount of 0
size_t rc_purge(Rc *rc)
{
	// TODO (opt): this should be sped up by keeping an array of the zero counts
	size_t deleted_n = 0;
	if(! rc) { return RC_INVALID; }

	for(size_t i   = 0,
			keys_n = rc->ptr_infos.keys_n;
		i < keys_n; ++i)
	{
		void *ptr = rc->ptr_infos.keys[i];
		RcInfo *info = rc_info(rc, ptr);
		if (info && info->refcount == 0)
		{
			++deleted_n;
			rc__map_remove(&rc->ptr_infos, ptr);
			// TODO (api): custom free fn
			free(ptr);
		}
	}
	return deleted_n;
}

#endif
