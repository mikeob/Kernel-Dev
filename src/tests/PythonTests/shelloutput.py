#!/usr/bin/python

# Contains the definitions for the output of the shell in Pintos

kernel = {'userprog': '../../userprog/build'}

# Pintos executable
shell = "pintos -v --filesys-size=2 -p ../../examples/login -a login -p ../../examples/shell -a shell -- -f -q run login"
