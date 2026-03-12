install source tree (`cd /usr` , `make src-create-shallow` or `make src-create`)  
  
 then cd to repo dir and run `make`  

 you'll have `flare.ko`  
load:  
 `kldload ./flare.ko`  
unload:  
 `kldunload flare`  
use:  
```
sudo cp /dev/stdin /dev/echo
sudo cp /dev/echo /dev/stdout
```
 `ctrl+d` on a new line will end then flush `stdin` stream.

to check out poll:  
first build app that makes it
```
c++ vegabond.cpp -o t
```
  
`sudo ./t`  
  
full throttle:  
  
two terminals. in first one load `flare.ko` so it's brand new. In second one run `sudo ./t`. Then whithin 15s `sudo cp /dev/stdin /dev/echo
` in first one.  
  ![Screen1](/VirtualBox_hydy_05_03_2026_13_09_46.png)
  
bsd sample ported. make_dev dragonfly manpage has some info  
https://github.com/thesjg/SJG-DragonFly-BSD-SoC/wiki/Locking-strategy  
  
https://trimstray.github.io/assets/pdfs/kqueue.pdf
  
vm:  
  
install:
```
xorg xf86-input-libinput xf86-input-evdev

```
add:
```
dbus_enable="YES"
hald_enable="YES"
```
to `/etc/rc.conf`  

```
kern.evdev.rcpt_mask=6
```
to `/etc/sysctl.conf`  

AND  

```
Section "InputClass"
    Identifier "KeyboardDefaults"
    MatchIsKeyboard "on"
    Driver "libinput"
EndSection
```
to `/usr/local/etc/X11/xorg.conf.d/10-keyboard.conf`
  
access files from host how to:  
  
make second 2gb hdd on same controller then  
`fdisk -u ad1`  
make first partition ,`size 1000000` (whatever that means for 500M) , `sysid = 11`  
`newfs_msdos -F32 /dev/ad1s4`  

```
mkdir f
mount_msdos ad1s4 ./f
```
  
then to access it from linux  you need to open `.vdi` . 7zip  can do it . for 24.04+ linux there's no gui in stores but there's one in the wild (it's standalone archiver with own name).   
