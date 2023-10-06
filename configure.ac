#--------------------------------------------------------
# cfrontend compile infrastructure
#--------------------------------------------------------
AC_PREREQ(2.69)
AC_INIT( [cfrontend], [0.0.0(unstable)], [agupta or bkragl@ist.ac.at])
AC_CONFIG_AUX_DIR(autotools)

# 'subdir-objects' option allows sources to be located in subdirectories
#
# https://www.mail-archive.com/debian-bugs-dist@lists.debian.org/msg1232579.html
# The following line is commented due to the the above bug in version 2.69
AM_INIT_AUTOMAKE #([subdir-objects])

AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADERS([cfrontend_config.h])

#--------------------------------------------------------
#Check for tools
#--------------------------------------------------------
#AC_PROG_CC
OLD_CXXFLAGS=$CXXFLAGS
AC_PROG_CXXCPP
AC_PROG_CXX
CXXFLAGS=$OLD_CXXFLAGS
#AC_PROG_YACC
#AM_PROG_LEX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
#--------------------------------------------------------
# diable verbose display of command lines
# One can un-silent the commands by using $make V=1
#--------------------------------------------------------
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

#--------------------------------------------------------
# why the following ???
#--------------------------------------------------------
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
#--------------------------------------------------------
# Check for host and setup a few things based on the host
#--------------------------------------------------------
AC_CANONICAL_HOST
case $host_os in
  darwin* )
    AM_CXXFLAGS="$AM_CXXFLAGS -I/opt/local/include"
    AM_LDFLAGS="$AM_LDFLAGS -L/opt/local/lib"
    default_location=/opt/local
    ;;
  linux*)
    AM_CXXFLAGS="$AM_CXXFLAGS -I/usr/include"
    AM_LDFLAGS="$AM_LDFLAGS -L/usr/lib"
    default_location=/usr
    ;;
  *)
    #Default Case
    AC_MSG_ERROR([Your platform is not currently supported])
    ;;
esac

#--------------------------------------------------------
# initialize compile arguments c and c++
#--------------------------------------------------------
# should we reomve CFLAGS?
dnl AM_CFLAGS="-W -Wall -Wno-deprecated"
AM_CXXFLAGS="$AM_CXXFLAGS -std=c++11 -Wall -Wextra -Wno-deprecated"
# the following resets to ensure that the flags added by 
# AC_PROG_CXXCPP AC_PROG_CXX are removed
dnl CFLAGS=""
dnl CXXFLAGS=""

#--------------------------------------------------------
# check for c++-11, this is a custom check 
#--------------------------------------------------------
AX_CXX_COMPILE_STDCXX_11([noext],[mandatory])
#--------------------------------------------------------
# Enable/Disable static linking
#--------------------------------------------------------
AC_ARG_ENABLE( statically,
  AC_HELP_STRING([--enable-statically],
                 [enable statically linked libraries (--static) (default=no)]),
  enablestatic=$enableval,
  enablestatic="no")
if test $enablestatic = "yes" ; then
  config_static="--static"
elif test $enablestatic = "no" ; then
  config_static="" 
else
  config_static=$enablestatic
fi
echo "setting static flags to... ${config_static:-none}"
AM_CXXFLAGS="$AM_CXXFLAGS $config_static"
AM_LDFLAGS="$AM_LDFLAGS $config_static"
#--------------------------------------------------------
# Enable all static in linker
#--------------------------------------------------------
dnl if test $enable_static = "yes"; then
dnl   AM_LDFLAGS="$AM_LDFLAGS -all-static"
dnl   echo "adding -all-static flag to linker"
dnl fi
#--------------------------------------------------------
# Enable/Disable profiling
#--------------------------------------------------------
# TODO: currently not working -pg option is not sitting well with 
# AC_CHECK_HEADERS
AC_ARG_ENABLE(profiling,
  AC_HELP_STRING([--enable-profiling], [enable profiling (-pg) (default=no)]),
  enableprofiling=$enableval,
  enableprofiling="no")
if test $enableprofiling = "yes" ; then
  config_profiling="-pg"
elif test $enableprofiling = "no" ; then
  config_profiling=""
else
  config_profiling=$enableprofiling
fi
echo "setting profiling flags to... ${config_profiling:-none}"
AM_CXXFLAGS="$AM_CXXFLAGS $config_profiling"
AM_LDFLAGS="$AM_LDFLAGS $config_profiling"
#--------------------------------------------------------
# Enable/Disable optimizations
#--------------------------------------------------------
AC_ARG_ENABLE(optimization,
  AC_HELP_STRING([--enable-optimization],
                 [enable optimization by compiler (-O3) (default=yes)]),
  enableoptimization=$enableval,
  enableoptimization="yes")
if test $enableoptimization = "yes" ; then
   config_optimization="-O3 -DOPTIMIZE -DNDEBUG"
   if test $enableprofiling = "no" ; then
      config_optimization="$config_optimization -fomit-frame-pointer"
   fi
elif test $enableoptimization = "no" ; then
  config_optimization=""
else
  config_optimization=$enableoptimization
fi
echo "setting optimization flags to... ${config_optimization:-none}"
AM_CXXFLAGS="$AM_CXXFLAGS $config_optimization"
dnl AM_LDFLAGS="$AM_LDFLAGS $config_optimization"
#--------------------------------------------------------
# Enable debug and assertion Checking
#--------------------------------------------------------
AC_ARG_ENABLE(debug,
  AC_HELP_STRING([--enable-debug],
                 [enable debug info and assertion checking (default=no)]),
  enabledebug=$enableval,
  enabledebug="no")

if test $enabledebug = "yes" ; then
  config_debug="-g -DPEDANTIC_DEBUG -DDEBUG"
elif test $enabledebug = "no" ; then
  config_debug=""
else
  config_debug=$enabledebug
fi
echo "setting debug flags to... ${config_debug:-none}"
AM_CXXFLAGS="$AM_CXXFLAGS $config_debug"
dnl AM_LDFLAGS="$AM_LDFLAGS $config_debug"
#--------------------------------------------------------
# Complain if both  debug and optimization are enabled
#--------------------------------------------------------
if test $enabledebug = "yes" -a $enableoptimization = "yes" ;
then
  echo "Error: cannot enable both debug and optimization"
  exit
fi

#=============================================================================
# Add compilation options related to libraries
#=============================================================================

#--------------------------------------------------------
# enable z3, allow user to locate z3 
#--------------------------------------------------------
AH_TEMPLATE( Z3_PATH, [absolute path to z3 binary] )
AC_ARG_WITH([z3],
	    [AS_HELP_STRING([--with-z3=prefix], [location of z3] )],
            [Z3PATH=1],
            [Z3PATH=0] )
if test $Z3PATH = 1 ; then
  AC_DEFINE_UNQUOTED( [Z3_PATH], ["$with_z3"], [path to z3 binary] )
  AM_CXXFLAGS="$AM_CXXFLAGS -fexceptions -I$with_z3/include"
  LIBS="$LIBS -L$with_z3/bin -L$with_z3/lib -lz3 -fopenmp"
  dnl AM_LDFLAGS="$AM_LDFLAGS -L$with_z3/bin -L$with_z3/lib -lz3 -fopenmp"
  AC_OPENMP # check if openmp option is available#TODO make a comprehensive test
  dnl lz3="-lz3 -fopenmp"
  dnl AC_SUBST(lz3)
fi
#--------------------------------------------------------
# llvm location and libs
#--------------------------------------------------------
dnl AH_TEMPLATE( LLVM_PATH, [llvm path] )
AC_ARG_WITH([llvm],
	    [AS_HELP_STRING([--with-llvm=prefix], [location of llvm] )],
            [llvm_custom=1],
            [llvm_custom=0] )
if test $llvm_custom = 1 ; then
   llvm_path=$with_llvm
else
  # TODO: auto detect llvm
   llvm_path=/usr/lib/llvm-3.6
fi

AC_CHECK_PROG(LLVM_CONFIG_CHECK,llvm-config,yes,no,$llvm_path/bin)
if test x"$LLVM_CONFIG_CHECK" != x"yes" ; then
    AC_MSG_ERROR([llvm-config is missing.])
fi
llvm_libs=$($llvm_path/bin/llvm-config --libs)
#--------------------------------------------------------
# Do we need all of these flags for llvm??
#--------------------------------------------------------
AM_CXXFLAGS="$AM_CXXFLAGS -I$llvm_path/include -D_GNU_SOURCE -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS -fvisibility-inlines-hidden -fno-rtti -fPIC -Woverloaded-virtual -Wcast-qual"
LIBS="$LIBS -L$llvm_path/lib $llvm_libs -lpthread -lffi -ltinfo -ldl -lm"
#--------------------------------------------------------
# Allow user to use a GMP lib in a non-standard location
#--------------------------------------------------------
AC_ARG_WITH([gmp],
	    [AS_HELP_STRING([--with-gmp=prefix],
			    [Use this when GMP is in a non-standard location])],
	    [NONSTDGMPPATH=1],
            [NONSTDGMPPATH=0])
if test $NONSTDGMPPATH = 1 ; then
  AM_CXXFLAGS="$AM_CXXFLAGS -I$with_gmp/include"
  AM_LDFLAGS="$AM_LDFLAGS -L$with_gmp/lib"  
fi
AM_LDFLAGS="$AM_LDFLAGS -lgmp -lgmpxx"

#=============================================================================
# checks if libraries really exist !!!!!
#=============================================================================
# AC_CHECK_HEADERS and AC_CHECK_LIB need CPPFLAGS and LDFLAGS
# to locate the headers and libs

# merge with AM_ variables and reset after library checks
OLD_CPPFLAGS=$CPPFLAGS;
CPPFLAGS="$AM_CXXFLAGS $CPPFLAGS"
OLD_CXXFLAGS=$CXXFLAGS;
CXXFLAGS="$AM_CXXFLAGS $CXXFLAGS"
OLD_LDFLAGS=$LDFLAGS;
LDFLAGS="$AM_LDFLAGS $LDFLAGS"

#--------------------------------------------------------
# Check for GMP header and object
#--------------------------------------------------------
GMP_HEADER_FOUND=1
if test $NONSTDGMPPATH = 1 ; then
   AC_CHECK_HEADERS($with_gmp/include/gmp.h,,
                    [AC_MSG_ERROR([gmp.h was not found.])])
else
   AC_CHECK_HEADERS(gmp.h,, [ AC_MSG_ERROR([gmp.h was not found.])])
fi
AC_CHECK_LIB(gmp, __gmpz_init, ,[AC_MSG_ERROR([GMP library not found])])
#--------------------------------------------------------
# Check for LLVM library object
#--------------------------------------------------------
AC_CHECK_HEADERS(llvm/Config/llvm-config.h,,
                 [ AC_MSG_ERROR([llvm/Config/llvm-config.h.])])
dnl AC_CHECK_LIB(LLVMCore, __..., ,[AC_MSG_ERROR([GMP library not found])])
# todo: check for clang
# AC_CHECK_PROG(LLVM_CONFIG_CHECK,clang,yes,no,$llvm_path/bin)
#--------------------------------------------------------
#TODO check boost
#--------------------------------------------------------
#.....
#--------------------------------------------------------
# check google/sparsehash
#--------------------------------------------------------
dnl AH_TEMPLATE( HAVE_SPARSE_HASH, [Defined to 1 if you have google/sparsehash ] )
dnl AC_CHECK_HEADERS( $default_location/include/google/sparsehash/sparseconfig.h,
dnl                    [AC_DEFINE( [HAVE_SPARSE_HASH] ) ] ,)
#TODO: check if ffi pthread tinfo dl m libs exist

# reset original CPPFLAGS, CXXFLAGS, and LDFLAGS
CPPFLAGS=$OLD_CPPFLAGS
CXXFLAGS=$OLD_CXXFLAGS
LDFLAGS=$OLD_LDFLAGS

#--------------------------------------------------------
# List of directories to include
#--------------------------------------------------------
AM_CXXFLAGS="-I\${top_srcdir}/src $AM_CXXFLAGS"

#--------------------------------------------------------
# subsitute value of these variables in Makefiles
#--------------------------------------------------------
AC_SUBST(AM_CFLAGS)
AC_SUBST(AM_CXXFLAGS)
AC_SUBST(AM_LDFLAGS)

#--------------------------------------------------------
# READ Makefile.in and PRODUCE the following Makefiles
#--------------------------------------------------------
AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([src/Makefile])
AC_CONFIG_FILES([src/bin/Makefile])
AC_CONFIG_FILES([src/cfrontend/Makefile])

AC_OUTPUT
