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
#pragma once
#if !defined(RAID_LLR_MATRIX_INVERSE_H_)
#define RAID_LLR_MATRIX_INVERSE_H_

/*
This module provides ways to compute the matrix inverse,
of matrices composed of `GF(2^8)` elements.

The functions in this module assume that the given input
matrix is invertible and there is no facility to fail if
the matrix is not invertible.
*/

/** llr_matrix_inverse_scratch_space_size
 *
 * @brief Compute how large the scratch space should
 * be to invert a matrix that is `w * w` in size.
 *
 * @param w - input, the width and height of the matrix.
 *
 * @return the number of bytes that the `scratch_space`
 * should be.
 */
unsigned int
llr_matrix_inverse_scratch_space_size(unsigned int w);

/** llr_matrix_inverse_init
 *
 * @brief Prepare the given scratch space for a matrix
 * inverse operation.
 *
 * @param w - input, the width and height of the matrix.
 * @param scratch_space - input/output, the scratch space
 * to use for the matrix inversion operation.
 * The space must be `llr_matrix_inverse_scratch_space_size(w)`
 * bytes.
 * @param matrix - input, the matrix to invert.
 * This must be `w * w` `GF(2^8)` elements.
 */
void llr_matrix_inverse_init(unsigned int w,
			     unsigned char* restrict scratch_space,
			     unsigned char const* restrict matrix);

/** llr_matrix_inverse
 *
 * @brief Calculate the matrix inverse in the given
 * scratch space.
 *
 * @param w - input, the width and height of the matrix.
 * @param scratch_space - input/output, the scratch space
 * initialized by `llr_matrix_inverse_init`.
 */
void llr_matrix_inverse(unsigned int w,
			unsigned char* scratch_space);

/** llr_matrix_inverse_get
 *
 * @brief Extract the resulting matrix inverse from the
 * given scratch space.
 *
 * @param w - input, the width and height of the matrix.
 * @param scratch_space - input, the scratch space after
 * being processed by `llr_matrix_inverse`.
 * @param matrix - output, the matrix to fill in with the
 * matrix inverse.
 */
void llr_matrix_inverse_get(unsigned int w,
			    unsigned char const* restrict scratch_space,
			    unsigned char* restrict matrix);

/** llr_matrix_inverse_compute
 *
 * @brief Single step computation of the matrix inverse,
 * reusing the given space for the matrix.
 *
 * @param w - input, the width and height of the matrix.
 * @param scratch_space - the scratch space to use for the
 * matrix inversion operation.
 * The space must be `llr_matrix_inverse_scratch_space_size(w)`
 * bytes.
 * @param matrix - input/output, the `w * w` matrix to
 * inverse.
 * The inverse ovewrites this stored data.
 */
static inline
void llr_matrix_inverse_compute(unsigned int w,
				unsigned char* restrict scratch_space,
				unsigned char* restrict matrix) {
	llr_matrix_inverse_init(w, scratch_space, matrix);
	llr_matrix_inverse(w, scratch_space);
	llr_matrix_inverse_get(w, scratch_space, matrix);
}

#endif /* !defined(RAID_LLR_MATRIX_INVERSE_H_) */
