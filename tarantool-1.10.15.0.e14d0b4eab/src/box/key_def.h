#ifndef TARANTOOL_BOX_KEY_DEF_H_INCLUDED
#define TARANTOOL_BOX_KEY_DEF_H_INCLUDED
/*
 * Copyright 2010-2016, Tarantool AUTHORS, please see AUTHORS file.
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
#include "trivia/util.h"
#include "error.h"
#include "diag.h"
#include <msgpuck.h>
#include <limits.h>
#include "field_def.h"
#include "coll_id.h"

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

/* MsgPack type names */
extern const char *mp_type_strs[];

struct key_part_def {
	/** Tuple field index for this part. */
	uint32_t fieldno;
	/** Type of the tuple field. */
	enum field_type type;
	/** Collation ID for string comparison. */
	uint32_t coll_id;
	/** True if a key part can store NULLs. */
	bool is_nullable;
};

extern const struct key_part_def key_part_def_default;

/**
 * Set key_part_def.coll_id to COLL_NONE if
 * the field does not have a collation.
 */
#define COLL_NONE UINT32_MAX

/** Descriptor of a single part in a multipart key. */
struct key_part {
	/** Tuple field index for this part */
	uint32_t fieldno;
	/** Type of the tuple field */
	enum field_type type;
	/** Collation ID for string comparison. */
	uint32_t coll_id;
	/** Collation definition for string comparison */
	struct coll *coll;
	/** True if a part can store NULLs. */
	bool is_nullable;
};

struct key_def;
struct tuple;

/** @copydoc tuple_compare_with_key() */
typedef int (*tuple_compare_with_key_t)(const struct tuple *tuple_a,
					const char *key,
					uint32_t part_count,
					struct key_def *key_def);
/** @copydoc tuple_compare() */
typedef int (*tuple_compare_t)(const struct tuple *tuple_a,
			       const struct tuple *tuple_b,
			       struct key_def *key_def);
/** @copydoc tuple_extract_key() */
typedef char *(*tuple_extract_key_t)(const struct tuple *tuple,
				     struct key_def *key_def,
				     uint32_t *key_size);
/** @copydoc tuple_extract_key_raw() */
typedef char *(*tuple_extract_key_raw_t)(const char *data,
					 const char *data_end,
					 struct key_def *key_def,
					 uint32_t *key_size);
/** @copydoc tuple_hash() */
typedef uint32_t (*tuple_hash_t)(const struct tuple *tuple,
				 struct key_def *key_def);
/** @copydoc key_hash() */
typedef uint32_t (*key_hash_t)(const char *key,
				struct key_def *key_def);

/* Definition of a multipart key. */
struct key_def {
	/** @see tuple_compare() */
	tuple_compare_t tuple_compare;
	/** @see tuple_compare_with_key() */
	tuple_compare_with_key_t tuple_compare_with_key;
	/** @see tuple_extract_key() */
	tuple_extract_key_t tuple_extract_key;
	/** @see tuple_extract_key_raw() */
	tuple_extract_key_raw_t tuple_extract_key_raw;
	/** @see tuple_hash() */
	tuple_hash_t tuple_hash;
	/** @see key_hash() */
	key_hash_t key_hash;
	/**
	 * Minimal part count which always is unique. For example,
	 * if a secondary index is unique, then
	 * unique_part_count == secondary index part count. But if
	 * a secondary index is not unique, then
	 * unique_part_count == part count of a merged key_def.
	 */
	uint32_t unique_part_count;
	/** True, if at least one part can store NULL. */
	bool is_nullable;
	/**
	 * True, if some key parts can be absent in a tuple. These
	 * fields assumed to be MP_NIL.
	 */
	bool has_optional_parts;
	/** Key fields mask. @sa column_mask.h for details. */
	uint64_t column_mask;
	/** The size of the 'parts' array. */
	uint32_t part_count;
	/** Description of parts of a multipart index. */
	struct key_part parts[];
};

/**
 * Duplicate key_def.
 * @param src Original key_def.
 *
 * @retval not NULL Duplicate of src.
 * @retval     NULL Memory error.
 */
struct key_def *
key_def_dup(const struct key_def *src);

/**
 * Swap content of two key definitions in memory.
 * The two key definitions must have the same size.
 */
void
key_def_swap(struct key_def *old_def, struct key_def *new_def);

/**
 * Delete @a key_def.
 * @param def Key_def to delete.
 */
void
key_def_delete(struct key_def *def);

typedef struct tuple box_tuple_t;

/* {{{ Module API */

/** \cond public */

typedef struct key_def box_key_def_t;

/** Key part definition flags. */
enum {
	BOX_KEY_PART_DEF_IS_NULLABLE = 1 << 0,
};

/**
 * It is recommended to verify size of <box_key_part_def_t>
 * against this constant on the module side at build time.
 * Example:
 *
 * | #if !defined(__cplusplus) && !defined(static_assert)
 * | #define static_assert _Static_assert
 * | #endif
 * |
 * | (slash)*
 * |  * Verify that <box_key_part_def_t> has the same size when
 * |  * compiled within tarantool and within the module.
 * |  *
 * |  * It is important, because the module allocates an array of key
 * |  * parts and passes it to <box_key_def_new_v2>() tarantool
 * |  * function.
 * |  *(slash)
 * | static_assert(sizeof(box_key_part_def_t) == BOX_KEY_PART_DEF_T_SIZE,
 * |               "sizeof(box_key_part_def_t)");
 *
 * This snippet is not part of module.h, because portability of
 * static_assert() / _Static_assert() is dubious. It should be
 * decision of a module author how portable its code should be.
 */
enum {
	BOX_KEY_PART_DEF_T_SIZE = 64,
};

/**
 * Public representation of a key part definition.
 *
 * Usage: Allocate an array of such key parts, initialize each
 * key part (call <box_key_part_def_create>() and set necessary
 * fields), pass the array into <box_key_def_new_v2>() function.
 *
 * Important: A module should call <box_key_part_def_create>()
 * to initialize the structure with default values. There is no
 * guarantee that all future default values for fields and flags
 * will be remain the same.
 *
 * The idea of separation from internal <struct key_part_def> is
 * to provide stable API and ABI for modules.
 *
 * New fields may be added into the end of the structure in later
 * tarantool versions. Also new flags may be introduced within
 * <flags> field. <collation> cannot be changed to a union (to
 * reuse for some other value), because it is verified even for
 * a non-string key part by <box_key_def_new_v2>().
 *
 * Fields that are unknown at given tarantool version are ignored
 * in general, but filled with zeros when initialized.
 */
typedef union PACKED {
	struct {
		/** Index of a tuple field (zero based). */
		uint32_t fieldno;
		/** Flags, e.g. nullability. */
		uint32_t flags;
		/** Type of the tuple field. */
		const char *field_type;
		/** Collation name for string comparisons. */
		const char *collation;
	};
	/**
	 * Padding to guarantee certain size across different
	 * tarantool versions.
	 */
	char padding[BOX_KEY_PART_DEF_T_SIZE];
} box_key_part_def_t;

/**
 * Create key definition with given field numbers and field types.
 *
 * May be used for tuple format creation and/or tuple comparison.
 *
 * \sa <box_key_def_new_v2>().
 *
 * \param fields array with key field identifiers
 * \param types array with key field types (see enum field_type)
 * \param part_count the number of key fields
 * \returns a new key definition object
 */
API_EXPORT box_key_def_t *
box_key_def_new(uint32_t *fields, uint32_t *types, uint32_t part_count);

/**
 * Initialize a key part with default values.
 *
 *  | Field       | Default value   | Details |
 *  | ----------- | --------------- | ------- |
 *  | fieldno     | 0               |         |
 *  | flags       | <default flags> |         |
 *  | field_type  | NULL            | [^1]    |
 *  | collation   | NULL            |         |
 *
 * Default flag values are the following:
 *
 *  | Flag                         | Default value |
 *  | ---------------------------- | ------------- |
 *  | BOX_KEY_PART_DEF_IS_NULLABLE | 0 (unset)     |
 *
 * Default values of fields and flags are permitted to be changed
 * in future tarantool versions. However we should be VERY
 * conservative here and consider any meaningful usage scenarios,
 * when doing so. At least new defaults should be consistent with
 * how tarantool itself doing key_def related operations:
 * validation, key extraction, comparisons and so on.
 *
 * All trailing padding bytes are set to zero. The same for
 * unknown <flags> bits.
 *
 * [^1]: <box_key_def_new_v2>() does not accept NULL as a
 *       <field_type>, so it should be filled explicitly.
 */
API_EXPORT void
box_key_part_def_create(box_key_part_def_t *part);

/**
 * Create a key_def from given key parts.
 *
 * Unlike <box_key_def_new>() this function allows to define
 * nullability, collation and other options for each key part.
 *
 * <box_key_part_def_t> fields that are unknown at given tarantool
 * version are ignored. The same for unknown <flags> bits.
 *
 * In case of an error set a diag and return NULL.
 * @sa <box_error_last>().
 */
API_EXPORT box_key_def_t *
box_key_def_new_v2(box_key_part_def_t *parts, uint32_t part_count);

/**
 * Duplicate key_def.
 * @param key_def Original key_def.
 *
 * @retval not NULL Duplicate of src.
 * @retval     NULL Memory error.
 */
API_EXPORT box_key_def_t *
box_key_def_dup(const box_key_def_t *key_def);

/**
 * Delete key definition
 *
 * \param key_def key definition to delete
 */
API_EXPORT void
box_key_def_delete(box_key_def_t *key_def);

/**
 * Dump key part definitions of given key_def.
 *
 * The function allocates key parts and storage for pointer fields
 * (e.g. collation names) on the box region.
 * @sa <box_region_truncate>().
 *
 * <box_key_part_def_t> fields that are unknown at given tarantool
 * version are set to zero. The same for unknown <flags> bits.
 *
 * In case of an error set a diag and return NULL.
 * @sa <box_error_last>().
 */
API_EXPORT box_key_part_def_t *
box_key_def_dump_parts(const box_key_def_t *key_def, uint32_t *part_count_ptr);

/**
 * Check that tuple fields match with given key definition.
 *
 * @param key_def  Key definition.
 * @param tuple    Tuple to validate.
 *
 * @retval 0   The tuple is valid.
 * @retval -1  The tuple is invalid.
 *
 * In case of an invalid tuple set a diag and return -1.
 * @sa <box_error_last>().
 */
API_EXPORT int
box_key_def_validate_tuple(box_key_def_t *key_def, box_tuple_t *tuple);

/**
 * Compare tuples using the key definition.
 * @param tuple_a first tuple
 * @param tuple_b second tuple
 * @param key_def key definition
 * @retval 0  if key_fields(tuple_a) == key_fields(tuple_b)
 * @retval <0 if key_fields(tuple_a) < key_fields(tuple_b)
 * @retval >0 if key_fields(tuple_a) > key_fields(tuple_b)
 */
API_EXPORT int
box_tuple_compare(const box_tuple_t *tuple_a, const box_tuple_t *tuple_b,
		  box_key_def_t *key_def);

/**
 * @brief Compare tuple with key using the key definition.
 * @param tuple tuple
 * @param key key with MessagePack array header
 * @param key_def key definition
 *
 * @retval 0  if key_fields(tuple) == parts(key)
 * @retval <0 if key_fields(tuple) < parts(key)
 * @retval >0 if key_fields(tuple) > parts(key)
 */

API_EXPORT int
box_tuple_compare_with_key(const box_tuple_t *tuple_a, const char *key_b,
			   box_key_def_t *key_def);

/**
 * Allocate a new key_def with a set union of key parts from
 * first and second key defs.
 *
 * Parts of the new key_def consist of the first key_def's parts
 * and those parts of the second key_def that were not among the
 * first parts.
 *
 * @retval not NULL  Ok.
 * @retval NULL      Memory error.
 *
 * In case of an error set a diag and return NULL.
 * @sa <box_error_last>().
 */
API_EXPORT box_key_def_t *
box_key_def_merge(const box_key_def_t *first, const box_key_def_t *second);

/**
 * Extract key from tuple by given key definition and return
 * buffer allocated on the box region with this key.
 * @sa <box_region_truncate>().
 *
 * This function has O(n) complexity, where n is the number of key
 * parts.
 *
 * @param key_def       Definition of key that need to extract.
 * @param tuple         Tuple from which need to extract key.
 * @param ignored       Ignored on this version of tarantool.
 * @param key_size_ptr  Here will be size of extracted key.
 *
 * @retval not NULL  Success.
 * @retval NULL      Memory allocation error.
 *
 * In case of an error set a diag and return NULL.
 * @sa <box_error_last>().
 */
API_EXPORT char *
box_key_def_extract_key(box_key_def_t *key_def, box_tuple_t *tuple,
			int ignored, uint32_t *key_size_ptr);

/**
 * Check a key against given key definition.
 *
 * Verifies key parts against given key_def's field types with
 * respect to nullability.
 *
 * A partial key (with less part than defined in @a key_def) is
 * verified by given key parts, the omitted tail is not verified
 * anyhow.
 *
 * Note: nil is accepted for nullable fields, but only for them.
 *
 * @param key_def       Key definition.
 * @param key           MessagePack'ed data for matching.
 * @param key_size_ptr  Here will be size of the validated key.
 *
 * @retval 0   The key is valid.
 * @retval -1  The key is invalid.
 *
 * In case of an invalid key set a diag and return -1.
 * @sa <box_error_last>().
 */
API_EXPORT int
box_key_def_validate_key(const box_key_def_t *key_def, const char *key,
			 uint32_t *key_size_ptr);

/**
 * Check a full key against given key definition.
 *
 * Verifies key parts against given key_def's field types with
 * respect to nullability.
 *
 * Imposes the same parts count in @a key as in @a key_def.
 * Absense of trailing key parts fails the check.
 *
 * Note: nil is accepted for nullable fields, but only for them.
 *
 * @param key_def       Key definition.
 * @param key           MessagePack'ed data for matching.
 * @param key_size_ptr  Here will be size of the validated key.
 *
 * @retval 0   The key is valid.
 * @retval -1  The key is invalid.
 *
 * In case of an invalid key set a diag and return -1.
 * @sa <box_error_last>().
 */
API_EXPORT int
box_key_def_validate_full_key(const box_key_def_t *key_def, const char *key,
			      uint32_t *key_size_ptr);

/** \endcond public */

/*
 * Size of the structure should remain the same across all
 * tarantool versions in order to allow to allocate an array of
 * them.
 */
static_assert(sizeof(box_key_part_def_t) == BOX_KEY_PART_DEF_T_SIZE,
	      "sizeof(box_key_part_def_t)");

/* }}} Module API */

static inline size_t
key_def_sizeof(uint32_t part_count)
{
	return sizeof(struct key_def) + sizeof(struct key_part) * part_count;
}

/**
 * Allocate a new key_def with the given part count
 * and initialize its parts.
 */
struct key_def *
key_def_new(const struct key_part_def *parts, uint32_t part_count);

/**
 * Dump part definitions of the given key def.
 */
void
key_def_dump_parts(const struct key_def *def, struct key_part_def *parts);

/**
 * Update 'has_optional_parts' of @a key_def with correspondence
 * to @a min_field_count.
 * @param def Key definition to update.
 * @param min_field_count Minimal field count. All parts out of
 *        this value are optional.
 */
void
key_def_update_optionality(struct key_def *def, uint32_t min_field_count);

/**
 * An snprint-style function to print a key definition.
 */
int
key_def_snprint_parts(char *buf, int size, const struct key_part_def *parts,
		      uint32_t part_count);

/**
 * Return size of key parts array when encoded in MsgPack.
 * See also key_def_encode_parts().
 */
size_t
key_def_sizeof_parts(const struct key_part_def *parts, uint32_t part_count);

/**
 * Encode key parts array in MsgPack and return a pointer following
 * the end of encoded data.
 */
char *
key_def_encode_parts(char *data, const struct key_part_def *parts,
		     uint32_t part_count);

/**
 * 1.6.6+
 * Decode parts array from tuple field and write'em to index_def structure.
 * Throws a nice error about invalid types, but does not check ranges of
 *  resulting values field_no and field_type
 * Parts expected to be a sequence of <part_count> arrays like this:
 *  [NUM, STR, ..][NUM, STR, ..]..,
 *  OR
 *  {field=NUM, type=STR, ..}{field=NUM, type=STR, ..}..,
 */
int
key_def_decode_parts(struct key_part_def *parts, uint32_t part_count,
		     const char **data, const struct field_def *fields,
		     uint32_t field_count);

/**
 * 1.6.0-1.6.5
 * TODO: Remove it in newer version, find all 1.6.5-
 * Decode parts array from tuple fieldw and write'em to index_def structure.
 * Does not check anything since tuple must be validated before
 * Parts expected to be a sequence of <part_count> 2 * arrays values this:
 *  NUM, STR, NUM, STR, ..,
 */
int
key_def_decode_parts_160(struct key_part_def *parts, uint32_t part_count,
			 const char **data, const struct field_def *fields,
			 uint32_t field_count);

/**
 * Returns the part in index_def->parts for the specified fieldno.
 * If fieldno is not in index_def->parts returns NULL.
 */
const struct key_part *
key_def_find(const struct key_def *key_def, uint32_t fieldno);

/**
 * Check if key definition @a first contains all parts of
 * key definition @a second.
 * @retval true if @a first is a superset of @a second
 * @retval false otherwise
 */
bool
key_def_contains(const struct key_def *first, const struct key_def *second);

/**
 * Allocate a new key_def with a set union of key parts from
 * first and second key defs. Parts of the new key_def consist
 * of the first key_def's parts and those parts of the second
 * key_def that were not among the first parts.
 * @retval not NULL Ok.
 * @retval NULL     Memory error.
 */
struct key_def *
key_def_merge(const struct key_def *first, const struct key_def *second);

/*
 * Check that parts of the key match with the key definition.
 * @param key_def Key definition.
 * @param key MessagePack'ed data for matching.
 * @param part_count Field count in the key.
 * @param allow_nullable True if nullable parts are allowed.
 * @param key_end[out] The end of the validated key.
 *
 * @retval 0  The key is valid.
 * @retval -1 The key is invalid.
 */
int
key_validate_parts(const struct key_def *key_def, const char *key,
		   uint32_t part_count, bool allow_nullable,
		   const char **key_end);

/**
 * Return true if @a index_def defines a sequential key without
 * holes starting from the first field. In other words, for all
 * key parts index_def->parts[part_id].fieldno == part_id.
 * @param index_def index_def
 * @retval true index_def is sequential
 * @retval false otherwise
 */
static inline bool
key_def_is_sequential(const struct key_def *key_def)
{
	for (uint32_t part_id = 0; part_id < key_def->part_count; part_id++) {
		if (key_def->parts[part_id].fieldno != part_id)
			return false;
	}
	return true;
}

/**
 * Return true if @a key_def defines has fields that requires
 * special collation comparison.
 * @param key_def key_def
 * @retval true if the key_def has collation fields
 * @retval false otherwise
 */
static inline bool
key_def_has_collation(const struct key_def *key_def)
{
	for (uint32_t part_id = 0; part_id < key_def->part_count; part_id++) {
		if (key_def->parts[part_id].coll != NULL)
			return true;
	}
	return false;
}

/** A helper table for key_mp_type_validate */
extern const uint32_t key_mp_type[];

/**
 * @brief Checks if \a field_type (MsgPack) is compatible \a type (KeyDef).
 * @param type KeyDef type
 * @param field_type MsgPack type
 * @param field_no - a field number (is used to store an error message)
 *
 * @retval 0  mp_type is valid.
 * @retval -1 mp_type is invalid.
 */
static inline int
key_mp_type_validate(enum field_type key_type, enum mp_type mp_type,
		     int err, uint32_t field_no, bool is_nullable)
{
	assert(key_type < field_type_MAX);
	assert((size_t) mp_type < CHAR_BIT * sizeof(*key_mp_type));
	uint32_t mask = key_mp_type[key_type] | (is_nullable * (1U << MP_NIL));
	if (unlikely((mask & (1U << mp_type)) == 0)) {
		diag_set(ClientError, err, field_no, field_type_strs[key_type]);
		return -1;
	}
	return 0;
}

/**
 * Compare two key part arrays.
 *
 * One key part is considered to be greater than the other if:
 * - its fieldno is greater
 * - given the same fieldno, NUM < STRING
 *
 * A key part array is considered greater than the other if all
 * its key parts are greater, or, all common key parts are equal
 * but there are additional parts in the bigger array.
 */
int
key_part_cmp(const struct key_part *parts1, uint32_t part_count1,
	     const struct key_part *parts2, uint32_t part_count2);

/**
 * Extract key from tuple by given key definition and return
 * buffer allocated on box_txn_alloc with this key. This function
 * has O(n) complexity, where n is the number of key parts.
 * @param tuple - tuple from which need to extract key
 * @param key_def - definition of key that need to extract
 * @param key_size - here will be size of extracted key
 *
 * @retval not NULL Success
 * @retval NULL     Memory allocation error
 */
static inline char *
tuple_extract_key(const struct tuple *tuple, struct key_def *key_def,
		  uint32_t *key_size)
{
	return key_def->tuple_extract_key(tuple, key_def, key_size);
}

/**
 * Extract key from raw msgpuck by given key definition and return
 * buffer allocated on box_txn_alloc with this key.
 * This function has O(n*m) complexity, where n is the number of key parts
 * and m is the tuple size.
 * @param data - msgpuck data from which need to extract key
 * @param data_end - pointer at the end of data
 * @param key_def - definition of key that need to extract
 * @param key_size - here will be size of extracted key
 *
 * @retval not NULL Success
 * @retval NULL     Memory allocation error
 */
static inline char *
tuple_extract_key_raw(const char *data, const char *data_end,
		      struct key_def *key_def, uint32_t *key_size)
{
	return key_def->tuple_extract_key_raw(data, data_end, key_def,
					      key_size);
}

/**
 * Compare keys using the key definition.
 * @param key_a key parts with MessagePack array header
 * @param part_count_a the number of parts in the key_a
 * @param key_b key_parts with MessagePack array header
 * @param part_count_b the number of parts in the key_b
 * @param key_def key definition
 *
 * @retval 0  if key_a == key_b
 * @retval <0 if key_a < key_b
 * @retval >0 if key_a > key_b
 */
int
key_compare(const char *key_a, const char *key_b, struct key_def *key_def);

/**
 * Compare tuples using the key definition.
 * @param tuple_a first tuple
 * @param tuple_b second tuple
 * @param key_def key definition
 * @retval 0  if key_fields(tuple_a) == key_fields(tuple_b)
 * @retval <0 if key_fields(tuple_a) < key_fields(tuple_b)
 * @retval >0 if key_fields(tuple_a) > key_fields(tuple_b)
 */
static inline int
tuple_compare(const struct tuple *tuple_a, const struct tuple *tuple_b,
	      struct key_def *key_def)
{
	return key_def->tuple_compare(tuple_a, tuple_b, key_def);
}

/**
 * @brief Compare tuple with key using the key definition.
 * @param tuple tuple
 * @param key key parts without MessagePack array header
 * @param part_count the number of parts in @a key
 * @param key_def key definition
 *
 * @retval 0  if key_fields(tuple) == parts(key)
 * @retval <0 if key_fields(tuple) < parts(key)
 * @retval >0 if key_fields(tuple) > parts(key)
 */
static inline int
tuple_compare_with_key(const struct tuple *tuple, const char *key,
		       uint32_t part_count, struct key_def *key_def)
{
	return key_def->tuple_compare_with_key(tuple, key, part_count, key_def);
}

#if defined(__cplusplus)
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* TARANTOOL_BOX_KEY_DEF_H_INCLUDED */
