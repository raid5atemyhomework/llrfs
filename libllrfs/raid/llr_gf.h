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
#if !defined(RAID_LLR_GF_H_)
#define RAID_LLR_GF_H_

/*
This module provides basic Galois-field math on 8-bit
words (`GF(2^8)`) using the polynomial `x^8 + x^4 +
x^3 + x^2 + 1`.

It implements multiplication by a logarithm and a
powers table.

This is not strongly optimized; it is intended to be
used only when inversing the encoding matrix in order
to recover data.

In addition, this is not sidechannel-safe, and MUST
NOT be used for cryptographic code.
*/

/** llr_gf_pow
 *
 * @brief the powers-of-2 table.
 */
extern unsigned char const llr_gf_pow[256];

/** llr_gf_log
 *
 * @brief the logarithms-by-2 table.
 */
extern unsigned char const llr_gf_log[256];

/** llr_gf_add
 *
 * @brief Adds two `GF(2^8)` elements together.
 */
static inline
unsigned char llr_gf_add(unsigned char a, unsigned char b) {
	return a ^ b;
}
#define llr_gf_sub(a, b) llr_gf_add(a, b)

/** llr_gf_mul
 *
 * @brief multiplies two `GF(2^8)` elements together.
 */
static inline
unsigned char llr_gf_mul(unsigned char a, unsigned char b) {
	unsigned char log_a;
	unsigned char log_b;
	unsigned long log_ab;

	/* Multiplication by 0 is 0.  */
	if (a == 0 || b == 0)
		return 0;

	log_a = llr_gf_log[a];
	log_b = llr_gf_log[b];

	log_ab = ((unsigned long) log_a) + ((unsigned long) log_b);
	if (log_ab > 255)
		log_ab -= 255;
	return llr_gf_pow[log_ab];
}

/** llr_gf_reciprocal
 *
 * @brief computes the multiplicative inverse of the
 * given `GF(2^8)` element.
 */
static inline
unsigned char llr_gf_reciprocal(unsigned char a) {
	unsigned char log_a = llr_gf_log[a];
	unsigned char log_reciprocal_a = 255 - log_a;
	return llr_gf_pow[log_reciprocal_a];
}

#endif /* !defined(RAID_LLR_GF_H_) */
