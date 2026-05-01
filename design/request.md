# Request Pipeline — `HttpRequest` and `HttpParser`

## Problem with the current design

```
HttpParser
├── State state_
├── std::string buf_
├── size_t bodyLength_
└── HttpRequest req_           <-- parser owns the request
                                   HttpRequest declares `friend class HttpParser`
```

The parser **owns** the request and writes private fields directly via `friend`.
The client gets at the request through `parser_.request()`. The consequences:

1. **Lifetime conflation.** The request exists for as long as the parser
   exists. Resetting the parser for the next keep-alive request also has to
   reset the request, and you have to remember to do both. There is no
   compile-time signal that you forgot one.
2. **The parser has two jobs.** Driving the state machine and storing the
   parsed result. These have different lifetimes (state is per-request, but
   lives in a long-lived parser; the request is per-exchange).
3. **`friend` couples the request to the parser class name.** Any future
   writer of the request (a fuzz test, a test fixture that constructs a
   request directly, a chunked-body decoder) needs to be added as a friend or
   reach in through some other backdoor.
4. **No one outside `Client` can write a request for testing.** Today the
   only way to populate an `HttpRequest` is to feed bytes through the parser.

## Recommended split

Make `HttpRequest` a plain data type owned by the caller. Make `HttpParser` a
pure algorithm that is told where to write its output.

```
HttpRequest                       HttpParser
├── method_                       ├── State state_
├── uri_                          ├── std::string buf_
├── version_              <----   ├── Reader r_
├── headers_                      └── size_t bodyLength_
└── body_                         (no HttpRequest member)
```

The relationship inverts: the parser **points at** a request the caller owns;
it does not contain one.

### `HttpRequest` API

A passive data container. Public read accessors stay as they are. Add public
**mutators** the parser can call by name — drop `friend class HttpParser`.

```cpp
class HttpRequest {
public:
    HttpRequest();

    // Read access (already present)
    const std::string& method()  const;
    const std::string& uri()     const;
    const std::string& version() const;
    const std::string& body()    const;
    const std::map<std::string, std::string>& headers() const;
    bool        hasHeader(const std::string& name) const;
    const std::string& header(const std::string& name) const;

    // Write access used by the parser (or by tests that build fixtures).
    void setMethod(const std::string& m);
    void setUri(const std::string& u);
    void setVersion(const std::string& v);
    void addHeader(const std::string& name, const std::string& value); // lowercases name
    void appendBody(const char* data, size_t n);

    // Reset for the next exchange on a keep-alive connection.
    void clear();

private:
    HttpRequest(const HttpRequest&);
    HttpRequest& operator=(const HttpRequest&);
    // members unchanged
};
```

Notes:

- Public mutators are **strictly better than `friend`** here. They give the
  parser exactly the access it needs, and let tests construct a request
  without touching the parser. They also document the request's "writeable
  surface" in the header file.
- `appendBody(data, n)` (instead of an assignable string) keeps the door open
  for chunked transfer-encoding later, where the body arrives in pieces.
- `clear()` is the per-exchange reset. It does not deallocate; `std::string`
  and `std::map` keep their nodes/buffers, which is what you want for
  keep-alive throughput.
- Keep the class non-copyable. It contains a `std::string` body that may be
  large, and you never need to copy it — it lives as a member of `Client`
  and is read by reference everywhere else.

### `HttpParser` API

The parser becomes a stateful algorithm. It still owns its FSM state and
its internal scratch buffer (`buf_`), but it does not own the result.

```cpp
class HttpParser {
public:
    HttpParser();

    // Feed bytes. The parser writes into `out` as it makes progress.
    // Returns false iff parsing has entered the error state.
    bool feed(const char* data, size_t n, HttpRequest& out);

    bool isComplete() const;
    bool hasError()  const;
    int  errorStatus() const;   // 400, 413, 414, 505... for the response

    // Drop FSM state and internal buffer. Does not touch the request.
    void reset();

private:
    HttpParser(const HttpParser&);
    HttpParser& operator=(const HttpParser&);

    enum State { /* unchanged */ };
    State       state_;
    std::string buf_;
    Reader      r_;
    size_t      bodyLength_;
};
```

Why pass `out` into `feed()` rather than into the constructor?

- It makes the dependency obvious at every call site.
- It lets a single parser instance be reused across keep-alive requests
  while different `HttpRequest` values are filled in turn.
- It avoids a dangling-reference footgun: a member reference stored at
  construction has a lifetime contract that is easy to violate.

Why expose `errorStatus()`?

- Today there is no path from a parse failure to the right HTTP status code.
  A request with a malformed request-line should produce `400 Bad Request`;
  a body bigger than `client_max_body_size` should produce `413`;
  an unsupported version should produce `505`. The parser is the only place
  that knows which rule was violated, so it is the right place to surface
  the code. The handler reads it when building the error response.

### Migration from the current code

The structural change is small:

1. Move `req_` out of `HttpParser` into `Client`.
2. Replace `friend class HttpParser` in `HttpRequest` with public mutators.
3. Change `HttpParser::feed(data, n)` to `feed(data, n, HttpRequest& out)`
   and replace every `req_.method_ = …` with `out.setMethod(…)` etc.
4. Add `HttpParser::reset()` and `HttpRequest::clear()`.
5. In `Client::onReceive`, the call becomes `parser_.feed(buf, n, request_);`.

Nothing about the grammar code in `Rules.cpp` or `Reader.hpp` changes.

## Where parsing ends and dispatch begins

`HttpParser::isComplete()` is the handoff point. When `Client::onReceive` sees
it, it transitions the connection's phase to `HANDLING` and asks the
application layer for a response. That layer is described in
[response.md](response.md).

The parser does **not** call into the handler, the response, or the writer.
It returns control to `Client`, which orchestrates the next step. Keeping the
parser inert is what makes it independently testable: feed bytes, assert on
the resulting `HttpRequest`, no mocks required.
