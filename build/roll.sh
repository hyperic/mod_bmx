#!/bin/sh
#
# Copyright (C) 2004-2007 Codemass, Inc.
#
# Export a mod_bmx tarball from CVS, prepare it for release, and then
# roll a tarball.

# Name of the CVS repository
CVSMODULE=mod_bmx

print_usage() {
    echo "Usage: $0 [-d CVSROOT] tag version [tarball]"
    echo "   CVSROOT - Export from this CVS repository"
    echo "       tag - CVS tag to export from CVSROOT"
    echo "   version - Update release files with this version string"
    echo "   tarball - Name to give the final tarball"
    echo "             (defaults to mod_bmx-\$version.tar.gz)"
    echo 
}

die() {
    echo $*
    exit -2
}

# Look for CVSROOT
if test "$1" = "-d"; then
    shift
    CVSROOT=$1
    shift
else
    if test "x$CVSROOT" = "x"; then
        echo "Must specify -d CVSROOT or set CVSROOT environment variable"
    echo
        print_usage
        exit -1
    fi
fi

if test $# -eq 1; then
    echo "Version parameter required"
    echo
    print_usage
    exit -1
fi

# Check remaining arguments
if test $# -lt 2 -o $# -gt 3; then
    print_usage
    exit -1
fi

TAG=$1
VERSION=$2
TARBALL=$3

# Set the TARBALL if not set above
if test "x$TARBALL" = "x"; then
    TARBALL="$CVSMODULE-$VERSION.tar.gz"
fi

EXPORTDIRNAME=`basename $TARBALL .tar.gz`

# Save starting directory so we can get back later
STARTDIR=`pwd`

# Check for an absolute or relative TARBALL name
case $TARBALL in
    /*) ;;
    *)
        TARBALL="${STARTDIR}/${TARBALL}"
    ;;
esac

# Set the CVS command if not set in ENV
if test "x$CVS" = "x"; then
    CVS=`which cvs`
fi

# Check that the cvs commmand exists
test "x$CVS" != "x" -a -x "$CVS" || die "CVS command not found or not executable"

# Set the tmpdir if not set in ENV
if test "x$TMPDIR" = "x"; then
    TMPDIR=$STARTDIR
fi

# Check that the tempdir exists and that we can change there
test -d $TMPDIR && cd $TMPDIR 2>/dev/null || die "failed to change to TMPDIR $TMPDIR"

# Export the repository
echo "Exporting data from CVS..."
$CVS -d $CVSROOT export -r $TAG -d $EXPORTDIRNAME $CVSMODULE || die "CVS command failed"
echo

# Prepare the code for distribution (run buildconf, etc...)
echo "Preparing $CVSMODULE source directory..."
(
    cd $EXPORTDIRNAME || die "unable to change to exported source directory"
    sh ./buildconf || die "buildconf failed"
    doxygen doxygen.conf 2>&1 > /dev/null # ignore doxygen errors
    rm -rf autom4te.cache
    rm -f .cvsignore
    touch .deps
) || die
echo

# Create the tarball
echo "Creating $TARBALL..."
(tar cvf - $EXPORTDIRNAME | gzip -c > $TARBALL) || die "failed to create tarball"
echo

if test "x$EXPORTDIRNAME" != "x"; then
    echo "Removing $EXPORTDIRNAME..."
    rm -rf ./${EXPORTDIRNAME}
    echo
fi
