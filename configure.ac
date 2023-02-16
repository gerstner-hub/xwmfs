dnl process this file with autoconf to produce a configure script.

dnl we want to have a current autoconf for producing the configure script
AC_PREREQ([2.71])

dnl Our wmfs application
AC_INIT([xwmfs],[0.92],[Matthias.Gerstner@nefkom.net])

dnl Stores some generated files in a separate directory for cleaner structure
AC_CONFIG_AUX_DIR(config_aux)

dnl produces config.h to be the config header produced by configure
AC_CONFIG_HEADERS([config.h])

dnl pull in automake in a current version
AM_INIT_AUTOMAKE([1.10.1 gnu])

dnl check for C and C++ compilers (we need a C99 compiler for fuse structure setup)
AC_PROG_CC
AC_PROG_CXX

dnl check for asciidoc availibility, optional
AC_CHECK_PROG([asciidoc], [a2x], [yes], [no])
AM_CONDITIONAL([FOUND_ASCIIDOC], [test "x$asciidoc" = xyes])

AC_COMPILE_IFELSE([AC_LANG_PROGRAM(
[[#ifdef __clang__
	#error no gcc
#elif !defined(__GNUC__)
	#error no gcc
#endif
]])], [is_gcc=true], [is_gcc=false])

AM_CONDITIONAL([IS_GCC], $is_gcc)

AC_ARG_ENABLE(dev, AS_HELP_STRING([--enable-dev],[set extra developer mode options like -Werror compiler flags]), [enable_dev=${enableval}])
AM_CONDITIONAL(DEV, test "${enable_dev}" = "yes")

dnl we need fuse
dnl AC_CHECK_LIB([fuse], [fuse_main], [], [AC_MSG_ERROR([You need the fuse userspace library to build this package])])
PKG_CHECK_MODULES([fuse], [ fuse >= 2.8.1 ])
dnl we also need X11
PKG_CHECK_MODULES([x11], [ x11 >= 1.2.2 ])

AC_SUBST([fuse_CFLAGS])
AC_SUBST([fuse_LIBS])

AC_SUBST([x11_CFLAGS])
AC_SUBST([x11_LIBS])

dnl generate the actual output
AC_CONFIG_FILES([Makefile])

dnl We need to specify an output file for every single Makefile that is generated in the project
dnl (i.e. for every subdirectory, too)
AC_CONFIG_FILES([
	src/Makefile
	docs/Makefile
	tests/Makefile
])
AC_OUTPUT
