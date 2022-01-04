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
#if !defined(RAID_LLR_CAUCHY_H_)
#define RAID_LLR_CAUCHY_H_
#include"llr_cauchy_seq.h"
#include"llr_gf.h"

/*
This module provides a function that computes the
multiplier for a particular data block when it gets
combined to a particular parity block.

This is effectively the values in a Generalized
Cauchy matrix of `GF(2^8)` elements, defined by our
sequence in the `llr_cauchy_seq` module.
*/

/** llr_cauchy
 *
 * @brief Compute the entry in our Cauchy matrix
 * at the given coordinates.
 *
 * @param data_idx - the index of the data block.
 * Also the column coordinate of the Cauchy matrix.
 * @param parity_idx - the index of the parity
 * block.
 * Also the row coordinate of the Cauchy matrix.
 *
 * @desc If either argument is 0, the result is
 * 1.
 */
static inline
unsigned char llr_cauchy(unsigned int data_idx, unsigned int parity_idx) {
	unsigned char c = llr_cauchy_seq_c[data_idx];
	unsigned char d = llr_cauchy_seq_d[parity_idx];
	unsigned char x = llr_cauchy_seq_x[data_idx];
	unsigned char y = llr_cauchy_seq_y[parity_idx];
	return llr_gf_mul(c, llr_gf_mul(d, llr_gf_reciprocal(llr_gf_sub(x, y))));
}

#endif /* !defined(RAID_LLR_CAUCHY_H_) */
