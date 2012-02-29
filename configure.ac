AC_PREREQ(2.59)
AC_COPYRIGHT([Copyright (c) 2007 Hyperic, Inc.])
AC_COPYRIGHT([Copyright (c) 2007 Codemass, Inc.])
AC_COPYRIGHT([All rights reserved.  Use is subject to the license terms.])
AC_REVISION($Revision: 1.16 $)
AC_INIT([mod_bmx], [0.9.2], [aaron-bmx@codemass.com])

AC_CONFIG_SRCDIR([mod_bmx.c])
AC_CONFIG_AUX_DIR([build])
AC_CONFIG_HEADER([config.h])

dnl Save the configure parameters for lazy people like me
APR_CONFIG_NICE(config.nice)

dnl Absolute source/build directories
abs_srcdir=`(cd $srcdir && pwd)`
abs_builddir=`pwd`

AC_ARG_WITH(apxs2,
[  --with-apxs2=[FILE]     Path to Apache 2.x's apxs (default from PATH)],
[
    if test "$withval" = "yes"; then
        AC_MSG_ERROR([--with-apxs2 takes the full path to apxs])
    elif test "$withval" = "no"; then
        AC_MSG_ERROR([apxs2 support can not be disabled])
    else
        APXS="$withval"
    fi
],[
    APXS=apxs
])

APXS_HTTPD=`$APXS -q SBINDIR`/`$APXS -q TARGET`

# Test that we're trying to configure with apache 2.x
# (code borrowed from PHP)
APACHE_VERSION=`$APXS_HTTPD -v | head -1 | cut -f3 -d' ' | cut -f2 -d'/' | cut -f1 -d'-' | awk 'BEGIN { FS = "."; } { printf "%d", ($1 * 1000 + $2) * 1000 + $3;}'`
if test "$APACHE_VERSION" -le 2000000; then
    AC_MSG_ERROR([path to Apache 2.x's apxs required (see --with-apxs2)])
fi

$APXS -q CFLAGS >/dev/null 2>&1
if test "$?" != "0"; then
    AC_MSG_ERROR([Error executing apxs])
fi

APACHE_CFLAGS=`$APXS -q CFLAGS`
APACHE_CPPFLAGS=`$APXS -q CPPFLAGS`
APACHE_LDFLAGS=`$APXS -q LDFLAGS`
EXTRA_INCLUDES=`$APXS -q EXTRA_INCLUDES`
exp_sbindir=`$APXS -q exp_sbindir`
exp_sysconfdir=`$APXS -q exp_sysconfdir`

#CFLAGS="$CFLAGS $APACHE_CFLAGS"
#CPPFLAGS="$CPPFLAGS $APACHE_CPPFLAGS $EXTRA_INCLUDES"
#LDFLAGS="$LDFLAGS $APACHE_LDFLAGS"

#AC_CHECK_HEADERS(httpd.h)
# FIXME: look for these functions for use in mod_bmx_status
#AC_CHECK_FUNCS([ap_get_server_version ap_get_server_banner])

AC_SUBST(APXS)
AC_SUBST(APACHE_CFLAGS)
AC_SUBST(APACHE_CPPFLAGS)
AC_SUBST(APACHE_LDFLAGS)
AC_SUBST(EXTRA_INCLUDES)

AC_OUTPUT(Makefile)

