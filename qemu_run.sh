qemu-system-i386 -kernel os_kernel -vga std -k en-us -m 2047M -hda ext2_hda.img -hdb ext2_hdb.img -hdc ext2_hdc.img -hdd ext2_hdd.img -net nic,model=e1000 -net dump,file=traffic.pcap -net user
