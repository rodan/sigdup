#!/bin/bash

SZBIN='/usr/bin/lsz'
RZBIN=`readlink -f ./rz`
FILE='z_10k'
SAMPLE_DIR='/tmp'

[ ! -e /tmp/pipe ] && mkfifo /tmp/pipe
rm -f ${FILE}
${SZBIN} "${SAMPLE_DIR}/${FILE}" < /tmp/pipe | ${RZBIN} > /tmp/pipe
rm -f ${FILE}

