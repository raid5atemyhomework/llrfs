Turf Transactional Storage Array
================================

The Turf layer combines multiple devices and creates a
transactional, encrypted array on top of the devices.
It is thus responsible for:

* Transactions.
* Log-structured Storage.
* Encryption.

Transactions
------------

To prevent corruption due to crashes, Turf needs to
implement transactional semantics.
Indeed, the so-called RAID5 write hole is due to the
lack of a transaction modifying both a data block
and the parity of that stripe in a single atomic step.

Now, if the Turf internal transactional system was
not exposed to higher layer, then higher layers will
need to implement a transaction system at their layer
too.
As transactions require some kind of log or journal,
we would encounter a "log on a log" situation:
https://www.usenix.org/system/files/conference/inflow14/inflow14-yang.pdf

This is especially relevant to LLRFS, which is
designed as multiple layers.
We want to collapse the logs to the lowest layer.
This requires that Turf expose transactions to higher
layers.

Turf is a simple key-value store, with some keys
storing entire data blocks, and others store small
pieces of data.
Data block keys are per-device (i.e. Turf exposes the
members of the underlying array) but small-data keys
are stored on all devices.

Transactions provided by the Turf layer allow both
insertions and deletions (i.e. "TRIM") to the store to
be executed in a single transaction.

Transactional file systems are not a new idea, and both
F2FS and NVMFS/DirectFS expose multi-block writes at
the file system level.
SQLITE3 supports the F2FS API, while MariaDB/MySQL
suports the NVMFS API, though both APIs are not
well-documented.
XFS designers have also proposed an `O_ATOMIC` open
flag, and have also proposed an alternative
atomicity primitive, `FIEXCHANGERANGE`.

Log-structured Storage
----------------------

Traditionally, transactions are implemented by some kind of
write-ahead log or rollback log.
Both incur a double-write overhead, i.e. a single write
within a transaction translates to a write to the log plus a
write to the "final location".
For write-ahead logs the data is first written in a log, then
later is written to the main location, while for rollback logs
the original data is first written to the log, then the actual
data is written to the main location.

The double write hurts use on flash media, thus failing our
Flexibility (if users avoid flash) or Hardware cost reduction
(if users *do* use flash) goals.

To implement transactions without the double-write overhead,
we use a log-structured storage, akin to log-structured
file systems like the original LFS, NILFS, and F2FS.

Encryption
----------

Encryption is always done on all data that hits storage
devices.

On arrays whose parameters allow the array to be reduced down
to a single operating device, then the encryption key is in
plaintext on all devices, unless the user specifies a
passphrase (in which case the encryption key is masked by
the key-lengthening of the passphrase).

However, on arrays where at least two devices need to remain
operating for the array to continue (i.e. `num_devices -
max_device_fails - num_spare_devices >= 2`) the encryption
key is split using Shamir's Secret Sharing, such that no
single device has the encryption key.
The user may still specify a passphrase in this mode (which
gets key-lengthened), if they wish.

The advantage here is that if at least two devices are
needed in order to open the array, then if the user has one
device fail and RMAs it to the manufacturer, the manufacturer
cannot access any of the user data even if say it was only
the controller that failed but not any of the actual storage
media.
When the user replaces the failing device, the secret is
re-shared; even if another device later fails and is RMAed to
the same manufacturer, because of the re-sharing, the secret
share extracted from the first device is not compatible with
the secret share from the second device.

(Of course, modern storage devices may retain multiple
copies of data, and the old share might still remain in the
second device; so much for encryption.)

Designing encryption into the file system, and using it
pervasively, allows us to use some parts of the encryption
scheme for other purposes:

* In a Shamir Secret Sharing, every shareholder has to
  have a unique X coordinate.
  In the LLRFS context, each device in the array is a
  shareholder.
  As it happens, we need a unique identifier for each
  device in an array --- and the unique X coordinate used
  in Shamir Secret Sharing doubles as the in-array
  identifier for a device.
* Authenticated encryption requires some MAC (message
  authentication code), which ensures that the
  ciphertext is not corrupted by malicious third
  parties.
  Storage devices maliciously lie about how reliable
  they actually are, so the MAC also serves as the data
  checksum.

Aside from the Shamir Secret Sharing scheme using the
field GF(2^8), we define a ciphersuite, a set of
algorithms:

* HASH - inputs a variable-sized bytevector, outputs a
  256-bit output.
* MAC - inputs a secret key and a variable-sized
  bytevector, outputs a 256-bit output.
* AEAD, a pair of algorithms:
  * AEAD-encrypt: inputs a secret key, a 192-bit nonce,
    and a veriable-sized bytevector called the plaintext,
    and an optional variable-sized bytevector called the
    associated data, outputs a 128-bit tag and a
    variable-sized bytevector (called the ciphertext) of
    equal size to the input plaintext bytevector.
  * AEAD-decrypt: inputs a secret key, a 192-bit nonce,
    a 128-bit tag, a variable-sized bytevector called the
    ciphertext, and an optional variable-sized bytevector
    called the associated data, and if the inputs matched
    a previous run of AEAD-encrypt, outputs a
    variable-sized bytevector called the plaintext.
    If the inputs do not match, outputs a failure to
    decode.
* Key-stretch - inputs a 256-bit salt, a variable-sized
  bytevector, and a number of rounds, and outputs a
  256-bit output.

https://github.com/dsprenkels/sss/blob/10e0a90cb64c30a7727e5c67839335da00a63d6e/hazmat.c

