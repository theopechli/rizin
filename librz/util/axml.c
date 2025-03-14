// SPDX-FileCopyrightText: 2021 keegan
// SPDX-License-Identifier: LGPL-3.0-only

#include <string.h>
#include <rz_types.h>
#include <rz_core.h>

#include "axml_resources.h"

enum {
	TYPE_STRING_POOL = 0x0001,
	TYPE_XML = 0x0003,
	TYPE_START_NAMESPACE = 0x100,
	TYPE_END_NAMESPACE = 0x101,
	TYPE_START_ELEMENT = 0x0102,
	TYPE_END_ELEMENT = 0x103,
	TYPE_RESOURCE_MAP = 0x180,
};

enum {
	RESOURCE_NULL = 0x00,
	RESOURCE_REFERENCE = 0x01,
	RESOURCE_STRING = 0x03,
	RESOURCE_FLOAT = 0x04,
	RESOURCE_INT_DEC = 0x10,
	RESOURCE_INT_HEX = 0x11,
	RESOURCE_BOOL = 0x12,
};

enum {
	FLAG_UTF8 = 1 << 8,
};

// Beginning of every header
RZ_PACKED(
	typedef struct {
		ut16 type;
		ut16 header_size;
		ut32 size;
	})
chunk_header_t;

// String pool referenced throughout the Binary XML, there must only be ONE
RZ_PACKED(
	typedef struct {
		ut32 string_count;
		ut32 style_count;
		ut32 flags;
		ut32 strings_offset;
		ut32 styles_offset;
		ut32 offsets[];
	})
string_pool_t;

RZ_PACKED(
	typedef struct {
		ut16 size;
		ut8 unused;
		ut8 type;
		union {
			ut32 d;
			float f;
		} data;
	})
resource_value_t;

RZ_PACKED(
	typedef struct {
		ut32 namespace;
		ut32 name;
		ut32 unused;
		resource_value_t value;
	})
attribute_t;

RZ_PACKED(
	typedef struct {
		ut32 line;
		ut32 comment;
		ut32 namespace;
		ut32 name;
		ut32 flags;
		ut16 attribute_count;
		ut16 unused0;
		ut16 unused1;
		ut16 unused2;
		attribute_t attributes[];
	})
start_element_t;

RZ_PACKED(
	typedef struct {
		ut32 line;
		ut32 comment;
		ut32 namespace;
		ut32 name;
	})
end_element_t;

RZ_PACKED(
	typedef struct {
		ut32 line;
		ut32 comment;
		ut32 prefix;
		ut32 uri;
	})
namespace_t;

static char *string_lookup(string_pool_t *pool, const ut8 *data, ut64 data_size, ut32 i, size_t *length) {
	if (i > rz_read_le32(&pool->string_count)) {
		return NULL;
	}

	ut32 offset = rz_read_le32(&pool->offsets[i]);
	ut8 *start = (ut8 *)((uintptr_t)data + rz_read_le32(&pool->strings_offset) + 8 + offset);

	char *name = NULL;
	if (pool->flags & FLAG_UTF8) {
		if ((ut64)start > (ut64)data + data_size - sizeof(ut16)) {
			return NULL;
		}

		// Ignore UTF-16LE encoded length
		ut32 n = *start++;
		if (n & 0x80) {
			n = ((n & 0x7f) << 8) | *start++;
		}

		if ((ut64)start > (ut64)data + data_size - sizeof(ut16)) {
			return NULL;
		}

		// UTF-8 encoded length
		n = *start++;
		if (n & 0x80) {
			n = ((n & 0x7f) << 8) | *start++;
		}

		name = calloc(n + 1, 1);

		if (n == 0) {
			if (length) {
				*length = 0;
			}

			return name;
		}

		if ((ut64)start > (ut64)data + data_size - sizeof(ut32) - n - 1) {
			free(name);
			return NULL;
		}

		memcpy(name, start, n);

		if (length) {
			*length = n;
		}
	} else {
		if ((ut64)start > (ut64)data + data_size - sizeof(ut32)) {
			return NULL;
		}

		ut16 *start16 = (ut16 *)start;

		// If >0x7fff, stored as a big-endian ut32
		ut32 n = rz_read_le16(start16++);
		if (n & 0x8000) {
			n |= ((n & 0x7fff) << 16) | rz_read_le16(start16++);
		}

		// Size of UTF-16LE without NULL
		n *= 2;

		name = calloc(n * 2 + 1, 1);

		if ((ut64)start16 > (ut64)data + data_size - sizeof(ut32) - n - 1) {
			free(name);
			return NULL;
		}

		// If UTF-16LE, decode to UTF-8 so we can print it to the screen
		if (rz_str_utf16_to_utf8((ut8 *)name, n * 2, (const ut8 *)start16, n, true) < 0) {
			free(name);
			RZ_LOG_ERROR("Failed to decode UTF16-LE\n");
			return NULL;
		}

		if (length) {
			*length = n;
		}
	}

	return name;
}

static char *resource_value(string_pool_t *pool, const ut8 *data, ut64 data_size,
	resource_value_t *value) {
	switch (value->type) {
	case RESOURCE_NULL:
		return rz_str_new("");
	case RESOURCE_REFERENCE:
		return rz_str_newf("@0x%x", value->data.d);
	case RESOURCE_STRING:
		return string_lookup(pool, data, data_size, rz_read_le32(&value->data.d), NULL);
	case RESOURCE_FLOAT:
		return rz_str_newf("%f", value->data.f);
	case RESOURCE_INT_DEC:
		return rz_str_newf("%d", value->data.d);
	case RESOURCE_INT_HEX:
		return rz_str_newf("0x%x", value->data.d);
	case RESOURCE_BOOL:
		return rz_str_newf(value->data.d ? "true" : "false");
	default:
		RZ_LOG_WARN("Resource type is not recognized: %#x\n", value->type);
		break;
	}
	return rz_str_new("null");
}

static bool dump_element(RzStrBuf *sb, string_pool_t *pool, namespace_t *namespace,
	const ut8 *data, ut64 data_size, start_element_t *element,
	const ut32 *resource_map, ut32 resource_map_length, st32 *depth, bool start) {
	ut32 i;

	char *name = string_lookup(pool, data, data_size, rz_read_le32(&element->name), NULL);
	for (i = 0; i < *depth; i++) {
		rz_strbuf_appendf(sb, "\t");
	}

	if (start) {
		rz_strbuf_appendf(sb, "<%s", name);
		ut16 count = rz_read_le16(&element->attribute_count);
		if (*depth == 0 && namespace) {
			char *key = string_lookup(pool, data, data_size, namespace->prefix, NULL);
			char *value = string_lookup(pool, data, data_size, namespace->uri, NULL);
			rz_strbuf_appendf(sb, " xmlns:%s=\"%s\"", key, value);
			free(key);
			free(value);
		}

		if (count != 0) {
			rz_strbuf_appendf(sb, " ");
		}

		for (i = 0; i < count; i++) {
			attribute_t a = element->attributes[i];
			ut32 key_index = rz_read_le32(&a.name);
			const char *key = (const char *)string_lookup(pool, data, data_size, key_index, NULL);
			bool resource_key = false;
			// If the key is empty, it is a cached resource name
			if (key && *key == '\0') {
				free((char *)key);
				resource_key = true;
				if (resource_map && key_index < resource_map_length) {
					ut32 resource = rz_read_le32(&resource_map[key_index]);
					if (resource >= 0x1010000) {
						resource -= 0x1010000;
						if (resource < ANDROID_ATTRIBUTE_NAMES_SIZE) {
							key = ANDROID_ATTRIBUTE_NAMES[resource];
						}
					} else {
						key = "null";
					}
				} else {
					key = "null";
				}
			}
			char *value = resource_value(pool, data, data_size, &a.value);
			// If there is a namespace on the value, and there is an active
			// namespace, assume it is the same
			if (rz_read_le32(&a.namespace) != -1 && namespace && namespace->prefix != -1) {
				char *ns = string_lookup(pool, data, data_size, namespace->prefix, NULL);
				rz_strbuf_appendf(sb, "%s:%s=\"%s\"", ns, key, value);
				free(ns);
			} else {
				rz_strbuf_appendf(sb, "%s=\"%s\"", key, value);
			}
			if (i != count - 1) {
				rz_strbuf_appendf(sb, " ");
			}
			if (!resource_key) {
				free((char *)key);
			}
			free(value);
		}
	} else {
		rz_strbuf_appendf(sb, "</%s", name);
	}

	rz_strbuf_appendf(sb, ">\n");
	free(name);
	return true;
}

/**
 * \brief Decode a buffer with Android XML to regular XML string representation
 *
 * \param data Buffer containing the AXML data
 * \param data_size Size of the buffer \p data
 * \return String with the regular XML string
 */
RZ_API RZ_OWN char *rz_axml_decode(RZ_NONNULL const ut8 *data, const ut64 data_size) {
	string_pool_t *pool = NULL;
	namespace_t *namespace = NULL;
	const ut32 *resource_map = NULL;
	ut32 resource_map_length = 0;
	RzStrBuf *sb = NULL;
	st32 depth = 0;

	rz_return_val_if_fail(data, NULL);
	if (data_size == 0) {
		return strdup("");
	}

	RzBuffer *buffer = rz_buf_new_with_pointers(data, data_size, false);
	if (!buffer) {
		RZ_LOG_ERROR("Error allocating RzBuffer\n");
		goto error;
	}

	ut64 offset = 0;

	chunk_header_t header = { 0 };
	if (rz_buf_fread_at(buffer, offset, (ut8 *)&header, "ssi", 1) != sizeof(header)) {
		goto bad;
	}

	if (header.type != TYPE_XML) {
		goto bad;
	}

	ut64 binary_size = header.size;
	if (binary_size > data_size) {
		goto bad;
	}

	offset += sizeof(header);

	sb = rz_strbuf_new("");

	while (offset < binary_size) {
		if (rz_buf_fread_at(buffer, offset, (ut8 *)&header, "ssi", 1) != sizeof(header)) {
			goto bad;
		}

		ut16 type = header.type;

		// After reading the original chunk header, read the type-specific
		// header
		offset += sizeof(header);

		switch (type) {
		case TYPE_STRING_POOL: {
			ut16 header_size = header.size;
			if (header_size == 0 || header_size > data_size) {
				goto bad;
			}

			pool = malloc(header_size);
			if (!pool) {
				goto bad;
			}

			if (rz_buf_read_at(buffer, offset, (void *)pool, header_size) != header_size) {
				goto bad;
			}
			break;
		}
		case TYPE_START_ELEMENT: {
			// The string pool must be the first header
			if (!pool) {
				goto bad;
			}

			ut16 header_size = header.size;
			if (header_size == 0 || header_size > data_size) {
				goto bad;
			}

			start_element_t *element = malloc(header_size);
			if (!element) {
				goto bad;
			}

			if (rz_buf_read_at(buffer, offset, (void *)element, header_size) != header_size) {
				free(element);
				goto bad;
			}

			if (!dump_element(sb, pool, namespace, data, data_size, element,
				    resource_map, resource_map_length, &depth, true)) {
				free(element);
				goto bad;
			}

			depth++;

			free(element);
			break;
		}
		case TYPE_END_ELEMENT: {
			depth--;
			if (depth < 0) {
				goto bad;
			}

			end_element_t end;
			if (rz_buf_read_at(buffer, offset, (void *)&end, sizeof(end)) != sizeof(end)) {
				goto bad;
			}

			// The beginning of the start and end element structs
			// are the same, so we can use this interchangably
			if (!dump_element(sb, pool, namespace, data, data_size, (start_element_t *)&end,
				    resource_map, resource_map_length, &depth, false)) {
				goto bad;
			}
			break;
		}
		case TYPE_START_NAMESPACE:
			// If there is already a start namespace, override it
			free(namespace);
			namespace = malloc(sizeof(*namespace));
			if (rz_buf_fread_at(buffer, offset, (ut8 *)namespace, "iiii", 1) != sizeof(*namespace)) {
				goto bad;
			}
			break;
		case TYPE_END_NAMESPACE:
			break;
		case TYPE_RESOURCE_MAP:
			resource_map = (ut32 *)(data + offset);
			resource_map_length = header.size;
			if (resource_map_length > data_size - offset) {
				goto bad;
			}
			resource_map_length /= sizeof(ut32);
			break;
		default:
			RZ_LOG_WARN("Type is not recognized: %#x\n", type);
		}

		offset += header.size - sizeof(header);
	}

	rz_buf_free(buffer);
	free(pool);
	free(namespace);
	return rz_strbuf_drain(sb);
bad:
	RZ_LOG_ERROR("Invalid Android Binary XML\n");
error:
	if (buffer)
		rz_buf_free(buffer);
	free(pool);
	rz_strbuf_free(sb);
	return NULL;
}
