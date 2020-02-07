// TODO:
// - debugging
// 		- add __LINE__, __FILE__ (and __func__, where possible)
// 		- keep `purged` ptrs for later review
// - where do I want to choose whether ptrs are freed when dec to 0?
// - add hash fn
// - add key array (delete by swapping in last key) to allow looping?
// 		- (this is for general hashmap, is it needed here?)
// - ensure you can't track NULL ptrs
// - allow relocatable heaps
//       mem: |   A   |  B  |     C    |
//       ptr:               ^
//       free B
//       mem: |   A   |-----|     C    |
//       ptr:               ^
//       shift C down to fill gap
//       mem: |   A   |     C    |------
//       ptr:               ^
//       make sure ptr(s) point to the right place (if > B, shift down by size of B)
//       mem: |   A   |     C    |------
//       ptr:         ^

#ifndef REFEREE_NOSTDLIB
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif//REFEREE_NOSTDLIB

#ifndef REFEREE_API
#define REFEREE_API static
#endif//REFEREE_API
#ifndef REFEREE_REALLOC
#define REFEREE_REALLOC(allocator, ptr, n, size) realloc(ptr, (n)*(size))
#endif//REFEREE_REALLOC
#ifndef REFEREE_FREE
#define REFEREE_FREE(allocator, ptr) free(ptr)
#endif//REFEREE_FREE

#if REFEREE_DEBUG // control whether callsite is recorded
#define REF_DBG(fn, ...) fn##_dbg(__VA_ARGS__, int line, char const *file, char const *func, char const *call)

#define ref_add(...)                ref_add_dbg(__VA_ARGS__,                __LINE__, __FILE__, __func__, "ref_add("#__VA_ARGS__")")
#define ref_add_n(...)              ref_add_n_dbg(__VA_ARGS__,              __LINE__, __FILE__, __func__, "ref_add_n("#__VA_ARGS__")")
#define ref_new(...)                ref_new_dbg(__VA_ARGS__,                __LINE__, __FILE__, __func__, "ref_new("#__VA_ARGS__")")
#define ref_new_n(...)              ref_new_n_dbg(__VA_ARGS__,              __LINE__, __FILE__, __func__, "ref_new_n("#__VA_ARGS__")")
#define ref_realloc(...)            ref_realloc_dbg(__VA_ARGS__,            __LINE__, __FILE__, __func__, "ref_realloc("#__VA_ARGS__")")
#define ref_realloc_n(...)          ref_realloc_n_dbg(__VA_ARGS__,          __LINE__, __FILE__, __func__, "ref_realloc_n("#__VA_ARGS__")")
#define ref_register_realloc_n(...) ref_register_realloc_n_dbg(__VA_ARGS__, __LINE__, __FILE__, __func__, "ref_register_realloc_n("#__VA_ARGS__")")

#define ref_add_n_(...)              ref_add_n_dbg(__VA_ARGS__,     line, file, func, call)
#define ref_new_n_(...)              ref_new_n_dbg(__VA_ARGS__,     line, file, func, call)
#define ref_realloc_n_(...)          ref_realloc_n_dbg(__VA_ARGS__, line, file, func, call)
#define ref_register_realloc_n_(...) ref_register_realloc_n_dbg(__VA_ARGS__, line, file, func, call)

// internal functions

#else //REFEREE_DEBUG
#define REF_DBG(fn, ...) fn(__VA_ARGS__)

#define ref_add_n_(...)              ref_add_n(__VA_ARGS__)
#define ref_new_n_(...)              ref_new_n(__VA_ARGS__)
#define ref_realloc_n_(...)          ref_realloc_n(__VA_ARGS__)
#define ref_register_realloc_n_(...) ref_register_realloc_n(__VA_ARGS__)
#endif//REFEREE_DEBUG

typedef struct Referee Referee;
typedef struct RefInfo RefInfo;

// suffix 'n' for breaking allocations to el_size/num_els
// suffix 'c' for referring to the refcount


// allocate some memory and start keeping track of references to it
// returns a pointer to the start of the allocated memory
REFEREE_API void *REF_DBG(ref_new,   Referee *ref, size_t alloc_size, size_t init_refs);
REFEREE_API void *REF_DBG(ref_new_n, Referee *ref, size_t el_n, size_t el_size, size_t init_refs);

// start refcounting an already existing ptr
// returns ptr
// if ptr is already being refcounted, this acts as ref_inc_c (with init_refs as count)
REFEREE_API void *REF_DBG(ref_add,   Referee *ref, void *ptr, size_t alloc_size, size_t init_refs);
REFEREE_API void *REF_DBG(ref_add_n, Referee *ref, void *ptr, size_t el_n, size_t el_size, size_t init_refs);
// stop tracking a ptr without deallocating it ("forget"?)
REFEREE_API void *ref_remove(Referee *ref, void *ptr);

// duplicate a given piece of memory that is already tracked by ref
// returns a pointer to the new memory block
REFEREE_API void *REF_DBG(ref_dup, Referee *ref, void *ptr, size_t init_refs);

// increment or decrement the count for a given reference
// returns ptr
REFEREE_API void *ref_inc  (Referee *ref, void *ptr);
REFEREE_API void *ref_inc_c(Referee *ref, void *ptr, size_t count);
REFEREE_API void *ref_dec  (Referee *ref, void *ptr);
REFEREE_API void *ref_dec_c(Referee *ref, void *ptr, size_t count);
// decrement the count, and free the memory if the new count hits 0
/* void *ref_dec_del(Referee *ref, void *ptr); */
/* void *ref_dec_c_del(Referee *ref, void *ptr, size_t count); */

// count of references to a particular ptr
REFEREE_API size_t ref_count(Referee *ref, void *ptr);

// frees and removes all pointers with a refcount of 0
// returns number removed
REFEREE_API size_t ref_purge(Referee *ref);

// TODO: work out how refs should work
// sort out naming convention with new above
// should there be some returned count of references?
// reallocate but maintain
REFEREE_API void *REF_DBG(ref_realloc,   Referee *ref, void *ptr, size_t new_size, size_t init_refs);
REFEREE_API void *REF_DBG(ref_realloc_n, Referee *ref, void *ptr, size_t el_n, size_t el_size, size_t init_refs);

// sets the size of an alloc
REFEREE_API size_t ref_resize(Referee *ref, void *ptr, size_t new_size);
// sets the element number of an alloc
REFEREE_API size_t ref_re_n(Referee *ref, void *ptr, size_t new_el_n);
// sets the count of an alloc
REFEREE_API size_t ref_recount(Referee *ref, void *ptr, size_t new_count);

// returns a pointer to the reference-count information on ptr.
// This can be read from or written to
// returns NULL if ptr is not being refcounted
REFEREE_API RefInfo *ref_info(Referee *ref, void *ptr);

// uses stdlib allocation (mainly for transition from normal malloc/free code)
/* void *ref_stdmalloc(size_t size); */
/* void *ref_stdcalloc(size_t size); */
/* void *ref_stdrealloc(void *ptr, size_t new_size); */
/* void *ref_stdmallocn(size_t el_size, size_t n); */
/* void *ref_stdcallocn(size_t el_size, size_t n); */
/* void *ref_stdreallocn(void *ptr, size_t el_size, size_t new_n); */
/* void  ref_stdfree(void *ptr); */

// force a free, regardless of number of references
REFEREE_API void *ref_free(Referee *ref, void *ptr); // TODO: size_t *refcount_out);?
// force a free, but only if number of refs is below the given max (should this be < or <=?)
/* REFEREE_API int ref_free_c(void *ptr, size_t max_refs); */

// returns total memory tracked by referee (in bytes)
REFEREE_API size_t ref_total_size(Referee *ref);
REFEREE_API void ref_dump_mem_usage(FILE *out, Referee *ref, int should_destructively_sort);

#if defined(REFEREE_IMPLEMENTATION) || defined(REFEREE_TEST)

#define REFEREE_INVALID (~((size_t)0))

struct RefInfo {
	size_t refcount; // is size_t excessive?
	size_t el_n;
	size_t el_size;

#if REFEREE_DEBUG
	size_t line;
	char const *file;
	char const *func;
	char const *call;
#endif//REFEREE_DEBUG
};


#define MAP_INVALID_VAL { REFEREE_INVALID, REFEREE_INVALID, REFEREE_INVALID }
#define MAP_TYPES (RefereePtrInfoMap, ref__map, void *, RefInfo)
#include "hash.h"

#define Referee_Test_Len 8
struct Referee {
	// Ordered so that this can be created with constants in any scope (including global)
	// e.g. Referee ref = { my_allocator, my_alloc, my_free };
	void *allocator;
	void *(*realloc)(void *allocator, void *ptr, size_t el_n, size_t el_size); // must allocate if given NULL ptr as per realloc
	void  (*free)   (void *allocator, void *ptr);

	RefereePtrInfoMap ptr_infos;
};

REFEREE_API RefInfo *
ref_info(Referee *ref, void *ptr)
{
    return ((ref && ptr)
            ? ref__map_ptr(&ref->ptr_infos, ptr)
            : 0);
}

REFEREE_API size_t
ref_count(Referee *ref, void *ptr)
{   
    RefInfo *info = ref_info(ref, ptr);
    return (info
            ? info->refcount
            : 0);
}


REFEREE_API void *
REF_DBG(ref_add_n, Referee *ref, void *ptr, size_t el_n, size_t el_size, size_t init_refs)
{
	if (! ref || ! ptr) { return 0; }

	RefInfo info  = {0};
	info.refcount = init_refs;
	info.el_n     = el_n;
	info.el_size  = el_size;

#if REFEREE_DEBUG
    info.line = line;
    info.file = file;
    info.func = func;
    info.call = call;
#endif//REFEREE_DEBUG

	int insert_result = ref__map_insert(&ref->ptr_infos, ptr, info);
	switch (insert_result)
	{
		default:          return 0;
		case MAP_absent:  return ptr;
		case MAP_present: return ref_inc_c(ref, ptr, init_refs);
	}
}

REFEREE_API inline void *
REF_DBG(ref_add, Referee *ref, void *ptr, size_t alloc_size, size_t init_refs)
{   return ref_add_n_(ref, ptr, 1, alloc_size, init_refs);   }

REFEREE_API inline void *
ref_remove(Referee *ref, void *ptr)
{   ref__map_remove(&ref->ptr_infos, ptr); return ptr;   }


REFEREE_API void *
ref_default_realloc(void *allocator, void *ptr, size_t el_n, size_t el_size)
{
    (void)allocator;
    return REFEREE_REALLOC(allocator, ptr, el_n, el_size);
}

REFEREE_API void
ref_default_free(void *allocator, void *ptr)
{
    (void)allocator;
    REFEREE_FREE(allocator, ptr);
}

REFEREE_API void
ref_set_default_allocator(Referee *ref)
{
    ref->realloc = ref_default_realloc;
    ref->free    = ref_default_free;
}

REFEREE_API inline void *
REF_DBG(ref_new_n, Referee *ref, size_t el_n, size_t el_size, size_t init_refs)
{
	if (! ref->realloc || ! ref->free) { ref_set_default_allocator(ref); }
    /* __itt_heap_allocate_begin(0, el_n * el_size, 0); */
	void *ptr = ref->realloc(ref->allocator, 0, el_n, el_size);
    /* __itt_heap_allocate_end(0, ptr, el_n * el_size, 0); */

    return (ptr
            ? ref_add_n_(ref, ptr, el_n, el_size, init_refs)
            : ptr);
}

// i.e. 1 block of the full size
REFEREE_API inline void *
REF_DBG(ref_new, Referee *ref, size_t size, size_t init_refs)
{   return ref_new_n_(ref, 1, size, init_refs);   }


// TODO(api): this is to realloc as add is to new - make naming scheme consistent
// doesn't perform the reallocation, just tracks that it has happened
REFEREE_API inline void *
REF_DBG(ref_register_realloc_n, Referee *ref, void *ptr, void *ptr_p, size_t el_n, size_t el_size, size_t init_refs)
{
    REFEREE_register_realloc(ptr, ptr_p, el_n, el_size, line, file, func, call);
    if (ptr)
    {
        // TODO: incorporate init_refs for existing ptrs?
        RefInfo info = ref__map_remove(&ref->ptr_infos, ptr_p);
        ref_add_n_(ref, ptr, el_n, el_size, (~ info.refcount
                                             ? info.refcount
                                             : init_refs));
    }
    return ptr;
}

REFEREE_API inline void *
REF_DBG(ref_realloc_n, Referee *ref, void *ptr, size_t el_n, size_t el_size, size_t init_refs)
{
	if (! ref->realloc || ! ref->free) { ref_set_default_allocator(ref); }
    /* __itt_heap_reallocate_begin(0, ptr, el_n * el_size, 0); */
    void *result = ref->realloc   (ref->allocator, ptr, el_n, el_size);
    /* __itt_heap_reallocate_end(0, ptr, result, el_n * el_size, 0); */

    ref_register_realloc_n_(ref, result, ptr, el_n, el_size, init_refs);
    return result;
}

REFEREE_API inline void *
REF_DBG(ref_realloc, Referee *ref, void *ptr, size_t size, size_t init_refs)
{   return ref_realloc_n_(ref, ptr, 1, size, init_refs);   }

REFEREE_API void *
REF_DBG(ref_dup, Referee *ref, void *ptr, size_t init_refs)
{
	void    *result = 0;
	RefInfo *info   = ref_info(ref, ptr);

	if (info)
	{
		result = ref_new_n_(ref, info->el_n, info->el_size, init_refs);
		memcpy(result, ptr, info->el_n * info->el_size);
	}
	return result;
}


REFEREE_API inline void *
ref_inc_c(Referee *ref, void *ptr, size_t c)
{
	RefInfo *info = ref_info(ref, ptr);
	if (info) {   info->refcount += c; return ptr;   }
	else return 0;
}
REFEREE_API inline void *
ref_inc(Referee *ref, void *ptr)
{
	RefInfo *info = ref_info(ref, ptr);
	if (info) {   ++info->refcount; return ptr;   }
	else return 0;
}

REFEREE_API void *
ref_dec_c(Referee *ref, void *ptr, size_t c)
{
	RefInfo *info = ref_info(ref, ptr);
	if (info) {
		if (info->refcount >= c) {   info->refcount -= c;   }
		else                     {   info->refcount  = 0;   } // TODO (api): is this the right behaviour? should it cause error?
		return ptr;
	}
	else return 0;
}
REFEREE_API void *
ref_dec(Referee *ref, void *ptr)
{
	RefInfo *info = ref_info(ref, ptr);
	if (info) {
		if (info->refcount > 0) {   --info->refcount;   }
		return ptr;
	}
	else return 0;
}

REFEREE_API void *
ref_free(Referee *ref, void *ptr)
{
    RefInfo info = ref__map_remove(&ref->ptr_infos, ptr);
    if (~ info.refcount)
    {
        assert(ref->free && "this should be set on initial allocation");
        /* __itt_heap_free_begin(0, ptr); */
        ref->free(ref->allocator, ptr);
        /* __itt_heap_free_end(0, ptr); */
    }
    return 0;
}

// clear all that have a refcount of 0
REFEREE_API size_t 
ref_purge(Referee *ref)
{
	// TODO (opt): this should be sped up by keeping an array of the zero counts
	size_t deleted_n = 0;
	if(! ref) { return REFEREE_INVALID; }

	for(size_t i = 0,
			   n = ref->ptr_infos.n;
		i < n; ++i)
	{
		void *ptr = ref->ptr_infos.keys[i];
		RefInfo *info = ref_info(ref, ptr);
		if (info && info->refcount == 0)
		{
			++deleted_n;
			ref__map_remove(&ref->ptr_infos, ptr);

            if (ref->free) { ref->free(ref->allocator, ptr); }
            else           { REFEREE_FREE(ref->allocator, ptr); }
		}
	}
	return deleted_n;
}

REFEREE_API size_t
ref_total_size(Referee *ref)
{
	size_t result = 0;
	RefInfo const *vals = ref->ptr_infos.vals;
	for(size_t i = 0,
			   n = ref->ptr_infos.n;
		i < n; ++i)
    {   result += vals[i].el_n * vals[i].el_size;   }
    return result;
}


static inline int
RefInfo_cmp_size(const void *a, const void *b)
{
    RefInfo ref_a = *(const RefInfo *)a,
            ref_b = *(const RefInfo *)b;
    size_t A = ref_a.el_n * ref_a.el_size,
           B = ref_b.el_n * ref_b.el_size;
    int result = (B < A) - (A < B);
    return result;
}

REFEREE_API void
ref_dump_mem_usage(FILE *out, Referee *ref, int should_destructively_sort)
{
    fprintf(out, "Total memory tracked: %zu\n", ref_total_size(ref));

    if (should_destructively_sort)
    {   qsort(ref->ptr_infos.vals, ref->ptr_infos.n, sizeof(ref->ptr_infos.vals[0]), RefInfo_cmp_size);   }

    for(size_t i = 0,
        n = ref->ptr_infos.n;
        i < n; ++i)
    {
        Addr    ptr  = ref->ptr_infos.keys[i];
        RefInfo info = ref->ptr_infos.vals[i];
        fprintf(out, //"0x%08llx:"
            "%zu bytes. %s - %s(%zu) - \t %s\n",
            //(unsigned long long)ptr,
            info.el_n * info.el_size,
            info.func, info.file, info.line, info.call);
    }
    fputc('\n', out);
}

#endif
