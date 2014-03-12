**libinvisible** is a bit of magic to spread the semantics of Freedesktop ".hidden" files to the rest of your Linux environment.

More technically, libinvisible is an `LD_PRELOAD` shim that hooks `readdir()`.

After installing libinvisible, GUI and command-line programs alike will find themselves unable to see any directory entries listed in a ".hidden" file in the same directory.


## How do I use it?

1. `make install`

2. Put `export USE_LIBINVISIBLE=1` somewhere convenient, like your bashrc. (**Don't** put it where it will affect system startup scripts!)

Hidden files should immediately stop being visible to any newly-launched process that inherits the env var. To get already-running processes to use libinvisible, you will probably have to reboot.


## How do I *stop* using it?

libinvisible works by putting an entry in your /etc/ld.so.preload. You can remove this entry to restore normal behavior.
