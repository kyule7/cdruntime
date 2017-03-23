AC_INIT([cdruntime], [1.0], [cds@lph.ece.utexas.edu])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
#AC_PROG_LIBTOOL
#AM_PROG_AR
LT_INIT
AC_PROG_CC
AC_PROG_CXX
dnl AC_PROG_RANLIB
AC_PREFIX_DEFAULT($PWD)
AC_CONFIG_MACRO_DIR([m4])
# debugging output via --enable-debug
# XXX:  _CD_DEBUG Could be a separate argument
AC_ARG_ENABLE([debug], 
    [AS_HELP_STRING([--enable-debug],[Turn on debugging output])])
AS_IF([test x"$enable_debug" = x"yes"], [WANT_DEBUG=1], [WANT_DEBUG=0])
AC_DEFINE_UNQUOTED(_DEBUG, $WANT_DEBUG,
          [Define to 1 to enable debugging output])
AC_DEFINE_UNQUOTED(_CD_DEBUG, $WANT_DEBUG,
          [Define to 1 for debugging file output, 2 for stdout])
AC_SUBST(cd_debug_enabled, $WANT_DEBUG)
AC_SUBST(cd_debug_dest, $WANT_DEBUG)

# tuning support
AC_ARG_ENABLE([tuning], 
    [AS_HELP_STRING([--enable-tuning],[Turn on tuning capability])])
AS_IF([test x"$enable_tuning" = x"yes"], [WANT_TUNING=1], [WANT_TUNING=0])
AC_DEFINE_UNQUOTED(_TUNING, $WANT_TUNING, [Define to 1 to enable tuning])
AC_SUBST(cd_tuning_enabled, $WANT_TUNING)

# runtime on/off switch used with tuning
AC_ARG_ENABLE([runtime], 
    [AS_HELP_STRING([--enable-runtime],[Turn on runtime with tuning])])
AS_IF([test x"$enable_runtime" = x"yes"], [WANT_RUNTIME=1], [WANT_RUNTIME=0])
AC_DEFINE_UNQUOTED(_CD_RUNTIME, $WANT_RUNTIME, [Define to 1 to enable runtime with
                                           tuning])
AC_SUBST(cd_runtime_enabled, $WANT_RUNTIME)

# profiling support
AC_ARG_ENABLE([profiler], 
    [AS_HELP_STRING([--enable-profiler],[Turn on profiler support])])
AS_IF([test x"$enable_profiler" = x"yes"], [WANT_PROFILER=1], [WANT_PROFILER=0])
AC_DEFINE_UNQUOTED(_PROFILER, $WANT_PROFILER, [Define to 1 to enable profiler])
AC_SUBST(cd_profiler_enabled, $WANT_PROFILER)

# profiling support
AC_ARG_ENABLE([viz], 
    [AS_HELP_STRING([--enable-viz],[Turn on visualization])])
AS_IF([test x"$enable_viz" = x"yes"], [WANT_VIZ=1], [WANT_VIZ=0])
AC_DEFINE_UNQUOTED(_SIGHT, $WANT_VIZ, [Define to 1 to enable visualization])
AC_SUBST(cd_viz_enabled, $WANT_VIZ)
dnl # some additonal preprocessor symbols to configure profiling
dnl # XXX: I have no idea what these do, so please update the descriptions!
dnl AC_DEFINE(_ENABLE_MODULE, [1], [Define to 1 for Module])
dnl AC_DEFINE(_ENABLE_HIERGRAPH, [0], [Define to 1 for CD Hierarchical Graph App])
dnl AC_DEFINE(_ENABLE_SCOPE, [0], [Define to 1 to profile Scope])
dnl # XXX: Old Makefile used _ENABLE_GRAPH, but code used _ENABLE_SCOPEGRAPH
dnl AC_DEFINE(_ENABLE_SCOPEGRAPH, [0], [Define to 1 for ScopeGraph])
dnl AC_DEFINE(_ENABLE_ATTR, [0], [Define to 1 for Attribute])
dnl AC_DEFINE(_ENABLE_COMP, [0], [Define to 1 for Comparison])
dnl AC_DEFINE(_ENABLE_CDNODE, [0], [Define to 1 for CDNode])

# comm_log is tested via ifdef, so this one is conditionally defined
#
# logging (seems to refer only to communications logging.)
# XXX: Rename?
AC_ARG_ENABLE([logging], 
    [AS_HELP_STRING([--enable-logging], [Turn on logging])])
AS_IF([test x"$enable_logging" = x"yes"], 
[
    WANT_LOGGING=1
    AC_DEFINE(comm_log, [1], [Define to enable communications logging])
],[
    WANT_LOGGING=0
    AC_DEFINE(comm_log, [0], [Define to enable communications logging])
])
AC_SUBST(cd_comm_log_enabled, $WANT_LOGGING)

# libc logging
# Also checked via #ifdef, so our AC_DEFINE is conditional.
# XXX:  Change these flags to be consistent with others!
AC_ARG_ENABLE([libc-logging], 
    [AS_HELP_STRING([--enable-libc-logging], [Turn on libc logging])])
AS_IF([test x"$enable_libc_logging" = x"yes"], 
[
    WANT_LIBC_LOGGING=1
    AC_DEFINE(libc_log, [1], [Define to enable libc logging])
],[
    WANT_LIBC_LOGGING=0
])
AC_SUBST(cd_libc_log_enabled, $WANT_LIBC_LOGGING)

dnl Test enable 
AC_ARG_ENABLE([cd_test], 
    [AS_HELP_STRING([--enable-cd-test], [Turn on internal test mode])])
AS_IF([test x"$enable_cd_test" = x"yes"], 
[
    WANT_CD_TEST=1
],[
    WANT_CD_TEST=0
])
AC_SUBST(cd_test_enabled, $WANT_CD_TEST)

# error injection
AC_ARG_ENABLE([error-injection], 
    [AS_HELP_STRING([--enable-error-injection], [Turn on error injection])])
AS_IF([test x"$enable_error_injection" = x"yes"], 
        [WANT_ERROR_INJECTION=1], [WANT_ERROR_INJECTION=0])
AC_DEFINE_UNQUOTED(_ERROR_INJECTION_ENABLED, $WANT_ERROR_INJECTION,
          [Define to 1 to enable error injection])
AC_SUBST(cd_error_injection_enabled, $WANT_ERROR_INJECTION)

# keep preservation files.
# XXX: Should this be a runtime option?
AC_ARG_ENABLE([keep-preservation-files],
    [AS_HELP_STRING([--enable-keep-preservation-files], 
    [Keep preservation files rather than deleting them.])])
AS_IF([test x"$enable_keep_preservation_files" = x"yes"], 
        [WANT_KEEP_PRESERVATION_FILES=1], [WANT_KEEP_PRESERVATION_FILES=0])
AC_DEFINE_UNQUOTED(_PRV_FILE_NOT_ERASED, $WANT_ERROR_INJECTION,
          [Define to 1 to keep preservation files after program exits.])
AC_DEFINE_UNQUOTED(_PRV_FILE_BASEPATH, $WANT_ERROR_INJECTION,
          [Define to 1 to keep preservation files after program exits.])

# parallel library support
AC_ARG_ENABLE([parallel],
    [AS_HELP_STRING([--enable-parallel=ARG], [ARG is mpi or pgas ])],
    [], enable_parallel=no)
WANT_SINGLE=0
WANT_PGAS=0
WANT_MPI=0
AS_IF([test "x$enable_parallel" = x"mpi"], [WANT_MPI=1],
      [test "x$enable_parallel" = x"pgas"], [WANT_PGAS=1],
      [test "x$enable_parallel" = x"no"], [WANT_SINGLE=1],
    AC_MSG_FAILURE([--enable-parallel must be mpi pgas or no]))
AC_DEFINE_UNQUOTED(_MPI_VER, $WANT_MPI, [Define to 1 for MPI support.])
AC_DEFINE_UNQUOTED(_PGAS_VER, $WANT_PGAS, [Define to 1 for PGAS support.])
AC_DEFINE_UNQUOTED(_SINGLE_VER, $WANT_SINGLE, [Define to 1 for serial library])
AC_SUBST(cd_mpi_enabled, $WANT_MPI)
AC_SUBST(cd_pgas_enabled, $WANT_PGAS)
AM_CONDITIONAL(HAVE_MPI, $WANT_MPI)

dnl CD_AUTOMATED
AC_ARG_ENABLE([cd_automated], 
    [AS_HELP_STRING([--enable-cd-automated], [Turn on automation mode])])
AS_IF([test x"$enable_cd_automated" = x"yes"], 
[
    WANT_CD_AUTOMATED=1
],[
    WANT_CD_AUTOMATED=0
])
AC_SUBST(cd_automated, $WANT_CD_AUTOMATED)
dnl AC_ARG_WITH([sight], 
dnl     [AS_HELP_STRING([--with-sight=DIR], [specify path to sight installation])],
dnl     [SIGHT_ROOT=$withval],
dnl     [SIGHT_ROOT=none])

# Check for incompatible build flags after argument processing is complete.
# (i.e. Don't put any AC_ARG_ENABLE or AC_ARG_WITH directives below this line.)
#
# --enable-logging is not supported in single-threaded builds
AS_IF([test "x$enable_parallel" = x"no" -a "x$enable_logging" = x"yes"],
    [AC_MSG_FAILURE([Logging is not supported in single-threaded builds.  Please specify --disable-logging.])]
)

AC_CONFIG_HEADERS([src/cd_config.h])
AC_CONFIG_FILES([
 Makefile
 src/Makefile
 src/cd_features.h
 test/Makefile
 test/test_simple_hierarchy/Makefile
 test/test_user_defined_hierarhcy/Makefile
 test/test_prv_ref_remote/Makefile
 test/test_serdes/Makefile
 test/test_tuning/Makefile
 examples/Makefile
])
AC_OUTPUT
