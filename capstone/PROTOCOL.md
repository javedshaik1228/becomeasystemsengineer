# NetForge wire protocol

## Transport and framing

NetForge uses TCP. Every message is:

| Field | Bytes | Encoding |
|---|---:|---|
| Payload length | 4 | Unsigned 32-bit, network byte order |
| Payload | declared length | UTF-8 command or response bytes |

The application limit is 65,536 payload bytes. TCP is a byte stream: a single
`send` is not a message, and one `recv` need not return a whole field. Readers
must accumulate exactly four header bytes before interpreting the length, reject
an excessive length before allocation, then accumulate exactly the payload.

## Requests

```text
PING
STATS
SET <safe-key> <non-empty value, spaces allowed>
GET <safe-key>
DEL <safe-key>
```

Keys are 1–128 ASCII letters, digits, `_`, `-`, `.`, or `:`. Embedded NUL bytes,
extra GET/DEL arguments, unsafe key characters, empty SET values, unknown verbs,
and oversized frames are invalid.

## Responses

```text
PONG
OK
VALUE <value>
DELETED
NOT_FOUND
keys=<n> submitted=<n> completed=<n>
ERR <bounded diagnostic>
```

## Important invariants

- An acknowledged mutation has been appended and `fsync`ed before it reaches the
  in-memory store.
- A malformed request never mutates state.
- A rejected length never controls an unbounded allocation.
- Every connection has one descriptor owner and one finite read/write timeout.
- Queue capacity bounds accepted-but-not-started work.

## Evolution questions

Version negotiation, checksums, authentication, request IDs, pipelining, binary
values, compression, streaming, and idempotency are deliberately absent. Treat
each as a system-design choice with compatibility, complexity, and attack-surface
cost—not an automatic improvement.
