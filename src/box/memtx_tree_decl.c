/*
 * Copyright 2010-2019, Tarantool AUTHORS, please see AUTHORS file.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the
 *    following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY <COPYRIGHT HOLDER> ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
 * <COPYRIGHT HOLDER> OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <stdbool.h>
#include <stdint.h>
#include "tuple_compare.h"
#include "memtx_tree.h"

/* {{{ Memtx tree of tuples class. ******************************/

/** Struct that is used as a key in BPS tree definition. */
struct memtx_tuple_tree_key_data {
	/** Sequence of msgpacked search fields. */
	const char *key;
	/** Number of msgpacked search fields. */
	uint32_t part_count;
};

#define MEMTX_TREE_NAME memtx_tuple_tree
#define memtx_tree_elem struct tuple *
#define memtx_tree_key struct memtx_tuple_tree_key_data
#define MEMTX_TREE_ELEM_CMP(elem_a_ptr, elem_b_ptr, key_def)			\
	tuple_compare(*elem_a_ptr, *elem_b_ptr, key_def)
#define MEMTX_TREE_ELEM_WITH_KEY_CMP(elem_ptr, key_ptr, key_def)		\
	tuple_compare_with_key(*elem_ptr, (key_ptr)->key,			\
			       (key_ptr)->part_count, key_def)
#define MEMTX_TREE_ELEM_SET(elem_ptr, tuple, key_def)				\
	({(void)key_def; *elem_ptr = tuple;})
#define MEMTX_TREE_KEY_SET(key_ptr, key_val, part_count_val, key_def)		\
	({(void)key_def; (key_ptr)->key = key_val;				\
	 (key_ptr)->part_count = part_count_val;})
#define MEMTX_TREE_ELEM_GET(elem_ptr) (*(elem_ptr))
#define MEMTX_TREE_KEY_GET(key_ptr, part_count_ptr)				\
	({*part_count_ptr = (key_ptr)->part_count; (key_ptr)->key;})

#include "memtx_tree_impl.h"

#undef memtx_tree_key
#undef memtx_tree_elem
#undef MEMTX_TREE_KEY_GET
#undef MEMTX_TREE_ELEM_GET
#undef MEMTX_TREE_KEY_SET
#undef MEMTX_TREE_ELEM_SET
#undef MEMTX_TREE_ELEM_WITH_KEY_CMP
#undef MEMTX_TREE_ELEM_CMP
#undef MEMTX_TREE_ELEM_EQUAL
#undef memtx_tree_key
#undef memtx_tree_elem
#undef MEMTX_TREE_NAME

/* }}} */

struct index *
memtx_tree_index_new(struct memtx_engine *memtx, struct index_def *def)
{
	return memtx_tuple_tree_index_new(memtx, def);
}
