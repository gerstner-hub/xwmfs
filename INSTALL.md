INSTALLATION INSTRUCTIONS
=========================

xwmfs uses autotools as a buildsystem. The following commands will build and
install everything with default settings:

<pre>
./bootstrap
./configure
make
make install
</pre>

For successful compilation the following dependencies are required:

- the x11 Xlib client library
- the FUSE (file system in userspace) library
- a C++20 compatible C++ compiler
- the two custom libraries libxpp and libcosmos are pulled in transparently
  via Git submodules.

For running the file system the fuse kernel module (`CONFIG_FUSE_FS`) also
needs to be available.

Only Linux on x86-64 has been tested by me so far.

There are currently no special configure options available for xwmfs besides
the default options provided by autotools.

DEBUG BUILD
===========

To create a debug build simply set CFLAGS and CXXFLAGS accordingly e.g.

```
export CFLAGS="-g -O"
export CXXFLAGS="$CFLAGS"
./configure
```

UNIT TESTS
==========

You can build `make check` to run a number of unit tests shipped with xwmfs.
These are written in python and run automatically without user interaction.

You can run isolated unit tests yourself by executing the appropriate scripts
from the tests directory. You either need to specify the *xwmfs* binary as
`-b <BINARY>` or via the *XWMFS* environment variable.

The tests are somewhat fragile and depend a lot on the window manager, thus I
do not recommend to run them automatically.
