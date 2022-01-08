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
#include"llr_gf.h"
#include"llr_matrix_inverse.h"
#include"llr_util.h"

unsigned int
llr_matrix_inverse_scratch_space_size(unsigned int w) {
	return 2 * w * w;
}

static inline
unsigned char*
mat_at(unsigned int w,
       unsigned char* mat,
       unsigned int i,
       unsigned int j) {
	return &mat[i + j * w * 2];
}

/* Gaussian elimiation.  */
void llr_matrix_inverse(unsigned int w,
			unsigned char* scratch_space) {
	unsigned char* mat = scratch_space;

	unsigned int i, j, jj;

	/* Process each row.  */
	for (j = 0; j < w; ++j) {
		unsigned char e, one_over_e;

		/* Get the diagonal.  */
		e = *mat_at(w, mat, j, j);

		/* Is the diagonal 0?
		 * Make it nonzero.  */
		if (e == 0x00) {
			/* Search for a row that has nonzero
			 * that column.
			 * Note the "w - j" --- we search from
			 * the bottom of the matrix.
			 * This is a bit of cheating, but we
			 * use this procedure for matrices
			 * where there is likely to be a row
			 * of all 1s, and using that row for
			 * this nonzero correction is problematic
			 * due to the all 1s cancelling out
			 * the other rows as well.
			 * That all 1s is not going to be the
			 * bottom of the matrix unless we are
			 * in RAID5 mode, but in RAID5 mode
			 * we do not bother with this matrix
			 * inverse anyway.
			 */
			for (jj = 0; jj < w; ++jj) {
				if (*mat_at(w, mat, j, w - jj - 1) != 0)
					break;
			}
			/* Add that row to this row.  */
			for (i = 0; i < w * 2; ++i) {
				*mat_at(w, mat, i, j) = llr_gf_add(
					*mat_at(w, mat, i, j),
					*mat_at(w, mat, i, w - jj - 1)
				);
			}
			/* The columns to the left of the diagonal
			 * are now possibly non-zero.
			 * Cancel them out.
			 */
			for (jj = 0; jj < j; ++jj) {
				e = *mat_at(w, mat, jj, j);
				if (e == 0)
					continue;
				/* Subtract the corresponding previous
				 * row times e, which was already an
				 * identity matrix.  */
				for (i = 0; i < w * 2; ++i) {
					*mat_at(w, mat, i, j) = llr_gf_sub(
						*mat_at(w, mat, i, j),
						llr_gf_mul(
							e,
							*mat_at(w, mat, i, jj)
						)
					);
				}
			}

			/* Re-sample the diagonal.  */
			e = *mat_at(w, mat, j, j);
		}

		/* If not 1, divide the entire row.  */
		if (e != 0x01) {
			one_over_e = llr_gf_reciprocal(e);
			for (i = 0; i < w * 2; ++i) {
				*mat_at(w, mat, i, j) = llr_gf_mul(
					one_over_e,
					*mat_at(w, mat, i, j)
				);
			}
		}

		/* For every *other* row, make the entries
		 * on the same column into 0.
		 */
		for (jj = 0; jj < w; ++jj) {
			unsigned char mult;
			if (jj == j)
				continue;
			mult = *mat_at(w, mat, j, jj);
			/* If already 0, do nothing.  */
			if (e == 0)
				continue;
			/* Multiply the elements of this
			 * row (j) by the mult, then
			 * subtract from the row jj.
			 */
			for (i = 0; i < w * 2; ++i) {
				*mat_at(w, mat, i, jj) = llr_gf_sub(
					*mat_at(w, mat, i, jj),
					llr_gf_mul(mult,
						   *mat_at(w, mat, i, j))
				);
			}
		}
	}
}

void llr_matrix_inverse_init(unsigned int w,
			     unsigned char* restrict scratch_space,
			     unsigned char const* restrict matrix) {
	unsigned int i, j;

	for (j = 0; j < w; ++j) {
		llr_memcpy(mat_at(w, scratch_space, 0, j), &matrix[j * w], w);
		for (i = 0; i < w; ++i) {
			*mat_at(w, scratch_space, w + i, j) =
				(i == j) ? 1 : 0 ;
		}
	}
}

void llr_matrix_inverse_get(unsigned int w,
			    unsigned char const* restrict scratch_space,
			    unsigned char* restrict matrix) {
	unsigned int j;
	for (j = 0; j < w; ++j) {
		llr_memcpy(&matrix[j * w], &scratch_space[w + j * w * 2], w);
	}
}
