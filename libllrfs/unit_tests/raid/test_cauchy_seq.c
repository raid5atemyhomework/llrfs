#undef NDEBUG
#if defined(HAVE_CONFIG_H)
# include"config.h"
#endif
#include"raid/llr_cauchy_seq.h"
#include"raid/llr_gf.h"
#include<assert.h>
#include<string.h>

static
unsigned char matrix[128][128];

/* Comppute the determinant of an n x n submatrix at offset oi,oj.  */
unsigned char compute_determinant(unsigned int oi, unsigned int oj, unsigned int n) {
	int i, ii, j, jj;

	unsigned char up_sum, dn_sum;
	unsigned char up_prd, dn_prd;

	up_sum = dn_sum = 0;
	for (i = 0; i < n; ++i) {
		up_prd = dn_prd = 1;
		for (j = 0; j < n; ++j) {
			jj = oj + j;
			if (jj >= 128)
				jj -= 128;
			/* Compute the up_prd.  */
			ii = oi + i - j;
			if (ii < 0)
				ii += 128;
			if (ii >= 128)
				ii -= 128;
			up_prd = llr_gf_mul(up_prd, matrix[ii][jj]);
			/* Compute the dn_prd.  */
			ii = oi + i + j;
			if (ii >= 128)
				ii -= 128;
			dn_prd = llr_gf_mul(dn_prd, matrix[ii][jj]);
		}
		/* Sum up.  */
		up_sum = llr_gf_add(up_sum, up_prd);
		dn_sum = llr_gf_add(dn_sum, dn_prd);
	}

	return llr_gf_sub(dn_sum, up_sum);
}

int main(void) {
	unsigned char xy[256];
	unsigned int i, ii, j;

	/* Copy x and y sequences into a single large sequence.  */
	memcpy(&xy[0], llr_cauchy_seq_x, 128);
	memcpy(&xy[128], llr_cauchy_seq_y, 128);

	/* No factor repeats in the combined sequence.  */
	for (i = 0; i < 255; ++i) {
		for (ii = i + 1; ii < 256; ++ii) {
			assert(xy[i] != xy[ii]);
		}
	}

	/* Neither C nor D have 0.  */
	for (i = 0; i < 128; ++i) {
		assert(llr_cauchy_seq_c[i] != 0);
		assert(llr_cauchy_seq_d[i] != 0);
	}

	/* Generate the matrix.  */
	for (i = 0; i < 128; ++i) {
		for (j = 0; j < 128; ++j) {
			matrix[i][j] = llr_gf_mul(llr_cauchy_seq_c[i], llr_gf_mul(llr_cauchy_seq_d[j], llr_gf_reciprocal(llr_cauchy_seq_x[i] ^ llr_cauchy_seq_y[j])));
		}
	}

	/* Topmost row must be all 1s.  */
	for (i = 0; i < 128; ++i) {
		assert(matrix[i][0] == 1);
	}
	/* Leftmost column must be all 1s.  */
	for (j = 0; j < 128; ++j) {
		assert(matrix[0][j] == 1);
	}

	/* The determinant must not be 0 (else it would be singular and
	 * thus non-invertible).  */
	/* Pretty much just random spot checks.  */
	assert(compute_determinant(0, 0, 128) != 0);
	assert(compute_determinant(1, 2, 64) != 0);
	assert(compute_determinant(28, 28, 100) != 0);
	assert(compute_determinant(5, 2, 12) != 0);
	assert(compute_determinant(120, 120, 8) != 0);
	assert(compute_determinant(0, 0, 2) != 0);
	assert(compute_determinant(1, 1, 3) != 0);
	assert(compute_determinant(3, 2, 4) != 0);
	assert(compute_determinant(4, 4, 5) != 0);

	return 0;
}
