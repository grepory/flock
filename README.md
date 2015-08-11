# flock

Originally retrieved from kernel.org:

https://www.kernel.org/pub/software/utils/script/flock/flock-2.0.tar.gz

## What?

The version of flock found at kernel.org uses the obsolete SA_ONESHOT.

This version uses SA_RESETHAND, as well it should.

## Why?

I couldn't figure out where to submit patches to the aforementioned distribution
of flock. Should you know where, please contact me here in the Issues section.
