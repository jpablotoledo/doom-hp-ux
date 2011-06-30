#!/bin/ksh
#
# Installs the Doom source (Acorn format) on a Unix machine.
# Execute this script from the directory containing Doom's c and h directories.

renameSources() {
  for i in $(ls -1 c); do
    mv c/${i} ${i}.c
  done

  for i in $(ls -1 h); do
    mv h/${i} ${i}.h
  done

  rmdir c
  rmdir h
}


renameSources
cd fastlz
renameSources
if [ -f Makefile.unix ]; then
  mv Makefile ../Acorn/Makefile.fastlz
  mv Makefile.unix Makefile
fi
cd ..

mv i_net.c Acorn/
mv i_video.c Acorn/
mv Makefile Acorn/
mv ROsupport.* Acorn/
cp -p linux-c/i_net.c .
cp -p linux-c/i_video.c .
cp -p linux-c/Makefile .
