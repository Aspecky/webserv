# Webserv — HTTP Layer Design

This document captures a recommended class design for the HTTP layer. It is
focused on **separation of concerns** and **clean object lifetimes** across
keep-alive requests. It does not prescribe code; it prescribes responsibilities,
ownership, and APIs.

The current code is C++98 (see `Makefile`: `-std=c++98`). The design avoids move
semantics and smart pointers from the standard library, and prefers
**reset-in-place** to per-request heap churn.

## Layered architecture

The server is split into four layers. Each layer only knows about the layer
directly below it.

```
+--------------------------------------------------------------+
| Application layer        RequestHandler / Router             |  pure logic
|                          (HttpRequest, Config) -> HttpResponse|
+--------------------------------------------------------------+
| Protocol layer           HttpParser, HttpResponseWriter      |  HTTP rules
|                          HttpRequest, HttpResponse (data)    |  (no I/O)
+--------------------------------------------------------------+
| Connection layer         Client                              |  per-conn state
|                          owns I/O buffers and the active HTTP|  (no socket
|                          exchange                            |   syscalls)
+--------------------------------------------------------------+
| Transport layer          Reactor, Server                     |  sockets only
|                          poll/recv/send/accept               |  (no HTTP)
+--------------------------------------------------------------+
```

Two rules follow from the layering:

1. **`Reactor` must not see HTTP types.** Today `Reactor::run` reads
   `client.request()`, prints headers, and hand-builds a response string. That
   is a layering leak. The reactor should only call `Client::onReceive`,
   `Client::responseData`/`responseSize`/`consumeResponse`, and
   `Client::shouldClose`. Anything HTTP-shaped happens inside `Client`.
2. **`HttpParser` must not see I/O.** It receives bytes via `feed()` and writes
   to a request value. It does not know about sockets, files, or buffers other
   than the one it is fed.

## Per-connection ownership

A `Client` owns everything tied to one TCP connection. Long-lived components
are constructed once per connection and **reset** between keep-alive requests.
Short-lived data (the parsed request, the response being built) is also owned
by `Client` but logically reset between exchanges.

```
Client
├── socket_              int                       (whole connection)
├── server_              Server&                   (whole connection)
├── parser_              HttpParser                (long-lived, resettable)
├── request_             HttpRequest               (per exchange, resettable)
├── response_            HttpResponse              (per exchange, resettable)
├── readBuffer_          std::string  (optional)   (long-lived)
├── writeBuffer_         std::string               (long-lived)
└── state_               enum Phase                (drives the FSM)
```

The key change vs. today: **the parser does not own the request**. The request
lives next to it inside `Client`. See [request.md](request.md) for why.

## Per-exchange lifecycle

A single HTTP exchange (one request, one response) goes through five phases.
The `Client` owns the phase variable and decides what `Reactor` should do next
based on it.

```
  RECEIVING_REQUEST  --recv bytes-->  parser_.feed(bytes, request_)
        |                                       |
        | parser_.isComplete()                  | parser_.hasError()
        v                                       v
  HANDLING            handler_.handle(    BUILDING_ERROR
        |             request_, cfg,            |
        |             response_)                |
        v                                       v
  WRITING_RESPONSE  <--writer.serialize(response_, writeBuffer_)
        |
        | writeBuffer_ drained, Connection: close ?
        |   yes -> CLOSING
        |   no  -> reset and back to RECEIVING_REQUEST (keep-alive)
        v
   CLOSING / IDLE
```

When the exchange ends and the connection stays open, **one** call resets
everything for the next request:

```
parser_.reset();
request_.clear();
response_.clear();
state_ = RECEIVING_REQUEST;
```

This is the answer to your concern about manual cleanup. The parser, request,
and response are three siblings inside `Client`. None of them owns the others,
so resetting them is a flat sequence of three calls — no order dependencies,
no destruction of nested objects.

## Why not introduce an `HttpTransaction` object?

A dedicated `HttpTransaction { HttpRequest req; HttpResponse res; Phase phase; }`
allocated fresh per request would also work and is what larger servers use.
For webserv I do not recommend it:

- It adds a heap allocation per request for no observable gain.
- Phases naturally belong to the connection-level state machine, because
  keep-alive means the *connection* has phases, not the request.
- Reset-in-place is trivial in C++98; per-request allocation is not.

If you later add features that genuinely need an exchange object with its own
lifetime (e.g. a CGI process whose response streams back over many poll
iterations after the next request has started parsing — which webserv does
not require), revisit this decision. Until then, keep `request_` and
`response_` as direct members of `Client`.

## Reading order

- [request.md](request.md) — `HttpRequest` and `HttpParser`, and how to
  break their current entanglement.
- [response.md](response.md) — `HttpResponse`, the request handler, and how
  bytes get back into the write buffer.
