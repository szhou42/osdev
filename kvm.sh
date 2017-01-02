sudo qemu-system-i386 -kernel os_kernel -vga std -k en-us -m 2047M -hda ext2_hda.img -hdb ext2_hdb.img -hdc ext2_hdc.img -hdd ext2_hdd.img \
-netdev tap,helper=/usr/lib/qemu-bridge-helper,id=simpleos_net -device rtl8139,netdev=simpleos_net,id=simpleos_nic \
-serial stdio \
-enable-kvm
