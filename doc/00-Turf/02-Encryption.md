Turf Encryption
===============

Turf encrypts data before it hits any storage devices.

Since LLRFS is run on an array, Turf can encrypt with a key that is
stored across devices of the array, by using Shamir's Secret Sharing
scheme.
This means that as long as the array is configured so that more than
one device is necessary for it to operate, no single device has the
encryption key.
This allows encryption without requiring that users input a passphrase
every time the file system is mounted.

As a fallback, if the array is configured so that it can survive down
to just one device, the Turf layer will store the key effectively in
plaintext.

The user may configure one passphrase as well, regardless of the
configuration, to protect the encryption key.

Digression: Disk Encryption Is Hard
-----------------------------------

The fastest schemes generate a random bit sequence that is XORed
with the plaintext to generate the ciphertext.
If the bit sequence is indistinguishable from true noise, then no
pattern in the plaintext will be visible in the ciphertext.
Generally, the random bit sequence is generated using some
deterministic algorithm from the encryption key.

However, care must be taken when using this mode.
There are a number of issues:

* Even if the plaintext is not visible, attackers can still try
  to XOR a bit in the ciphertext to cause a bitflip in the
  plaintext, if they have an idea of how the plaintext is
  structured.
  For example, if the plaintext indicates how much salary some
  employee is supposed to have, the employee can give themselves
  a raise from say $1,000 a month to $5,000 a month by flipping
  bit 2 of the first byte of their salary.
* Storage devices are huge compared to the individual blocks
  that are read from them.
  Typically, we want to read from and write to a small selection
  of the blocks on the storage device.
  Ideally, that means that we should be able to generate the
  masking bit sequence without having to start it from the start.

Both issues are fixed by using Authenticated Encryption (AE).
Authenticated Encryption schemes have the following:

* Aside from the encryption key, the AE algorithm accepts a
  "nonce", a number only used once.
  If we can associate a unique nonce for each block on the
  array, then we can generate unique bit masks for each block
  without having to generate bit masks for the entire device.
  * We already have a metadata tree that is strictly acyclic; we
    can store the nonce for blocks we reference with the metadata
    node that is referencing, up to the superblock.
* After encryption, a Message Authentication Code (MAC) on the
  ciphertext is generated.
  In AE this MAC is known as the "tag".
  Conceptually, this MAC is a hash function that takes the
  encryption key and some data, and outputs a digest.
  If the ciphertext is tampered with, the MAC will not match
  and we can treat it as bitrot.
  Because the encryption key is part of the MAC input, and is
  presumed to be unknown to an attacker, if they tamper with the
  ciphertext, they will not be able to generate the correct MAC
  for the tampered ciphertext.
  * Similarly to the nonce, we can keep the tag with the
    metadata node referencing the encrypted block.

Both metadata nodes and data blocks are encrypted.
The root of the metadata tree, the superblock, is also encrypted.

Authenticated Encryption with Associated Data (AEAD) is an extension
of AE which modifies the MAC and bit mask according to some data
(the "associated data") known at both encryption and decryption.
The associated data is intended to form a "context", to prevent
replay attacks where an attacker replays ciphertext from one context
in a different context, where it may successfully get decrypted
but be misinterpreted.

For data blocks, the associated data is "LLRFSTURFDATA" without
any C terminating nul character.

For metadata nodes, the associated data is "LLRFSTURFMETA"
without any C terminating nul character, followed by a
little-endian 64-bit number, the distance from the B+ tree
root.
The B+ tree root has an index 0, the first command buffer
from that has index 1, and incrementing by one until the
superblock root reference.
B+ tree internal and leaf nodes have an index that is the
distance from the B+ tree root.

For the superblock, the associated data is "LLRFSTURFSUPER"
without any C terminating nul character.

Ciphersuites
------------

Encrpytion algorithms, once broken, cannot be *un*broken, so
we must be prepared to change the algorithm.

Typically, on most protocols that include encryption, there is
some kind of version number that is transmitted in plaintext.
This version number or indicator effectively selects the set of
encryption algorithms.

We observe that a Shamir Shared Secret scheme is unlikely to be
replaced.
Encryption algorithms get broken because they often have some
component that is modeled as a true random oracle, but in
practice that random oracle is implemented using non-mystical
parts that might individually have some weakness which then leads
to the encryption algorithm as a whole being broken.
However, SSS by itself is information-theoretic secure.

What we can do would be to *also* secret share the version number.
Once a sufficient quorum of devices has been assembled, we can
perform the SSS extraction and determine the version number.

We use a 256-bit encryption key for the header, which is
secret-shared across the array devices.
We can take the top 8 bits as the version.
This means there is 248 bits of entropy for the encryption key,
which should be more than sufficient.

We define the following sets of algorithms:

* Version 0:
  * HASH: SHA3-256.
  * MAC: HMAC-SHA3-256.
  * AEAD: XChaCha20-Poly1305, 64-bit counter variant.
    * 192-bit nonce, 128-bit tag.
  * Key-stretch: Argon2id.

We expect that low-numbered ciphersuite versions will predominate,
thus we secret-share the version number as well.

Secret Sharing
--------------

The below code is compatibly licensed, and should be used as the
basis of our secret sharing code.

https://github.com/dsprenkels/sss/blob/10e0a90cb64c30a7727e5c67839335da00a63d6e/hazmat.c

Secret sharing is on `GF(2^8)` with polynomial `x^8 + x^4 + x^3 + x + 1`.

