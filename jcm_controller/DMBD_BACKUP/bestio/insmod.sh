#!/bin/bash

insmod go.ko memory_size=1024 back_major=8 back_minor=32
modprobe nbd
nbd-client 192.168.161.129 -p 12346 /dev/nbd8 -N export_test1
