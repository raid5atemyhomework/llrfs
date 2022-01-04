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
#include"llr_encode.h"
#include"llr_util.h"
#include"llr_xorgf.h"

void llr_encode(void const* const* data_blocks,
		unsigned int num_data_blocks,
		void* const* parity_blocks,
		unsigned int num_parity_blocks) {
	unsigned int i, j;

	if (num_parity_blocks == 0)
		/* Nothing to do...?  */
		return;

	if (num_data_blocks == 0) {
		/* No parity...  */
		for (j = 0; j < num_parity_blocks; ++j) {
			llr_memzero(parity_blocks[j], LLR_XORGF_BLOCK_SIZE);
		}
		return;
	}

	/* Initialize the parity blocks from the first data block.  */
	/* assert(llr_cauchy(0, j) == 1); */
	if (!data_blocks[0]) {
		for (j = 0; j < num_parity_blocks; ++j) {
			llr_memzero(parity_blocks[j], LLR_XORGF_BLOCK_SIZE);
		}
	} else {
		for (j = 0; j < num_parity_blocks; ++j) {
			llr_memcpy(parity_blocks[j], data_blocks[0], LLR_XORGF_BLOCK_SIZE);
		}
	}

	for (i = 1; i < num_data_blocks; ++i) {
		/* Skip data blocks that are all-0s/nonexistent.  */
		if (!data_blocks[i])
			continue;

		/* Parity block 0 is just RAID5.  */
		/* assert(llr_cauchy(i, 0) == 1); */
		llr_xorgf_acc_mul(parity_blocks[0], 1, data_blocks[i]);

		for (j = 1; j < num_parity_blocks; ++j) {
			unsigned char factor = llr_cauchy(i, j);
			llr_xorgf_acc_mul(parity_blocks[j], factor, data_blocks[i]);
		}
	}
}

void llr_encode_modify(void const* restrict delta_data_block,
		       unsigned int data_idx,
		       void* const* parity_blocks,
		       unsigned int num_parity_blocks) {
	unsigned int i = data_idx;
	unsigned int j;

	if (num_parity_blocks == 0)
		/* Nothing to do...?  */
		return;

	/* Parity block 0 is just RAID5.  */
	llr_xorgf_acc_mul(parity_blocks[0], 1, delta_data_block);

	for (j = 1; j < num_parity_blocks; ++j) {
		unsigned char factor = llr_cauchy(i, j);
		llr_xorgf_acc_mul(parity_blocks[j], factor, delta_data_block);
	}
}
