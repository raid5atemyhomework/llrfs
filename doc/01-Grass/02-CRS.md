Cauchy Reed Solomon Coding
==========================

From this paper: [An XOR-Based Erasure-Resilient Coding Scheme](https://www.researchgate.net/profile/Michael-Luby-2/publication/2643899_An_XOR-Based_Erasure-Resilient_Coding_Scheme/links/02bfe50f19c46ba3cc000000/An-XOR-Based-Erasure-Resilient-Coding-Scheme.pdf).

For some reason, the technique described in the document is not
used in FLOSS RAID implementations.
As far as I can tell it is not patented.
Initial testing suggests it massively increases throughput
compared to equivalent ZFS code.

The paper has two primary insights; the first insight is where
almost all the performance increase comes from, the second
insight allows building the encoding matrix faster.

1.  Instead of treating each byte as a `GF(2^8)` element, treat
    each processor word as containing one bit from multiple
    `GF(2^8)` elements, and perform multiplication-by-scalar
    on multiple elements simultaneously.
    This is expressed as replacing every `GF(2^8)` element in
    the encoding and decoding matrices with `GF(2)` matrices
    equivalent to those elements.
    Using this encoding of `GF(2^8)` elements provides massive
    increase in encoding, modification, *and* decoding speeds,
    and non-SIMD code is competitive with SIMD code with
    `PSHUFB` on the typical 1-byte-1-`GF(2^8)` element.
    Further, modern GCC can, at `-O3` or `-ftree-vectorize`,
    convert non-SIMD code to SIMD instructions.
2.  Use a Cauchy matrix to form the part of the matrix that
    encodes the parity blocks.
    This is easier to generate on-the-fly than starting with a
    Vandermonde matrix.
    In a Chaucy matrix we have two non-overlapping sequences of
    `GF(2^8)` elements, with no repeated elements (i.e. each
    possible element exists at most once, in one of the two
    sequences) and the data-block index serves as an index to one
    sequence `x[i]` while the parity-block index serves as an
    index to the other sequence `y[j]`, and each element is just
    `1 / (x[i] - y[j])`.

Grass uses a *Generalized* Cauchy matrix for the parity blocks.
A Generalized Cauchy matrix requires two more sequences, `c[i]`
and `d[j]`, and each element of the matrix is
(c[i] * d[j]) / (x[i] / y[j])`.

Generating the Chaucy Matrix
----------------------------

From this LKML post: [Triple parity and beyond](https://www.mail-archive.com/linux-btrfs@vger.kernel.org/msg28964.html).

The above LKML post presents an Extended Chaucy matrix that can
be used for 6 parity, and is compatible with existing RAID5 and
RAID6 encodings.
This is done by setting `x[i] = 2^-i`, and `y[0] = 0`, then just
using `2^0` to `2^3` for `y[1]` to `y[4]`.
It then divides each row by its leftmost element, so that the
leftmost column is all 1s.
This is a Generalized Chaucy matrix, with the value
`(c[i] * d[j]) / (x[i] - y[j])`, where sequence `d` is set to
`d[j] = x[0] - y[j]` and sequence `c` is all 1s.
Unlke the Cauchy sequences `x` and `y`, the `c` and `d` sequences
need not contain unique numbers.

However, we should note that the above LKML post is motivated
by the need to keep back-compatibility with existing RAID6,
which uses `2^i` for its Q parity row; the above construction
makes the `1 / x[i] - y[j]` for `j = 0` (the row corresponding
to the Q parity row, since this is an *Extended* Chaucy matrix)
equal to `2^i`.

Since Grass is using a completely different way of expressing
`GF(2^8)` elements, it cannot be back-compatible with any
existing Freedom software RAID6 scheme (and LLRFS, with its
transactional array layer, is completely incompatible anyway).
Thus, we can choose to use a Chaucy matrix with greater
efficiency.

The original paper for Chaucy Reed Solomon suggests just
starting with 0, 1, 2, .... and have `y[j]` start where `x[i]`
ends.
This paper disagrees: [Optimizing Chaucy Reed-Solomon Codes
for Fault-Tolerant Storage Applications](https://web.eecs.utk.edu/~jplank/plank/papers/CS-05-569.pdf).

The paper does not describe the "Good Cauchy" algorithm
well enough for me to actually implement it, but the point
is that some Cauchy matrices are more expensive to compute
during encoding than others.

We can observe that generally, users want to keep the number
of data disks and parity disks low compared to the highest
theoretical possible.
In particular, using `GF(2^8)`, the total of data disks and
parity disks is 256, but we imagine that most users will have
maybe a few dozen disks only.

We can arrange so that we optimize for the case of fewer
data and parity disks.
So, we arrange sequences `c` and `d` so that the top row is
all 1s and the leftmost row is all 1s.
This makes the first parity disk trivial (just the XOR of
all data disks) and loading the first data disk is also
trivial (we just `memcpy` the first data block to the buffer
for parity before applying the rest of the matrix to the
succeeding data blocks).

In addition, we observe that each factor, on multiplication,
has different scores (i.e. number of XORs needed to perform
the multiplication).
We sort the factors by score (ties are broken arbitrarily,
in our implementation the `GF(2^8)` element that is numerically
lower comes first for a deterministic result).
While the top row is all 1s, we arrange the second row so that
it is the sequence of possible factors, sorted from lowest
score (i.e. fewest XORs) to highest.

Generating Multiplication Functions
-----------------------------------

The original paper describes passing around matrices of
`GF(2)` elements, then performing or not performing XORs
based on whether an entry in that matrix had a 1 or not,
i.e. the innermost loop had branches.
Unfortunately, on modern pipelined architectures having
branches in a tight loop hurts performance.

Instead, we define 256 different functions, which multiply
a block of data by a specific `GF(2^8)` element, then XORs
it with another block (i.e. the parity block being built).
Thus, we use the Chaucy matrix containing `GF(2^8)` to
select which function to use for each data block to compute
particular parity blocks.

To generate each function, we generate the equivalent
`8 * 8` matrix for the `GF(2^8)` matrix, and just XOR
into the result the specific words from the data block
if the corresponding entry in the matrix has a 1, or
generate no code for 0s.

For example, the matrix for `0x02`, using the polynomial
`x^8 + x^4 + x^3 + x^2 + 1`:

    0 0 0 0 0 0 0 1
    1 0 0 0 0 0 0 0
    0 1 0 0 0 0 0 1
    0 0 1 0 0 0 0 1
    0 0 0 1 0 0 0 1
    0 0 0 0 1 0 0 0
    0 0 0 0 0 1 0 0
    0 0 0 0 0 0 1 0

gets translated to code much like this:

```c
typedef uint64_t unit_type;
#define span ((4096 / 8) / sizeof(unit_type))
void accumulate_mul_by_2(void* restrict v_acc,
			 void const* restrict v_inp) {
	unsigned int i;
	unit_type* acc = (unit_type*) v_acc;
	unit_type const* inp = (unit_type const*) v_inp;
	for (i = 0; i < span; ++i) {
		acc[0 * span] ^= inp[7 * span];
		acc[1 * span] ^= inp[0 * span];
		acc[2 * span] ^= inp[1 * span] ^ inp[7 * span];
		acc[3 * span] ^= inp[2 * span] ^ inp[7 * span];
		acc[4 * span] ^= inp[3 * span] ^ inp[7 * span];
		acc[5 * span] ^= inp[4 * span];
		acc[6 * span] ^= inp[5 * span];
		acc[7 * span] ^= inp[6 * span];
	}
}
```

This allows tight loops without branches in them, which are
good for modern architectures.
This also makes it easier for the compiler to automatically
compile to SIMD instructions, as some SIMD architectures
may not work well with branches in tight loops.

The functions are then put in a lookup array, so that we only
need to refer to the `GF(2^8)` Cauchy matrix (never translating
explicitly to the binary matrxi) and call the appropriate
function to generate the parity block from each data block.

More specifically, since Grass works on blocks of 4096
bytes, we split each block to 8 entries of 512 bytes each.
Each entry represents one bit of 4096 `GF(2^8)` elements.
Each of the multiplication functions then processes the
512-byte entries, using the word size of the processor to
perform multiple XORs at once.

The program generating the above functions can also
generate the score (i.e. number of XORs) for each of the
elements.
