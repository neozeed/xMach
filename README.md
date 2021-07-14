# xMach
 xMach kernel (Mach4+Lites) from soruceforge
https://sourceforge.net/projects/xmach/

This is just a mirror of this original project.

I've been able to cross compile xMach + Lites from Linux using gcc 2.7.2.3 and binutils from OSKIT
or 2.12.1 works as well.

Make sure you have i586-linux-gcc in your path!

general instructions are;
```bash
mkdir kernel-build  
cd kernel-build  
../kernel/configure  --host=i586-linux --target=i586-linux --build=i586-linux --enable-elf --enable-libmach --enable-linuxdev --prefix=/usr/local/xmach
```

you will have to alter the Makeconf, or copy the one from 'updated-conf'

building Lites is very similar:
```bash
mkdir lites-build  
cd lites-build  
../lites/configure  --host=i586-linux --target=i586-linux --build=i586-linux --enable-mach4 --prefix=/usr/local/xmach --with-mach4=../kernel
```

likewise you will need to use an updated config file from 'updated-conf'.  If you try to run make before this it will not only fail, but a make clean will not actually clean the source correctly you will have to destroy the directory to try agin.

In 2021 this is not terribly practical, but I thought it was interesting enough to put somewhere.  Back when this was current I worked quite a bit to get this building on Linux as there was few and far working copies, and it took forever to compile.  These days an i9 can compile it well under a minute.
