# SaWa (Samba Wannabe)

SaWa is an attempt to write a networking filesystem on Linux.

The concept is to have a block device driver on one end talking to a fileserver (running in userspace) which provides access to the data. Communications are done through TCP/IP.

## The device driver

The Linux kernel driver part is still in development and not working yet.

## The file server

The fileserver is however functional. It listens on port 5000, uses a flat file for the filesystem and accepts three commands:

- Info, which returns the number of sectors
- Read, which reads a certain number of bytes
- Write, which writes some data at a certain offset

    $ ./daemon/sawad
    Server started on port 5000
    
To test the fileserver, `sawa-client` is also provided:

    $ ./daemon/sawa-client info 0 0
    $ ./daemon/sawa-client write 10 100
    $ ./daemon/sawa-client read 10 100
    $ ./daemon/sawa-client test

## Web Server

`sawad` is designed to handle multiple connections (using different threads). In order to test how well `sawad` handles workload, it has an `-http` option which turns it into a rudimentary Web server, so it can be tested by tools such as `ab`.

    ./daemon/sawad -http
    ./daemon/sawad -http -debug
