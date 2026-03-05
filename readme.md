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
two terminals. in first one load `flare.ko` so it's brand new. In second one run `./t`. Then whithin 15s `sudo cp /dev/stdin /dev/echo
` in first one.

  
bsd sample ported. make_dev dragonfly manpage has some info  
https://github.com/thesjg/SJG-DragonFly-BSD-SoC/wiki/Locking-strategy  
  
https://www.usenix.org/legacy/events/usenix01/freenix01/full_papers/lemon/lemon.pdf
  
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
