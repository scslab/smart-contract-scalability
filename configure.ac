#
# Copyright 2023 Geoffrey Ramseyer
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

AC_INIT([smart-contract-scalability], [0])
AM_INIT_AUTOMAKE([subdir-objects])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])

OPT_FLAGS="-march=native -O3 -flto=auto"
DEBUG_FLAGS="-ggdb -O0 -march=native"

CXXFLAGS="-std=gnu++2b $DEBUG_FLAGS $CXXFLAGS"
CXX="g++"
dnl clang --sysroot=/Library/Developer/CommandLineTools/SDKs/MacOSX.sdk

AC_PROG_CXX
AC_PROG_CC
AC_LANG(C++)
AX_APPEND_COMPILE_FLAGS([-pthread])
AS_IF([test -z "${WFLAGS+set}"], AX_APPEND_COMPILE_FLAGS([-Wall], WFLAGS))

CXXFLAGS="$CXXFLAGS $WFLAGS"

AC_ARG_ENABLE([rpc],
[  --enable-rpc    Turn on rpc impl],
[case "${enableval}" in
  yes) rpc=true ;;
  no)  rpc=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-rpc]) ;;
esac],[rpc=false])
AM_CONDITIONAL([USE_RPC_AM], [test x$use-rpc = xtrue])

PKG_PROG_PKG_CONFIG
PKG_CHECK_MODULES([libsodium], [libsodium])
PKG_CHECK_MODULES([tbb], [tbb]);
PKG_CHECK_MODULES([Catch2], [catch2])
PKG_CHECK_MODULES([lmdb], [lmdb])
PKG_CHECK_MODULES([gmp], [gmp])
PKG_CHECK_MODULES([gmpxx], [gmpxx])

if test "x$rpc" == "xtrue" ; then
	AC_MSG_NOTICE(rpcs enabled)
	PKG_CHECK_MODULES([grpc], [grpc])
	PKG_CHECK_MODULES([grpcpp], [grpc++])
	PKG_CHECK_MODULES([protobuf], [protobuf])
	AC_CHECK_PROG([PROTOC], [protoc], [protoc])
	AC_DEFINE_UNQUOTED([USE_RPC], [1], "Define to 1 if RPCs enabled")
else
	AC_MSG_NOTICE(rpcs disabled)
 	AC_DEFINE_UNQUOTED([USE_RPC], [0], "Define to 1 if RPCs enabled")
fi


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
export CXX

AX_PKGCONFIG_SUBDIR(lib/xdrpp)
AC_MSG_CHECKING(for xdrc)
AS_IF([test -n "$XDRC"],,
	    [test -n "$xdrpp_INTERNAL" -a x"$cross_compiling" != xyes],
	    [XDRC='$(top_builddir)/lib/xdrpp/xdrc/xdrc$(EXEEXT)'],
	    [AC_PATH_PROG(XDRC, [xdrc])])
if test -z "$XDRC"; then
   AC_MSG_ERROR(Cannot find xdrc)
fi

AC_MSG_RESULT($XDRC)
AC_SUBST(XDRC)

ABS_TOP_BUILDDIR=$ac_pwd
ABS_TOP_SRCDIR=$ac_pwd
export ABS_TOP_BUILDDIR
export ABS_TOP_SRCDIR

AC_DEFINE_UNQUOTED([TESTS_RELDIR], ["$ac_pwd"], [top source dir: used for tests in submodules to find data files])
AC_DEFINE_UNQUOTED([WASMAPI_TESTS_RELDIR], ["$ac_pwd/lib/wasm_api/"], [wasm api tests subfolder])

export xdrpp_INTERNAL

global_XDRC=$XDRC
export global_XDRC

export xdrpp_CFLAGS
export xdrpp_LIBS

dnl AX_PKGCONFIG_SUBDIR(lib/wasm_api)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
