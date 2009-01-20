dnl
dnl Borrowed from APR's apr_common.m4
dnl FIXME: adhere to ASF licensing terms before distributing this.
dnl

dnl APR_CONFIG_NICE(filename)
dnl
dnl Saves a snapshot of the configure command-line for later reuse
dnl
AC_DEFUN(APR_CONFIG_NICE,[
  rm -f $1
  cat >$1<<EOF
#! /bin/sh
#
# Created by configure

EOF
  if test -n "$CC"; then
    echo "CC=\"$CC\"; export CC" >> $1
  fi
  if test -n "$CFLAGS"; then
    echo "CFLAGS=\"$CFLAGS\"; export CFLAGS" >> $1
  fi
  if test -n "$CPPFLAGS"; then
    echo "CPPFLAGS=\"$CPPFLAGS\"; export CPPFLAGS" >> $1
  fi
  if test -n "$LDFLAGS"; then
    echo "LDFLAGS=\"$LDFLAGS\"; export LDFLAGS" >> $1
  fi
  if test -n "$LTFLAGS"; then
    echo "LTFLAGS=\"$LTFLAGS\"; export LTFLAGS" >> $1
  fi
  if test -n "$LIBS"; then
    echo "LIBS=\"$LIBS\"; export LIBS" >> $1
  fi
  if test -n "$INCLUDES"; then
    echo "INCLUDES=\"$INCLUDES\"; export INCLUDES" >> $1
  fi
  if test -n "$NOTEST_CFLAGS"; then
    echo "NOTEST_CFLAGS=\"$NOTEST_CFLAGS\"; export NOTEST_CFLAGS" >> $1
  fi
  if test -n "$NOTEST_CPPFLAGS"; then
    echo "NOTEST_CPPFLAGS=\"$NOTEST_CPPFLAGS\"; export NOTEST_CPPFLAGS" >> $1
  fi
  if test -n "$NOTEST_LDFLAGS"; then
    echo "NOTEST_LDFLAGS=\"$NOTEST_LDFLAGS\"; export NOTEST_LDFLAGS" >> $1
  fi
  if test -n "$NOTEST_LIBS"; then
    echo "NOTEST_LIBS=\"$NOTEST_LIBS\"; export NOTEST_LIBS" >> $1
  fi

  # Retrieve command-line arguments.
  eval "set x $[0] $ac_configure_args"
  shift

  for arg
  do
    APR_EXPAND_VAR(arg, $arg)
    echo "\"[$]arg\" \\" >> $1
  done
  echo '"[$]@"' >> $1
  chmod +x $1
])dnl

dnl Iteratively interpolate the contents of the second argument
dnl until interpolation offers no new result. Then assign the
dnl final result to $1.
dnl
dnl Example:
dnl
dnl foo=1
dnl bar='${foo}/2'
dnl baz='${bar}/3'
dnl APR_EXPAND_VAR(fraz, $baz)
dnl   $fraz is now "1/2/3"
dnl 
AC_DEFUN([APR_EXPAND_VAR], [
ap_last=
ap_cur="$2"
while test "x${ap_cur}" != "x${ap_last}";
do
  ap_last="${ap_cur}"
  ap_cur=`eval "echo ${ap_cur}"`
done
$1="${ap_cur}"
])dnl

