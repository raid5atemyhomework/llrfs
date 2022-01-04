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
#if !defined(LLR_UTIL_H_)
#define LLR_UTIL_H_

/*
This module provides basic memory utilities.

These utilities are traditionally in <string.h>, but some
environments may not provide this, or provide their own
version.
*/

/* TODO: Use <string.h> if available, or a specific version
if we are being compiled for that environment.
*/

/** llr_memcpy
 *
 * @brief Copies data from `src` to `dst`.
 * Just like `memcpy`.
 *
 * @param dst - The destination memory area.
 * @param src - The source memory area.
 * The source and destination must not overlap.
 * @param nbytes - The number of bytes to copy.
 */
void llr_memcpy(void* restrict dst, void const* restrict src, unsigned int nbytes);

/** llr_memzero
 *
 * @brief Sets the memory area to zero.
 *
 * @param dst - The memory area to clear.
 * @param nbytes - The number of bytes to zero out.
 */
void llr_memzero(void* dst, unsigned int nbytes);

#endif /* !defined(LLR_UTIL_H_) */
