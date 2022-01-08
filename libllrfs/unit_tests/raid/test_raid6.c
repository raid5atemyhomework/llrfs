#if defined(HAVE_CONFIG_H)
# include"config.h"
#endif
#undef NDEBUG
#include"raid/llr_decoder.h"
#include"raid/llr_encode.h"
#include"raid/llr_xorgf.h"
#include"userspace/llr_testvectors.h"
#include<assert.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>

int main(void) {
	unsigned int i, j;

	void const* sample_data_blocks[8];
	void* parity_blocks[2];

	unsigned int lost0, lost1;

	llr_decoder decoder;
	unsigned int matrix_storage_size;
	unsigned int scratch_space_size;
	unsigned char* matrix_storage = NULL;
	unsigned char* scratch_space = NULL;

	unsigned int lost_data_block_indices[2];
	unsigned int lost_parity_block_indices[2];

	void const* remaining_blocks[8];
	void* lost_data_blocks[2];

	parity_blocks[0] = malloc(LLR_XORGF_BLOCK_SIZE);
	parity_blocks[1] = malloc(LLR_XORGF_BLOCK_SIZE);
	lost_data_blocks[0] = malloc(LLR_XORGF_BLOCK_SIZE);
	lost_data_blocks[1] = malloc(LLR_XORGF_BLOCK_SIZE);
	for (i = 0; i < 8; ++i)
		sample_data_blocks[i] = llr_testvectors_sampledata[i];

	/* Encode.  */
	llr_encode(sample_data_blocks, 8, parity_blocks, 2);

	/* Emulate loss of two recovery blocks.  */
	for (lost0 = 0; lost0 < 7; ++lost0) {
		for (lost1 = lost0 + 1; lost1 < 8; ++lost1) {
			lost_data_block_indices[0] = lost0;
			lost_data_block_indices[1] = lost1;

			/* Set up space.  */
			llr_decoder_sizes(&matrix_storage_size,
					  &scratch_space_size,
					  8, 2,
					  lost_data_block_indices, 2,
					  lost_parity_block_indices, 0);
			matrix_storage = realloc(matrix_storage,
						 matrix_storage_size);
			scratch_space = realloc(scratch_space,
						scratch_space_size);

			/* Set up decoder.  */
			llr_decoder_init(&decoder, 8, 2,
					 lost_data_block_indices, 2,
					 lost_parity_block_indices, 0,
					 matrix_storage,
					 scratch_space);

			/* Set up data.  */
			for (i = 0, j = 0; i < 8; ++i) {
				if (i == lost0 || i == lost1)
					continue;
				remaining_blocks[j] = sample_data_blocks[i];
				++j;
			}
			remaining_blocks[6] = parity_blocks[0];
			remaining_blocks[7] = parity_blocks[1];

			/* Decode!  */
			llr_decoder_decode(&decoder,
					   lost_data_blocks,
					   remaining_blocks);

			/* Validate.  */
			assert(0 == memcmp(lost_data_blocks[0], sample_data_blocks[lost0], LLR_XORGF_BLOCK_SIZE));
			assert(0 == memcmp(lost_data_blocks[1], sample_data_blocks[lost1], LLR_XORGF_BLOCK_SIZE));
		}
	}

	/* Emulate loss of one data and one parity block.  */
	for (lost0 = 0; lost0 < 8; ++lost0) {
		for (lost1 = 0; lost1 < 2; ++lost1) {
			lost_data_block_indices[0] = lost0;
			lost_parity_block_indices[0] = lost1;

			/* Set up space.  */
			llr_decoder_sizes(&matrix_storage_size,
					  &scratch_space_size,
					  8, 2,
					  lost_data_block_indices, 1,
					  lost_parity_block_indices, 1);
			matrix_storage = realloc(matrix_storage,
						 matrix_storage_size);
			scratch_space = realloc(scratch_space,
						scratch_space_size);

			/* Set up decoder.  */
			llr_decoder_init(&decoder, 8, 2,
					 lost_data_block_indices, 1,
					 lost_parity_block_indices, 1,
					 matrix_storage,
					 scratch_space);

			/* Set up data.  */
			for (i = 0, j = 0; i < 8; ++i) {
				if (i == lost0)
					continue;
				remaining_blocks[j] = sample_data_blocks[i];
				++j;
			}
			remaining_blocks[7] = lost1 == 0 ? parity_blocks[1] : parity_blocks[0];

			/* Decode!  */
			llr_decoder_decode(&decoder,
					   lost_data_blocks,
					   remaining_blocks);

			/* Validate.  */
			assert(0 == memcmp(lost_data_blocks[0], sample_data_blocks[lost0], LLR_XORGF_BLOCK_SIZE));
		}
	}

	free(matrix_storage);
	free(scratch_space);

	free(lost_data_blocks[1]);
	free(lost_data_blocks[0]);
	free(parity_blocks[1]);
	free(parity_blocks[0]);
	return 0;
}
