#!/bin/sh

VINFODIR=.

usage()
{
    cat <<EOF
ltversion.sh -- prepare version information for package releases
Copyright (C) 2003 Juergen "George" Sawinski
All rights reserved.

Options:
    -h,--help            Usage information

    --version-info       Generate the version info

    --version            Generate the version as appended to the library name (x.x)
    --release            Generate release version (x.y.z)
    
    --start              Reset the version information

    source-changed       The source has changed
    interface-added      Methods have been added to the API
    interface-removed    Methods have been removed from the API

    inc-major            Increase the major version
    inc-minor            Increase the minor version    
EOF
}

modify_vinfo () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    eval $*
    major="`grep major $VINFODIR/.vinfo | cut -f2 -d:`"
    minor="`grep minor $VINFODIR/.vinfo | cut -f2 -d:`"
    micro="`grep micro $VINFODIR/.vinfo | cut -f2 -d:`"
    current="`grep current $VINFODIR/.vinfo | cut -f2 -d:`"
    revision="`grep revision $VINFODIR/.vinfo | cut -f2 -d:`"
    age="`grep age $VINFODIR/.vinfo | cut -f2 -d:`"
    cat <<EOF > $VINFODIR/.vinfo
major:$(( $major + ${MAJOR:-0} ))
minor:$(( $minor + ${MINOR:-0} ))
micro:$(( $micro + ${MICRO:-0} ))
current:$(( $current + ${CURRENT:-0} ))
revision:$(( $revision + ${REVISION:-0} ))
age:$(( $age + ${AGE:-0} ))
EOF
}

do_source_change () {
    modify_vinfo REVISION=1 MICRO=1
}

do_api_change () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    revision=`grep revision $VINFODIR/.vinfo | cut -f2 -d:`
    modify_vinfo CURRENT=1 REVISION=$(( $revision * -1)) MICRO=1
}

do_api_add () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    revision=`grep revision $VINFODIR/.vinfo | cut -f2 -d:`
    modify_vinfo CURRENT=1 REVISION=$(( $revision * -1)) AGE=1 MICRO=1
}

do_api_remove () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    age=`grep age $VINFODIR/.vinfo | cut -f2 -d:`
    revision=`grep revision $VINFODIR/.vinfo | cut -f2 -d:`
    modify_vinfo CURRENT=1 REVISION=$(( $revision * -1)) AGE=$(( $age * -1 )) MICRO=1
}

do_inc_major () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    minor=`grep minor $VINFODIR/.vinfo | cut -f2 -d:`
    micro=`grep micro $VINFODIR/.vinfo | cut -f2 -d:`
    modify_vinfo MAJOR=1 MINOR=$(( $minor * -1 )) MICRO=$(( $micro * -1 ))
}

do_inc_minor () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    micro=`grep micro $VINFODIR/.vinfo | cut -f2 -d:`
    modify_vinfo MINOR=1 MICRO=$(( $micro * -1 ))
}

gen_version_info () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    current="`grep current $VINFODIR/.vinfo | cut -f2 -d:`"
    revision="`grep revision $VINFODIR/.vinfo | cut -f2 -d:`"
    age="`grep age $VINFODIR/.vinfo | cut -f2 -d:`"
    echo ${current:-0}:${revision:-0}:${age:-0}
}

gen_version () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    major="`grep major $VINFODIR/.vinfo | cut -f2 -d:`"
    minor="`grep minor $VINFODIR/.vinfo | cut -f2 -d:`"
    echo ${major:-0}.${minor:-0}
}

gen_release () {
    [ -f $VINFODIR/.vinfo ] || echo "No version info file. Usage --start to generate one." || exit -1
    major="`grep major $VINFODIR/.vinfo | cut -f2 -d:`"
    minor="`grep minor $VINFODIR/.vinfo | cut -f2 -d:`"
    micro="`grep micro $VINFODIR/.vinfo | cut -f2 -d:`"
    echo ${major:-0}.${minor:-0}.${micro:-0}
}

reset_versioning() {
    cat <<EOF > $VINFODIR/.vinfo
major:0
minor:0
micro:0
current:0
revision:0
age:0
EOF
}

#############################################################################
for arg; do
    case $arg in
    -h|--help)
	usage
	exit
    ;;
    --dir=*) 
	optarg="`echo X$arg | sed -e 's/[-_a-zA-Z0-9]*=//'`"
	export VINFODIR=$optarg
    ;;
    --version-info)
	gen_version_info
    ;;
    --version)
	gen_version
    ;;
    --release)
	gen_release
    ;;
    --start)
	[ -f $VINFODIR/.vinfo ] || do=yes
	[ -f $VINFODIR/.vinfo ] && read -p "Do really want to reset the version information? (yes/NO) " do;
	[ x$do = xyes ] && reset_versioning
    ;;
    source-changed)
	do_source_change
    ;;
    interface-added)
	do_api_add
    ;;
    interface-removed)
	do_api_remove
    ;;
    inc-major)
	do_inc_major
    ;;
    inc-minor)
	do_inc_minor
    ;;
    *)
	echo "Unknow option $arg"
	echo
	usage
	exit
    ;;
    esac
done
