kill -f qemu
sudo qemu-system-i386 -s -S -kernel os_kernel -vga std -k en-us -hda ext2_hda.img -hdb ext2_hdb.img -hdc ext2_hdc.img -hdd ext2_hdd.img -net nic,model=rtl8139 -net dump,file=traffic.pcap -netdev tap,id=br0 -device rtl8139,netdev=br0
./remote_debug.sh

