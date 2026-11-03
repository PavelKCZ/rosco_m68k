# Safe replacement procedure

From `code/software/libs`:

```sh
# First make a full backup from the parent software directory!
cd ..
tar -czf libs_before_install_$(date +%Y%m%d_%H%M%S).tar.gz libs/
cd libs
# After verifying the backup, replace only the old termios source directory:
mv src/termios src/termios.previous
tar -xzf /path/to/libtermios-1.0-libs-install.tar.gz
make clean
make all
# Inspect: ls -l src/termios/libtermios.a build/lib/libtermios.a build/include/termios.h
```

**Important:** `make clean` on the top-level libs Makefile removes object files and archives for **all** libraries, as requested. The new include.mk adds cleanup for staged termios artifacts only. If the build fails, restore `src/termios.previous` and the original backup. After verifying success, delete the previous directory if desired.
