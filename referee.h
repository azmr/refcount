#ifndef REFEREE_NOSTDLIB
#include <stdlib.h>
#endif//REFEREE_NOSTDLIB

// TODO:
// - debugging
// 		- add __LINE__, __FILE__ (and __func__, where possible)
// 		- keep `purged` ptrs for later review
// - where do I want to choose whether ptrs are freed when dec to 0?
// - add hash fn
// - add key array (delete by swapping in last key) to allow looping?
// 		- (this is for general hashmap, is it needed here?)
// - ensure you can't track NULL ptrs

struct Referee;
typedef struct Referee Referee;

struct RefereeInfo;
typedef struct RefereeInfo RefereeInfo;

// suffix 'n' for breaking allocations to el_size/num_els
// suffix 'c' for referring to the refcount


// allocate some memory and start keeping track of references to it
// returns a pointer to the start of the allocated memory
void *ref_new  (Referee *ref, size_t alloc_size, size_t init_refs);
void *ref_new_n(Referee *ref, size_t el_size, size_t el_n, size_t init_refs);

// start refcounting an already existing ptr
// returns ptr
// if ptr is already being refcounted, this acts as ref_inc_c (with init_refs as count)
void *ref_add  (Referee *ref, void *ptr, size_t alloc_size, size_t init_refs);
void *ref_add_n(Referee *ref, void *ptr, size_t el_size, size_t el_n, size_t init_refs);
// stop tracking a ptr
void *ref_remove(Referee *ref, void *ptr);

// duplicate a given piece of memory that is already tracked by ref
// returns a pointer to the new memory block
void *ref_dup(Referee *ref, void *ptr);

// increment or decrement the count for a given reference
// returns ptr
void *ref_inc  (Referee *ref, void *ptr);
void *ref_inc_c(Referee *ref, void *ptr, size_t count);
void *ref_dec  (Referee *ref, void *ptr);
void *ref_dec_c(Referee *ref, void *ptr, size_t count);
// decrement the count, and free the memory if the new count hits 0
/* void *ref_dec_del(Referee *ref, void *ptr); */
/* void *ref_dec_c_del(Referee *ref, void *ptr, size_t count); */

// count of references to a particular ptr
size_t ref_count(Referee *ref, void *ptr);

// frees and removes all pointers with a refcount of 0
// returns number removed
size_t ref_purge(Referee *ref);

// sets the size of an alloc
size_t ref_resize(Referee *ref, void *ptr, size_t new_size);
// sets the count of an alloc
size_t ref_recount(Referee *ref, void *ptr, size_t new_count);

// returns a pointer to the reference-count information on ptr.
// This can be read from or written to
// returns NULL if ptr is not being refcounted
RefereeInfo *ref_info(Referee *ref, void *ptr);

// uses stdlib allocation (mainly for transition from normal malloc/free code)
/* void *ref_stdmalloc(size_t size); */
/* void *ref_stdcalloc(size_t size); */
/* void *ref_stdrealloc(void *ptr, size_t new_size); */
/* void *ref_stdmallocn(size_t el_size, size_t n); */
/* void *ref_stdcallocn(size_t el_size, size_t n); */
/* void *ref_stdreallocn(void *ptr, size_t el_size, size_t new_n); */
/* void  ref_stdfree(void *ptr); */

// forefe a free, regardless of number of references
int ref_free(void *ptr);
// forefe a free, but only if number of refs is below the given max
int ref_free_c(void *ptr, size_t max_refs);

#if defined(REFCOUNT_IMPLEMENTATION) || defined(REFCOUNT_TEST)

#define REFEREE_INVALID (~((size_t)0))

struct RefereeInfo {
	size_t refcount; // is size_t excessive?
	size_t el_n;
	size_t el_size;

#ifdef REFEREE_DEBUG
#ifdef  REFEREE_DEBUG_FUNC
	char *func;
#endif//REFEREE_DEBUG_FUNC
	char *file;
	size_t line_n;
#endif//REFEREE_DEBUG
};


#define MAP_INVALID_VAL { REFEREE_INVALID, REFEREE_INVALID, REFEREE_INVALID }
#define MAP_TYPES(T) T(RefereePtrInfoMap, ref__map, void *, RefereeInfo)
#include "hash.h"

#define Referee_Test_Len 8
struct Referee {
	RefereePtrInfoMap ptr_infos;

	// functions for custom allocators?
};

RefereeInfo *ref_info(Referee *ref, void *ptr) {
	return (ref && ptr)
		? ref__map_ptr(&ref->ptr_infos, ptr)
		: 0;
}

size_t ref_count(Referee *ref, void *ptr) {   
	RefereeInfo *info = ref_info(ref, ptr);
	return (info)
		? info->refcount
		: 0;
}


void *ref_add_n(Referee *ref, void *ptr, size_t el_size, size_t el_n, size_t init_refs) {
	if (! ref || ! ptr) { return 0; }

	RefereeInfo info = {0};
	info.refcount = init_refs;
	info.el_n = el_n;
	info.el_size = el_size;

	int insert_result = ref__map_insert(&ref->ptr_infos, ptr, info);
	switch(insert_result) {
		default: return 0;
		case 0:  return ptr;
		case 1:  return ref_inc_c(ref, ptr, init_refs);
	}
}

void *ref_add(Referee *ref, void *ptr, size_t alloc_size, size_t init_refs)
{   return ref_add_n(ref, ptr, alloc_size, 1, init_refs);   }

void *ref_remove(Referee *ref, void *ptr)
{   ref__map_remove(&ref->ptr_infos, ptr); return ptr;   }


// TODO (api): custom allocators
void *ref_new_n(Referee *ref, size_t el_size, size_t el_n, size_t init_refs) {
	void *ptr = malloc(el_size * el_n);
	return (ptr)
		? ref_add_n(ref, ptr, el_size, el_n, init_refs)
		: ptr;
}

// i.e. 1 block of the full size
void *ref_new(Referee *ref, size_t size, size_t init_refs)
{   return ref_new_n(ref, size, 1, init_refs);   }

void *ref_dup(Referee *ref, void *ptr, size_t init_refs) {
	// TODO (api): use referee-specific functions if given
	void *ref_dup(Referee *ref, void *ptr);
	RefereeInfo *info = ref_info(ref, ptr);
	if (info)
	{
		result = ref_new_n(ref, info->el_size, info->el_n, init_refs);
		memcpy(result, ptr, bytes_n);
	}
	return result;
}


void *ref_inc_c(Referee *ref, void *ptr, size_t c) {
	RefereeInfo *info = ref_info(ref, ptr);
	if (info) {   info->refcount += c; return ptr;   }
	else return 0;
}
void *ref_inc(Referee *ref, void *ptr) {
	RefereeInfo *info = ref_info(ref, ptr);
	if (info) {   ++info->refcount; return ptr;   }
	else return 0;
}

void *ref_dec_c(Referee *ref, void *ptr, size_t c) {
	RefereeInfo *info = ref_info(ref, ptr);
	if (info) {
		if (info->refcount >= c) {   info->refcount -= c;   }
		else                     {   info->refcount  = 0;   } // TODO (api): is this the right behaviour? should it cause error?
		return ptr;
	}
	else return 0;
}
void *ref_dec(Referee *ref, void *ptr) {
	RefereeInfo *info = ref_info(ref, ptr);
	if (info) {
		if (info->refcount > 0) {   --info->refcount;   }
		return ptr;
	}
	else return 0;
}

// clear all that have a refcount of 0
size_t ref_purge(Referee *ref)
{
	// TODO (opt): this should be sped up by keeping an array of the zero counts
	size_t deleted_n = 0;
	if(! ref) { return REFEREE_INVALID; }

	for(size_t i   = 0,
			keys_n = ref->ptr_infos.keys_n;
		i < keys_n; ++i)
	{
		void *ptr = ref->ptr_infos.keys[i];
		RefereeInfo *info = ref_info(ref, ptr);
		if (info && info->refcount == 0)
		{
			++deleted_n;
			ref__map_remove(&ref->ptr_infos, ptr);
			// TODO (api): custom free fn
			free(ptr);
		}
	}
	return deleted_n;
}

#endif
