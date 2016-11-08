#!/bin/bash
# run_bochs.sh
# mounts the correct loopback device, runs bochs, then unmounts.
rm -rf bochsout.txt
sudo /sbin/losetup /dev/loop0 floppy.img
sudo bochs -f bochsrc.txt
sudo /sbin/losetup -d /dev/loop0
