#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __APPLE__
#include <malloc/malloc.h>
#elif defined(__linux__)
#include <malloc.h>
#endif

#ifndef GLT_ALIGNOF
#define GLT_ALIGNOF _Alignof
#endif

typedef struct
{
	size_t size;
	size_t alignment;
	size_t len;
	size_t cap;
} vtVecInfo;

#define vtVec(T) T*

#define _vtGetVecInfoPtr(vec) ((vtVecInfo*) (((void*) vec) - sizeof(vtVecInfo)))

#define vtInit(T, n) ((T*) _vtInitVec(sizeof(T), GLT_ALIGNOF(T), n))
#define vtFree(vec) (_vtFreeVec(_vtGetVecInfoPtr(vec)))

#define vtLen(vec) ((_vtGetVecInfoPtr(vec))->len)
#define vtCap(vec) ((_vtGetVecInfoPtr(vec))->cap)
#define vtSubvec(vec, start, end) _vtSubVec(vec, start, end)

#define vtReserve(vec, new_size)                                                                             \
	do                                                                                                   \
	{                                                                                                    \
		if (vtVecCap(vec) < new_size)                                                                \
		{                                                                                            \
			vtVecInfo* info_ptr = _vtGetVecInfoPtr(vec);                                         \
			info_ptr = realloc(info_ptr, sizeof(vtVecInfo) + new_size * info_ptr->size);         \
			info_ptr->cap = new_size;                                                            \
			vec = ((void*) info_ptr) + sizeof(vtVecInfo);                                        \
		}                                                                                            \
	}                                                                                                    \
	while (false)

#define vtPushArr(vec, arr, n)                                                                               \
	do                                                                                                   \
	{                                                                                                    \
		for (int i = 0; i < n; i++)                                                                  \
			vtPush(vec, arr[i]);                                                                 \
	}                                                                                                    \
	while (false)

#define vtPush(vec, n)                                                                                       \
	({                                                                                                   \
		vtVecInfo* info_ptr = _vtGetVecInfoPtr(*vec);                                                \
		info_ptr->len++;                                                                             \
		info_ptr = vtVecShouldRealloc(info_ptr);                                                     \
		*vec = (void*) info_ptr + sizeof(vtVecInfo);                                                 \
		(*vec)[info_ptr->len - 1] = n;                                                               \
		info_ptr->len - 1;                                                                           \
	})

#define vtPop(vec)                                                                                           \
	({                                                                                                   \
		vtVecInfo* info_ptr = _vtGetVecInfoPtr(vec);                                                 \
		info_ptr->len--;                                                                             \
		vec[info_ptr->len];                                                                          \
	})

#define vtPopn(vec, n)                                                                                       \
	{                                                                                                    \
		vtVecInfo* info_ptr = _vtGetVecInfoPtr(vec);                                                 \
		info_ptr->len -= n;                                                                          \
	}

#define vtRemoveElem(vec, index) _vtVecRemoveElems(_vtGetVecInfoPtr(vec), index, 1)
#define vtVecRemoveElems(vec, index, num_elems) _vtVecRemoveElems(_vtGetVecInfoPtr(vec), index, num_elems)

#define vtForeach(vec, i, body)                                                                              \
	do                                                                                                   \
	{                                                                                                    \
		for (int _vtForeachIndex = 0; _vtForeachIndex < vtLen(vec); _vtForeachIndex++)               \
		{                                                                                            \
			typeof(*vec) i = vec[_vtForeachIndex];                                               \
			body;                                                                                \
		}                                                                                            \
	}                                                                                                    \
	while (false)

#define vtTransform(vec, i, body)                                                                            \
	do                                                                                                   \
	{                                                                                                    \
		for (int _vtForeachIndex = 0; _vtForeachIndex < vtLen(vec); _vtForeachIndex++)               \
		{                                                                                            \
			typeof(*vec) i = vec[_vtForeachIndex];                                               \
			vec[_vtForeachIndex] = (body);                                                       \
		}                                                                                            \
	}                                                                                                    \
	while (false)

static inline void* _vtInitVec(size_t size, size_t alignment, int num)
{
	void* alloc_ptr = malloc(sizeof(vtVecInfo) + size * num);
	*(vtVecInfo*) alloc_ptr = (vtVecInfo){size, alignment, num, num};
	return alloc_ptr + sizeof(vtVecInfo);
}

static inline void* _vtSubVec(void* vec, size_t start, size_t num_elems)
{
	vtVecInfo* vec_info = _vtGetVecInfoPtr(vec);
	void* ret = _vtInitVec(vec_info->size, vec_info->alignment, num_elems);
	memcpy(ret, vec + vec_info->size * start, vec_info->size * (num_elems));
	return ret;
}

static inline void _vtFreeVec(vtVecInfo* vec)
{
	free(vec);
}

static inline void _vtVecRemoveElems(vtVecInfo* info_ptr, size_t index, size_t num)
{
	void* vec_start = ((void*) info_ptr) + sizeof(vtVecInfo);
	void* start_of_elems_to_remove = vec_start + info_ptr->size * index;
	void* end_of_elems_to_remove = start_of_elems_to_remove + info_ptr->size * num;

	memmove(start_of_elems_to_remove, end_of_elems_to_remove,
		(info_ptr->len - index - num) * info_ptr->size);
}

static inline void* aligned_realloc(void* ptr, size_t align, size_t size)
{
	if ((size == 0) || (align <= GLT_ALIGNOF(void*)))
	{
		return realloc(ptr, size);
	}

	size_t new_size = (size + (align - 1)) & (~(align - 1));
	void* new_ptr = aligned_alloc(align, new_size);
	if (new_ptr != NULL)
	{
		size_t old_usable_size = new_size;
#ifdef __APPLE__
		old_usable_size = malloc_size(ptr);
#elif defined(__linux__)
		old_usable_size = malloc_usable_size(ptr);
#endif
		size_t copy_size = new_size < old_usable_size ? new_size : old_usable_size;
		if (ptr != NULL)
		{
			memcpy(new_ptr, ptr, copy_size);
			free(ptr);
		}
	}

	return new_ptr;
}

static inline vtVecInfo* vtVecShouldRealloc(vtVecInfo* vec)
{
	if (vec->len >= vec->cap)
	{

		vec->cap = (vec->cap * 3 / 2) + 1;
		vec = aligned_realloc(vec, vec->alignment, vec->cap * vec->size + sizeof(vtVecInfo));
	}

	return vec;
}
