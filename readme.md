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
idea and half of code - bsd sample. make_dev dragonfly manpage also has some info