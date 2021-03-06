AC_INIT([avecado],
	[0.0.1],
	[https://github.com/MapQuest/avecado/issues],
	[avecado-0.0.1],
	[https://github.com/MapQuest/avecado])
AM_INIT_AUTOMAKE([subdir-objects parallel-tests])
LT_INIT
AC_SUBST([LIBTOOL_DEPS])

AM_SILENT_RULES([yes])
AC_CONFIG_HEADERS([include/config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_HEADER_STDC
AC_LANG_CPLUSPLUS

AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])
AX_BOOST_BASE([1.56], , [AC_MSG_ERROR([cannot find Boost libraries, which are are required for building avecado. Please install libboost-dev.])])
AX_BOOST_PROGRAM_OPTIONS
AX_PTHREAD
AX_BOOST_THREAD
AX_BOOST_ASIO

PKG_CHECK_MODULES([LIBCURL], [libcurl >= 7.30])
AC_SUBST([LIBCURL_CFLAGS])
AC_SUBST([LIBCURL_LIBS])

AM_PATH_PYTHON([2.6])
AX_BOOST_PYTHON
AM_CONDITIONAL([HAVE_BOOST_PYTHON], [test -n "$BOOST_PYTHON_LIB"])

REQUIRE_MAPNIK([3.0.0])
REQUIRE_PROTOC

# check for SQLite, which we use for HTTP cache information
AX_LIB_SQLITE3([3.6.16])
AM_CONDITIONAL([HAVE_SQLITE3], [test -n "$SQLITE3_VERSION"])

# optionally enable coverage information
CHECK_COVERAGE

AC_CONFIG_FILES([Makefile])

# Debian resets this to no, but this break both Spot and the libtool
# test suite itself.  Instead of requiring developer to install a
# non-patched version of Libtool on any Debian they use, we just
# cancel the effect of Debian's patch here.
# see: http://git.lrde.epita.fr/?p=spot.git;a=commitdiff;h=0e74b76521341f670f6b76f8ef24a6dcf6e3813b
link_all_deplibs=yes
link_all_deplibs_CXX=yes

AC_OUTPUT
