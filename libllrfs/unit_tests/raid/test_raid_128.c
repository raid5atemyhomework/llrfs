#if defined(HAVE_CONFIG_H)
# include"config.h"
#endif
#undef NDEBUG
#include"raid/llr_decoder.h"
#include"raid/llr_encode.h"
#include"raid/llr_xorgf.h"
#include"userspace/llr_testvectors.h"
#include<assert.h>
#include<stdlib.h>
#include<string.h>

/*
This test checks for the case where we have 128
data blocks and 128 parity, and all data blocks
are lost.
*/

int main(void) {
	unsigned int i;

	void* sample_data_blocks[128];
	void const* data_blocks[128];
	void* parity_blocks[128];
	void* recovered_blocks[128];
	void const* remaining_blocks[128];

	llr_decoder decoder;

	unsigned char* matrix_storage = NULL;
	unsigned int matrix_storage_size;
	unsigned char* scratch_space = NULL;
	unsigned int scratch_space_size;

	unsigned int lost_data_block_indices[128];

	for (i = 0; i < 128; ++i) {
		sample_data_blocks[i] = malloc(LLR_XORGF_BLOCK_SIZE);
		memcpy(sample_data_blocks[i], llr_testvectors_sampledata[i % 8],
		       LLR_XORGF_BLOCK_SIZE);
		data_blocks[i] = sample_data_blocks[i];

		parity_blocks[i] = malloc(LLR_XORGF_BLOCK_SIZE);
		remaining_blocks[i] = parity_blocks[i];

		recovered_blocks[i] = malloc(LLR_XORGF_BLOCK_SIZE);

		lost_data_block_indices[i] = i;
	}

	/* Encode!  */
	llr_encode(data_blocks, 128, parity_blocks, 128);

	/* Set up decoder.  */
	llr_decoder_sizes(&matrix_storage_size,
			  &scratch_space_size,
			  128, 128,
			  lost_data_block_indices, 128,
			  0, 0);
	matrix_storage = realloc(matrix_storage, matrix_storage_size);
	scratch_space = realloc(scratch_space, scratch_space_size);
	llr_decoder_init(&decoder,
			 128, 128,
			 lost_data_block_indices, 128,
			 0, 0,
			 matrix_storage,
			 scratch_space);

	/* Decode!  */
	llr_decoder_decode(&decoder,
			   recovered_blocks,
			   remaining_blocks);

	/* Validate.  */
	for (i = 0; i < 128; ++i)
		assert(0 == memcmp(recovered_blocks[i], sample_data_blocks[i],
				   LLR_XORGF_BLOCK_SIZE));

	free(matrix_storage);
	free(scratch_space);
	for (i = 0; i < 128; ++i) {
		free(sample_data_blocks[i]);
		free(parity_blocks[i]);
		free(recovered_blocks[i]);
	}
	return 0;
}
