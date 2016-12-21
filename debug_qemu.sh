kill -f qemu
sudo qemu-system-i386 -s -S -kernel os_kernel -vga std -k en-us -hda ext2_hda.img -hdb ext2_hdb.img -hdc ext2_hdc.img -hdd ext2_hdd.img \
-netdev tap,helper=/usr/local/libexec/qemu-bridge-helper,id=thor_net0 -device rtl8139,netdev=thor_net0,id=thor_nic0 &
./remote_debug.sh
