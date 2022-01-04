Hay Device Cache
================

The Hay Device Cache implements a read cache for the Grass
layer, if the user installs cache devices to the array.
If given multiple cache devices, beyond the `max_cache_fails`
setting, then it implements a writeback cache.

Rationale
---------

Log-structured file systems were originally designed for HDDs, on
the observation that RAM prices and capacities were growing faster
than HDDs, and that it would become practical to store large amounts
of the HDD storage into RAM.
Indeed, the design of modern OSs has been to treat RAM as a page
cache for disk; when an OS loads a program, it does not allocate
RAM for it, it allocates swap space, and then uses RAM as a cache
for the swap space.

Since log-structured file systems tend to "scatter" the storage of
blocks all over disk, reading tends to be less efficient for disk
due to increased seek times.
The idea was that the copious amounts of RAM available would speed
up reads.

However, the trend since then has been for HDD prices per unit
capacity to drop faster than RAM.
Thus, the amount of data that can be stored in on-RAM cache,
compared to HDD capacity, has tended to get lower for typical
systems.

However however, in the interim, non-volatile memories --- flash
packaged into "SSD"s --- have also arisen as common commodity
devices.

These SSDs provide an opportunity to extend the RAM cache with
another layer of caching, with a different speed/capacity
tradeoff --- SSDs are larger, but somewhat slower than RAM.
SSDs are more expensive than HDDs per byte, but are cheaper than
RAM per byte.

Thus, the Hay layer is added to the stack of layers in LLRFS.
If the user decides to use HDDs (which remain cheaper per
byte than SSDs) as backing store, the user also has the
flexibility to add one or a few SSD devices to speed up read
operations.

Encryption
----------

Since Grass goes through the trouble of encrypting data before
it hits any device, Hay should also do this.

Grass needs to expose additional access to encryption services.
These additional operations need to be exposed by Grass to the
Hay layer, with the array encryption key used as the key:

* AEAD encrypt.
* AEAD validate+decrypt.
* HMAC.

Log Structuring
---------------

Cache devices are also log-structured.

Unlike the Grass layer, each cache device is a separate log.

There is no strict assignment of which block goes to which
cache device.
To help speed up lookups, each cache device has a large
Bloom filter of all the entries cached on it.

Read And Read/Write Caching
---------------------------

Depending on the number of cache devices and the
`max_cache_fails` setting, the Hay layer may work as a read
cache, or as a writeback (Read/Write) cache.

The `max_cache_fails` setting defaults to the `max_device_fails`
setting.

If `num_caches < max_cache_fails + 1` then Hay operates *only*
as a read cache.
However, if there are more caches than `max_cache_fails + 1`,
Hay will operate as a read/write cache.
