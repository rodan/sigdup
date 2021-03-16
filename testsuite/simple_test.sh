#!/bin/bash

SZBIN='/usr/bin/lsz'
RZBIN=`readlink -f ./rz`

[ ! -e /tmp/pipe ] && mkfifo -f /tmp/pipe
rm -f resolv.conf
${SZBIN} --quiet /etc/resolv.conf < /tmp/pipe | ${RZBIN} > /tmp/pipe
rm -f resolv.conf

