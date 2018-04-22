# Block Driver Architecture

Block Drivers in Linux are device drivers for devices which store data that can be accessed randomly. See [https://lwn.net/images/pdf/LDD3/ch16.pdf](https://lwn.net/images/pdf/LDD3/ch16.pdf) for further information about block drivers.

Block Drivers operate in sectors (512 bytes by default), and most often read and write 4096-bytes pages of data at a time.
ss
The code here is based on [http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/](http://blog.superpat.com/2010/05/04/a-simple-block-driver-for-linux-kernel-2-6-31/). The original code implements a Ramdisk, allowing to access memory as a filesystem. The current block driver goes beyond and communicates to a remote server throught TCP/IP to access data.

Here is the data workflow:

- `main.c` / `sawa_driver_init()`: the driver declares the block device. It sets a spinlock (`sawa_dev.lock`) and declares a request queue which has a callback function `blockdev_request()`.
- `main.c` / `blockdev_request()`: this callback function is called whenever Linux has a read/write request to access the data.
    - This method first calls `set_next_request()` which sets `current_req` as the current work request.
    - This is where the device driver could perform some further optimizations. For instance, if two requests ask to read two contiguous pages, the driver could bundle them into a single operation.
    - It then calls `schedule_delayed_work()` to process the request as a [work queue](https://www.safaribooksonline.com/library/view/understanding-the-linux/0596005652/ch04s08.html).
    - The reason for using work queue is because `blockdev_request()` is supposed to take as little time as possible - especially as it holds the `sawa_dev.lock` spinlock. Performing TCP/IP operations directly inside this method would just hang Linux.
    - A tasklet cannot be used instead of a work queue because a tasklet is based on Softirqs, and Softirqs cannot sleep. This is incompatible with TCP/IP calls which call `sleep()` when waiting for a communication.
- `main.c` / `work_handler()`: this function is what is actually called by the work queue and runs in a separate kernel thread.
    - It calls `blockdev_transfer()` to process the read/write request
    - When completed, it removes the current work request from the queue. Note that it needs to hold the `sawa_dev.lock` spinlock to do so.
    - It checks if there is no other work request in the queue. If there is, it processes it. This process loops until there is no more work request in the queue.
    - The whole mechanism is designed to process only one work request at a time.
- `main.c` / `blockdev_transfer()`: this function is passed the operation type (read or write), the first sector to read/write, how many sectors as well as a memory buffer which either contains the data to write to the device or contains the location where the driver should copy the data which has to be read.
- `sawa.c` / `sawa_read_data()` and `sawa_write_data()`: those two functions craft a TCP/IP message to send to the SaWa server.
- `tcp.c` / `tcp_client_send()` and `tcp_client_receive()`: those two functions are used to respectively send and receive a message to and from the SaWa server using TCP/IP.
