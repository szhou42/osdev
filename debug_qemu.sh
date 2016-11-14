kill -f qemu
qemu-system-i386 -s -S -kernel os_kernel -vga std -k en-us -hda ext2_hda.img -hdb ext2_hdb.img -hdc ext2_hdc.img -hdd ext2_hdd.img&
./remote_debug.sh

