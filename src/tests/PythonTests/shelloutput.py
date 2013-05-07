#!/usr/bin/python

# Contains the definitions for the output of the shell in Pintos

kernel = {'userprog': '../../userprog/build'}

# Pintos executable
startup = "pintos -v --filesys-size=2 -p ../../examples/login -a login \
-p ../../examples/shell -a shell -p ../../examples/passwd -a passwd \
-p ../../examples/chmod -a chmod -p ../../examples/getty -a getty \
-p ../../examples/echo -a echo -p ../../examples/sudo -a sudo -p \
../sudoers -a etc/sudoers -p ../group -a etc/group -p ../passwd -a \
etc/passwd -p ../shadow -a etc/shadow -p ../../examples/cat \
-a cat -- -f -q "

login = startup + "run login"
getty = startup + "run getty"

prompt = "--"
