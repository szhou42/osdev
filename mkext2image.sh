#!/bin/bash
#dd if=<(yes $'\01' | tr -d "\n") of=ext2_hda.img bs=1k count=100000
dd if=/dev/zero of=ext2_hda.img bs=1k count=100000 > /dev/zero
mkfs -t ext2 -i 1024 -b 1024 -F ext2_hda.img > /dev/zero
fdisk ext2_hda.img <<EOF 
x
c
10
h
16
s
63
r
n
p
1
a
1
w
EOF

cp ext2_hda.img ext2_hdb.img
cp ext2_hda.img ext2_hdc.img
cp ext2_hda.img ext2_hdd.img
printf "ext2-formatted disk, hda, hdb, hdc, and hdd created\n"

