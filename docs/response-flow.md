# Response Build Flow — `RequestHandler`

This diagram traces what happens inside [RequestHandler::handle()](../src/Http/RequestHandler.cpp#L27)
from the moment a parsed `HttpRequest` arrives until an `HttpResponse` is fully populated.

## High-level flow

```mermaid
flowchart TD
    A([handle req, res]) --> B[res.clear]
    B --> C[processLocation_<br/>match URI prefix to LocationConfig]
    C --> C1{location<br/>found?}
    C1 -- no --> E404[handleError 404 NOT_FOUND]
    E404 --> END([return response])

    C1 -- yes --> D[processMethodValidation_<br/>check loc.methods contains req.method]
    D --> D1{method<br/>allowed?}
    D1 -- no --> E405[handleError 405 NOT_ALLOWED]
    E405 --> END

    D1 -- yes --> R[processRedirect_<br/>inspect loc.redirect]
    R --> R1{redirect<br/>set?}
    R1 -- yes --> RR[setStatus 301 MOVED_PERMANENTLY<br/>setHeader Location = loc.redirect]
    RR --> END

    R1 -- no --> M[dispatchMethod_<br/>switch on req.method]
    M --> M1{which<br/>method?}
    M1 -- GET  --> G[handleGet_  withBody=true]
    M1 -- HEAD --> H[handleHead_]
    M1 -- POST --> P[handlePost_]
    M1 -- DELETE --> DEL[handleDelete_]
    M1 -- other --> E501[handleError 501 NOT_IMPLEMENTED]

    G --> END
    H --> END
    P --> END
    DEL --> END
    E501 --> END
```

## Step-by-step

| # | Step | Source | What it does |
|---|------|--------|--------------|
| 1 | clear | [RequestHandler.cpp:29](../src/Http/RequestHandler.cpp#L29) | Reset any previous state on the response object. |
| 2 | match location | [processLocation_](../src/Http/RequestHandler.cpp#L45) → [matchLocation_](../src/Http/RequestHandler.cpp#L98) | Longest-prefix match of `req.uri()` against `config_.locations`. `404` if none match. |
| 3 | validate method | [processMethodValidation_](../src/Http/RequestHandler.cpp#L59) → [isMethodAllowed_](../src/Http/RequestHandler.cpp#L119) | Reject with `405` if `req.method()` isn't in `loc.methods`. |
| 4 | redirect | [processRedirect_](../src/Http/RequestHandler.cpp#L68) | If `loc.redirect` is non-empty, emit `301` + `Location` and stop. |
| 5 | dispatch | [dispatchMethod_](../src/Http/RequestHandler.cpp#L78) | Route to `handleGet_` / `handleHead_` / `handlePost_` / `handleDelete_`; unknown verbs get `501`. |

## Error path

```mermaid
flowchart LR
    X[handleError status, res] --> X1[res.clear<br/>res.setStatus status]
    X1 --> X2[writeErrorBody_]
    X2 --> X3{custom error<br/>page configured?}
    X3 -- yes & readable --> X4[readFile_<br/>set content-type via mimeTypes_<br/>set content-length<br/>setBody]
    X3 -- no / unreadable --> X5[generate inline HTML<br/>'<h1>code reason</h1>'<br/>content-type=text/html]
    X4 --> XEND([response ready])
    X5 --> XEND
```

Implemented in [handleError](../src/Http/RequestHandler.cpp#L191) and [writeErrorBody_](../src/Http/RequestHandler.cpp#L198).

## Filesystem helpers used by method handlers

- [readFile_](../src/Http/RequestHandler.cpp#L169) — slurps a file into a string for the body.
- [fileExists_](../src/Http/RequestHandler.cpp#L182) / [isDirectory_](../src/Http/RequestHandler.cpp#L160) — `stat`-based checks.
- [buildDirectoryListing_](../src/Http/RequestHandler.cpp#L131) — generates an HTML autoindex page.
- [mimeTypes_](../src/Http/RequestHandler.cpp#L224) — resolves `Content-Type` from the path's extension via `RequestHelpers::getContentType`.
