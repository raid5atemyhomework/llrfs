Turf Transactions
=================

Turf provides atomicity to higher layers.
This allows upper layers to avoid having to build their own logging
system just to avoid inconsistencies.

It is also used to plug the RAID5 write hole.
With Turf transactions, partial stripe writes cannot be
desynchronized; either the transaction containing both
data and parity updates committed to all disks in the array,
or the transaction failed and no disk has any updated data.

Transactions may abort due to encountering an unresolvable
inconsistency at the Turf layer.
Higher layers need to retry the transaction if any operation
fails due to a "needs-retry" failure.

Transactions in Turf are implemented as a form of multi-version
concurrency control but using references to command buffers
instead of numeric versions.

Command Buffer Versioning
-------------------------

Every command buffer node accessible from the root reference
represents a new version of the virtual storage space.
Thus, transactions can implement a form of MVCC.

Part of the global root structure is an in-memory 64-bit
version.
This is only used to quickly coordinate between transactions,
and is not recorded on-disk.

Transactions, when begun, can lock the global root, record the
root reference for all array devices, and record the version
of the global root, then unlock.
Then in the future, transactions can check if another
transaction has conflicted with it by re-locking the global
root, and checking if the global root version is different.
If it is different, the transaction traverses command buffers
from the root reference back to to the recorded root references
for the transaction, and checking if any of its read or written
blocks were in an intervening command buffer.
If so, the transaction must be aborted.

Transaction Operation
---------------------

When a transaction is begun, we optimistically begin it in a
"read-only" mode.
As mentioned, it locks the global root, records the global root
references into transaction root references, copies the global
version to the transaction version, and then unlocks the global
root.

Transactions keep an "accessed" set, implemented as a Bloom
filter.
Reads simply use the sampled root references for lookup, and
also update the transaction's accessed set.

If a transaction is committed or aborted while in read-only
mode, then we do nothing, just free the memory resources of
the transaction.

When a write is performed on a transaction that is currently
in read-only mode, we re-lock the global root.
We then perform validation: if the global version is still
the same as our recorded version, everything is fine and
we unlock the global root and switch to read-write mode,
and perform the write.

Otherwise, we copy the global root refrences to a
transaction-local variable "current root reference", and
unlock the global root, then for each device traverse from
the current root references to the transaction root references.
For each command buffer, we check each entry if it exists
in our accessed set.
If any entry is in our accessed set, we abort the
transaction.
Otherwise, if we reach the transaction root reference without
seeing any updates that conflict with our accessed set, we
change our transaction root references to the current root
references, our global version to the current global
version, and switch the transaction to read-write mode.

Once a transaction is in read-write mode, we keep track
of a "currently building command buffer" for each device,
containing a sorted array of blocks.
We send the data to be written to the log head and the
address of the block to the currently-building command
buffer.

If the currently-building command buffer for a device is
full, we also send it to the log head, point the
transaction root reference of the appropriate device
at it, and clear the currently-building command buffer.

In read-write mode, read lookups are done by looking up
the currently-building command buffer first, then on the
transaction root references.

When a transaction in read-write mode is committed,
we lock the global root and perform validation.

If the global version is the same as the transaction
global version, we unlock the global root, then flush
out any non-empty currently-building command buffers
(which updates our transaction root references).
Then we lock again, and check if the version is still
the same.
If the version is still the same, we then copy the
transaction root references into the global root
references and increment the global version, then
unlock and free transaction resources and signal
successful commit of the transaction.

If either version check fails, we copy the global
root references into another "current root references"
array, and copy the current global version, then unlock
the global root.
For each device, we perform validation, traversing
command buffers until we reach the transaction
root reference, looking for anything that modifies our
accessed set.
If an intervening command buffer has modified any
block in our accessed set, the transaction is marked
as needing retry and its resources freed.

If validation still succeeds, we re-create our
command buffers starting from the latest root reference
(transactions could keep in-memory copies of the
command buffers), setting the last re-created
command buffer as our current root reference.
Then we set our transaction root reference to the latest
root reference and our transaction global version to
the latest global version, and retry the commit.

Concurrency With Garbage Collector / Command Processor
------------------------------------------------------

The garbage collector and command processor, unfortunately,
break our transactions by invalidating the initial root
references of our transactions.

A simple solution would be to simply fail transaction
validation, if we reach a B+ tree root node instead
of a command buffer without seeing the initial root
reference.
This implies that either the garbage collector or the
command processor has modified the metadata tree in the
meantime, invalidating our initial root reference.

However, the above solution does imply that validation
work is done for transactions that are known to be doomed.
An alternate, simpler solution would be to use an in-memory
wraparound counter protected by the global root lock.
This counter is *separate* from the global version counter.
Transaction-begin records the value of the counter into a
transaction variable "transaction gc generation".
Whenever transactions lock the global root in order to
check the global version, they first check if the
GC generation counter matches the one recorded in the
transaction.
If they differ, the transaction simply aborts.

The garbage collector and the command processor then
increment this GC generation whenever they change the
global root references, while protected by the lock.

The counter is never recorded on-disk and is reset to 0
at each array open; it is only used to coordinate between
transaction-using processes and the command processor /
garbage collector.

Transaction Commit Vs `fsync`
-----------------------------

Transaction commits ensure atomicity of updates, but do
not imply that the changes have been flushed to the
backing storage devices.

A separate `fsync` is what actually writes the superblock
out to the backing devices.

Sub-Transactions
----------------

Because transactions are implemented at the lowest layer
of LLRFS, false sharing may appear at Turf which may
actually be easily resolved by higher layers.

For example, the Grass layer uses erasure coding to get
redundancy.
Suppose we have a simple array with data blocks `A` and
`B` and parity block `P`:

    A    B    P

Suppose a layer on top of Grass starts two transactions,
one to update `A` and the other to update `B`.
The first transaction will cause Grass to perform writes to
`A` and `P`, while the second will cause Grass to perform
writes to `B` and `P`.
Because they both modify `P`, however, Turf will abort one
or the other and force it to completely restart, *including*
posting the write to their data block `A` nr `B`.
This wastes bandwidth at the Turf interface.

Turf provides sub-transactions, where the client can begin a
transaction inside another transaction, with each transaction
only having at most one sub-transaction running at any one
time.

The rules for sub-transactions are simple:

* Sub-transactions start at read-only mode and copy the
super-transaction's current root references as the
sub-transaction's current root references.
They copy the initial root references of the super-transaction.
* While a sub-transaction is running you cannot perform any
operations on its super-transaction, i.e. only the innermost
sub-transaction supports operations.
* When a sub-transaction transitions to read-write mode or
wants to commit, it performs validation on the *global* root.
At each command buffer it validates from the super-most
transaction's acessed set, downwards towards the
sub-transaction's accessed set.
If validation fails for a transaction, that transaction and
all its *sub*-transactions are restarted.
* Sub-transactions commit to their direct super-transaction's
current root references if validation succeeds.
They also merge their accessed set with their super-transaction.

To avoid the false sharing of `P` above, Grass can do:

1.  Start transaction.
2.  Read old `A`/`B`.
2.  Write `A`/`B`.
3.  Start sub-transaction.
4.  Read old `P`.
5.  Compute new `P` from XOR of old `P` and old `A`/`B` and new `A`/`B`.
6.  Write new `P`.
7.  Commit sub-transaction.
8.  Commit transaction.

In the above, if the other transaction got to write to `P`
first, then only the inner sub-transaction has to abort and
be restarted; the outer transaction that writes to `A`/`B`
does not need to re-perform the write.
This is helpful if larger amounts of data need to be written
that then needs to be indexed.
