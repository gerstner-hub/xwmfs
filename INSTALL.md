INSTALLATION INSTRUCTIONS
-------------------------

xwmfs uses autotools as a buildsystem. The following commands will build and
install everything with default settings:

<pre>
./bootstrap
./configure
make
make install
</pre>

For successful compilation the following dependencies are required:

- the x11 client library
- the fuse (file system in userspace) library
- a C++11 compatible C++ compiler

For running the file system the fuse kernel module (`CONFIG_FUSE_FS`) also
needs to be available.

Only Linux on x86-64 has been tested by me so far.

There are currently no special configure options available for xwmfs besides
the default options provided by autotools.

DEBUG BUILD
------------

To create a debug build simply set CFLAGS and CXXFLAGS accordingly e.g.

	- CFLAGS="-g -O"
	- CXXFLAGS="$CFLAGS"

before running configure.
