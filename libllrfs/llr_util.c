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
#endif /* defined(HAVE_CONFIG_H) */
#include"llr_util.h"

/* Most basic possible implementation.  We should use better ones
depending on the environment we are compiling in.
*/
void llr_memcpy(void* restrict vdst, void const* restrict vsrc, unsigned int nbytes) {
	char* dst = (char*) vdst;
	char const* src = (char const*) vsrc;
	for (; nbytes != 0; --nbytes, ++dst, ++src)
		*dst = *src;
}

void llr_memzero(void* vdst, unsigned int nbytes) {
	char* dst = (char*) vdst;
	for (; nbytes != 0; --nbytes, ++dst)
		*dst = 0;
}
