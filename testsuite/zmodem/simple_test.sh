#!/bin/bash

SZBIN='/usr/bin/lsz'
RZBIN=`readlink -f ./rz`
FILE='z_10k'
SAMPLE_DIR='/root/tmp/testfiles/'

[ ! -e /tmp/pipe ] && mkfifo -f /tmp/pipe
rm -f ${FILE}
${SZBIN} -vvv "${SAMPLE_DIR}/${FILE}" < /tmp/pipe | ${RZBIN} > /tmp/pipe
rm -f ${FILE}

