# Response Pipeline — `HttpResponse`, handler, and serialization

The response side does not exist yet. This document specifies it from
scratch, in a way that mirrors the request side so the two pipelines stay
symmetric and easy to reason about.

## Three responsibilities, three components

Producing a response is three different jobs. Today the placeholder code in
`Reactor::run` does all three at once with a hard-coded string literal. The
recommendation is to give each job its own component.

| Job                                 | Component             | Layer       |
|-------------------------------------|-----------------------|-------------|
| Decide what to reply with           | `RequestHandler`      | application |
| Hold the structured reply           | `HttpResponse`        | protocol    |
| Encode the reply into bytes         | `HttpResponseWriter`  | protocol    |

Keeping these separate matches the request side (router/parser/bytes split)
and makes each piece independently testable.

## `HttpResponse` — the data container

A passive value type, symmetric with `HttpRequest`. Owned by `Client`,
populated by `RequestHandler`, read by `HttpResponseWriter`.

```cpp
class HttpResponse {
public:
    HttpResponse();

    // Writers (used by the handler)
    void setStatus(int code);                       // implies a default reason
    void setStatus(int code, const std::string& reason);
    void setHeader(const std::string& name, const std::string& value);
    void setBody(const std::string& body);
    void appendBody(const char* data, size_t n);    // for streaming producers

    // Readers (used by the writer and by tests)
    int  statusCode() const;
    const std::string& statusReason() const;
    const std::map<std::string, std::string>& headers() const;
    bool hasHeader(const std::string& name) const;
    const std::string& header(const std::string& name) const;
    const std::string& body() const;

    // Reset for the next exchange on a keep-alive connection.
    void clear();

private:
    HttpResponse(const HttpResponse&);
    HttpResponse& operator=(const HttpResponse&);
    // members
};
```

Design notes:

- **No serialization on this class.** `HttpResponse` does not know how to
  turn itself into HTTP wire bytes. That is the writer's job. Mixing the two
  is the same mistake as letting `HttpRequest` hold parser state.
- **`setStatus(int)` looks up a default reason phrase** ("OK", "Not Found",
  …) from a small static table. Handlers should rarely need to pass a custom
  reason. Keep the table next to `HttpResponse.cpp`, not scattered.
- **Header storage matches `HttpRequest`** (case-insensitive map keyed by
  lowercase name). `setHeader` overwrites; if you ever need multi-valued
  headers (`Set-Cookie`), add `addHeader` then.
- **Body is held in a `std::string`.** This is fine for static files smaller
  than `client_max_body_size`. If you implement a sendfile-style streaming
  path or CGI streaming later, the body becomes a producer interface (a
  callback or an fd) rather than a string. Until then, the simple form is
  enough.
- **`clear()` mirrors `HttpRequest::clear()`** so the per-exchange reset in
  `Client` is uniform.

## `RequestHandler` — the application layer

Translates a parsed request plus the matching server config into a populated
response. This is where routing, method-allow checks, file resolution,
redirects, directory listings, and CGI dispatch live. It is the only HTTP
component that may touch the filesystem or fork/exec.

```cpp
class RequestHandler {
public:
    explicit RequestHandler(const ServerConfig& cfg);

    // Either way is fine; pick one and stick to it.
    //   void handle(const HttpRequest&, HttpResponse&);
    //   HttpResponse handle(const HttpRequest&);   // returns by value
    //
    // For C++98, prefer the out-parameter form: it lets `Client` reuse its
    // member `response_` without copying a body-sized string.
    void handle(const HttpRequest& req, HttpResponse& res);
};
```

Design notes:

- **`RequestHandler` takes the config by reference at construction.** It
  does not need to be passed on every call. One handler per `Server` is a
  reasonable lifetime; it can also live as a member of `Client` if you want
  per-client state (e.g. per-client rate limiting later on).
- **The handler is the only place that builds error responses.** When the
  parser reports an error, `Client` still calls into the handler with a
  flag (or a separate `handleError(int status, HttpResponse&)` method). This
  keeps "what does a 404 look like" in one file, not sprinkled across the
  codebase.
- **The handler returns synchronously for static content.** For CGI it
  cannot — the response arrives over time from a child process. When you
  add CGI, the handler will need an "is the response ready yet" mechanism
  and `Client` will gain a `WAITING_FOR_CGI` phase. Keep that out of the
  initial design; the synchronous form composes cleanly with an async
  extension later because the handler is the only place that needs to know.
- **`RequestHandler` is the natural seam for tests.** Hand it a request and
  a config, inspect the response, no sockets involved.

## `HttpResponseWriter` — bytes onto the wire

A pure function from `HttpResponse` to bytes. It writes into a buffer that
`Client` already owns, so there is no extra copy. It does not call `send()`;
that is still `Reactor`'s job.

```cpp
struct HttpResponseWriter {
    // Append the wire-format encoding of `res` to `out`.
    static void serialize(const HttpResponse& res, std::string& out);
};
```

Design notes:

- **Static / namespaced free function is fine.** The writer has no state.
  Putting it in a struct is just for organization.
- **It is responsible for `Content-Length`.** If the response carries a body
  and the handler did not set `Content-Length`, the writer fills it in from
  `res.body().size()`. The handler should not have to remember this.
- **It is responsible for the framing rules.** Status line, CRLF separators,
  blank line before body. Centralizing these here keeps the framing rules
  off the handler's hot path.
- **It may add `Connection: close` or `Connection: keep-alive`** based on
  what `Client` decides about the connection's future. Either pass that in
  as a parameter or have the handler set the header explicitly — pick one
  rule and follow it everywhere.

## How `Client` glues these together

This is the per-exchange flow inside `Client`:

```cpp
void Client::onReceive(const char* buf, size_t n) {
    if (state_ == RECEIVING_REQUEST) {
        parser_.feed(buf, n, request_);

        if (parser_.hasError()) {
            handler_.handleError(parser_.errorStatus(), response_);
            HttpResponseWriter::serialize(response_, writeBuffer_);
            state_ = WRITING_RESPONSE;
            return;
        }
        if (parser_.isComplete()) {
            handler_.handle(request_, response_);
            HttpResponseWriter::serialize(response_, writeBuffer_);
            state_ = WRITING_RESPONSE;
        }
    }
}

// Called by Reactor after writeBuffer_ has been fully drained.
void Client::onResponseSent() {
    if (shouldClose()) {
        state_ = CLOSING;
        return;
    }
    parser_.reset();
    request_.clear();
    response_.clear();
    state_ = RECEIVING_REQUEST;
}
```

Things to notice:

- **`Client` is the orchestrator**, but it contains no HTTP rule logic of
  its own. Every HTTP decision goes through one of `parser_`, `handler_`,
  or `HttpResponseWriter`.
- **One reset call per component, no nesting.** This is what the layered
  ownership in `README.md` buys you: there are no objects-inside-objects
  whose lifetimes you have to think about.
- **`Reactor` only sees `responseData()`/`responseSize()`/`consumeResponse()`
  and `shouldClose()`**. It still has no idea what HTTP is, which is the
  goal.

## Test surfaces

The split gives you four independent test targets, none of which need a
socket:

| Tests                       | Inputs                          | Asserts                  |
|-----------------------------|---------------------------------|--------------------------|
| Parser tests                | bytes                           | populated `HttpRequest`  |
| Handler tests               | `HttpRequest` + `ServerConfig`  | populated `HttpResponse` |
| Writer tests                | `HttpResponse`                  | byte sequence            |
| Client integration tests    | bytes in                        | bytes out                |

The first three never instantiate a `Client` or a `Reactor`. That is what
"separation of concerns" buys you in practice — not prettier diagrams, but
tests that fail in one place when one thing is wrong.
