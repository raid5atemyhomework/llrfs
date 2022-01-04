Grass Striping And Erasure Coding
=================================

The RAID5 Write Hole, Fixed
---------------------------

As mentioned in the 0th document, our journey began with the RAID5
write hole.

To fix this, we create a mechanism to atomically update across
multiple devices.
This atomic update is implemented by log-structuring each device,
thus avoiding overwrites in case of desynchronization, and then
updating all devicess atomically if a disk synchronization is
triggered.

This means that we log-structure each device first, then create
an atomic update for the array (to close the RAID5 write hole),
and *then* RAID on top of that.

An alternative design would have been to put the RAID at the
bottom and then log-structure on top of the RAID, with the rule
that the log must write entire stripes, avoiding the RAID5 write
hole by never having a partial stripe write.
This would have improved performance, as we would never need to
do a read-modify-write implied by a partial stripe write.
However, this would have required that every device in the array
have the same zones, and thus would fail the flexibility goal
(devices with different zonings would not be able to coexist in
the same array), and flexibility is higher in our priority list
than performance.

How do we ensure that partial stripe writes do not negatively
affect the primary goal of reliability?
Suppose we have three devices in a RAID5, with data blocks
`A`, `B`, and parity `P`:

    A    B    P

If `B` is updated, then `P` must also be updated.

Grass fixes this by not having each log-structured disk have its
own checkpoint area (i.e. what the 0th document has been calling
the superblock area).
Instead, each superblock/checkpoint contains either the state of
both devices being updated, or neither.
This atomicity ensures that partial stripe updates do not cause
data loss, and closes the write hole.

Stripes
-------

The array is allowed to have devices of unequal size, to support
the goal of flexibility.
To handle this, Grass splits the larger devices, combines them
in an array with the smaller devices, then (if still above the
`max_device_fails`) combines the remaining space of the larger
devices.
Finally, it effectively SPANs the sub-arrays.

For example, if there are 1Tb, 2Tb, and 2Tb devices in an
array with `max_device_fails == 1`, the 2Tb devices are split
into 2x 1Tb halves, the first half of both 2Tb devices are
RAID5'ed to the 1Tb device, and the remaining half of both
devices are RAID1'ed together, then the RAID5 and the RAID1
are SPANned together.

The details will be described in a later document.
For now, for the above example, we shall call the RAID5'ed
region a "span", and the RAID1'ed region is another "span".

Basically, a "span" is a region across all devices, where
the stripe width is fixed.
In the above example, the RAID5'ed span has a stripe width
of 3, and the RAID1'ed span has a stripe width of 1.

Striping, then, is simple: we have a mapping from the
virtual sectors exposed by Grass to stripes on the array.
Reads are redirected to the correct address on the log
structured storage of each device.
Single writes imply writes across multiple devices, but
we assure atomicity across devices by the shared superblock.

Improving RAID5 Write Latency
-----------------------------

When we encounter a single-block write to a RAID5 region,
we suffer a read-modify-write overhead.
At the minimum, we read the old version of the data block,
the old version of the parity block, and then XOR the old
data block with the new data block, then XOR it again with
the old parity block, to generate the new parity block.
Overheads are even worse when we use higher number of parity
blocks.

Writing a full stripe is more efficient since there is
no need for the reads of old data to be done.
If only we could instead write a shorter-than-normal, but
still full stripe, then we would avoid the above partial
write overhead.

Actually we can.

We increase the keyspace of the disks, appending an
additional byte to the high end.
We need this anyway to store metadata about segments.

Suppose we have a stripe at block index 0 of all
devices:

    A    B    P

We have mapping of `0x0 0000000000000000` on all devices
for that stripe.

Then we get a one-block update of block `B`.
We then compute a short stripe, containing only the new
version `B'` and the parity for a two-block stripe with
one parity `P'` (this is really just RAID1, but the
same concept generalizes for larger number of devices
with two or more blocks that are not enough for the
entire stripe).

We store the short stripe into address `0x1 0000000000000000`.
We also store a bitmap into key `0x2 0000000000000000`,
indicating that the short stripe is composed of `B'` and
`P'`, in the same atomic step as storing the short stripe
(we store this bitmap on all devices that the stripe is on, so
device containing `A` also has to write something, but all those
writes are done in parallel and since each device is
log-structured anyway, writes are fast).
We can then signal completion to higher layers, as the data is
now reliably stored on-disk.

However, we still need to eventually merge the short stripe
back into the "main" stripe.
Thus, even if the higher layer has completed, we still have
to read in the old `B` and `P` blocks and compute the parity
for the full stripe, `P''`.
Then, we atomically delete the `0x1 0000000000000000` blocks,
remap the storage location of `B'` to `0x0 0000000000000000`,
write the new `P''` to the same address on the parity disk,
and delete the bitmap at `0x2 0000000000000000`.

To support the above, the garbage collector has to be modified.
Data blocks which were originally at `0x1 xxxxxxxxxxxxxxxx`
may have since been remapped to the corresponding
`0x0 xxxxxxxxxxxxxxxx`, so if it sees the former mapping
while scanning blocks in a victim segment, it has to check
both the `0x0` and `0x1` addresses.
Fortunately, at each atomic snapshot there is only one mapping
for a physical sector.
As an additional note, such remaps must also be handled by
the garbage collector when it sees that transactions have
occurred after it has gotten its snapshot, i.e. if a data
block is live at `0x1 ...` and when it goes update the root
it sees that the root has changed since it snapshot, it should
re-check for remaps from the root to its snapshot as well,
and correct them.

On array open, we must also scan the keyspace for
`0x2 xxxxxxxxxxxxxxxx`, and if there are any bitmaps, launch
background tasks to perform the short-stripe merging into
the full stripe.

This is not a pure performance optimization, unfortunately,
since it trades off raw throughput for faster latency on
partial-stripe writes.
The third device here needs to write parity twice, and the
first device also has to store at least the bitmap updates
for reliability.
This also increases pressure to garbage collect *and*
complicates garbage collection.
However, higher layers may expose the tradeoff to users,
by e.g. exposing a per-volume or per-inode setting similar
to ZFS `logbias=throughput|latency`.
In particular, for SSD, this optimization increases wear
due to the extra writes involved, which fails our hardware
cost goal (which is higher priority than performance), so
our default setting should be to not use this technique.

Improving RAID5 Read Speed
--------------------------

Common wisdom holds that RAID1 is faster on read than
RAID5 or RAID6.
This is because, on a RAID1, if one disk is busy reading
some particular block, we can read a different block
residing on a different disk without waiting for the
first disk.
On a RAID5/6, if we want to read two distant blocks
residing on a single disk, the second read has to wait
for the first read to finish.

However, we should note that we can generalize RAID1
as really being a parity scheme which happens to have
only one data block per stripe.
A two-disk RAID1 is really a RAID5 with one data disk
and one parity disk, and the XORs of all the data disks
are just the actual values of the data disks, thus the
parity just so happens to *coincidentally* be equal to
the data disks.
Thus, what the RAID1 optimization is actually doing
is recovering the data using the parity computations!

Suppose we have this RAID5 setup:

    A  B  P1
    C  P2 D

Suppose we get a read for block `A`, followed by a
parallel read for block `C`.
Instead of the read for block `C` waiting for the
disk to finish with block `A`, we read `P2` and `D`
and recover `C`.
This is useful if the second and third devices are not
busy anyway.

This is a pure performance improvement, though we do
need estimates on how busy disks are in order to
use this technique.
