#include <stdio.h>
#include <assert.h>
// assert
#include <inttypes.h>
// uint32_t
#include <string.h>
// memset
#include <x86intrin.h>
//#include <emmintrin.h>
#include <limits.h>
// CHAR_BIT
#include <endian.h>
#include "aim.h"

//#define VARIANT_SIMPLE
#define VARIANT_DNC // Divide aNd Conquer
// 1 test16384.pbm
// OMP
// 1.180
// 1.051
// 1.004
// 0.691
// 0.489
// 0.400
// 0.339
// 0.148

//#define VARIANT_32
#define VARIANT_ACCESS
#define OPT_GET_CACHE
#define OPT_ROTATE_LARGE
// OMP (parallel execution):
#define OPT_OMP_LARGE
#define OPT_OMP_SMALL_Y
#define OPT_OMP_SMALL_X

//#define AREA_SMALL_DIM (2 * 4096)
// benchmark: 5.94075
//#define AREA_SMALL_DIM (4096)
// up: too much
// down: small enough
#define AREA_SMALL_DIM (2048)
// benchmark: 5.27825
//#define AREA_SMALL_DIM (1024)
//#define AREA_SMALL_DIM (512)
// 2.351
//#define AREA_SMALL_DIM 256
// 2.318 2.300
//#define AREA_SMALL_DIM 128
// 2.338
//#define AREA_SMALL_DIM 64
// 2.358
//#define AREA_SMALL_DIM 32
// 2.477
#define AREA_SMALL (AREA_SMALL_DIM * AREA_SMALL_DIM)

#ifdef VARIANT_ACCESS

// User defined:
//typedef uint32_t AccessWordType;
//#define ACCESS_WORD_BITS (8)
#define ACCESS_WORD_BITS (32)
//#define ACCESS_WORD_BITS (64)
// ACCESS_WORD_BITS must be equal to (sizeof(AccessWordType) / CHAR_BIT)
//#define ACCESS_BYTES (2)
//#define ACCESS_BYTES (8)
#define ACCESS_BYTES (16)
// ACCESS_BYTES must be a multiple of sizeof(AccessWordType)

// Automatic:
#define BYTE_BITS (CHAR_BIT)
//assert(ACCESS_WORD_BITS % BYTE_BITS == 0);
#define ACCESS_WORD_BYTES (ACCESS_WORD_BITS / BYTE_BITS)

// Define access_htobe and access_betoh
#if (ACCESS_WORD_BITS == 8)
typedef uint8_t AccessWordType;
#define access_htobe
#define access_betoh
#elif (ACCESS_WORD_BITS == 16)
typedef uint16_t AccessWordType;
#define access_htobe htobe16
#define access_betoh be16toh
#elif (ACCESS_WORD_BITS == 32)
typedef uint32_t AccessWordType;
#define access_htobe htobe32
#define access_betoh be32toh
#elif (ACCESS_WORD_BITS == 64)
typedef uint64_t AccessWordType;
#define access_htobe htobe64
#define access_betoh be64toh
#else
#error Unsupported ACCESS_WORD_BITS; must be 8, 16, 32 or 64.
#endif

#if (ACCESS_WORD_BYTES == 1)
#define access_htobe
#define access_betoh
#elif (ACCESS_WORD_BYTES == 2)
#define access_htobe htobe16
#define access_betoh be16toh
#elif (ACCESS_WORD_BYTES == 4)
#define access_htobe htobe32
#define access_betoh be32toh
#elif (ACCESS_WORD_BYTES == 8)
#define access_htobe htobe64
#define access_betoh be64toh
#else
#error Unsupported ACCESS_WORD_BYTES; must be 2, 4 or 8.
#endif

typedef __attribute__((aligned(ACCESS_BYTES), vector_size(ACCESS_BYTES)))
	AccessWordType AccessType;
//assert(ACCESS_BYTES % ACCESS_WORD_BYTES == 0);
#define ACCESS_WORDS (ACCESS_BYTES / ACCESS_WORD_BYTES)

#define ACCESS_BITS (ACCESS_BYTES * BYTE_BITS)
#define ACCESS_AREA_SMALL_DIM (AREA_SMALL_DIM / ACCESS_BITS)
#define ACCESS_AREA_SMALL (ACCESS_AREA_SMALL_DIM * ACCESS_AREA_SMALL_DIM)

#if (ACCESS_WORDS == 1)
static const AccessType ACCESS_ONE = {1};
static const AccessType ACCESS_ZERO = {0};
#elif (ACCESS_WORDS == 2)
static const AccessType ACCESS_ONE = {1, 1};
static const AccessType ACCESS_ZERO = {0, 0};
#elif (ACCESS_WORDS == 4)
static const AccessType ACCESS_ONE = {1, 1, 1, 1};
static const AccessType ACCESS_ZERO = {0, 0, 0, 0};
#else
#error Unsupported ACCESS_WORDS; must be 1, 2 or 4.
#endif

static inline __attribute__((hot))
AccessWordType access_word_decode(const AccessWordType word)
{
#if (ACCESS_WORD_BITS == 32)
	return __builtin_bswap32(word);
	/*return (word << 24) | ((word << 8) & 0x00ff0000)
		| ((word >> 8) & 0x0000ff00) | (word >> 24);*/
#elif (ACCESS_WORD_BITS == 64)
	return __builtin_bswap64(word);
	/*return ((((word) & 0xff00000000000000ull) >> 56)
          | (((word) & 0x00ff000000000000ull) >> 40)
          | (((word) & 0x0000ff0000000000ull) >> 24)
          | (((word) & 0x000000ff00000000ull) >>  8)
          | (((word) & 0x00000000ff000000ull) <<  8)
          | (((word) & 0x0000000000ff0000ull) << 24)
          | (((word) & 0x000000000000ff00ull) << 40)
          | (((word) & 0x00000000000000ffull) << 56));*/
#else
	return access_htobe(word);
#endif
}

static inline __attribute__((hot))
AccessWordType access_word_encode(const AccessWordType word)
{
#if (ACCESS_WORD_BITS == 32)
	return __builtin_bswap32(word);
	/*return (word << 24) | ((word << 8) & 0x00ff0000)
		| ((word >> 8) & 0x0000ff00) | (word >> 24);*/
#elif (ACCESS_WORD_BITS == 64)
	return __builtin_bswap64(word);
	/*return ((((word) & 0xff00000000000000ull) >> 56)
          | (((word) & 0x00ff000000000000ull) >> 40)
          | (((word) & 0x0000ff0000000000ull) >> 24)
          | (((word) & 0x000000ff00000000ull) >>  8)
          | (((word) & 0x00000000ff000000ull) <<  8)
          | (((word) & 0x0000000000ff0000ull) << 24)
          | (((word) & 0x000000000000ff00ull) << 40)
          | (((word) & 0x00000000000000ffull) << 56));*/
#else
	return access_betoh(word);
#endif
}

static inline __attribute__((hot))
AccessType decode(const AccessType n)
{
	AccessType result;
	for (unsigned int i = 0; i < ACCESS_WORDS; i++) {
		result[i] = access_word_decode(n[i]);
	}
	return result;
}

static inline __attribute__((hot))
AccessType encode(const AccessType n)
{
	AccessType result;
	for (unsigned int i = 0; i < ACCESS_WORDS; i++) {
		result[i] = access_word_encode(n[i]);
	}
	return result;
}

static inline __attribute__((hot))
void rotate_part_atomic(
	const AccessType * const restrict in, AccessType * const restrict out,
	const unsigned int rowwidth,
	const unsigned int in_x, const unsigned int in_y,
	const unsigned int out_x, const unsigned int out_y)
{
#ifdef OPT_GET_CACHE
	AccessType get_rows[ACCESS_BITS];
	for (unsigned int y = 0; y < ACCESS_BITS; y++) {
		get_rows[y] = decode(in[(in_y * ACCESS_BITS + y) * rowwidth + in_x]);
	}
#endif // OPT_GET_CACHE
	/*for (unsigned int get_y = 0; get_y < ACCESS_BITS; get_y++) {
		unsigned int put_x = ACCESS_BITS - 1 - get_y;
		unsigned int put_word = put_x / ACCESS_WORD_BITS;
		unsigned int put_bit = put_x % ACCESS_WORD_BITS;
		AccessType get_data_row = decode(in[(in_y * ACCESS_BITS + get_y) *
			rowwidth + in_x]);
		for (unsigned int get_x = 0; get_x < ACCESS_BITS; get_x++) {
			unsigned int put_y = get_x;
			unsigned int get_word = get_x / ACCESS_WORD_BITS;
			unsigned int get_bit = get_x % ACCESS_WORD_BITS;
			AccessWordType get_data = (get_data_row[get_word] >>
				(ACCESS_WORD_BITS - 1 - get_bit)) & 1;
			AccessWordType put_data = get_data <<
				(ACCESS_WORD_BITS - 1 - put_bit);
			//put_rows[put_y][put_word] = put_rows[put_y][put_word] | put_data;
			unsigned int put_index = put_y * ACCESS_WORDS + put_word;
			put_rows_words[put_index] = put_rows_words[put_index] | put_data;
		}
	}*/
#if (ACCESS_WORDS == 2)
	AccessType put_rows[ACCESS_BITS];
	memset(put_rows, 0, ACCESS_BITS * ACCESS_BYTES);
	for (unsigned int get_y = 0; get_y < ACCESS_WORD_BITS; get_y++) {
		unsigned int put_x = ACCESS_WORD_BITS - 1 - get_y;
		AccessType get_data[ACCESS_WORDS];
		for (unsigned int i = 0; i < ACCESS_WORDS; i++) {
#ifdef OPT_GET_CACHE
			get_data[i] = get_rows[get_y + (i * ACCESS_WORD_BITS)];
#else
			get_data[i] = decode(in[((in_y * ACCESS_BITS) + get_y +
				(i * ACCESS_WORD_BITS)) * rowwidth + in_x]);
#endif // OPT_GET_CACHE
		}
		AccessType mid_data[ACCESS_WORDS];
		mid_data[0] = __builtin_shuffle(get_data[0], get_data[1], {2, 0});
		mid_data[1] = __builtin_shuffle(get_data[0], get_data[1], {3, 1});
		for (unsigned int get_x = 0; get_x < ACCESS_WORD_BITS; get_x++) {
			unsigned int put_y = get_x;
			AccessType put_data[ACCESS_WORDS];
			for (unsigned int i = 0; i < ACCESS_WORDS; i++) {
				put_data[i] = mid_data[i];
				put_data[i] = put_data[i] >> (ACCESS_WORD_BITS - 1 - get_x);
				put_data[i] = put_data[i] & ACCESS_ONE;
				put_data[i] = put_data[i] << (ACCESS_WORD_BITS - 1 - put_x);
				put_rows[put_y + (i * ACCESS_WORD_BITS)] |= put_data[i];
			}
		}
	}
	for (unsigned int put_y = 0; put_y < ACCESS_BITS; put_y++) {
		out[(out_y * ACCESS_BITS + put_y) * rowwidth + out_x] =
			encode(put_rows[put_y]);
		/*for (unsigned int x = 0; x < ACCESS_BITS; x++) {
			unsigned int p_word = x / ACCESS_WORD_BITS;
			unsigned int p_bit = ACCESS_WORD_BITS - 1 - (x % ACCESS_WORD_BITS);
			bool p = (out_rows[y][p_word] >> (p_bit)) & 1;
			putchar(".#"[p]);
		}
		putchar('\n');*/
	}
#else
	for (unsigned int put_y = 0; put_y < ACCESS_BITS; put_y++) {
		AccessType put_data_access = ACCESS_ZERO;
		unsigned int get_x = put_y;
		unsigned int get_word = get_x / ACCESS_WORD_BITS;
		unsigned int get_bit = get_x % ACCESS_WORD_BITS;
		for (unsigned int put_x = 0; put_x < ACCESS_BITS; put_x++) {
			unsigned int put_word = put_x / ACCESS_WORD_BITS;
			unsigned int put_bit = put_x % ACCESS_WORD_BITS;
			unsigned int get_y = ACCESS_BITS - 1 - put_x;
#ifdef OPT_GET_CACHE
			AccessType get_data_access = get_rows[get_y];
#else
			AccessType get_data_access =
				decode(in[(in_y * ACCESS_BITS + get_y) * rowwidth + in_x]);
#endif // OPT_GET_CACHE
			AccessWordType get_data_word = (get_data_access[get_word] >>
				(ACCESS_WORD_BITS - 1 - get_bit)) & 1;
			AccessWordType put_data_word = get_data_word <<
				(ACCESS_WORD_BITS - 1 - put_bit);
			put_data_access[put_word] = put_data_access[put_word] |
				put_data_word;
		}
		out[(out_y * ACCESS_BITS + put_y) * rowwidth + out_x] =
			encode(put_data_access);
	}
#endif
}

static inline void
rotate_part_small(
	const AccessType * const restrict in, AccessType * const restrict out,
	const unsigned int rowwidth,
	const unsigned int in_x, const unsigned int in_y,
	const unsigned int out_x, const unsigned int out_y,
	const unsigned int width, const unsigned int height)
{
#ifdef OPT_OMP_SMALL_Y
#pragma omp parallel for
#endif // OPT_OMP_SMALL_Y
	for (unsigned int y = 0; y < height; y++) {
		unsigned int sub_in_y = in_y + y;
		unsigned int sub_out_x = out_x + height - 1 - y;
#ifdef OPT_OMP_SMALL_X
#pragma omp parallel for
#endif // OPT_OMP_SMALL_X
		for (unsigned int x = 0; x < width; x++) {
			unsigned int sub_in_x = in_x + x;
			unsigned int sub_out_y = out_y + x;
			rotate_part_atomic(in, out, rowwidth, sub_in_x, sub_in_y,
				sub_out_x, sub_out_y);
		}
	}
}

#ifdef OPT_ROTATE_LARGE
static void
rotate_part_large(
	const AccessType * const restrict in, AccessType * const restrict out,
	const unsigned int rowwidth,
	const unsigned int in_x, const unsigned int in_y,
	const unsigned int out_x, const unsigned int out_y,
	const unsigned int width, const unsigned int height)
{
	if (width * height <= ACCESS_AREA_SMALL) { // small
		rotate_part_small(in, out, rowwidth, in_x, in_y, out_x, out_y,
			width, height);
	} else { // large
		if (width > height) { // vertical cut
			assert(width % 2 == 0);
			const unsigned int sub_width = width / 2;
			const unsigned int sub_height = height;
			
			const unsigned int sub_in_y = in_y;
			const unsigned int sub_out_x = out_x;
#ifdef OPT_OMP_LARGE
#pragma omp parallel for
#endif // OPT_OMP_LARGE
			for (unsigned int part = 0; part < 2; part++) {
				unsigned int sub_in_x = 0;
				unsigned int sub_out_y = 0;
				switch (part) {
					case 0:
						sub_in_x = in_x;
						sub_out_y = out_y;
						break;
					case 1:
						sub_in_x = in_x + sub_width;
						sub_out_y = out_y + sub_width;
						break;
				}
				rotate_part_large(in, out, rowwidth, sub_in_x, sub_in_y,
					sub_out_x, sub_out_y, sub_width, sub_height);
			}
		} else { // horizontal cut
			assert(height % 2 == 0);
			const unsigned int sub_height = height / 2;
			const unsigned int sub_width = width;
			
			const unsigned int sub_in_x = in_x;
			const unsigned int sub_out_y = out_y;
#ifdef OPT_OMP_LARGE
#pragma omp parallel for
#endif // OPT_OMP_LARGE
			for (unsigned int part = 0; part < 2; part++) {
				unsigned int sub_in_y = 0;
				unsigned int sub_out_x = 0;
				switch (part) {
					case 0:
						sub_in_y = in_y;
						sub_out_x = out_x + sub_height;
						break;
					case 1:
						sub_in_y = in_y + sub_height;
						sub_out_x = out_x;
						break;
				}
				rotate_part_large(in, out, rowwidth, sub_in_x, sub_in_y,
					sub_out_x, sub_out_y, sub_width, sub_height);
			}
		}
		
		/*assert(width % 2 == 0);
		assert(height % 2 == 0);
		const unsigned int sub_width = width / 2;
		const unsigned int sub_height = height / 2;
#ifdef OPT_OMP_LARGE
#pragma omp parallel for
#endif // OPT_OMP_LARGE
		for (unsigned int quarter = 0; quarter < 4; quarter++) {
			unsigned int sub_in_x = 0;
			unsigned int sub_in_y = 0;
			unsigned int sub_out_x = 0;
			unsigned int sub_out_y = 0;
			switch (quarter) {
				case 0:
					sub_in_x = in_x;
					sub_in_y = in_y;
					sub_out_x = out_x + sub_width;
					sub_out_y = out_y;
					break;
				case 1:
					sub_in_x = in_x + sub_width;
					sub_in_y = in_y;
					sub_out_x = out_x + sub_width;
					sub_out_y = out_y + sub_height;
					break;
				case 2:
					sub_in_x = in_x + sub_width;
					sub_in_y = in_y + sub_height;
					sub_out_x = out_x;
					sub_out_y = out_y + sub_height;
					break;
				case 3:
					sub_in_x = in_x;
					sub_in_y = in_y + sub_height;
					sub_out_x = out_x;
					sub_out_y = out_y;
					break;
			}
			rotate_part_large(in, out, rowwidth, sub_in_x, sub_in_y,
				sub_out_x, sub_out_y, sub_width, sub_height);
		}*/
	}
}
#endif // OPT_ROTATE_LARGE

#endif // VARIANT_ACCESS

void
exercise(struct image * restrict in, struct image * restrict out)
{
#ifdef VARIANT_SIMPLE
	for (unsigned int y = 0; y < in->rows; y++) {
		for (unsigned int x = 0; x < in->cols; x++) {
			bool p = image_getpixel(in, x, y);
			image_putpixel(out, in->cols - 1 - y, in->rows - 1 - x, p);
		}
	}
#endif // VARIANT_SIMPLE
#ifdef VARIANT_ACCESS
	const AccessType * const in_access =
		(const AccessType * const)in->bitmap;
	AccessType * const out_access = (AccessType * const)out->bitmap;

	assert(image_rowbytes(in) == image_rowbytes(out));
	assert(image_rowbytes(in) % ACCESS_BYTES == 0);
	const unsigned int rowwidth_access = image_rowbytes(in) / ACCESS_BYTES;

	assert(in->cols == out->rows);
	assert(in->cols % ACCESS_BITS == 0);
	const unsigned int width_access = in->cols / ACCESS_BITS;

	assert(in->rows == out->cols);
	assert(in->rows % ACCESS_BITS == 0);
	const unsigned int height_access = in->rows / ACCESS_BITS;

#ifdef OPT_ROTATE_LARGE
	rotate_part_large(in_access, out_access, rowwidth_access, 0, 0, 0, 0,
		width_access, height_access);
#else
	rotate_part_small(in_access, out_access, rowwidth_access, 0, 0, 0, 0,
		width_access, height_access);
#endif // OPT_ROTATE_LARGE
	// TODO Tweak small / large - divide parts between cores efficiently.
#endif // VARIANT_ACCESS
}
