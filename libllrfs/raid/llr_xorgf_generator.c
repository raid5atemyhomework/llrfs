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
#if defined(HAVE_CONFIG_H)
# include"config.h"
#endif
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>

/* 8x8 Boolean Matrix.  */
typedef struct {
	uint64_t num;
} matrix;

/* Raw identity matrix.  */
static
bool const mul1_base[64] = {
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 0, 0, 0, 0,
	0, 0, 1, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 1
};
/* Raw multiply-by-2 matrix.
 * To change the polynomial, modify the
 * rightmost column.
 * In that column, row 0 is 1, row 1 is
 * x, row 2 is x^2, etc. and x^8 is
 * implied.
 * For example for x^8 + x^4 + x^3 + x + 1,
 * the rightmost column should be:
 *   1
 *   1
 *   0
 *   1
 *   1
 *   0
 *   0
 *   0
 */
static
bool const mul2_base[64] = {
	0, 0, 0, 0, 0, 0, 0, 1,
	1, 0, 0, 0, 0, 0, 0, 0,
	0, 1, 0, 0, 0, 0, 0, 1,
	0, 0, 1, 0, 0, 0, 0, 1,
	0, 0, 0, 1, 0, 0, 0, 1,
	0, 0, 0, 0, 1, 0, 0, 0,
	0, 0, 0, 0, 0, 1, 0, 0,
	0, 0, 0, 0, 0, 0, 1, 0
};

/* Convert a raw matrix to our compact form.  */
static
matrix from_base(bool const* base) {
	int i;
	matrix rv;

	rv.num = 0;
	for (i = 0; i < 64; ++i) {
		rv.num = rv.num >> 1;
		if (base[i])
			rv.num |= 0x8000000000000000ULL;
	}

	return rv;
}

/* Address individual bits in the matrix.  */
static
bool matrix_get(matrix m, unsigned int i, unsigned int j) {
	return !!(m.num & (((uint64_t) 1) << (i + j * 8)));
}
static
void matrix_set(matrix* m, unsigned int i, unsigned int j, bool bit) {
	if (bit) {
		uint64_t mask = (((uint64_t) 1) << (i + j * 8));
		m->num |= mask;
	} else {
		uint64_t mask = ~(((uint64_t) 1) << (i + j * 8));
		m->num &= mask;
	}
}

/* Add two matrices together.  */
static
matrix matrix_add(matrix a, matrix b) {
	matrix rv;
	rv.num = a.num ^ b.num;
	return rv;
}
/* Multiply two matrices together.  */
static
matrix matrix_mul(matrix a, matrix b) {
	matrix rv;
	unsigned int i, j, k;

	rv.num = 0;
	for (i = 0; i < 8; ++i) {
		for (j = 0; j < 8; ++j) {
			bool bit = 0;
			for (k = 0; k < 8; ++k) {
				bit ^= matrix_get(a, i, k) & matrix_get(b, k, j);
			}
			matrix_set(&rv, i, j, bit);
		}
	}

	return rv;
}

/* All the matrices.  */
static
matrix all[256];

static
void all_init(void) {
	unsigned int i, j;

	/* Initialize 0, 1, 2.  */
	all[0].num = 0;
	all[1] = from_base(mul1_base);
	all[2] = from_base(mul2_base);
	/* Initialize powers of 2.  */
	for (i = 4; i < 256; i <<= 1) {
		all[i] = matrix_mul(all[i >> 1], all[2]);
	}

	/* Load all.  */
	for (i = 3; i < 256; ++i) {
		/* Skip powers of two.  */
		if (i == 4 || i == 8 || i == 16 || i == 32 || i == 64 || i == 128)
			continue;

		/* Start with 0.  */
		all[i] = all[0];
		/* Add in powers of two.  */
		for (j = 1; j < 256; j <<= 1) {
			if ((i & j) != 0)
				all[i] = matrix_add(all[i], all[j]);
		}
	}
}

static
void print_license(void) {
	printf("/*\n");
	printf("llr_xorgf_generator has the license below, which\n");
	printf("also applies to this generated code, as it contains\n");
	printf("substantial parts of llr_xorgf_generator:\n");
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

static
void make_mul_macro(unsigned int index) {
	matrix curr = all[index];
	unsigned int i, j;

	/* Dump the matrix.  */
	printf("/* %d\n", index);
	for (j = 0; j < 8; ++j) {
		for (i = 0; i < 8; ++i) {
			if (matrix_get(curr, i, j))
				printf(" 1");
			else
				printf(" 0");
		}
		printf("\n");
	}
	printf("*/\n");
	printf("#define MUL%d(r0, r1, r2, r3, r4, r5, r6, r7, a0, a1, a2, a3, a4, a5, a6, a7) \\\n",
	       index);
	printf("\tdo { \\\n");

	for (j = 0; j < 8; ++j) {
		printf("\t\tunit_type tmp_mul_a%d_ = (a%d); \\\n", j, j);
	}

	/* TODO: optimize number of XORs.  */
	for (j = 0; j < 8; ++j) {
		bool first = true;
		printf("\t\tr%d =", j);
		for (i = 0; i < 8; ++i) {
			if (!matrix_get(curr, i, j))
				continue;
			if (first)
				first = false;
			else
				printf(" ^");
			printf(" tmp_mul_a%d_", i);
		}
		if (first)
			printf(" 0");
		printf("; \\\n");
	}

	printf("\t} while(0)\n");
	printf("\n");
}

void make_acc_mul(void) {
	unsigned int i, j;

	/* Generate the individual accumulator functions.  */
	printf("static void llr_xorgf_acc_mul_0(void* restrict acc, void const* restrict a) { /* Do Nothing.  */ }\n");
	printf("static void llr_xorgf_acc_mul_1(void* restrict orig_acc, void const* restrict orig_a) {\n");
	printf("\tunit_type* acc = (unit_type*) orig_acc;\n");
	printf("\tunit_type const* a = (unit_type const*) orig_a;\n");
	printf("\tunsigned int i;\n\n");
	printf("\tfor (i = 0; i < span * 8; ++i) {\n");
	printf("\t\tacc[i] ^= a[i];\n");
	printf("\t}\n");
	printf("}\n");
	for (i = 2; i < 256; ++i) {
		printf("static void llr_xorgf_acc_mul_%u(void* restrict orig_acc, void const* restrict orig_a) {\n", i);
		printf("\tunit_type* acc = (unit_type*) orig_acc;\n");
		printf("\tunit_type const* a = (unit_type const*) orig_a;\n");
		printf("\tunsigned int i;\n");
		for (j = 0; j < 8; ++j)
			printf("\tunit_type t%u;\n", j);
		printf("\n");
		printf("\tfor (i = 0; i < span; ++i) {\n");
		printf("\t\tMUL%u(\n", i);
		for (j = 0; j < 8; ++j) {
			printf("\t\t\tt%u,\n",j);
		}
		for (j = 0; j < 8; ++j) {
			printf("\t\t\ta[%u * span]%s\n", j, j == 7 ? "" : ",");
		}
		printf("\t\t\t);\n");
		for (j = 0; j < 8; ++j) {
			printf("\t\tacc[%u * span] ^= t%u;\n", j, j);
		}
		printf("\t\t++acc;\n");
		printf("\t\t++a;\n");
		printf("\t}\n");
		printf("}\n");
	}

	/* Generate the table.  */
	printf("typedef void (*llr_xorgf_acc_mul_func)(void* restrict acc, void const* restrict a);\n");
	printf("static llr_xorgf_acc_mul_func const llr_xorgf_acc_mul_table[256] = {\n");
	for (i = 0; i < 256; ++i) {
		printf("\tllr_xorgf_acc_mul_%u%s\n",
		       i, (i == 255) ? "" : ",");
	}
	printf("};\n");

	/* Generate the dispatch function.  */
	printf("\nvoid llr_xorgf_acc_mul(void* restrict acc, unsigned char c, void const* restrict a) {\n");
	printf("\tllr_xorgf_acc_mul_table[c](acc, a);");
	printf("}\n");
}

void generate_ones() {
	unsigned int i, j, count;
	uint64_t num;
	printf("#include\"llr_xorgf_ones.h\"\n");
	printf("\n");
	printf("unsigned int const llr_xorgf_ones[256] = {\n");
	for (i = 0; i < 256; ++i) {
		count = 0;
		num = all[i].num;
		for (j = 0; j < 64; ++j) {
			if ((num & 1) == 1)
				++count;
			num >>= 1;
		}
		printf("\t%u%s /* 0x%02x */\n", count, i == 255 ? "" : ",", i);
	}
	printf("};\n");
}

int main(int argc, char** argv) {
	unsigned int i;

	all_init();

	printf("/* This file was generated by llr_xorgf_generator.  */\n");

	print_license();

	if (argc >= 2) {
		generate_ones();
		return 0;
	}

	printf("#include\"llr_xorgf.h\"\n#include<stdint.h>\n#include<string.h>\n\n");
	/* Support completely overriding the unit_type.   */
	printf("#if defined(LLR_XORGF_UNIT_TYPE)\n");
	printf("typedef LLR_XORGF_UNIT_TYPE unit_type;\n");
	printf("#else /* defined(LLR_XORGF_UNIT_TYPE) */ \n");
	/* uint_fast32_5 should be a 32-bit word for 32-bit processors, a
	 * 64-bit word for 64-bit processors.  */
	printf("typedef uint_fast32_t unit_type\n");
	/* Support adding __attribute__((vector_size(x))).  */
	printf("#if defined(LLR_XORGF_VECTOR_SIZE)\n");
	printf("\t__attribute__((vector_size(LLR_XORGF_VECTOR_SIZE)))\n");
	printf("#endif /* defined(LLR_XORGF_VECTOR_SIZE) */\n");
	printf(";\n");
	printf("#endif /* !defined(LLR_XORGF_UNIT_TYPE)*/ \n");
	printf("\n");
	printf("static unsigned int const span = (LLR_XORGF_BLOCK_SIZE / 8) / sizeof(unit_type);\n\n");

	for (i = 2; i < 256; ++i)
		make_mul_macro(i);

	make_acc_mul();

	return 0;
}
