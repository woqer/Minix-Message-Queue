#!/bin/sh

cp -pf callnr.h /usr/src/include/minix/callnr.h
cp -pf table.c /usr/src/servers/pm/table.c
cp -pf proto.h /usr/src/servers/pm/proto.h
cp -pf mqueuelib.h /usr/src/include/mqueuelib.h
cp -pf message_queue.c /usr/src/servers/pm/message_queue.c
cp -pf message_queue.h /usr/src/servers/pm/message_queue.h
cp -pf Makefile /usr/src/servers/pm/Makefile

cd /usr/src/releasetools

make hdboot && sync && reboot

