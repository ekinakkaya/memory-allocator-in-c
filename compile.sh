libname="memalloc.so"

#compile as library
gcc -o memalloc.so -fPIC -shared main.c

echo "[test] exporting... LD_PRELOAD=$PWD/$libname"
export LD_PRELOAD=$PWD/$libname
echo $LD_PRELOAD

#ls

export LD_PRELOAD=


