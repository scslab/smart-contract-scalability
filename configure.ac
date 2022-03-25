AC_INIT([speedex], [0])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

dnl note to future geoff: -flto seems to break lmdb, so don't use it -- 12/2/2021 this is wrong, seems to have been a result of accidental stack use-after-scope
OPT_FLAGS="-march=native -O3 -flto"
DEBUG_FLAGS="-ggdb -O0"

CXXFLAGS="-std=gnu++2a -fcoroutines $OPT_FLAGS $CXXFLAGS"
CXX="g++"

AC_PROG_CXX
AC_PROG_CC
AC_LANG(C++)
dnl AX_CXX_COMPILE_STDCXX(17,,mandatory)
AX_APPEND_COMPILE_FLAGS([-pthread])
AS_IF([test -z "${WFLAGS+set}"], AX_APPEND_COMPILE_FLAGS([-Wall], WFLAGS))

CXXFLAGS="$CXXFLAGS $WFLAGS"

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([libsodium], [libsodium])
PKG_CHECK_MODULES([tbb], [tbb]);

PKG_CHECK_MODULES(LIBFYAML, [ libfyaml ], HAVE_LIBFYAML=1, HAVE_LIBFYAML=0)

if test "x$HAVE_LIBFYAML" != "x1" ; then
	AC_MSG_ERROR([failed to find libfyaml])
fi

AC_SUBST(HAVE_LIBFYAML)
AC_SUBST(LIBFYAML_CFLAGS)
AC_SUBST(LIBFYAML_LIBS)
AC_DEFINE_UNQUOTED([HAVE_LIBFYAML], [$HAVE_LIBFYAML], [Define to 1 if you have libfyaml available])
AM_CONDITIONAL([HAVE_LIBFYAML], [ test x$HAVE_LIBFYAML = x1 ])

export CXXFLAGS

AX_PKGCONFIG_SUBDIR(xdrpp)
AC_MSG_CHECKING(for xdrc)
AS_IF([test -n "$XDRC"],,
	    [test -n "$xdrpp_INTERNAL" -a x"$cross_compiling" != xyes],
	    [XDRC='$(top_builddir)/xdrpp/xdrc/xdrc$(EXEEXT)'],
	    [AC_PATH_PROG(XDRC, [xdrc])])
if test -z "$XDRC"; then
   AC_MSG_ERROR(Cannot find xdrc)
fi
AC_MSG_RESULT($XDRC)
AC_SUBST(XDRC)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
