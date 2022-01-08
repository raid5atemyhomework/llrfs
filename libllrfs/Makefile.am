
AM_CFLAGS = -Wall -Werror

noinst_LIBRARIES = libllrfs.a

EXTRA_PROGRAMS = \
	llr_cauchy_seq_generator \
	llr_xorgf_generator

libllrfs_a_SOURCES = \
	libllrfs.h \
	llr_util.c \
	llr_util.h \
	raid/llr_cauchy.h \
	raid/llr_cauchy_seq.c \
	raid/llr_cauchy_seq.h \
	raid/llr_decoder.c \
	raid/llr_decoder.h \
	raid/llr_encode.c \
	raid/llr_encode.h \
	raid/llr_gf.c \
	raid/llr_gf.h \
	raid/llr_matrix_inverse.c \
	raid/llr_matrix_inverse.h \
	raid/llr_xorgf.c \
	raid/llr_xorgf.h \
	userspace/llr_testvectors.c \
	userspace/llr_testvectors.h

llr_xorgf_generator_SOURCES = \
	raid/llr_xorgf_generator.c
llr_xorgf_generator_LDADD =

llr_cauchy_seq_generator_SOURCES = \
	raid/llr_cauchy_seq_generator.c \
	raid/llr_xorgf_ones.c \
	raid/llr_xorgf_ones.h
llr_cauchy_seq_generator_LDADD =

raid/llr_xorgf.c : $(llr_xorgf_generator_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) llr_xorgf_generator$(EXEEXT)
	./llr_xorgf_generator$(EXEEXIT) > $@
raid/llr_xorgf_ones.c : $(llr_xorgf_generator_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) llr_xorgf_generator$(EXEEXT)
	./llr_xorgf_generator$(EXEEXIT) ones > $@
raid/llr_cauchy_seq.c : $(llr_cauchy_seq_generator_SOURCES)
	$(MAKE) $(AM_MAKEFLAGS) llr_cauchy_seq_generator$(EXEEXT)
	./llr_cauchy_seq_generator$(EXEEXIT) > $@

# SIMD configurations:
# ./configure CFLAGS="-DLLR_XORGF_VECTOR_SIZE=16 -mmmx"
# ./configure CFLAGS="-DLLR_XORGF_VECTOR_SIZE=32 -mavx"
# ./configure CFLAGS="-DLLR_XORGF_VECTOR_SIZE=64 -mavx512f"

maintainer-clean-local :
	rm -f $(srcdir)/raid/llr_cauchy.c
	rm -f $(srcdir)/raid/llr_xorgf.c
	rm -f $(srcdir)/raid/llr_xorgf_ones.c

LDADD = libllrfs.a

ACLOCAL_AMFLAGS = -I m4

TESTS = \
	unit_tests/raid/test_cauchy_seq \
	unit_tests/raid/test_gf \
	unit_tests/raid/test_matrix_inverse \
	unit_tests/raid/test_raid6
check_PROGRAMS = $(TESTS)

if USE_VALGRIND
LOG_COMPILER = valgrind
AM_LOG_FLAGS = --leak-check=full --error-exitcode=1
AM_CFLAGS += -DUSE_VALGRIND
endif USE_VALGRIND
