#!/bin/bash

cat << EOF | fdisk /dev/bestioa
o
n




w
EOF
