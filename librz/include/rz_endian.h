#ifndef RZ_ENDIAN_H
#define RZ_ENDIAN_H

#include <rz_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Endian agnostic functions working on single byte. */

static inline ut8 rz_read_ble8(const void *src) {
	if (!src) {
		return UT8_MAX;
	}
	return *(const ut8 *)src;
}

static inline ut8 rz_read_at_ble8(const void *src, size_t offset) {
	return rz_read_ble8(((const ut8 *)src) + offset);
}

static inline void rz_write_ble8(void *dest, ut8 val) {
	*(ut8 *)dest = val;
}

static inline void rz_write_at_ble8(void *dest, ut8 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_ble8(d, val);
}

/* Big Endian functions. */

static inline ut8 rz_read_be8(const void *src) {
	return rz_read_ble8(src);
}

static inline ut8 rz_read_at_be8(const void *src, size_t offset) {
	return rz_read_at_ble8(src, offset);
}

static inline void rz_write_be8(void *dest, ut8 val) {
	rz_write_ble8(dest, val);
}

static inline void rz_write_at_be8(void *dest, ut8 val, size_t offset) {
	rz_write_at_ble8(dest, val, offset);
}

static inline ut16 rz_read_be16(const void *src) {
	const ut8 *s = (const ut8 *)src;
	return (((ut16)s[0]) << 8) | (((ut16)s[1]) << 0);
}

static inline ut16 rz_read_at_be16(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_be16(s);
}

static inline void rz_write_be16(void *dest, ut16 val) {
	rz_write_be8(dest, val >> 8);
	rz_write_at_be8(dest, (ut8)val, sizeof(ut8));
}

static inline void rz_write_at_be16(void *dest, ut16 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_be16(d, val);
}

static inline ut32 rz_read_be24(const void *src) {
	return rz_read_be8(src) << 16 | rz_read_be8((const ut8 *)src + 1) << 8 |
		rz_read_be8((const ut8 *)src + 2);
}

static inline ut32 rz_read_at_be24(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_be24(s);
}

static inline void rz_write_be24(void *_dest, ut32 val) {
	ut8 *dest = (ut8 *)_dest;
	rz_write_be8(dest++, val >> 16);
	rz_write_be8(dest++, val >> 8);
	rz_write_be8(dest, val);
}

static inline ut32 rz_read_be32(const void *src) {
	const ut8 *s = (const ut8 *)src;
	return (((ut32)s[0]) << 24) | (((ut32)s[1]) << 16) |
		(((ut32)s[2]) << 8) | (((ut32)s[3]) << 0);
}

static inline ut32 rz_read_at_be32(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_be32(s);
}

static inline void rz_write_be32(void *dest, ut32 val) {
	rz_write_be16(dest, val >> 16);
	rz_write_at_be16(dest, val, sizeof(ut16));
}

static inline void rz_write_at_be32(void *dest, ut32 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_be32(d, val);
}

static inline ut64 rz_read_be64(const void *src) {
	ut64 val = ((ut64)(rz_read_be32(src))) << 32;
	val |= rz_read_at_be32(src, sizeof(ut32));
	return val;
}

static inline ut64 rz_read_at_be64(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_be64(s);
}

static inline void rz_write_be64(void *dest, ut64 val) {
	rz_write_be32(dest, val >> 32);
	rz_write_at_be32(dest, (ut32)val, sizeof(ut32));
}

static inline void rz_write_at_be64(void *dest, ut64 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_be64(d, val);
}

static inline float rz_read_be_float(const void *src) {
	union {
		ut32 bits;
		float flt;
	} p;
	p.bits = rz_read_be32(src);
	return p.flt;
}

static inline float rz_read_at_be_float(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_be_float(s);
}

static inline void rz_write_be_float(void *dest, float val) {
	union {
		ut32 bits;
		float flt;
	} p;
	p.flt = val;
	rz_write_be32(dest, p.bits);
}

static inline void rz_write_at_be_float(void *dest, float val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_be_float(d, val);
}

static inline double rz_read_be_double(const void *src) {
	union {
		ut64 bits;
		double dbl;
	} p;
	p.bits = rz_read_be64(src);
	return p.dbl;
}

static inline double rz_read_at_be_double(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_be_double(s);
}

static inline void rz_write_be_double(void *dest, double val) {
	union {
		ut64 bits;
		double dbl;
	} p;
	p.dbl = val;
	rz_write_be64(dest, p.bits);
}

static inline void rz_write_at_be_double(void *dest, double val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_be_double(d, val);
}

/* Little Endian functions. */

static inline ut8 rz_read_le8(const void *src) {
	if (!src) {
		return UT8_MAX;
	}
	return rz_read_ble8(src);
}

static inline ut8 rz_read_at_le8(const void *src, size_t offset) {
	return rz_read_at_ble8(src, offset);
}

static inline void rz_write_le8(void *dest, ut8 val) {
	rz_write_ble8(dest, val);
}

static inline void rz_write_at_le8(void *dest, ut8 val, size_t offset) {
	rz_write_at_ble8(dest, val, offset);
}

static inline ut16 rz_read_le16(const void *src) {
	if (!src) {
		return UT16_MAX;
	}
	const ut8 *s = (const ut8 *)src;
	return (((ut16)s[1]) << 8) | (((ut16)s[0]) << 0);
}

static inline ut16 rz_read_at_le16(const void *src, size_t offset) {
	if (!src) {
		return UT16_MAX;
	}
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_le16(s);
}

static inline void rz_write_le16(void *dest, ut16 val) {
	rz_write_le8(dest, (ut8)val);
	rz_write_at_le8(dest, val >> 8, sizeof(ut8));
}

static inline void rz_write_at_le16(void *dest, ut16 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_le16(d, val);
}

static inline void rz_write_le24(void *_dest, ut32 val) {
	ut8 *dest = (ut8 *)_dest;
	rz_write_le8(dest++, val);
	rz_write_le8(dest++, val >> 8);
	rz_write_le8(dest, val >> 16);
}

static inline ut32 rz_read_le32(const void *src) {
	if (!src) {
		return UT32_MAX;
	}
	const ut8 *s = (const ut8 *)src;
	return (((ut32)s[3]) << 24) | (((ut32)s[2]) << 16) |
		(((ut32)s[1]) << 8) | (((ut32)s[0]) << 0);
}

static inline ut32 rz_read_at_le32(const void *src, size_t offset) {
	if (!src) {
		return UT32_MAX;
	}
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_le32(s);
}

static inline void rz_write_le32(void *dest, ut32 val) {
	rz_write_le16(dest, val);
	rz_write_at_le16(dest, val >> 16, sizeof(ut16));
}

static inline void rz_write_at_le32(void *dest, ut32 val, size_t offset) {
	ut8 *d = ((ut8 *)dest) + offset;
	rz_write_le32(d, val);
}

static inline ut64 rz_read_le64(const void *src) {
	ut64 val = ((ut64)(rz_read_at_le32(src, sizeof(ut32)))) << 32;
	val |= rz_read_le32(src);
	return val;
}

static inline ut64 rz_read_at_le64(const void *src, size_t offset) {
	const ut8 *s = ((const ut8 *)src) + offset;
	return rz_read_le64(s);
}

static inline void rz_write_le64(void *dest, ut64 val) {
	rz_write_le32(dest, (ut32)val);
	rz_write_at_le32(dest, val >> 32, sizeof(ut32));
}

static inline void rz_write_at_le64(void *dest, ut64 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_le64(d, val);
}

static inline float rz_read_le_float(const void *src) {
	union {
		ut32 bits;
		float flt;
	} p;
	p.bits = rz_read_le32(src);
	return p.flt;
}

static inline float rz_read_at_le_float(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_le_float(s);
}

static inline void rz_write_le_float(void *dest, float val) {
	union {
		ut32 bits;
		float flt;
	} p;
	p.flt = val;
	rz_write_le32(dest, p.bits);
}

static inline void rz_write_at_le_float(void *dest, float val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_le_float(d, val);
}

static inline double rz_read_le_double(const void *src) {
	union {
		ut64 bits;
		double dbl;
	} p;
	p.bits = rz_read_le64(src);
	return p.dbl;
}

static inline double rz_read_at_le_double(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_le_double(s);
}

static inline void rz_write_le_double(void *dest, double val) {
	union {
		ut64 bits;
		double dbl;
	} p;
	p.dbl = val;
	rz_write_le64(dest, p.bits);
}

static inline void rz_write_at_le_double(void *dest, double val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_le_double(d, val);
}

/* Middle Endian functions. */

static inline ut8 rz_read_me8(const void *src) {
	return src ? rz_read_ble8(src) : UT8_MAX;
}

static inline ut8 rz_read_at_me8(const void *src, size_t offset) {
	return rz_read_at_ble8(src, offset);
}

static inline void rz_write_me8(void *dest, ut8 val) {
	rz_write_ble8(dest, val);
}

static inline void rz_write_at_me8(void *dest, ut8 val, size_t offset) {
	rz_write_at_ble8(dest, val, offset);
}

static inline ut16 rz_read_me16(const void *src) {
	if (!src) {
		return UT16_MAX;
	}
	const ut8 *s = (const ut8 *)src;
	return (((ut16)s[0]) << 8) | (((ut16)s[1]) << 0);
}

static inline ut16 rz_read_at_me16(const void *src, size_t offset) {
	if (!src) {
		return UT16_MAX;
	}
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_me16(s);
}

static inline void rz_write_me16(void *dest, ut16 val) {
	rz_write_me8(dest, val >> 8);
	rz_write_at_me8(dest, (ut8)val, sizeof(ut8));
}

static inline void rz_write_at_me16(void *dest, ut16 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_me16(d, val);
}

static inline ut32 rz_read_me32(const void *src) {
	if (!src) {
		return UT32_MAX;
	}
	const ut8 *s = (const ut8 *)src;
	return (((ut32)s[2]) << 24) | (((ut32)s[3]) << 16) |
		(((ut32)s[0]) << 8) | (((ut32)s[1]) << 0);
}

static inline ut32 rz_read_at_me32(const void *src, size_t offset) {
	if (!src) {
		return UT32_MAX;
	}
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_me32(s);
}

static inline void rz_write_me32(void *dest, ut32 val) {
	rz_write_me16(dest, val);
	rz_write_at_me16(dest, val >> 16, sizeof(ut16));
}

static inline void rz_write_at_me32(void *dest, ut32 val, size_t offset) {
	ut8 *d = ((ut8 *)dest) + offset;
	rz_write_me32(d, val);
}

static inline ut64 rz_read_me64(const void *src) {
	ut64 val = ((ut64)(rz_read_at_me32(src, sizeof(ut32)))) << 32;
	val |= rz_read_me32(src);
	return val;
}

static inline ut64 rz_read_at_me64(const void *src, size_t offset) {
	const ut8 *s = ((const ut8 *)src) + offset;
	return rz_read_me64(s);
}

static inline void rz_write_me64(void *dest, ut64 val) {
	rz_write_me32(dest, (ut32)val);
	rz_write_at_me32(dest, val >> 32, sizeof(ut32));
}

static inline void rz_write_at_me64(void *dest, ut64 val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_me64(d, val);
}

static inline float rz_read_me_float(const void *src) {
	union {
		ut32 bits;
		float flt;
	} p;
	p.bits = rz_read_me32(src);
	return p.flt;
}

static inline float rz_read_at_me_float(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_me_float(s);
}

static inline void rz_write_me_float(void *dest, float val) {
	union {
		ut32 bits;
		float flt;
	} p;
	p.flt = val;
	rz_write_me32(dest, p.bits);
}

static inline void rz_write_at_me_float(void *dest, float val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_me_float(d, val);
}

static inline double rz_read_me_double(const void *src) {
	union {
		ut64 bits;
		double dbl;
	} p;
	p.bits = rz_read_me64(src);
	return p.dbl;
}

static inline double rz_read_at_me_double(const void *src, size_t offset) {
	const ut8 *s = (const ut8 *)src + offset;
	return rz_read_me_double(s);
}

static inline void rz_write_me_double(void *dest, double val) {
	union {
		ut64 bits;
		double dbl;
	} p;
	p.dbl = val;
	rz_write_me64(dest, p.bits);
}

static inline void rz_write_at_me_double(void *dest, double val, size_t offset) {
	ut8 *d = (ut8 *)dest + offset;
	rz_write_me_double(d, val);
}

/* Helper functions */

static inline ut16 rz_read_ble16(const void *src, bool big_endian) {
	return big_endian ? rz_read_be16(src) : rz_read_le16(src);
}

static inline ut32 rz_read_ble32(const void *src, bool big_endian) {
	return big_endian ? rz_read_be32(src) : rz_read_le32(src);
}

static inline ut64 rz_read_ble64(const void *src, bool big_endian) {
	return big_endian ? rz_read_be64(src) : rz_read_le64(src);
}

static inline float rz_read_ble_float(const void *src, bool big_endian) {
	return big_endian ? rz_read_be_float(src) : rz_read_le_float(src);
}

static inline double rz_read_ble_double(const void *src, bool big_endian) {
	return big_endian ? rz_read_be_double(src) : rz_read_le_double(src);
}

static inline ut16 rz_read_at_ble16(const void *src, size_t offset, bool big_endian) {
	return big_endian ? rz_read_at_be16(src, offset) : rz_read_at_le16(src, offset);
}

static inline ut32 rz_read_at_ble32(const void *src, size_t offset, bool big_endian) {
	return big_endian ? rz_read_at_be32(src, offset) : rz_read_at_le32(src, offset);
}

static inline ut64 rz_read_at_ble64(const void *src, size_t offset, bool big_endian) {
	return big_endian ? rz_read_at_be64(src, offset) : rz_read_at_le64(src, offset);
}

static inline float rz_read_at_ble_float(const void *src, size_t offset, bool big_endian) {
	return big_endian ? rz_read_at_be_float(src, offset) : rz_read_at_le_float(src, offset);
}

static inline double rz_read_at_ble_double(const void *src, size_t offset, bool big_endian) {
	return big_endian ? rz_read_at_be_double(src, offset) : rz_read_at_le_double(src, offset);
}

static inline ut64 rz_read_ble(const void *src, bool big_endian, int size) {
	switch (size) {
	case 8:
		return (ut64)((const ut8 *)src)[0];
	case 16:
		return rz_read_ble16(src, big_endian);
	case 32:
		return rz_read_ble32(src, big_endian);
	case 64:
		return rz_read_ble64(src, big_endian);
	default:
		return UT64_MAX;
	}
}

static inline void rz_write_ble16(void *dest, ut16 val, bool big_endian) {
	big_endian ? rz_write_be16(dest, val) : rz_write_le16(dest, val);
}

static inline void rz_write_ble24(void *dest, ut32 val, bool big_endian) {
	big_endian ? rz_write_be24(dest, val) : rz_write_le24(dest, val);
}

static inline void rz_write_ble32(void *dest, ut32 val, bool big_endian) {
	big_endian ? rz_write_be32(dest, val) : rz_write_le32(dest, val);
}

static inline void rz_write_ble64(void *dest, ut64 val, bool big_endian) {
	big_endian ? rz_write_be64(dest, val) : rz_write_le64(dest, val);
}

static inline void rz_write_ble(void *dst, ut64 val, bool big_endian, int size) {
	switch (size) {
	case 8:
		((ut8 *)dst)[0] = (ut8)val;
		break;
	case 16:
		rz_write_ble16(dst, (ut16)val, big_endian);
		break;
	case 24:
		rz_write_ble24(dst, (ut32)val, big_endian);
		break;
	case 32:
		rz_write_ble32(dst, (ut32)val, big_endian);
		break;
	case 64:
		rz_write_ble64(dst, val, big_endian);
		break;
	default:
		break;
	}
}

static inline void rz_write_ble_float(void *src, float val, bool big_endian) {
	big_endian ? rz_write_be_float(src, val) : rz_write_le_float(src, val);
}

static inline void rz_write_ble_double(void *src, double val, bool big_endian) {
	big_endian ? rz_write_be_double(src, val) : rz_write_le_double(src, val);
}

/*swap*/
static inline ut16 rz_swap_ut16(ut16 val) {
	return (val << 8) | (val >> 8);
}

static inline st16 rz_swap_st16(st16 val) {
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}

static inline ut32 rz_swap_ut32(ut32 val) {
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | (val >> 16);
}

static inline st32 rz_swap_st32(st32 val) {
	val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
	return (val << 16) | ((val >> 16) & 0xFFFF);
}

static inline ut64 rz_swap_ut64(ut64 val) {
	val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
	val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
	return (val << 32) | (val >> 32);
}

static inline st64 rz_swap_st64(st64 val) {
	val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
	val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
	return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}

/* Some "secured" functions, to do basic operation (mul, sub, add...) on integers */
static inline int UT64_ADD(ut64 *r, ut64 a, ut64 b) {
	if (UT64_MAX - a < b) {
		return 0;
	}
	if (r) {
		*r = a + b;
	}
	return 1;
}

static inline int UT64_MUL(ut64 *r, ut64 a, ut64 b) {
	if (a && UT64_MAX / a < b) {
		return 0;
	}
	if (r) {
		*r = a * b;
	}
	return 1;
}

static inline int UT64_SUB(ut64 *r, ut64 a, ut64 b) {
	if (b > a) {
		return 0;
	}
	if (r) {
		*r = a - b;
	}
	return 1;
}

static inline int UT32_ADD(ut32 *r, ut32 a, ut32 b) {
	if (UT32_MAX - a < b) {
		return 0;
	}
	if (r) {
		*r = a + b;
	}
	return 1;
}

static inline int UT32_MUL(ut32 *r, ut32 a, ut32 b) {
	if (a && UT32_MAX / a < b) {
		return 0;
	}
	if (r) {
		*r = a * b;
	}
	return 1;
}

static inline int UT32_SUB(ut32 *r, ut32 a, ut32 b) {
	if (b > a) {
		return 0;
	}
	if (r) {
		*r = a - b;
	}
	return 1;
}

static inline int UT16_ADD(ut16 *r, ut16 a, ut16 b) {
	if (UT16_MAX - a < b) {
		return 0;
	}
	if (r) {
		*r = a + b;
	}
	return 1;
}

static inline int UT16_MUL(ut16 *r, ut16 a, ut16 b) {
	if (a && UT16_MAX / a < b) {
		return 0;
	}
	if (r) {
		*r = a * b;
	}
	return 1;
}

static inline int UT16_SUB(ut16 *r, ut16 a, ut16 b) {
	if (b > a) {
		return 0;
	}
	if (r) {
		*r = a - b;
	}
	return 1;
}

static inline int UT8_ADD(ut8 *r, ut8 a, ut8 b) {
	if (UT8_MAX - a < b) {
		return 0;
	}
	if (r) {
		*r = a + b;
	}
	return 1;
}

static inline int UT8_MUL(ut8 *r, ut8 a, ut8 b) {
	if (a && UT8_MAX / a < b) {
		return 0;
	}
	if (r) {
		*r = a * b;
	}
	return 1;
}

static inline int UT8_SUB(ut8 *r, ut8 a, ut8 b) {
	if (b > a) {
		return 0;
	}
	if (r) {
		*r = a - b;
	}
	return 1;
}

#ifdef __cplusplus
}
#endif

#endif
