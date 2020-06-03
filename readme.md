install source tree (`cd /usr` , `make src-create-shallow` or `make src-create`)  
  
 then cd to repo dir and run `make`  

 you'll have `flare.ko`  
load:  
 `kldload ./flare.ko`  
unload:  
 `kldunload flare`  
use:  
```
echo anyth >/dev/echo
cp /dev/echo /dev/stdout
```
to check out poll:  
first build app that makes it
```
c++ vegabond.cpp -o t
```
  
`./t`  
  
bsd sample ported. make_dev dragonfly manpage has some info  
https://github.com/thesjg/SJG-DragonFly-BSD-SoC/wiki/Locking-strategy  
  
https://www.usenix.org/legacy/events/usenix01/freenix01/full_papers/lemon/lemon.pdf

