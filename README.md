# SaWa (Samba Wannabe)

SaWa is networking filesystem on Linux. It is composed of two pieces:

- A file server running in userspace which is accessible through TCP port 5000
- A block driver, which acts as the client and talks to the server through TCP/IP

## The file server

The fileserver listens to port 5000, uses a flat file for the filesystem and accepts three commands:

- Info, which returns the number of sectors
- Read, which reads a certain number of bytes
- Write, which writes some data at a certain offset

    $ ./daemon/sawad [-debug]
    Server started on port 5000
    
To test the fileserver, `sawa-client` is also provided:

    $ ./daemon/sawa-client info 0 0
    $ ./daemon/sawa-client write 10 100
    $ ./daemon/sawa-client read 10 100
    $ ./daemon/sawa-client test 1 1

## The device driver

The device driver is a Linux block driver which connects to the file server using TCP/IP (currently only on localhost) when it needs to access the data.

You must be logged as root to install and test it:

- `make`
- `make test`: installs the driver
- `fdisk /dev/sawa0`: creates a partition
    - Enter `n` (to create a new partition), select all default options
    - Enter `w` to commit the changes
- `mkfs /dev/sawa0`: formats the filesystem
- `mount /dev/sawa0 /mnt`: mounts the filesystem

After this the filesystem is available, e.g.

    # cd /mnt/
    # echo "Hello World" > hello.txt
    # cat hello.txt

To uninstall the driver:

- `umount /mnt`: unmounts the filesystem
- `make end`: uninstalls the driver

## Web Server

`sawad` is designed to handle multiple connections (using different threads). In order to test how well `sawad` handles workload, it has an `-http` option which turns it into a rudimentary Web server, so it can be tested by tools such as `ab`.

    ./daemon/sawad -http
    ./daemon/sawad -http -debug
