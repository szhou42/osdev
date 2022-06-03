# edu OS
An operating system based on <a href = "http://www.osdever.net/bkerndev/Docs/title.htm"> Brans' Kernel Development Tutorial</a>  
<a href = "https://www.youtube.com/watch?v=846d4m0wHQo"> Click me to see the video demo! </a>  

Future goals:
1. Userspace, Port Newlib, Multi-process, multi-thread, IPC, write syscalls, etc(process related stuff)
2. Implement a hobby level TCP/IP stack(Ethernet/ARP/DHCP/IP/UDP/TCP with congestion control and everything..)
3. Graphical User Interface(C/S GUI Server, Composite via IPC, now the GUI is in kernel)
4. Write three GUI user apps(Shell, TCP ChatRoom, Text Editor)

# Screenshots  
![Alt text](/os_screenshots/ss15.png?raw=true "ss15")
![Alt text](/os_screenshots/ss0.png?raw=true "ss0")
![Alt text](/os_screenshots/ss1.png?raw=true "ss1")




# How to run and test edu OS kernel
## Preparation(Ubuntu14.04 is a recommended environment for compiling and running edu os)
eduos use elf-i686-gcc as cross-compiler, please build the cross compiler on your system first.  
Remember to replace "CC=i686-pc-btos-gcc" in Makefile with your cross compiler.

<a href = "http://wiki.osdev.org/GCC_Cross-Compiler">How to build a cross compiler</a>  
Or, a rewritten/translated version of the above wiki article in Simplified Chinese  

Link: <a href = "http://pan.baidu.com/s/1hsg6AEg">环境搭建与交叉编译器</a> Download password: 7ksb  

Then, install nasm 
```
sudo apt-get install nasm
```

Run the following script to generate 4 hard disk image
```
./mkext2image.sh
```

Run the following script to setup the tap network device
```
./setup_tap_device.sh
```

Note: The path of executable qemu-bridge-helper may vary, and so you should manually replace this path in qemu_run.sh, kvm.sh and debug_qemu.sh first. 
Use this command to find the path of qemu-bridge-helper 
```
cd ~
find . -iname qemu-bridge-helper
```

If you would like to run the os in GUI mode, set the constant GUI_MODE (defined in kmain.c) to 1. 
If you would like to run the os in Network mode, set the constant NETWORK_MODE (defined in kmain.c) to 1. 

Now run the following script to compile the kernel
```
./compile.sh
```

And then choose one of the following simulator to boot edu os. QEMU is more recommended.

## QEMU
1.  Run sudo apt-get install qemu-system
2.  Run ./qemu_run.sh to boot edu os(or ./kvm.sh if your environment supports kvm)


### Debug with QEMU
1. run ./debug_qemu.sh


# Plan
#### Mem management  
&#160; &#160; &#160; &#160;0 Higher half loader                [✔]  
&#160; &#160; &#160; &#160;1 physical memory management        [✔]  
&#160; &#160; &#160; &#160;2 paging(virtual memory management) [✔]  
&#160; &#160; &#160; &#160;3 kmalloc                           [✔]  

#### Filesystem  
&#160; &#160; &#160; &#160;0 PCI Enumerate,R/W     [✔]  
&#160; &#160; &#160; &#160;1 ATA/DMA Driver        [✔]  
&#160; &#160; &#160; &#160;2 Ext2 Filesystem       [✔]  
&#160; &#160; &#160; &#160;3 Virtual Filesystem    [✔]  

#### Graphical Interface  
&#160; &#160; &#160; &#160;0 vesa driver, 1024 * 768 color(32-bit color)，framebuffer mode                                              [✔]  
&#160; &#160; &#160; &#160;1 Milestone(try loading and showing a wallpaper from hardisk!)                                               [✔]  
&#160; &#160; &#160; &#160;2 Windows compositor, support fonts, buttons,and etc                                                         [✔]  
&#160; &#160; &#160; &#160;3 Write a shell                                                                                              []  
&#160; &#160; &#160; &#160;4 Write a file browser                                                                                       []  

#### Miscellaneous  
&#160; &#160; &#160; &#160;1 Usermode                                                               [✔]  
&#160; &#160; &#160; &#160;1 Multi-tasking/Simple scheduler                                         []  
&#160; &#160; &#160; &#160;2 Executable loader(elf)                                                 [✔]  
&#160; &#160; &#160; &#160;3 spinlock, mutex, and other pthread primitives                          []  
&#160; &#160; &#160; &#160;4 Add more syscalls, like fork/exec, open, read, etc..                   []  
&#160; &#160; &#160; &#160;5 Port newlib                                                            []  
&#160; &#160; &#160; &#160;6 Write or port some baisc utilities such asenv, rm, cp, mkdir, reboot   []  
&#160; &#160; &#160; &#160;7 Real time clock                                                        [✔]  
&#160; &#160; &#160; &#160;9 stadard input/output stuff                                             []  

#### Network protocols  
&#160; &#160; &#160; &#160;0 Send/Recv raw packets  [✔]  
&#160; &#160; &#160; &#160;1 IP                     [✔]  
&#160; &#160; &#160; &#160;2 UDP                    [✔]  
&#160; &#160; &#160; &#160;3 ARP                    [✔]  
&#160; &#160; &#160; &#160;4 DHCP                   [✔]  
&#160; &#160; &#160; &#160;5 TCP                    []  
&#160; &#160; &#160; &#160;6 HTTP                   []  
