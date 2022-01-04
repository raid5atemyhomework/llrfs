#undef NDEBUG
#if defined(HAVE_CONFIG_H)
# include"config.h"
#endif
#include"raid/llr_gf.h"
#include<assert.h>

int main(void) {
	unsigned int x, y;

	for (x = 0; x < 256; ++x) {
		if (x != 0) {
			assert(llr_gf_mul(x, llr_gf_reciprocal(x)) == 1);
		}
		assert(llr_gf_add(x, x) == 0);
		for (y = 0; y < 256; ++y) {
			if (x == 0) {
				assert(llr_gf_mul(x, y) == 0);
				assert(llr_gf_add(x, y) == y);
				continue;
			}
			if (y == 0) {
				assert(llr_gf_mul(x, y) == 0);
				assert(llr_gf_add(x, y) == x);
				continue;
			}
			if (x == 1) {
				assert(llr_gf_mul(x, y) == y);
				continue;
			}
			if (y == 1) {
				assert(llr_gf_mul(x, y) == x);
				continue;
			}

			assert(llr_gf_mul(x, y) != x);
			assert(llr_gf_mul(x, y) != y);
			assert(llr_gf_mul(x, y) == llr_gf_mul(y, x));
			assert(llr_gf_mul(llr_gf_mul(x, y), llr_gf_reciprocal(x)) == y);
			assert(llr_gf_add(llr_gf_add(x, y), x) == y);

			assert(llr_gf_mul(llr_gf_add(x, y), 0x42) == llr_gf_add(llr_gf_mul(x, 0x42), llr_gf_mul(y, 0x42)));
		}
	}

	return 0;
}
