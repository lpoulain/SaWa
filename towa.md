# ToWa (Tomcat Wannabe)

An optional extension of the Web Server is to act as Java Application Server.

This in construction and is barely working at this point.

### Architecture

When `sawad` is started with the `-http` (Web Server) and `-towa` (ToWa) flags, all the HTTP requests are forwarded to ToWa.

- A `towa` process is spawned and acts as the application pool running the servlets
- `sawad` checks that the `towa` process exists before sending a communicatin, and spawns the process again if that is not the case.
- `sawad` and `towa` communicate using a class deriving from `TowaIPC`. Right now only `TowaPipe` is available, and is using two named pipes to communicate: `towa_ping` (the HTTP request sent from `sawad` to `towa`) and `towa_pong` (the response from `towa`)
- `towa` is using JNI to call the Java Servlet. It is using the Java classes `TowaRequest` and `TowaResponse` to pass and receive data to the servlet.

### Future Enhancements

- Add more reliability to the `towa` process. Right now, if any error happens in the JNI layer (like the requested servlet is not available), the JNI layer issues a SIGSEGV which crashes the `towa` process.
- Scan the `classpath` folder for any `.war` file, unzip them if it hasn't already been done and add the path to the classpath
- Allow the Web Server to both look at the filesystem and servlet requests by reading the `web.xml` mappings
- Add support for more IPC options between `sawad` and `towa`
