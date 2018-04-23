# Client/Server Communication Protocol

The protocol is relatively simple, and contains 4 options:

Any message sent to the SaWa Server starts with:

- A 4-bytes unsigned integer which tells how long is the message (excluding the present integer)
- A 1-byte command

The integers are all unsigned 32-bits, stored with the machine's default Endian (so little Endian on Linux x64)

## Info

This allows to know how many sectors does the filesystem contain. The message is always 5 bytes long.

- Query: [0x01 00 00 00] [0x01]
- Response: [4-bytes message size]

## Read

Asks to get [size] bytes of data starting at [offset]. The query message is always 9 bytes (hence a size of 0x09 00 00 00).

- Query: [0x09 00 00 00] [0x02] [4-bytes message offset] [4-bytes message size]
- Response: either a one-byte error code, or the data

The reason for the response is so that the Block Driver can copy the response directly to the buffer provided by the operating system.

## Write

Asks to write a certain number of bytes of data starting at [offset]. The number of bytes to write is derived from the message size.

- Query: [4-bytes message size] [0x03] [4-bytes message offset] [data to write]
- Response: one-byte response

## Stop

This instructs the SaWa Server to shut down. This command is useful when the server is in daemon mode.

- Request: [0x01 00 00 00] [0x04]
- Response: n/a
