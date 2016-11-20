# simpleos
A toy operating system based on <a href = "http://www.osdever.net/bkerndev/Docs/title.htm"> Brans' Kernel Development Tutorial</a>  
The major goals of this project is to build a operating system that has:  
1. Memory manager(from virtual page allocation to kmalloc) [Done]  
2. File system(Hard disk driver, Support ext2, and virtual filesystem) [Done]  
3. Graphical User Interface  
4. Premptive multitasking  
5. Network(TCP, UDP, HTTP)  
6. Sound  
7. User apps(File browser, downloader, editor and etc)

I am also planning to start a similar project called simplegcc(A basic C compiler with both front-end and back-end), after simpleos.

# How to run and test simpleos kernel

## Ubuntu(recommended, not required)
For OS X users, please use Vagrant to setup a virtual ubuntu environment first  
<a href = "https://www.vagrantup.com/downloads.html"> Download Vagrant here</a>

Be sure to install virtual box and ruby first.  

A vagrant file is provided here:  
```
# -*- mode: ruby -*-
# vi: set ft=ruby :

Vagrant.configure(2) do |config|
  config.vm.box = "ubuntu/trusty32"
  config.ssh.forward_x11 = true
  config.vm.provider "virtualbox" do |vb|
    vb.memory = "1024"
  end
end
```
Create a file named Vagrantfile in any directory you like, then simply do
```
vagrant up
vagrant ssh
```

## Preparation
Simpleos use elf-i686-gcc as cross-compiler, please build the cross compiler on your system first.

<a href = "http://wiki.osdev.org/GCC_Cross-Compiler">How to build a cross compiler</a>  
Or, a rewritten/translated version of the above wiki article in Simplified Chinese  

Link: <a href = "http://pan.baidu.com/s/1hsg6AEg">环境搭建与交叉编译器</a> Download password: 7ksb  

Then, install nasm 
```
sudo apt-get install nasm
```

Now clone the repo to your vm and simply run ./compile.sh to compile the kernel, and then choose one of the following simulator to boot simpleos. QEMU is more recommended.

## QEMU
1.  Run sudo apt-get install qemu-system
2.  Run ./qemu_run.sh to boot simpleos


### Debug with QEMU
1. run ./debug_qemu.sh

## Bochs
1.  sudo apt-get install bochs
2.  sudo apt-get install bochs-x
3.  sudo apt-get install bochs-sdl
4.  If you have problems with bochs-x(which in my case it is), add a line "display_library: sdl" in bochsrc.txt, if you're using my bochsrc.txt, just ignore this step
5.  run ./update_image.sh to poke your kernel binary into the floppy image file
6.  run ./run_bochs to boot simpleos


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
&#160; &#160; &#160; &#160;2 Windows compositor, support fonts, optimize GUI speed                                                      [In Progress]  
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
&#160; &#160; &#160; &#160;8 Kernel-module loading                                                  []  
&#160; &#160; &#160; &#160;9 stadard input/output stuff                                             []  

#### Network protocols  
&#160; &#160; &#160; &#160;0 TCP/IP                []  
&#160; &#160; &#160; &#160;1 UDP                   []  
&#160; &#160; &#160; &#160;2 HTTP                  []  

#### Sound  
&#160; &#160; &#160; &#160;0 sound driver          []

#### Port  
&#160; &#160; &#160; &#160;0 Port vim to my os     []

The plan is subject to change  

# Screenshots  
![Alt text](/os_screenshots/ss7.png?raw=true "ss7")
![Alt text](/os_screenshots/ss12.png?raw=true "ss12")

