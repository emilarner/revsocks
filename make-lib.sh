cd lib/unix
gcc -c ../../*.c
ar rcs librevsocks.a *.o
rm *.o

echo "The static library has been compiled to lib/unix/librevsocks.a"
