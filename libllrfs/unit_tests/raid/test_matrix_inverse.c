#if defined(HAVE_CONFIG_H)
# include"config.h"
#endif
#undef NDEBUG
#include"raid/llr_matrix_inverse.h"
#include<assert.h>
#include<stdio.h>

static void
print3x3(unsigned char mat[9]) {
	unsigned int i, j;
	for (j = 0; j < 3; ++j) {
		for (i = 0; i < 3; ++i)
			printf("%4u", mat[i + j * 3]);
		printf("\n");
	}
}

/* Test 3x3 matrix inversion.  */
static void
test3x3(unsigned char a00, unsigned char a10, unsigned char a20,
        unsigned char a01, unsigned char a11, unsigned char a21,
	unsigned char a02, unsigned char a12, unsigned char a22) {


	unsigned char mat[9];
	unsigned char scratch[18];

	mat[0] = a00;
	mat[1] = a10;
	mat[2] = a20;
	mat[3] = a01;
	mat[4] = a11;
	mat[5] = a21;
	mat[6] = a02;
	mat[7] = a12;
	mat[8] = a22;

	printf("Input matrix:\n");
	print3x3(mat);

	/* Inverting twice should result in the same matrix.  */
	llr_matrix_inverse_compute(3, scratch, mat);
	printf("Inverse matrix:\n");
	print3x3(mat);
	llr_matrix_inverse_compute(3, scratch, mat);
	printf("Inverse-inverse matrix:\n");
	print3x3(mat);

	assert(mat[0] == a00);
	assert(mat[1] == a10);
	assert(mat[2] == a20);
	assert(mat[3] == a01);
	assert(mat[4] == a11);
	assert(mat[5] == a21);
	assert(mat[6] == a02);
	assert(mat[7] == a12);
	assert(mat[8] == a22);
}

int main(void) {
	test3x3(1, 0, 0,
		0, 1, 0,
		0, 0, 1);
	test3x3(2, 0, 0,
		0, 2, 0,
		0, 0, 2);
	test3x3(1, 1, 1,
		1, 2, 4,
		1, 4, 16);
	test3x3(1, 0, 0,
		1, 2, 4,
		1, 4, 16);
	test3x3(0x02, 0x8E, 0x04,
		0xE2, 0xC8, 0x80,
		0x66, 0x8B, 0xA0);
	/* Glider!  */
	test3x3(0, 1, 0,
		0, 0, 1,
		1, 1, 1);
	return 0;
}
