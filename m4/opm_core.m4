dnl -*- autoconf -*-
 # Macros needed to find OPM-core and dependent libraries.  They are called by
# the macros in ${top_src_dir}/dependencies.m4, which is generated by
# "dunecontrol autogen"
AC_DEFUN([OPM_CORE_CHECKS],
[

# Language features
GXX0X
STATIC_ASSERT_CHECK
NULLPTR_CHECK

# Checks for libraries.

# Bring in numerics support (standard library component)
AC_SEARCH_LIBS([sqrt], [m])

OPM_LAPACK

OPM_BOOST_BASE([1.37])
AX_BOOST_SYSTEM
AX_BOOST_DATE_TIME
AX_BOOST_FILESYSTEM
AX_BOOST_UNIT_TEST_FRAMEWORK
AX_DUNE_ISTL
OPM_PATH_SUPERLU
OPM_AGMG

# Checks for header files.
AC_CHECK_HEADERS([float.h limits.h stddef.h stdlib.h string.h])

AC_CHECK_HEADERS([suitesparse/umfpack.h],
                 [umfpack_header=yes],
                 [umfpack_header=no])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_TYPE_SIZE_T
AC_CHECK_TYPES([ptrdiff_t])

# Checks for library functions.
AC_CHECK_FUNCS([floor memset memmove strchr strtol sqrt pow])
AC_FUNC_STRTOD

# Search for UMFPACK direct sparse solver.
AC_SEARCH_LIBS([amd_free],             [amd])
AC_SEARCH_LIBS([camd_free],            [camd])
AC_SEARCH_LIBS([colamd_set_defaults],  [colamd])
AC_SEARCH_LIBS([ccolamd_set_defaults], [ccolamd])
AC_SEARCH_LIBS([cholmod_l_start],      [cholmod])
AC_SEARCH_LIBS([umfpack_dl_solve],     [umfpack],dnl
               [umfpack_lib=yes],      [umfpack_lib=no])

AM_CONDITIONAL([UMFPACK],
  [test "x$umfpack_header" != "xno" -a "x$umfpack_lib" != "xno"])

m4_ifdef([AM_COND_IF],
[AM_COND_IF([UMFPACK], [],
 [AC_MSG_NOTICE([Found no working installation of UMFPACK.
                 UMFPACK support is disabled.])])
])

])

# Additional checks needed to find eWoms
# This macro should be invoked by every module which depends on dumux, but
# not by dumux itself
AC_DEFUN([OPM_CORE_CHECK_MODULE],
[
  OPM_CORE_CHECK_MODULES([opm-core],[opm/core/grid.h],[create_grid_empty()])
])
