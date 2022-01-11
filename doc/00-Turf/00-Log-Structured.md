Turf Log-Structured Storage
===========================

The RAID5 Write Hole
--------------------

Our journey begins with the RAID5 write hole.

Briefly: if we have existing data on an array of disks arranged
in RAID5, and we write-in-place some block of data, very
transiently, the stripe containing that block will be desynchronized.
If your system crashes during this small window of time that the
stripe is desynchronized, *and* one of the devices fails, you will
lose data even though RAID5 is supposed to keep operating even if
up to one device fails.

In order to fix the RAID5 write hole, we cannot update-in-place.

This leads us to log-structuring.
In a log-structured storage system, whenever the client "updates"
some block of some object, what the system does is that it writes
the new block in a log, and also logs a command to remap that
part of the object to the new block.

This has the nice side effect that we never overwrite existing
blocks.
Instead, as free space on the disk for writing the log runs out,
older parts of the log are checked, and if there are now many
blocks that have been superseded by newer versions, the remaining
blocks are copied into fresh space and the old log section
(called a "segment") is declared "clear", i.e. is now free space.
Thus, data is never overwritten unless it has been either been
replaced with newer data, or has been moved elsewhere on the
storage device.

The same hole exists in RAID6 and in variants that support
three or more parity blocks.

Log-Structured Array
--------------------

For the paper that began log-structuring:
https://web.stanford.edu/~ouster/cgi-bin/papers/lfs.pdf

Briefly, in a typical log-structured data store, the entire
storage device is a giant log.
The device is split into large sections, called segments.
Segments are the unit of cleaning the device; the garbage
collection process selects one or more segments to scavenge,
moves live data out of it, and makes it available for use.

Now, we want reliable data storage, and to achieve that, we
need to take multiple devices and put them in an array of
devices.
Then we need to arrange our data such that even if up to
`max_device_fails` devices fail, we can recover the data.

Conceptually, the array provides what is basically a
"standard" RAID1, RAID5, RAID6, or RAID6+ (3 or more
parity blocks).

What Turf does is that each device in the block is
*individually* log-structured, independently of the
other devices in the array.
However, the array has a unified superblock, containing
the "root sectors" of all devices.

(Indeed, prehistoric iterations of the LLRFS design had the
striping layer below the log-structuring layer, i.e. there
was a single log that was striped across devices.
This would have been much better performance wise.
However, that design is not usable with multiple zoned devices
with different zoning patterns; for that usage, you want
each device to have its own log structure.
As flexibility is a design goal that is higher than performance,
LLRFS was redesigned so that the log-structuring is per device,
providing transactional semantics across devices, then layering
the striping on top of the transactional array.)

For now, we focus on the structure within one device.

The Wandering Tree Problem
--------------------------

In a copy-on-write file system (and purely log-structured file
systems are copy-on-write), a particular random-write behavior
greatly increases the load on the garbage collector:

* The client modifies one block.
* The client immediately `fsync`s.

The problem is that this single block must be kept track
of by some indirection metadata node.
Since this is copy-on-write, the indirection metadata node
has to be copied elsewhere with the modified location of
the block.
The indirection metadata node has its own parent node which
also has to be modified, and copied-on-write elsewhere, and
so on until we reach the superblock node.

Assuming some kind of O(log n)-depth structure, then to
write-`fsync` *one* block, an extra O(log n) block writes are
needed, where n is the size of live data on the disk.

Since extra blocks have to be allocated, this increases
pressure on garbage collection.
For storage devices like SSDs with a maximum number of write
events, it also wears down the device faster.

On F2FS, the wandering tree problem is solved by not actually
using a metadata tree; instead, many metadata blocks have two
areas, written alternately with each other, effectively creating
a "mini log" for each metadata block.

### The Wandering Tree And LFS Garbage Collector

Interestingly, the original LFS paper *relies on* the wandering
tree to reduce the processing time of the garbage collector.

Basically: when a segment has to be cleaned, all live data and
metadata blocks inside it have to be recovered.
Yet the original LFS only really keeps track of live data, not
metadata.

The reason for this is that because of the wandering tree, any
data blocks that go into a segment will be followed by metadata
blocks --- the wandering parts of the metadata tree --- that
directly or indirectly refer to those data blocks, all the way
up to root nodes referred to from the checkpoint region.

When the cleaner inspects a segment *data* block, then:

* If a data block is live, it will be moved out.
  * Due to Wandering Tree, all metadata nodes that referred to
    that data block will be moved out of the segment, since the
    data block is being moved.
* If a data block is dead, then logically, it means that a
  metadata node tracking that data block must have been modified
  and copied elsewhere, meaning all the metadata nodes that
  referred (directly or indirectly) to the now-dead data block
  are now also dead.

By tracking *only* the live data blocks in a segment being
cleaned, the cleaner only needs to traverse the part of the
metadata tree that point to data blocks inside the segment
(the "liveness" check requires that the cleaner check if the
inode and block offset of the block in the segment still points
to that block).
The cleaner does not need to traverse the *entire* metadata
tree to look for metadata in the segment being cleaned.
Since a segment is tiny compared to the total capacity of the
storage, this requires traversing only a small part of the
metadata tree.

Thus, the wandering tree problem actually means the garbage
collector is more efficient, as it does not have to traverse
the entire metadata tree.

However, we should note that writes to the file system are
(ideally) more common than garbage collections.
Indeed, garbage collection is completely unnecessary unless
*some* writes happen.
The wandering tree improves the speed of garbage collection,
at the cost of increasing the cost of small writes.

If we change the trade-offs so that small writes are faster
at the expense of making garbage collection longer, we should
also ensure that the garbage collector can be run concurrently
with user writes.

Command Buffers
---------------

Rather than perform the tree update strictly, we can instead
defer tree updates and perform them lazily.
The thinking is that the laziness lets us buffer up some
inserts, deletes, and updates (i.e. commands), and thus have
to rewrite less of the actual tree later once we have buffered
up a large number of commands.

The root of the metadata tree can be the actual tree root,
or a reference to a command buffer node.

A command buffer node contains:

* A type byte to differentiate command buffer nodes from
  tree nodes.
* A reference to another root (either an actual tree root,
  or another command buffer node).
* A reference to a previous segment summary node (to be
  described later).
* A 64-byte Bloom filter of all keys that are contained in
  this node.
* A byte indicating the number of entries in this node.
* A sorted array of entries.

On writing, instead of copying metadata nodes from the leaf to
the root, Turf creates a number of command buffers (ideally
just one).
Command buffers represent a series of pending modifications
to the metadata tree.

In the case where the client performs a single-block write
and then `fsync`s it immediately, we just need to write
the block to be written and a command buffer node containing
that block.

Thus, rather than a pure tree, the metadata is composed of a
singly-linked list of command buffers terminated by an actual
tree.

The command buffer concept is highly similar to log-structured
merge-trees or LSM trees.
If command buffers were supported at every node of a B-tree
rather than just the root, then it would be highly similar to
a copy-on-write variant of B-epsilon trees.
The singly-linked list of command buffers is effectively a
log of the metadata updates we intend to eventually do, so it
can be considered an implementation of a write-ahead log or
intent log.

The design of command buffers turns out to have many
advantages that we shall see later.

Lookups
-------

In order to locate a physical block backing a virtual block,
we start at the superblock and check the root node of the
metadata tree.
If the root reference in the superblock is a null reference,
then the map is empty and there are no physical blocks
backing any virtual blocks.

The first byte of the root node determines its type.
If it is 0, then the node is definitely a normal copy-on-write
B+ tree node, and we proceed with normal B+ tree lookup.

Otherwise, if the first byte is non-zero, it is a command
buffer.
We look up the key being searched for in the Bloom filter,
and if lookup fails, follow the "next root" reference of
the command buffer.
Otherwise, we search through the sorted array using binary
search, and if we find an entry matching the key, we can
check its command.
If the key turns out not to be in the array in the command
buffer, we proceed to the "next root" reference.

Inside a command buffer, the value associated with a key
is not *necessarily* the actual value.
Instead, it is a command:

* It could be a DELETE, indicating that the key should
  eventually be deleted.
* It could be an INSERT-OR-REPLACE, indicating that we
  can stop lookup at this point and return the attached
  value.
* It could be an ADD, indicating that the value is a
  64-bit number, and we should continue looking up;
  we then add this number (with overflow, so it is
  effectively a 2's complement signed integer).

Segment Summaries
-----------------

The garbage collector, on selecting a victim segment to
clean, has to know what blocks within those segments are
candidates for recovery, and how to locate those blocks
in the metadata tree so it can determine liveness.

In the original LFS, this was done by adding segment
summary blocks containing pointers to data blocks and which
inode+offset (equivalent to the key in Turf) the block
belonged to.
The segment cleaner would then be able to find data blocks,
and also look up the inode+offset key in its metadata tree
to check if the data block in the victim segment was still
the most recent data block for that inode+offset.

Now, if all writes are done by adding command buffers in
Turf, it just so happens that command buffers also contain
references to data blocks and the key under which the data
block "should" be found.

Thus, command buffers can also double as segment summaries.

This is why command buffers also have a reference to a
previous segment summary node: we have two singly-linked
lists.
One singly-linked list is for command buffers that are
pending for the metadata tree; this list can span multiple
segments, and it is possible for a background compactor
(the command processor, to be discussed later) to orphan the
first few nodes in this list, if the entries in those nodes
can all fit in a single command buffer node (which can help
speed up lookups).
The other singly-linked list is for segment summary blocks;
the list is strictly within the segment and is strictly
extended without orphaning any nodes.

Now, as we noted, by fixing the wandering tree problem, we
can no longer track *just* data.
Thus, we actually have multiple types of segment summary blocks:

* Command buffer node.
* Normal segment summary node.

Command buffers are already described.

A normal segment summry node only contains references
to both metadata and data blocks in the same segment.
Data blocks include the block address, but metadata is
only the block itself.
They are useful when the log head is about to end one
segment; instead of inserting a command buffer, we insert a
segment summary block containing the blocks that were
written but not yet in a command buffer.

We should note that command buffers are *also* metadata
blocks.
But the command buffers themselves *are* already on the
list of segment summary nodes, so the garbage collector
can already see them.

Command Processor
-----------------

Random writes can be made fast by simply putting up a
command buffer node and updating the segment summary
reference and root node reference.
However, lookup suffers, as multiple command buffers
need to be scanned to find a particular value.

The command processor is invoked after some number
of writes have been performed, i.e. when the number
of command buffers at the root exceeds some threshold.

When invoked, the command processor will lock the
global root.
It records the current root reference into an
"initial root reference" variable, then unlocks.

The command processor then checks the root it got.
It first checks if it can compact the command buffers.
It sums up the number of entries in all command buffers,
and if the sum is less than or equal to half of the
maximum possible for its triggerin threshhold, it just
rewrites the command buffers to new ones, with
the entries compacted, and updates the root again.
This handles the case where the command buffers are
individually sparsely populated due to lots of tiny
atomic writes.
In that case, it just writes the new command buffers,
replaces them on the tree, and adds them to the
segment summary lists, then writes them out to disk.
It now has a pristine root node, with fewer command
buffers than the previous.

Otherwise, it has to collapse the command buffers into
the B+ tree.
The command processor then starts processing commands
in the command buffer nodes, starting with the oldest
command buffer (i.e. the one nearest to the tree, not
to the superblock).
It creates a copy-on-write version of the B+ tree
(i.e. in-memory representations of the B+ tree are
marked as "clean" i.e. they are the same as some
on-disk block, or "dirty" i.e. they need to be
rewritten elsewhere) and starts applying the commands
to the B+ tree.
Once it has finished the commands, it writes out the
dirty B+ tree nodes to new blocks, and now has a
pristine root node.

Whichever thing it did (just compact command buffers,
or actually process them into the B+ tree), it now
has a pristine root node that it wants to use as
the root in the superblock.

At this point, it now re-locks the global root.

It checks if the current root reference is still
equal to the "initial root reference" it stored.
If so, we are happy and we just replace the root
reference with the pristine root node and write
the superblock to disk and unlock the global root.

Otherwise, it means that other threads performed
writes to storage while we were working on
command buffers.
While still locked, we scan through command
buffers from the current root reference until
we find the initial root reference (i.e. the
"snapshot" at which we started processing).
The command processor simply copies the
commands into new command buffer nodes (while
still locked), writes them out to disk, then
updates the superblock to point to the new
command buffer nodes, then finally unlocks.
If the number of new command buffer nodes
that were rewritten exceeds the threshold, the
command processor re-triggers itself.

Thus, for the most part, the command processor
can run concurrently with other writers.

Garbage Collector Concurrency
-----------------------------

The garbage collector can use a similar technique:
it can lock the global root, sample the current
root reference and store it as the "initial root
reference", do work, then re-lock and check if the
current root reference is still the same as the
initial root reference, and if not, re-append any
new command buffer nodes it finds (and if this
results in too many command buffer nodes, trigger
the command processor to compact them).

In effect, the Turf garbage collector is a
snapshot-at-the-beginning concurrent collector.
Updates are simply appended to the command buffer.

However, a complication is that the garbage
collector can *free* a segment.

The reason this is a complication is that a
concurrent reader might still be in the process
of looking up using the *old* structure.
This is fine with the command processor, since
we "never" overwrite anything, and if the reader
is using an older version of the tree, that is
perfectly fine; the old version is not
overwritten and remains valid.

But for the garbage collector, the old version of
the tree is doomed.
Ideally, if the underlying device supports it,
the garbage collector will TRIM the just-cleaned
segment (to reduce the effect of log-on-log if
the backing device is an SSD with a log-structured
FTL).
Even if it does not TRIM, if some reader goes to sleep
while doing the lookup, then there is a risk that
the log head might decide to go into the newly-freed
segment and start overwriting the data the reader
is trying to read.

To coordinate between readers and the garbage
collector, we have two atomic counters, and a
pointer (protected by the global root lock) to
one of those atomic counters.

When a reader wants to read, it locks the global
root and reads the pointer-to-counter and
stores it in a local variable.
Then it increments the counter being pointed to
(sequentially-consistent), then reads the root
reference.
Then it unlocks the global root and proceeds
with lookup.
Once the lookup completes, the reader decrements
(sequentially-consistent) the counter that is
pointed to by the local variable.

When the garbage collector has completed its run, it
is holding a reference to a pristine root, which it
knows has absolutely no references to any blocks in
some victim segment.
It locks the global root and fixes up any additional
command buffers, then replaces the root reference and
commits the superblock.
The superblock is modified with an additional field,
an "intent to free", containing the segment that the
collector has just completed cleaning.
While holding the lock, it looks at the current
pointer-to-counter and records it in a local
variable.
Then it changes the pointer-to-counter to point to
the *other* counter and releases the lock.

While the lock is released, the garbage collector
then waits for the counter pointed to by the local
variable to drop to 0.
Once it does, it can TRIM the cleaned segment, and
also insert a command buffer to mark the segment as
available and add it to the metadata tree, also
atomically removing the "intent to free" field on
the superblock.

(alternately the reader can check if the value
decrements to 0, and if so, re-lock the global root
and check if the pointer-to-counter is different
from the local variable, and if so, handle the
intent-to-free.)

The garbage collector cannot run concurrently with
the command processor, as they both keep track of an
"initial root reference" and expect it to be found
along the singly-linked list.

Garbage Collection
------------------

Garbage collection is not much different from the
original LFS segment cleaner.

At the start of collection, we lock the global root,
then record the current root reference into two
local variables, an initial root reference and a
shadow root reference, and then unlock.
As the collector works, the shadow root reference will
very likely eventually point to an in-memory shadow copy
(marked dirty) of the initial root.

A difference from the original LFS cleaner is that we
need to also recover metadata nodes, since we have fixed
the wandering tree problem.
As noted above:

* B+ tree nodes are recorded in segment summary blocks.
* Command buffer nodes *are* segment summaries and
  will be encountered as we scan the segment summaries
  in the victim segment.

When we scan the segment summaries, if the current
segment summary node is a command buffer, we also
check if that command buffer is reachable from the
root before encountering the B+ tree.
If so, the command buffer and every parent up to
the root (i.e. the first command buffer referenced
directly as the root reference) is marked dirty.

When scanning summaries and see a metadata node, we
know the minimum and maximum keys that the in-segment
metadata node covers.
This lets us check the B+ tree node; we only need to
traverse down one pointer on each internal node.
If we encounter the in-segment node while traversing
down the B+ tree, we mark it and every parent (and
every command buffer) dirty.
If we descend into some internal node and find that
the range of the in-segment node does not fit within
any of the child nodes of the internal node, we know
the in-segment node is dead.
If we descend into leaf nodes and it is not the
in-segment node, we know as well the in-segment node
is dead.

When scanning command buffers or plain data block
segment summaries, we look up the data block virtual
address using our shadow tree.
If we reach a command buffer or B+ tree leaf node
containing the virtual address, and it points to
the same data block, we know the data block needs to
be recovered.

Since we have already traversed and loaded the
metadata tree to look for the entry, we can easily
just mark the node, and every parent, as dirty, then
copy the data block into a new location and update
the entry.

Once the garbage collector has finished scanning,
it then serializes the dirty nodes of in-memory
shadow tree.
This is essentially the same procedure as in
the command processor.
Similarly, at the end of processing it re-locks the
global root and checks if the root node can be
replaced directly, or if additional command buffers
have been added since it started processing.

Superblock
----------

So far, we have been discussing the superblock as
if it were a magical location on the storage array
that is somehow immune to data write non-atomicity.

In particular, we should remember that the log-structuring
we have just discussed in this document is *only* for a
single storage device in the array.

How do we ensure atomicity across multiple devices in
the array?

We have a large 255-entry in-memory array containing block
references to the root block of each device.
This array is protected by the global root lock, and in
the previous discussion, every time we mentioned the
global root being updated, we really meant that the global
root *for a particular device* is being updated in this
in-memory array.
This superblock also contains a monotonically incrementing
64-bit counter.

Whenever we update a superblock, we just append it to the
most recently-written block in the current superblock
segment.
When a superblock segment is filled, we clear the other
superblock segment (by TRIM, or by restting the zone
write pointer).

When we update the superblock to disk, we lock the global
root, increment the counter, then make a copy of all the
data needed to serialize the superblock.
Then we unlock the global root and trigger cache flushes
on all storage devices, waiting for all of them to complete.
Then we assemble the superblock and encrypts it, then
write it to the superblock segments of all devices in thte
array.
Superblock writes are performed as forced unbuffered writes.

The superblock is large, and will take up more than 4096
bytes.
This means that writing a superblock is vulnerable to
"torn" writes.
The superblock is encrypted, and includes its own 192-bit
nonce and 128-bit tag for the encryption of the rest of
the superblock.
If a superblock was only partially written, the tag is
unlikely to match.

When opening an array, we read in all superblock segments
in all devices, searching for the superblock entry which
validates (indicating it was written completely) and
has the highest generation number.
This is the latest consistent state of the entire array.

Block References
----------------

A reference to a block is composed of these:

* 64-bit block index.
* 128-bit nonce.
* 128-bit tag.

Blocks written to disk are encrypted with the given
nonce.
The nonce is generated from a pseudorandom number
generator.

The tag is generated from the encrypted data.
Then, the block is written to the tip of the block.

Zoned Storage
-------------

Log-structed storage systems are, coincidentally, ideal
for zoned storage devices.
(This was not deliberately designed; I was only trying
to solve the RAID5 write hole.)
In fact, we can implement segments inside a zoned device as
the zones themselves.

In particular, for sequential-write zones, we only need
to keep track of which zone we are currently writing into.
The location of the next writable block is tracked by
the device for each zone.

Some storage devices can only write to a limited number
of sequential-write zones at any one time.
As long as the device supports up to two open zones at a
time, Turf can work with them --- one open zone is for
the log head, the other open zone is for the superblock
segment.

In fact, what we need to do is to somehow emulate zoning
on *non-*zoned devices.
For completely non-zoned devices, we simply zone the
device by 256M zones.

We can observe that, as long as the log head is inside a
segment, we first write some number of data blocks, some
number of metadata blocks which describe how those data
blocks fit into the overall tree, and the final metadata
block is a command buffer or B+ tree root that will be
referred to from the superblock.
Thus, the root reference for a device in the superblock
also doubles as the log head for non-sequential-write
zones.

For non-sequential-write zones, we need to have a
"free list" of zones.
We can use a simple singly-linked list, whose head
is stored in the superblock, one for each device in
the array.

For sequential-write zones this might not match what the
device sees as the position of the write head, for
example if in a previous life we were writing some
blocks to the write head but crashed before we could
checkpoint.
Thus, we only use this for non-sequential-write zones,
and leave the write head of sequential-write zones to
the device.

Fundamentally, the only part of the Turf format that
*needs* a conventional zone is the header block, which
is used to identify devices in an array and for
cryptographic shares.
However, as this block is always intended to be
overwritten during rare array-reshape operations, we can
take the rule that the header is in the first conventional
zone, and if there are no conventional zones, it takes up
the entire first sequential-write zone, which is always
emptied at each modification of the header (i.e. at each
reshape request).

Hole Plugging Vs Deferred GC Freeing
------------------------------------

Hole plugging is a technique where, instead of
garbage collecting an entire segment and removing
live data from it, we instead "plug holes" by
identifying the dead data in it, and overwrite those
data.

This can be done in two forms:

* Garbage collection: select *two* victim segments,
  and move the live data from one into the holes of
  the other.
* Log: Modify the log head so instead of writing
  contiguously within a segment, it threads through
  the segment's holes.

Hole plugging is known to be less effective than
just garbage collecting when capacity utilization is
low, but becomes more effective when capacity
utilization is high.
As capacitiy utilization rises, the garbage collector
needs to be invoked more often, diverting drive
bandwidth from user operations.

F2FS uses an adaptive logging form, where at low
capacity utilization it logs into entire contiguous
segments and occassionally invokes the garbage collector
to make segments contiguously free, but at high capacity
utilization it stops calling the garbage collector and
starts hole-plugging its log.
See also:
https://citeseerx.ist.psu.edu/viewdoc/download;jsessionid=C6E2E8127FF636ADB66E3BF32785DB51?doi=10.1.1.73.6701&rep=rep1&type=pdf

Unfortunately, we cannot use any form of hole plugging
in a zoned device.
Sequential-write zones are sequential-write, and cannot
support hole plugging.

This paper, however, suggests an alternative for keeping
garbage collection overhead low: GC Journaling:
http://nyx.skku.ac.kr/wp-content/uploads/2014/07/07177770.pdf
It compares GC journaling (without hole plugging) to adaptive
logging and finds them to have similar performance.

The idea is that garbage collection needs to synchronize
the locations of moved sectors before actually freeing
the segments containing old locations.
This requires flushing caches and writing the superblock
to the checkpoint area, before continuing operations, to
ensure that if a crash occurs, either the old locations
are in use but have never been overwritten, or the new
locations are in use and the old locations may or may not
have been overwritten.
This is the main cause for the reduced performance of
garbage collection at high utilization, as the GC is
invoked more often and disruptive cache flushes occur.

Rather than the garbage collector immediately forcing a
heavy `fsync` when it finishes cleaning a segment, we
defer freeing the segment; we just note it in the
in-memory superblock.
When the superblock is actually written later by an
application `fsync`, or on a periodic `fsync` timer,
that is when the segments are actually added to the
free segments.
If GC pressure increases with such to-be-freed segments,
only then do we force an `fsync` to synchronize the
freeing.
If after a GC cycle the system crashes befoer the
superblock is put on disk, then the segments cleaned
were never freed and the on-disk state is still
consistent with the pre-GC state, except that some
zones may now have "useless" data (the data that was
salvaged from the cleaned segment, which has reverted
back to its unfreed state).
