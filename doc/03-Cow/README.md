Cow Storage Sharing Infrastructure
==================================

The Cow layer handles storage sharing.
It has the responsibilities:

* Implementation of sparse address space.
* Storage sharing.
* Compression.

Rationale
---------

We should note that the Grass layer already implements a
virtual mapping to actual on-device blocks.
Why could we not design Grass so it exposes a sparse
address space and the storage sharing?
Why a Cow?

An issue is that storage sharing is not how the original
log-structured file systems were designed.
In particular, the garbage collector is not implemented as a
tracing garbage collector.

Instead, the garbage collector in a log-structured file
system simply checks the blocks in a to-be-scavenged
segment to see if the latest version of its inode-offset
mapping matches the block address.
If this version is the same, then the block is live and is
simply copied into new space.

This inherently assumes that a data block belongs to a
single particular inode at a single particular offset.
(In the case of Grass, the assumption is that a data block
belongs to a single virtual address)

Once structure sharing is implemented, however, a data
block may now belong to multiple inodes, possibly at
different offsets, or even the same inode at different
offsets.

The most general algorithm to handle shared objects is
to trace in a floodfill algorithm.
But that implies that the entire metadata tree for the
filesystem has to be traversed even if we only want to
scavenge a single segment.

Thus, in LLRFS, storage sharing is not implemented at the
log-structured Grass level.
As far as the log-structured level is concerned, an
on-disk block is mapped to a single virtual device
address.
This lets the garbage-collection process of the
log-structured layer be largely unchanged from classic
cleaners for log-structured file systems.

Instead, storage sharing is implemented one layer
higher, at the Cow level.

Cow implements storage sharing garbage collection using
in-place reference counts, as there are no cycles ---
Cow-virtual addresses point to Grass-virtual addresses,
but not vice versa.
However, Grass itself cannot use reference counts, as
the log structure requires moving live data and updating
all references to that data.

Sparse Address Space
--------------------

The Cow layer provides a vast 128-bit address space.
Each address labels a distinct 4096-byte block, or
points to nothing.

Not all virtual addresses are backed by an actual
block on some disk; the virtual address space is
sparse.
Higher layers may punch holes into this virtual
address space, or attempt to write, which (if te
virtual address is currently unmapped) searches
for some Grass-level block to back the Cow-level
address.

Storage Sharing
---------------

The Cow layer also supports operations to manipulate
the mapping of virtual block.

Aside from the unmapping of a virtual block (i.e. a
TRIM operation), Cow supports copying mappings
by BLOCK-COPY and exchanging mappings by BLOCK-EXCHANGE.
These are used by Human to implement snapshots and
various atomic write operation primitives.

Compression
-----------

Cow also provides transparent compression, reducing
the number of physical sectors that back virtual
blocks.

Compression is always enabled, as it is almost always a
benefit on modern systems.
