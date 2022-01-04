Grass Atomic Redundant Storage
==============================

The Grass layer is responsible for these:

* Redundancy/erasure coding.
* Array Shape Management.

The Grass layer is log-structured.

Erasure Coding And Storage Pooling
----------------------------------

The Grass layer takes a number of devices, `num_devices`, and
combines their storage.
To combine that storage, the Grass layer gets the following
settings from the user:

* `0 <= max_device_fails <= 128`
* `0 <= num_spare_devices <= 1`
* `1 <= num_device_groups <= num_devices / 2`
  * `(num_devices - num_spare_devices) % num_device_groups == 0`
  * `(num_devices - num_spare_devices) / num_device_groups >= (max_device_fails + 1)`

The `max_device_fails` is how many devices are allowed to
fail simultaneously while the array can still continue
serving correct data.
If more than that number of devices fail before the array
can resilver, then the array becomes broken and data is lost.

The `num_spare_devices` is how much to overprovision; the
total storage area is discounted by the size of that many
devices.
This allows for automated recovery in case an entire
device fails; the Grass layer will start resilvering data
from the failing device, knowing there is enough spare
space to do this without user intervention.

`num_device_groups` increases the speed at which resilvering is
done, and affects read and write speeds as well.
It trades off capacity for operating speed; if
`num_device_groups == 1` then the Grass layer can provide the
greatest capacity for the given number of possible failures, and
the given number of spare devices.
Increasing the `num_device_groups` will speed up resilvering speed
and read and write operations, but lowers the available capacity
of the array.

