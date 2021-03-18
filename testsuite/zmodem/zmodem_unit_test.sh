#!/bin/bash

set -e

TEST_ITER=1000000
TESTFILE_DIR='/tmp/lrzsz/testfiles'
RECV_DIR='/tmp/lrzsz/recv'
RECV_LOG='/tmp/lrzsz/recv/log'
PIPE='/tmp/lrzsz/pipe'
SZBIN='/usr/bin/lsz'
#RZBIN='/usr/bin/lrz'
RZBIN=`readlink -f ./rz`

GOOD=$'\e[32;01m'
WARN=$'\e[33;01m'
BAD=$'\e[31;01m'
HILITE=$'\e[36;01m'
BRACKET=$'\e[34;01m'
NORMAL=$'\e[0m'
ENDCOL=$'\e[A\e['$(( COLS - 8 ))'C'


usage()
{
    echo "$0 commands:"
    echo "  -h    help"
    echo "  -n    perform test on newly created files"
    echo "  -e    perform test on existing files"
    echo "  -r [path]  location of the lrz binary, defaults to $RZBIN"
    echo "  -s [path]  location of the lsz binary, defaults to $SZBIN"
}

ebegin() {
        echo -e " ${GOOD}*${NORMAL} $*"
}

eend() {
        local retval="${1:-0}" efunc="${2:-eerror}" msg
        shift 2

        if [[ ${retval} == "0" ]] ; then
                msg="${BRACKET}[ ${GOOD}ok${BRACKET} ]${NORMAL}"
        else
                msg="${BRACKET}[ ${BAD}!!${BRACKET} ]${NORMAL} $*"
        fi
        echo -e "${ENDCOL} ${msg}"
}

ewarn() {
        local retval="${1:-0}" efunc="${2:-eerror}" msg
        shift 2

        if [[ ${retval} == "0" ]] ; then
                msg="${BRACKET}[ ${GOOD}ok${BRACKET} ]${NORMAL}"
        else
                msg="${BRACKET}[ ${WARN}!!${BRACKET} ]${NORMAL} $*"
        fi
        echo -e "${ENDCOL} ${msg}"
}

prepare_fs()
{
    [ ! -e "${TESTFILE_DIR}" ] && mkdir -p "${TESTFILE_DIR}"
    [ ! -e "${RECV_DIR}" ] && mkdir -p "${RECV_DIR}"
    [ ! -e "${PIPE}" ] && mkfifo "${PIPE}"
    rm -f /tmp/lrzsz/recv/*
    return 0
}

create_testfile()
{
    multiplier=$(($RANDOM % 10 + 1))
    file_sz=0
    file=$(mktemp ${TESTFILE_DIR}/XXXXXXXXXX)

    for ((i=0;i<${multiplier};i++)); do
        file_sz=$((file_sz + RANDOM))
    done

    (( file_sz <  128 )) && file_sz=300
    (( file_sz > 212975 )) && file_sz=200322

    dd if=/dev/urandom of=${file} bs=${file_sz} count=1 status=none || {
    #dd if=/dev/zero of=${file} bs=${file_sz} count=1 status=none || {
        echo 'dd error, exiting'
        return 1
    }
    echo "${file}"
    return 0
}

test_transfer()
{
    file="$1"
    [ ! -f "${file}" ] && {
        echo "${file} is an invalid file, exiting"
        return 1
    }

    [ ! -s "${file}" ] && {
        echo "${file} is empty, exiting"
        return 1
    }

    base_name=`basename "${file}"`

    cd "${RECV_DIR}"
    ${SZBIN} --quiet "${file}" < "${PIPE}" | ${RZBIN} --quiet > "${PIPE}"

    cks_in=`sha256sum < "${file}"`
    cks_out=`sha256sum < "${RECV_DIR}/${base_name}"`

    [ "${cks_in}" != "${cks_out}" ] && {
        echo "copy checksum failure for $file, exiting"
        return 1
    }

    #echo ${cks_out} ok

    rm -f "${RECV_DIR}/${base_name}"
    ${keep_file} || rm -f "${file}"

    return 0
}

test_existing()
{
    keep_file=true
    find "${TESTFILE_DIR}" -type f | while read file; do
        true
        test_transfer "${file}"
    done
}

test_new()
{
    keep_file=false
    for ((i=0;i<TEST_ITER;i++)); do
        file=`create_testfile`
        test_transfer "${file}"
    done
}

TEST_NEW=false
TEST_EXISTING=false

while (( "$#" )); do

	if [ "$1" = "-h" ]; then
		usage
        exit 0
	elif [ "$1" = "-n" ]; then
		shift;
        TEST_NEW=true
	elif [ "$1" = "-e" ]; then
		shift;
        TEST_EXISTING=true
	elif [ "$1" = "-r" ]; then
		shift;
        RZBIN=$1
		shift;
	elif [ "$1" = "-s" ]; then
		shift;
        SZBIN=$1
		shift;
    else
        usage
        exit 1
    fi
done

if [ "${TEST_NEW}" == "true" -o "${TEST_EXISTING}" == "true" ]; then
    prepare_fs

    ${TEST_EXISTING} && {
        file_count=`find "${TESTFILE_DIR}" -type f | wc -l`
        ebegin "test ${file_count} existing files"
        test_existing
        eend $? "failed"
    }

    ${TEST_NEW} && {
        ebegin "test ${TEST_ITER} new files"
        test_new
        eend $? "failed"
    }
else
    echo "nothing to do, exiting"
    exit 0
fi

exit 0

