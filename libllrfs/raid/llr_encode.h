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
#if !defined(RAID_LLR_ENCODE_H_)
#define RAID_LLR_ENCODE_H_

/*
This module performs encoding, i.e. computing the
parity blocks from the data blocks.
It also provides a function to perform modification,
i.e. given the difference between two data blocks,
modify the parity block.
*/

/** llr_encode
 *
 * @brief Computes all parity blocks from the given
 * data blocks.
 *
 * @param data_blocks - An array of pointers to the
 * data blocks.
 * Data blocks are of LLR_XORGF_BLOCK_SIZE bytes.
 * An entry in this array may be NULL, indicating
 * that we should treat the data block as being all
 * 0.
 * @param num_data_blocks - The number of data blocks.
 * @param parity_blocks - An array of pointers to the
 * parity blocks.
 * Parity blocks are of LLR_XORGF_BLOCK_SIZE bytes.
 * All entries must be non-NULL.
 * Parity blocks must have unique locations in memory,
 * i.e. they cannot share storage with each other or
 * with any data blocks.
 * @param num_parity_blocks - The number of parity
 * blocks.
 */
void llr_encode(void const* const* data_blocks,
		unsigned int num_data_blocks,
		void* const* parity_blocks,
		unsigned int num_parity_blocks);

/** llr_encode_modify
 *
 * @brief Modifies the parity blocks, given the delta
 * between the old data block and the new data block.
 *
 * @param delta_data_block - A buffer containing the
 * XOR between the old version of the data block and
 * the new version of the data block.
 * @param data_idx - The index of the data block being
 * modified.
 * @param parity_blocks - An array of pointers to the
 * parity blocks.
 * Parity blocks are of LLR_XORGF_BLOCK_SIZE bytes.
 * All entries must be non-NULL.
 * Parity blocks must have unique locations in memory,
 * i.e. they cannot share storage with each other or
 * with the delta data block.
 * @param num_parity_blocks - The number of parity
 * blocks.
 */
void llr_encode_modify(void const* restrict delta_data_block,
		       unsigned int data_idx,
		       void* const* parity_blocks,
		       unsigned int num_parity_blocks);

#endif /* !defined(RAID_LLR_ENCODE_H_) */
