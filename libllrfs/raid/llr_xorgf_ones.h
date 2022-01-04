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
#if !defined(RAID_LLR_XORGF_ONES_H_)
#define RAID_LLR_XORGF_ONES_H_
#if defined(HAVE_CONFIG_H)
# include"config.h"
#endif

/*
This module provides the number of ones in the bit
matrix for each GF(2^8) element.
This serves as the cost of multiplying a block by
that element, and is used to reduce the cost for the
generated Cauchy matrix.
*/

/** llr_xorgf_ones
 *
 * @brief For each GF(2^8) element, indicates the
 * number of ones in the equivalent bit matrix for
 * that element.
 */
extern unsigned int const llr_xorgf_ones[256];

#endif /* !defined(RAID_LLR_XORGF_ONES_H_) */
