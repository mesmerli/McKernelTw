McKernelTw
==========
This GitHub respository is a based on McKernel 1.2.6 from http://www.pccluster.org/ja/mckernel/download.html
Some modification will be done, and it's suspected as my personal playground.
If you prefer the native one, it might be necessary for you to download the native tar file from the above URL.

Overview
--------
This is a snapshot of our research and development effort that includes
the following software:
 - IHK: Interface for Heterogeneous Kernels
 - McKernel: Lightweight kernel for HPC on manycore systems

User mailing list
-----------------
mckernel-users@pccluster.org is the user mailing list.
Please visit the following URL to subscribe this mailing list:
   http://www.pccluster.org/mailman/listinfo/mckernel-users
If you have any questions, please contact us through this mailing list.

Platform
--------
We have tested McKernel on the following combinations of OS distribution and
platform:
* CentOS 7.2 and 7.3 minimal running on Intel Xeon
* CentOS 7.2 and 7.3 minimal running on VMware
* Ubuntu 14.04.2 running on Intel Xeon

Installation
------------
CentOS 7.2 / Ubuntu 14.04.2 running on Intel Xeon processor
Download "mckernel-1.2.6.tar.gz" file and unpack using tar xzf <filename>,
e.g.
```Bash
$ wget http://www.pccluster.org/ja/mckernel/mckernel-1.2.6.tar.gz
$ tar xzf mckernel-1.2.6.tar.gz
```
Or via git
```Bash
$ git clone https://github.com/mesmerli/McKernelTw
```
${TOP} variable denotes the "mckerneltw" directory in the followings.

### Build IHK and McKernel
For building by yourself you will need to go through the following steps.

**Step 1) Change Linux settings**
In the following instructions, log in as the root user or switch to root 
user with sudo by typing in a terminal

```Bash
$ sudo su -
```
* Disable irqbalance:
```Bash
# systemctl disable irqbalance
```
* Disable SELinux:
```Bash
# vi /etc/selinux/config
```
change to SELINUX=disabled

**Step 2) Reboot**
Reboot the host machine:
```Bash
$ sudo reboot
```
**Step 3) Prepare packages, kernel function table**
* Perform the following if kernel-devel package isn't installed.

For CentOS:
```Bash
$ sudo yum install kernel-devel-`uname -r`
```
For Ubuntu:
```Bash
$ sudo apt-get install linux-headers-'uname -r'-generic
```
* Perform the following if /usr/src/kernels/`uname -r` doesn't exist
```Bash
$ sudo ln -s /usr/src/kernels/<longer kernel version> \
  /usr/src/kernels/`uname -r`
```
* Perform the following to make sure that the
/lib/modules/`uname -r`/build symlink points to /usr/src/kernels/`uname -r`
```Bash
$ ls -ld /lib/modules/`uname -r`/build
```
* Perform the following to grant read permission to the System.map
file of your kernel version:
```Bash
$ sudo chmod a+r /boot/System.map-`uname -r`
```
* NUMA library is required
```Bash
$ sudo apt-get install libnuma-dev 
```
**Step 4) Configure, Compile and Install**
Assume the ${TOP} variable denotes the "mckernel-1.2.0" directory.
Run the following command when doing automatically with our script.
```Bash
$ config_and_build_smp_x86.sh ${TOP} ${TOP}/build ${TOP}/install
```
Or follow the following steps when doing manually.
First, execute the following commands to create the directory
layout.
```Bash
$ export TOP=${HOME}/mckerneltw
$ cd ${TOP}
$ mkdir install
```
Second, run the configuration scripts from the source directories:
```Bash
$ cd ${TOP}/src/ihk
$ sudo ./configure --with-target=smp-x86 --prefix=${TOP}/install
$ cd ${TOP}/src/mckernel
$ sudo ./configure --with-target=smp-x86 --prefix=${TOP}/install
```
Third, compile and install IHK and McKernel:
```Bash
$ cd ${TOP}/src/ihk
$ make && make install
$ cd ${TOP}/src/mckernel
$ make && make install
```
The IHK Linux kernel modules, the McKernel image and the reboot scripts
should be available under ${TOP}/install.

**Step 5) Boot McKernel**
Boot McKernel with the following commands:
```Bash
$ cd ${TOP}/install
$ sudo sbin/mcreboot.sh
```
Check McKernel boot log with the following commands:
```Bash
${TOP}/install/sbin/ihkosctl 0 kmsg|grep "MCK/IHK booted"
```
By default, the mcreboot script will take half of the CPU cores and
512MB RAM from NUMA node 0, but you can modify it. Note that you
need at least two CPU cores in your system.

Usage
-----
For the instructions to use the McKernel environment, please read the
HOW_TO_USE.txt under the doc directory.  Several example programs are located
under the doc/tutorial/sample.

Last but not least
------------------
Enjoy,
	McKernel Development Team
