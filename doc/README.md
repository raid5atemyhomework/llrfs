Layers
======

LLRFS is composed of multiple layers.
The layers are, from lowest to highest:

* Turf Transactional Storage Array
* Grass Redundant Storage
* Hay Device Cache
* Cow Storage Sharing Infrastructure
* Human File System Interface

Rationale
---------

Inter-layer interfaces must necessarily be limited; if the
interface between two layers is too "thick", one would start
to wonder if there is a true separation between the layers.
Yet it is precisely because the interface between layers is
limited that inefficiencies will inevitably exist.

We should recall our design goals, which is an *ordered*
list:

1.  Reliability.
2.  Hardware cost reduction.
3.  Flexibility.
4.  Performance.

Layers allow for improved reliability while *implementing*
LLRFS.
We can create unit tests for a single layer at a time,
emulating the interfaces for upper and lower layers,
and thus be able to perform focused testing on smaller
parts of the codebase before integration with the rest.

Layers also help support flexibility.
For instance, Grass hides the shape of the underlying
array from higher layers, exposing only a fixed-size
block device to the Cow layer.
When the user reshapes the array, Grass can continue
providing the same interface, only indicating an
impending change in the size of the blok device, and
higher layers continue operating completely ignorant
of the changes hidden away by Grass.
As another example, the Turf layer does not use a
full-tracing garbage collector (which would be a
heavy load, especially just to clean a few segments
at a time), because it does not implement storage
sharing.
Storage sharing being a separate layer (Cow) lets
Grass have a simpler, faster garbage collector;
Cow uses refcounts for a similar purpose, but due to
the log-structured nature of Turf it has to move
blocks, and thus refcounts cannot be used at the
Turf layer.

Turf Transactional Storage Array
--------------------------------

The lowest layer, Turf, takes one or more backing devices
and overlays a transactional update system on top.
The rest of the layers rely on the existence of this
transaction system.

Turf exposes the underlying devices to higher layers, it
just provides a mechanism for ensuring that updates to
multiple different devices are done atomically, i.e.
either all intra-transaction updates on multiple devices
succeed, or none of them are recorded.
Higher layers must still manage the multiple devices.

As a convenience, Turf also provides a small key-value
store that is replicated on all devices.

Grass Redundant Storage
-----------------------

The next layer, Grass, takes the multiple backing devices
exposed by Turf and provides a single block device (with
transactional capability) to the higher layers.
Aside from typical read operations, the provided
block device supports performing multiple write and trim
operations in a single atomic transaction.

The user controls these parameters of Grass:

* The devices used for backing.
* `max_device_fails`.
* `num_spare_devices`.
* `num_device_groups`.

Hay Device Cache
----------------

The next layer up, Hay, takes the Grass block device, and
several fast storage devices to be used for caching the
Grass virtual device.
It defaults to being a plain read cache, but if there are
enough cache devices beyond `max_cache_fails` it becomes a
writeback cache.
It otherwise provides the same interface as Grass.

The user controls these parameters of Hay:

* `max_cache_fails`, defaults to `max_device_fails`.

Cow Storage Sharing Infrastructure
----------------------------------

Cow takes the emulated block device from the Hay/Grass
layer, and uses it to back a virtual, sparse, 128-bit
address space of 4096-byte blocks.
In addition, Cow allows storage sharing, letting one
part of the virtual space be backed by the same
blocks as another part;
if either part is later modified, they are given
distinct unshared ranges instead, i.e. copy-on-write
(COW).

The Cow device also supports transactional multi-block
writes, just like the lower layers.

Human File System Interface
---------------------------

The topmost layer is Human, which takes the sparse
128-bit address space from Cow and implements a
POSIX filesystem out of it.
Its design is inspired by DirectFS/NVMFS.

As POSIX does not define transactions, Human does
not expose the underlying transaction facility
provided by Turf and other lower layers.
However, extensions to POSIX functionality to
provide atomic updates can be implemented by Human.
