/*
Copyright 2022 raid5atemyhomework

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
#include"llr_xorgf_ones.h"
#include<assert.h>
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

/* Our own GF(2^8) tables, because we cannot use
 * the library (since we generate files for the
 * library).
 */
static const uint8_t log[256] = {
	0xFF, 0x00, 0x01, 0x19, 0x02, 0x32, 0x1a, 0xc6,
	0x03, 0xdf, 0x33, 0xee, 0x1b, 0x68, 0xc7, 0x4b,
	0x04, 0x64, 0xe0, 0x0e, 0x34, 0x8d, 0xef, 0x81,
	0x1c, 0xc1, 0x69, 0xf8, 0xc8, 0x08, 0x4c, 0x71,
	0x05, 0x8a, 0x65, 0x2f, 0xe1, 0x24, 0x0f, 0x21,
	0x35, 0x93, 0x8e, 0xda, 0xf0, 0x12, 0x82, 0x45,
	0x1d, 0xb5, 0xc2, 0x7d, 0x6a, 0x27, 0xf9, 0xb9,
	0xc9, 0x9a, 0x09, 0x78, 0x4d, 0xe4, 0x72, 0xa6,
	0x06, 0xbf, 0x8b, 0x62, 0x66, 0xdd, 0x30, 0xfd,
	0xe2, 0x98, 0x25, 0xb3, 0x10, 0x91, 0x22, 0x88,
	0x36, 0xd0, 0x94, 0xce, 0x8f, 0x96, 0xdb, 0xbd,
	0xf1, 0xd2, 0x13, 0x5c, 0x83, 0x38, 0x46, 0x40,
	0x1e, 0x42, 0xb6, 0xa3, 0xc3, 0x48, 0x7e, 0x6e,
	0x6b, 0x3a, 0x28, 0x54, 0xfa, 0x85, 0xba, 0x3d,
	0xca, 0x5e, 0x9b, 0x9f, 0x0a, 0x15, 0x79, 0x2b,
	0x4e, 0xd4, 0xe5, 0xac, 0x73, 0xf3, 0xa7, 0x57,
	0x07, 0x70, 0xc0, 0xf7, 0x8c, 0x80, 0x63, 0x0d,
	0x67, 0x4a, 0xde, 0xed, 0x31, 0xc5, 0xfe, 0x18,
	0xe3, 0xa5, 0x99, 0x77, 0x26, 0xb8, 0xb4, 0x7c,
	0x11, 0x44, 0x92, 0xd9, 0x23, 0x20, 0x89, 0x2e,
	0x37, 0x3f, 0xd1, 0x5b, 0x95, 0xbc, 0xcf, 0xcd,
	0x90, 0x87, 0x97, 0xb2, 0xdc, 0xfc, 0xbe, 0x61,
	0xf2, 0x56, 0xd3, 0xab, 0x14, 0x2a, 0x5d, 0x9e,
	0x84, 0x3c, 0x39, 0x53, 0x47, 0x6d, 0x41, 0xa2,
	0x1f, 0x2d, 0x43, 0xd8, 0xb7, 0x7b, 0xa4, 0x76,
	0xc4, 0x17, 0x49, 0xec, 0x7f, 0x0c, 0x6f, 0xf6,
	0x6c, 0xa1, 0x3b, 0x52, 0x29, 0x9d, 0x55, 0xaa,
	0xfb, 0x60, 0x86, 0xb1, 0xbb, 0xcc, 0x3e, 0x5a,
	0xcb, 0x59, 0x5f, 0xb0, 0x9c, 0xa9, 0xa0, 0x51,
	0x0b, 0xf5, 0x16, 0xeb, 0x7a, 0x75, 0x2c, 0xd7,
	0x4f, 0xae, 0xd5, 0xe9, 0xe6, 0xe7, 0xad, 0xe8,
	0x74, 0xd6, 0xf4, 0xea, 0xa8, 0x50, 0x58, 0xaf
};
static const uint8_t pow[256] = {
	0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80,
	0x1d, 0x3a, 0x74, 0xe8, 0xcd, 0x87, 0x13, 0x26,
	0x4c, 0x98, 0x2d, 0x5a, 0xb4, 0x75, 0xea, 0xc9,
	0x8f, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0xc0,
	0x9d, 0x27, 0x4e, 0x9c, 0x25, 0x4a, 0x94, 0x35,
	0x6a, 0xd4, 0xb5, 0x77, 0xee, 0xc1, 0x9f, 0x23,
	0x46, 0x8c, 0x05, 0x0a, 0x14, 0x28, 0x50, 0xa0,
	0x5d, 0xba, 0x69, 0xd2, 0xb9, 0x6f, 0xde, 0xa1,
	0x5f, 0xbe, 0x61, 0xc2, 0x99, 0x2f, 0x5e, 0xbc,
	0x65, 0xca, 0x89, 0x0f, 0x1e, 0x3c, 0x78, 0xf0,
	0xfd, 0xe7, 0xd3, 0xbb, 0x6b, 0xd6, 0xb1, 0x7f,
	0xfe, 0xe1, 0xdf, 0xa3, 0x5b, 0xb6, 0x71, 0xe2,
	0xd9, 0xaf, 0x43, 0x86, 0x11, 0x22, 0x44, 0x88,
	0x0d, 0x1a, 0x34, 0x68, 0xd0, 0xbd, 0x67, 0xce,
	0x81, 0x1f, 0x3e, 0x7c, 0xf8, 0xed, 0xc7, 0x93,
	0x3b, 0x76, 0xec, 0xc5, 0x97, 0x33, 0x66, 0xcc,
	0x85, 0x17, 0x2e, 0x5c, 0xb8, 0x6d, 0xda, 0xa9,
	0x4f, 0x9e, 0x21, 0x42, 0x84, 0x15, 0x2a, 0x54,
	0xa8, 0x4d, 0x9a, 0x29, 0x52, 0xa4, 0x55, 0xaa,
	0x49, 0x92, 0x39, 0x72, 0xe4, 0xd5, 0xb7, 0x73,
	0xe6, 0xd1, 0xbf, 0x63, 0xc6, 0x91, 0x3f, 0x7e,
	0xfc, 0xe5, 0xd7, 0xb3, 0x7b, 0xf6, 0xf1, 0xff,
	0xe3, 0xdb, 0xab, 0x4b, 0x96, 0x31, 0x62, 0xc4,
	0x95, 0x37, 0x6e, 0xdc, 0xa5, 0x57, 0xae, 0x41,
	0x82, 0x19, 0x32, 0x64, 0xc8, 0x8d, 0x07, 0x0e,
	0x1c, 0x38, 0x70, 0xe0, 0xdd, 0xa7, 0x53, 0xa6,
	0x51, 0xa2, 0x59, 0xb2, 0x79, 0xf2, 0xf9, 0xef,
	0xc3, 0x9b, 0x2b, 0x56, 0xac, 0x45, 0x8a, 0x09,
	0x12, 0x24, 0x48, 0x90, 0x3d, 0x7a, 0xf4, 0xf5,
	0xf7, 0xf3, 0xfb, 0xeb, 0xcb, 0x8b, 0x0b, 0x16,
	0x2c, 0x58, 0xb0, 0x7d, 0xfa, 0xe9, 0xcf, 0x83,
	0x1b, 0x36, 0x6c, 0xd8, 0xad, 0x47, 0x8e, 0x01
};
uint8_t gf_mul(uint8_t a, uint8_t b) {
	/* Multiplication by 0 is 0.  */
	if (a == 0 || b == 0)
		return 0;

	unsigned int log_a = log[a];
	unsigned int log_b = log[b];
	unsigned int log_ab = log_a + log_b;
	if (log_ab > 255)
		log_ab -= 255;
	return pow[log_ab];
}
uint8_t gf_reciprocal(uint8_t a) {
	unsigned int log_a = log[a];
	unsigned int log_reciprocal_a = 255 - log_a;
	return pow[log_reciprocal_a];
}

static
void print_license(void) {
	printf("/*\n");
	printf("llr_cauchy_seq_generator has the license below, which\n");
	printf("also applies to this generated code, as it contains\n");
	printf("substantial parts of llr_cauchy_seq_generator:\n");
	printf("\n");
	printf("Copyright 2022 raid5atemyhomework\n");
	printf("\n");
	printf("Permission is hereby granted, free of charge, to any person obtaining a copy\n");
	printf("of this software and associated documentation files (the \"Software\"), to deal\n");
	printf("in the Software without restriction, including without limitation the rights\n");
	printf("to use, copy, modify, merge, publish, distribute, sublicense, and/or sell\n");
	printf("copies of the Software, and to permit persons to whom the Software is\n");
	printf("furnished to do so, subject to the following conditions:\n");
	printf("\n");
	printf("The above copyright notice and this permission notice shall be included in all\n");
	printf("copies or substantial portions of the Software.\n");
	printf("\n");
	printf("THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR\n");
	printf("IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,\n");
	printf("FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE\n");
	printf("AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER\n");
	printf("LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,\n");
	printf("OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE\n");
	printf("SOFTWARE.\n");
	printf("*/\n");
}

/* We arrange the matrix so that the top row (row 0) is all 1s, to
optimize for arrays with low numbers of parity blocks.

To help optimize low numbers of data and parity blocks further, we
want to arrange the matrix so that the row just below the top (row 1)
has factors with low cost (as per llr_xorgf_ones) near the left side.

Below is the "best" possible row 1, which we generate with the
function generate_best_row1.
*/
static
uint8_t best_row1[128];

struct factor_cost {
	uint8_t factor;
	unsigned int cost;
};

static
int factor_cost_compar(void const* vl, void const* vr) {
	struct factor_cost const* l = (struct factor_cost*) vl;
	struct factor_cost const* r = (struct factor_cost*) vr;
	if (l->cost < r->cost)
		return -1;
	if (l->cost > r->cost)
		return 1;
	if (l->factor < r->factor)
		return -1;
	if (l->factor > r->factor)
		return 1;
	return 0;
}

static
void generate_best_row1(void) {
	unsigned int i;

	/* Build the array to be sorted.  */
	struct factor_cost fcs[255];
	/* Skip 0.  */
	for (i = 1; i < 256; ++i) {
		fcs[i - 1].factor = i;
		fcs[i - 1].cost = llr_xorgf_ones[i];
	}

	/* Sort the array.  */
	qsort(fcs, 256, sizeof(fcs[0]), &factor_cost_compar);

	/* Copy the first 128 entries.  */
	for (i = 0; i < 128; ++i) {
		best_row1[i] = fcs[i].factor;
	}
}

static
uint8_t c_seq[128];
static
uint8_t d_seq[128];
static
uint8_t x_seq[128];
static
uint8_t y_seq[128];

static
uint8_t matrix[128][128];


static
void seq_init(void) {
	unsigned int i, j;
	unsigned int jj;

	generate_best_row1();

/*
Given:
   y[0] = 0
   y[1] = adj_factor
   d[0] = 1
   d[1] = 1 / adj_factor
We want to select x[i] and c[i] such that:

    (1)   (c[i] * d[0]) / (x[i] + y[0]) = 1
    (2)   (c[i] * d[1]) / (x[i] + y[1]) = best_row1[i]

So:

    (3)   (c[i] * 1) / (x[i] + 0) = 1; subst givens into (1)
    (4)   c[i] = x[i]; manipulate (3), x[i] != 0
    (5)   c[i] / (adj_factor * (x[i] + adj_factor)) = best_row1[i]; subst givens into (2)
    (6)   c[i] / (adj_factor * x[i] + adj_factor * adj_factor) = best_row1[i]; manipulate (5)
    (7)   x[i] / (adj_factor * x[i] + adj_factor * adj_factor) = best_row1[i]; subst givens into (6)
    (8)   x[i] = adj_factor * x[i] * best_row1[i] + adj_factor * adj_factor * best_row1[i]; manipulate (7)
    (9)   x[i] - adj_factor * x[i] * best_row1[i] = adj_factor * adj_factor * best_row1[i]; manipulate (8)
    (10)  x[i] * (1 - adj_factor * best_row1[i]) = adj_factor * adj_factor * best_row1[i]; manipulate (9)
    (11)  x[i] = adj_factor * adj_factor * best_row1[i] / (1 - adj_factor * best_row1[i]); manipulate (10), 1 - adj_factor * best_row1[i] != 0

Note: The generated sequence C does not match X, despite eqn (4)
above.
This is because d[0] is derived as 0x8C instead.
Nevertheless, the generated sequence causes row 1 to be correct.
*/
	/* Determine a good adjustment factor.*/
	unsigned int adj_factor;
	bool adj_factor_ok;
	for (adj_factor = 1; adj_factor < 256; ++adj_factor) {
		adj_factor_ok = true;
		for (i = 0; i < 128; ++i) {
			if (gf_mul(adj_factor, best_row1[i]) == 0x01) {
				adj_factor_ok = false;
				break;
			}
		}
		if (adj_factor_ok)
			break;
	}
	if (!adj_factor_ok) {
		fprintf(stderr, "Could not find an adj_factor??\n");
		exit(1);
	}
	uint8_t adj_factor_squared = gf_mul(adj_factor, adj_factor);
	/* Load x.  */
	for (i = 0; i < 128; ++i) {
		x_seq[i] = gf_mul(adj_factor_squared, gf_mul(best_row1[i], gf_reciprocal(1 ^ gf_mul(adj_factor, best_row1[i]))));
		if (x_seq[i] == 0) {
			fprintf(stderr, "x[%u] == 0, which is reserved for y[0]\n", i);
			exit(1);
		}
		if (x_seq[i] == adj_factor) {
			fprintf(stderr, "x[%u] == adj_factor(%u), which is reserved for y[1]\n", i, adj_factor);
			exit(1);
		}
	}
	/* Load y.  */
	y_seq[0] = 0;
	y_seq[1] = adj_factor;
	for (j = 2; j < 128; ++j) {
		/* Look for a value that is not already present.  */
		unsigned int candidate;
		for (candidate = 0; candidate < 256; ++candidate) {
			bool found;
			found = false;
			for (i = 0; i < 128; ++i) {
				if (x_seq[i] == candidate) {
					found = true;
					break;
				}
			}
			if (found)
				continue;
			for (jj = 0; jj < j; ++jj) {
				if (y_seq[jj] == candidate) {
					found = true;
					break;
				}
			}
			if (!found)
				break;
		}
		y_seq[j] = candidate;
	}

	/* Generate the matrix using C[i] = 1, D[j] = 1.  */
	for (i = 0; i < 128; ++i) {
		for (j = 0; j < 128; ++j) {
			matrix[i][j] = gf_reciprocal(x_seq[i] ^ y_seq[j]);
		}
	}

	/* Optimization: for each row, divide it by the leftmost
	column.
	This makes the leftmost column of each row into 1, which is
	fastest and requires only 8 XORs.
	In fact, it means that the leftmost column (i.e. the data from
	the first data block) can be copied directly to initialize the
	parity block, then the succeeding data blocks are multiplied
	and accumulated into the parity block.

	In particular, for arrays with low numbers of data blocks, this
	effect is strong, and it also makes RAID1 the degenerate case
	of having only one data block (the leftmost column being 1 means
	the first data block is copied to all "parity" blocks, and if
	there are no other data blocks (i.e. RAID1) in a stripe, then
	each "parity" block is exactly the first data block).

	Sequence D is the per-row multiplier, so make it the reciprocal
	of the first column.
	*/
	for (j = 0; j < 128; ++j) {
		uint8_t factor = gf_reciprocal(matrix[0][j]);
		d_seq[j] = factor;
		/* Change the actual matrix.  */
		if (factor == 0x01)
			continue;
		for (i = 0; i < 128; ++i) {
			matrix[i][j] = gf_mul(matrix[i][j], factor);
		}
	}
	/* Optimization: for each column except the leftmost, divide
	it by the topmost row.
	This makes the topmost row of each column into 1, the fastest
	etc. etc.

	In particular, because the topmost row is all 1s, this makes
	traditional parity RAID5 a degenerate case where there is
	only one parity block.

	By making the leftmost column and topmost row all 1s, we
	optimize for smaller arrays with small numbers of data blocks
	and parity blocks, which we expect to be common.
	*/
	c_seq[0] = 0x01;
	for (i = 1; i < 128; ++i) {
		uint8_t factor = gf_reciprocal(matrix[i][0]);
		c_seq[i] = factor;
		/* Change the actual matrix.  */
		if (factor == 0x01)
			continue;
		for (j = 0; j < 128; ++j) {
			matrix[i][j] = gf_mul(matrix[i][j], factor);
		}
	}
}

static
void print_matrix(void) {
	unsigned int i, j;

	printf("/* best_row1\n");
	for (i = 0; i < 128; ++i) {
		printf(" %02X(%2u)", best_row1[i], llr_xorgf_ones[best_row1[i]]);
	}
	printf("\n*/\n");

	printf("/* FACTOR(COST)\n");
	for (j = 0; j < 128; ++j) {
		for (i = 0; i < 128; ++i) {
			uint8_t in_matrix = matrix[i][j];
			uint8_t in_seq = gf_mul(c_seq[i], gf_mul(d_seq[j], gf_reciprocal(x_seq[i] ^ y_seq[j])));
			assert(in_matrix == in_seq);
			printf(" %02X(%2u)", in_matrix, llr_xorgf_ones[in_matrix]);
		}
		printf("\n");
	}
	printf("*/\n");
}

static
void print_sequence(const char* name, uint8_t const* seq) {
	unsigned int i;

	printf("unsigned char const %s[128] = {\n", name);
	for (i = 0; i < 128; ++i) {
		printf("\t0x%02X%s\n", seq[i], i == 127 ? "" : ",");
	}
	printf("};\n");
}

int main(void) {
	printf("/* This file was generated by llr_cauchy_seq_generator.  */\n");
	print_license();

	printf("#if defined(HAVE_CONFIG_H)\n");
	printf("# include\"config.h\"\n");
	printf("#endif /* defined(HAVE_CONFIG_H) */\n");
	printf("#include\"llr_cauchy_seq.h\"\n");
	printf("\n");
	fflush(stdout);

	seq_init();
	print_matrix();

	print_sequence("llr_cauchy_seq_c", c_seq);
	printf("\n");
	print_sequence("llr_cauchy_seq_d", d_seq);
	printf("\n");
	print_sequence("llr_cauchy_seq_x", x_seq);
	printf("\n");
	print_sequence("llr_cauchy_seq_y", y_seq);

	return 0;
}
