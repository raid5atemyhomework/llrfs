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
#include"llr_cauchy.h"
#include"llr_decoder.h"
#include"llr_matrix_inverse.h"
#include"llr_util.h"
#include"llr_xorgf.h"

static
enum llr_decoder_type
get_decoder_type(unsigned int num_data_blocks,
		 unsigned int num_lost_data_blocks,
		 unsigned int const* lost_parity_blocks,
		 unsigned int num_lost_parity_blocks) {
	/* Simple case.  */
	if (num_data_blocks == 1)
		return llr_decoder_type_raid1;
	/* Check if only one data block is missing, and there are
	 * either no lost parity blocks, or the first missing
	 * parity block is not the 0th parity block (the XOR
	 * parity block).
	 * In that case we can just XOR all the remaining blocks
	 * to recover the missing one.
	 */
	if ((num_lost_data_blocks == 1) &&
	    ((num_lost_parity_blocks == 0) || (lost_parity_blocks[0] != 0)))
		return llr_decoder_type_raid5;
	/* Otherwise use the matrix.  */
	return llr_decoder_type_multi;
}

void llr_decoder_sizes(unsigned int* matrix_storage_size,
		       unsigned int* scratch_space_size,

		       unsigned int num_data_blocks,
		       unsigned int num_parity_blocks,
		       unsigned int const* lost_data_blocks,
		       unsigned int num_lost_data_blocks,
		       unsigned int const* lost_parity_blocks,
		       unsigned int num_lost_parity_blocks) {
	enum llr_decoder_type type = get_decoder_type(num_data_blocks,
						      num_lost_data_blocks,
						      lost_parity_blocks,
						      num_lost_parity_blocks);

	if (type != llr_decoder_type_multi) {
		/* No need for extra storage in RAID1 or RAID5
		 * modes.  */
		*matrix_storage_size = 0;
		*scratch_space_size = 0;
		return;
	}

	*matrix_storage_size = num_data_blocks * num_data_blocks;
	*scratch_space_size = llr_matrix_inverse_scratch_space_size(num_data_blocks);
}

void llr_decoder_init(llr_decoder* decoder,
		      unsigned int num_data_blocks,
		      unsigned int num_parity_blocks,
		      unsigned int const* lost_data_blocks,
		      unsigned int num_lost_data_blocks,
		      unsigned int const* lost_parity_blocks,
		      unsigned int num_lost_parity_blocks,
		      unsigned char* matrix_storage,
		      unsigned char* scratch_space) {
	unsigned int const* orig_lost_data_blocks;
	unsigned int orig_num_lost_data_blocks;

	unsigned int w = num_data_blocks;
	unsigned int data_idx, parity_idx;
	unsigned int j;

	decoder->type = get_decoder_type(num_data_blocks,
					 num_lost_data_blocks,
					 lost_parity_blocks,
					 num_lost_parity_blocks);

	/* For simple cases, exit early.  */
	if (decoder->type == llr_decoder_type_raid1)
		return;
	if (decoder->type == llr_decoder_type_raid5) {
		decoder->num_remaining = num_data_blocks;
		return;
	}

	/* Save some variables.  */
	orig_lost_data_blocks = lost_data_blocks;
	orig_num_lost_data_blocks = num_lost_data_blocks;

	/* Generate a version of our encoding matrix with the lost
	 * data and parity rows missing.
	 * This is the matrix that will be inversed.
	 */
	llr_memzero(matrix_storage, w * w);
	j = 0;

	/* Initialize the top part of the matrix.  */
	for (data_idx = 0; data_idx < w; ++data_idx) {
		/* Is this data index lost?  */
		if ((num_lost_data_blocks > 0) &&
		    (lost_data_blocks[0] == data_idx)) {
			/* Skip this row.  */
			++lost_data_blocks;
			--num_lost_data_blocks;
			continue;
		}
		/* Set 1 for the data block.  */
		matrix_storage[j + data_idx] = 1;
		j += w;
	}

	/* Initialize the bottom part of the matrix.  */
	for (parity_idx = 0; parity_idx < num_parity_blocks; ++parity_idx) {
		/* Is tthis parity index lsot?  */
		if ((num_lost_parity_blocks > 0) &&
		    (lost_parity_blocks[0] == parity_idx)) {
			/* Skip this row.  */
			++lost_parity_blocks;
			--num_lost_parity_blocks;
			continue;
		}
		/* Generate the Cauchy matrix.  */
		for (data_idx = 0; data_idx < w; ++data_idx) {
			matrix_storage[j + data_idx] = llr_cauchy(data_idx,
								  parity_idx);
		}
		j += w;
		/* Do we have enough to recover?  */
		if (j >= w * w)
			break;
	}

	/* Perform inverse.  */
	llr_matrix_inverse_compute(w, scratch_space, matrix_storage);

	/* Recover saved variables.  */
	lost_data_blocks = orig_lost_data_blocks;
	num_lost_data_blocks = orig_num_lost_data_blocks;

	/* Copy the rows corresponding to the lost data.  */
	for (j = 0; j < num_lost_data_blocks; ++j) {
		unsigned lost = lost_data_blocks[j];

		/* Skip copying rows that are already in the
		 * correct place.  */
		if (j == lost)
			continue;

		llr_memcpy(&matrix_storage[j * w],
			   &matrix_storage[lost * w],
			   w);
	}

	/* Fill in the decoder object.  */
	decoder->num_remaining = w;
	decoder->num_lost_data_blocks = orig_num_lost_data_blocks;
	decoder->matrix = matrix_storage;
}

void llr_decoder_decode(llr_decoder const* decoder,
			void* const* restrict lost_data_blocks,
			void const* const* restrict remaining_blocks) {
	unsigned int i, j;

	if (decoder->type == llr_decoder_type_raid1) {
		/* Just memcpy the first block to the data block.  */
		llr_memcpy(lost_data_blocks[0], remaining_blocks[0],
			   LLR_XORGF_BLOCK_SIZE);
		return;
	}
	if (decoder->type == llr_decoder_type_raid5) {
		/* Copy the first remaining block.  */
		llr_memcpy(lost_data_blocks[0], remaining_blocks[0],
			   LLR_XORGF_BLOCK_SIZE);
		for (i = 1; i < decoder->num_remaining; ++i) {
			llr_xorgf_acc_mul(lost_data_blocks[0],
					  1,
					  remaining_blocks[i]);
		}
		return;
	}

	/* Otherwise multiply by the submatrix.  */

	/* Initialize the lost blocks to 0.  */
	for (j = 0; j < decoder->num_lost_data_blocks; ++j)
		llr_memzero(lost_data_blocks[j], LLR_XORGF_BLOCK_SIZE);

	/* Accumulate into the lost blocks.  */
	for (i = 0; i < decoder->num_remaining; ++i) {
		for (j = 0; j < decoder->num_lost_data_blocks; ++j) {
			unsigned char m;
			m = decoder->matrix[i + j * decoder->num_remaining];
			llr_xorgf_acc_mul(lost_data_blocks[j],
					  m,
					  remaining_blocks[i]);
		}
	}
}
