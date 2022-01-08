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
#if !defined(RAID_LLR_DECODER_H_)
#define RAID_LLR_DECODER_H_ 

/*
This module provides a decoder object, an object capable of
recovering the data from a stripe with a missing number of
blocks.

The object is constructed with a specific setup, and can
only recover the specified lost data blocks.
*/

/** typedef llr_decoder
 *
 * @brief A decoder to recover data blocks, from a stripe
 * of data and parity blocks created by `llr_encode`.
 */
struct llr_decoder_s;
typedef struct llr_decoder_s llr_decoder;

/** enum llr_decoder_type
 *
 * @brief Whether we are decoding in RAID1 or RAID5 mode,
 * or generalized multi-parity mode.
 */
enum llr_decoder_type {
	llr_decoder_type_raid1,
	llr_decoder_type_raid5,
	llr_decoder_type_multi
};

struct llr_decoder_s {
	/** The type of data recovery.  */
	enum llr_decoder_type type;

	/** The number of expected data and parity blocks.  */
	unsigned int num_remaining;
	/** The number of data blocks to recover.  */
	unsigned int num_lost_data_blocks;

	/** The matrix to use during recovery.  */
	unsigned char const* matrix;
};

/** llr_decoder_sizes
 *
 * @brief Return the sizes needed for various buffers.
 *
 * @desc The decoder needs two buffers, the `matrix_storage`
 * and the `scratch_space` buffer.
 * The `matrix_storage` must be retained for the lifetime
 * of the decoder, while the `scratch_space` is only used
 * during initialization.
 * This function determines the sizes needed for these
 * two buffers.
 *
 * @param matrix_storage_size - output, the size of the
 * `matrix_storage` buffer, in bytes.
 * @param scratch_space_size - output, the size of the
 * `scratch_space` buffer, in bytes.
 * @param num_data_blocks - input, the number of actual
 * data blocks in each stripe.
 * @param num_parity_blocks - input, the number of actual
 * pairty blocks in each stripe.
 * `num_parity_blocks != 0`
 * @param lost_data_blocks - input, the 0-based indices of
 * the lost data blocks.
 * Must be in sorted order.
 * @param num_lost_data_blocks - input, the number of lost
 * data blocks.
 * `1 <= num_lost_data_blocks <= num_data_blocks`
 * @param lost_parity_blocks - input, the 0-based indices
 * of the lost parity blocks.
 * Must be in sorted order.
 * @param num_lost_parity_blocks - input, the number of
 * lost parity blocks.
 * `0 <= num_lost_parity_blocks <= num_parity_blocks`
 */
void llr_decoder_sizes(unsigned int* matrix_storage_size,
		       unsigned int* scratch_space_size,

		       unsigned int num_data_blocks,
		       unsigned int num_parity_blocks,
		       unsigned int const* lost_data_blocks,
		       unsigned int num_lost_data_blocks,
		       unsigned int const* lost_parity_blocks,
		       unsigned int num_lost_parity_blocks);

/** llr_decoder_init
 *
 * @brief Initialize a decoder object.
 *
 * @param decoder - output, the decoder object to initialize.
 * @param num_data_blocks - input, the number of actual data
 * blocks.
 * @param num_parity_blocks - input, the number of actual
 * parity blocks.
 * `num_parity_blocks != 0`
 * @param lost_data_blocks - input and retain, the 0-based
 * indices of the lost data blocks.
 * Must be in sorted order.
 * Do not free this until you finish with the decoder.
 * @param num_lost_data_blocks - input, the number of lost
 * data blocks.
 * `1 <= num_lost_data_blocks <= num_data_blocks`
 * @param lost_parity_blocks - input and retain, the 0-based
 * indices of the lost parity blocks.
 * Must be in sorted order.
 * Do not free this until you finish with the decoder.
 * @param num_lost_parity_blocks - input, the number of
 * lost parity blocks.
 * `0 <= num_lost_parity_blocks <= num_parity_blocks`
 * @param matrix_storage - input and retain, a buffer to use
 * for recovery information.
 * Its size must be from `llr_decoder_sizes`.
 * Do not free this until you finish with the decoder.
 * @param scratch_space - input, a buffer to use to compute
 * the recovery information
 * Its size must be from `llr_decoder_sizes`.
 * Free this after calling `llr_decoder_init`.
 */
void llr_decoder_init(llr_decoder* decoder,
		      unsigned int num_data_blocks,
		      unsigned int num_parity_blocks,
		      unsigned int const* lost_data_blocks,
		      unsigned int num_lost_data_blocks,
		      unsigned int const* lost_parity_blocks,
		      unsigned int num_lost_parity_blocks,
		      unsigned char* matrix_storage,
		      unsigned char* scratch_space);

/** llr_decoder_decode
 *
 * @brief Recover the missing data blocks by providing
 * remaining data and parity blocks.
 *
 * @param decoder - input, the decoder to perform decoding.
 * @param lost_data_blocks - output, an array of pointers to
 * buffers, each buffer being LLR_XORGF_BLOCK_SIZE.
 * Recovered data will be placed here.
 * Recovered data blocks will be ordered from lowest index to
 * highest.
 * @param remaining_blocks - input, an array of pointers to
 * buffers, each buffer being the remaining data and parity
 * blocks.
 * Data blocks come first, then parity.
 * Data blocks will be ordered from lowest index to highest,
 * then parity blocks will be ordered from lowest index to
 * highest.
 */
void llr_decoder_decode(llr_decoder const* decoder,
			void* const* restrict lost_data_blocks,
			void const* const* restrict remaining_blocks);

#endif /* !defined(RAID_LLR_DECODER_H_) */
