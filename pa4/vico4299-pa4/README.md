Vince Coghlan - CSCI3753 - pa4
------------------------------

This is my project #4 for CSCI3753, Operating systems.  This is an
encrypted filesystem that uses FUSE to run.  It is designed for a Linux
system and uses specific linux commands, so this wont work on anything
else.  It is heavily reliant on the provided example code written by
asaylor, as well as code from a tutorial found here :http://www.cs.nmsu.edu/~pfeiffer/fuse-tutorial/
In order to run, first type

> make

then:

> ./pa4 <key> <source directory> <mount directory>

this will mount the file system at the mount directory.
