Hay Cache Device Format
=======================

The Hay device format is very similar to the Grass device format
and Hay uses a lot of the code of Grass.

Some differences are noted here.

Header
------

A Hay cache device has a different magic number from a Grass backing
device.

Like Grass, a Hay cache has a unique identifier.
Unlike Grass, a Hay cache has no in-array device identifier and
does not have any Shamir Secret Sharing shares.

Instead, after the unique identifier, the Hay cache has a MAC.
This is the MAC of the Hay cache unique identifier, using the
array encryption key.

When an array is opened, Hay will look for devices with the Hay
magic number and check if the MAC (using the array encryption
key) of their unique identifier matches with the recorded MAC.
If so, it has identified a Hay cache device.

Bloom Filter
------------

Each Hay device has two Bloom filters, a "current" filter and an
optional "next" filter.

* The current filter is mapped to virtual block at address
  0x FFFF FFFF FFFF FFFF.
* The next filter, if present, is mapped to virtual block at
  address
  0x FFFF FFFF FFFF FFFE.

It does imply that arrays of the maximum size that LLRFS is
capable cannot be fully cached, as those virtual addresses are
reserved for Bloom filters, but that is unlikely (and if you
have an array that large, you probably will not care if two
piddling 4096-byte blocks are not cacheable).
