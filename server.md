# SaWa Server Architecture

The main complexity of the SaWa Server is the thread pool, or how each new TCP connection is handled by a separate thread. The server keeps a pool of idle thread and reuses them.

The workflow is as follows:

- `sawad.c` / `sawa_server_start()`: the server listens to any new connection, and calls `handle_new_connection()` for any new connection.
- `thread_pool.c` / `handle_new_connection()`: this function's role is to find a thread to process the request. Either reuse an existing thread (`reuse_thread()`) or create a new thread (`new_thread()`)
- `thread_pool.c` / `reuse_thread()`: this function looks at an existing thread in the idle queue (`idle_threads`). If it finds one it awaks it by calling `pthread_kill(thread_info->thread, SIGUSR1)`
- `thread_pool.c` / `new_thread()`: creates a new thread, calling `connection_handler()` in that thread.
- `thread_pool.c` / `connection_handler()`: this function handles the new socket. It keeps calling `op_listen()` until the connection is closed. Once it is, the thread is moved to the unassigned queue and goes to sleep.
- `op_listen` is a function pointer which can either point to `sawa_srv.c` / `sawa_listen()` or `http_srv.c` / `http_listen()`, depending on whether the server is configured to run as a SaWa or HTTP server.
- Both functions process the SaWa or HTTP request and perform the appropriate action.

## The Thread Pool

The thread pool is composed of a FILA (First In, Last Out) queue `idle_threads`. It contains a linked list of `struct connection_thread` objects, each object containing information about the thread (id, socket id, thread statistics). Any modification of that queue must first acquire the `idle_threads_lock` mutex.

I toyed with the idea of having a FIFO queue where the oldest idle thread gets picked up first. This would allow to have two independent mutex - one to add a thread to the queue and one to remove a thread from the queue. This would allow to add a thread to the idle queue while another thread is being removed from it. The problem is when the idle queue contains one or less threads. In order not to mess with the linked list, one function would need to hold both mutexes, which .

The upside of a FILA queue is that it requires only one mutex (`idle_threads_lock`), and adding/removing a `struct connection_thread` to the queue is fairly quick.

## Display

The server has three different types of display:

- Default (`sawad`): thread statistics are displayed in the screen using NCurses.
- Debug (`sawad -debug`): the degug information is printed to the stdio.
- Daemon (`sawad -daemon`): no debug information is produced (in a future version errors will be stored in a log file)

The `screen` object points to a `struct display` which contains the appropriate methods.
