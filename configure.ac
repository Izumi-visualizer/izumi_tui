AC_INIT([izumi_tui], [0.2.1])

AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE([foreign -Wall -Werror])
AM_SILENT_RULES([yes])

AC_PROG_CC
AM_PROG_AR
AC_PATH_PROG([PYTHON3], [python3])
LT_INIT

AC_ARG_WITH([system_libizumi],
    [AS_HELP_STRING([--with-system-libizumi],
        [link against the system libizumi, build it otherwise (default is yes)])
    ],
    [],
    [with_system_libizumi=yes]
)

AC_DEFUN([AX_CHECK_LIBIZUMI_SOURCES],
    [AC_CACHE_CHECK([whether libizumi sources are available],
        ax_cv_libizumi_sources,
        [AS_IF(AS_EXECUTABLE_P([$srcdir/libizumi/configure]),
            [ax_cv_libizumi_sources=yes],
            [ax_cv_libizumi_sources=no]
        )]
    )]
)
AM_CONDITIONAL([SYSTEM_LIBIZUMI], [test "x$with_system_libizumi" == xyes])

AS_IF([test "x$with_system_libizumi" == xno], [
    AX_CHECK_LIBIZUMI_SOURCES
    AS_IF([test "x$ax_cv_libizumi_sources" != xyes], [
        AC_MSG_ERROR([libizumi sources not found (should be in libizumi directory, next to configure.ac)])
    ])
    AX_SUBDIRS_CONFIGURE(
        [libizumi], [--disable-doxygen-doc], []
    )
    
    LIBIZUMI_CPPFLAGS=-I$\(abs_top_srcdir\)/libizumi/src/include
    LIBIZUMI_LA=$\(abs_top_builddir\)/libizumi/src/libizumi.la
    AC_SUBST([LIBIZUMI_CPPFLAGS])
    AC_SUBST([LIBIZUMI_LA])
])

AS_IF([test "x$with_system_libizumi" == xyes], [
    AX_CHECK_LIBRARY(
        [LIBIZUMI], [izumi/parser.h], [izumi],
        [],
        [AC_MSG_ERROR([libizumi not found])]
    )
])

AX_WITH_CURSES
if test "x$ax_cv_curses" != xyes || test "x$ax_cv_curses_color" != xyes; then
    AC_MSG_ERROR([requires an X/Open-compatible Curses library with color])
fi

# Native Language Support through gettext/libintl (part of glibc; other C
# libraries may provide it in a separate libintl).
AC_ARG_ENABLE([nls],
    [AS_HELP_STRING([--disable-nls], [do not translate program messages])],
    [],
    [enable_nls=yes]
)
AS_IF([test "x$enable_nls" = xyes], [
    AC_CHECK_HEADER([libintl.h], [], [enable_nls=no])
])
AS_IF([test "x$enable_nls" = xyes], [
    AC_SEARCH_LIBS([bindtextdomain], [intl], [], [enable_nls=no])
])
AS_IF([test "x$enable_nls" = xyes], [
    AC_DEFINE([ENABLE_NLS], [1], [Define to 1 to translate program messages])
])
AC_MSG_CHECKING([whether to translate program messages])
AC_MSG_RESULT([$enable_nls])

AC_PATH_PROG([MSGFMT], [msgfmt], [no])
AC_PATH_PROG([XGETTEXT], [xgettext], [no])
AC_PATH_PROG([MSGMERGE], [msgmerge], [no])
AS_IF([test "x$enable_nls" = xyes && test "x$MSGFMT" = xno], [
    AC_MSG_WARN([msgfmt not found, translations will not be compiled nor installed])
])
AM_CONDITIONAL([BUILD_MO], [test "x$enable_nls" = xyes && test "x$MSGFMT" != xno])

AX_CONFIG_FEATURE_VERBOSE
AX_CONFIG_FEATURE_DEFAULT_DISABLED
AX_CONFIG_FEATURE(
    [sanitizers], [turns on/off ASAN flags (only intended for development)],
    HAVE_SANITIZERS, [Define if ASAN is desired (unused)],
    [
        AX_APPEND_COMPILE_FLAGS([-fsanitize=address,leak,undefined])
    ]
)

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    po/Makefile
    src/Makefile
    tests/Makefile
])

AC_OUTPUT
