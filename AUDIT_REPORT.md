# Task 08 — Protocol and Memory Safety Audit

## Scope and baseline

Reviewed the C17 protocol codec, framer, query/response serializers and deserializers, message ownership helpers, TCP helpers, server/client lifecycle, and existing unit/integration tests.

Baseline before source changes:

- `cmake -S . -B build`: passed.
- `cmake --build build`: passed with the project's `-Wall -Wextra -Wpedantic -Werror` settings.
- `ctest --test-dir build --output-on-failure`: 10/10 passed.

The passing baseline did not cover all malformed-input cases described below.

## Findings and fixes

### F-01 — Framer accepts an unknown message type

- **File/function:** `src/protocol/framer.c`, `append()` and `protocol_framer_extract()`
- **Severity:** Medium
- **Status:** Confirmed; fixed
- **Problem:** Once five header bytes are present, the framer read the declared payload length but did not validate the message-type byte. A header with type `0xFF` could be reported as a complete frame and only rejected later by a message-specific deserializer.
- **Reproduction:** Append a five-byte header with type `0xFF` and a zero payload length. Before the fix, `append()` returned `PROTOCOL_SUCCESS`.
- **Expected behavior:** Reject an unknown message type as `PROTOCOL_INVALID` at the framing boundary.
- **Minimal fix:** Validate the type against `PROTOCOL_MESSAGE_QUERY` and `PROTOCOL_MESSAGE_RESPONSE` in append and extract.
- **Regression test:** `framer_rejects_invalid_message_type_test`.

### F-02 — Response deserializer accepts undeclared trailing payload bytes

- **File/function:** `src/protocol/response_deserializer.c`, `deserialize_response()`
- **Severity:** Medium
- **Status:** Confirmed; fixed
- **Problem:** The parser checked `value_end > frame_size`, which rejected overlong values but accepted a frame whose declared payload contained extra bytes after the value. Query parsing already requires an exact payload match.
- **Reproduction:** Send a response declaring a four-byte payload with status `SUCCESS`, value length zero, and one extra payload byte. Before the fix, the response parsed successfully and the extra byte was silently ignored.
- **Expected behavior:** The declared payload must consist exactly of the status, value-length field, and value bytes.
- **Minimal fix:** Require `value_end == frame_size`.
- **Regression test:** `test_trailing_payload_bytes_rejected`.

### F-03 — Embedded NUL bytes are accepted into C-string fields

- **File/functions:** `src/protocol/deserializer.c`, `deserialize_query()`; `src/protocol/response_deserializer.c`, `deserialize_response()`
- **Severity:** Medium
- **Status:** Confirmed; fixed
- **Problem:** The wire format carries explicit string lengths, but public fields are `char *`/`const char *` and downstream code uses C-string operations. An embedded NUL was copied and followed by a terminator, causing string consumers to observe a shorter value than the encoded length represented.
- **Reproduction:** Encode a key or response value containing an embedded `0x00`. Before the fix, deserialization succeeded and C-string operations observed only the prefix.
- **Expected behavior:** Given the existing C-string API and serializer's `strlen()` contract, reject embedded NUL bytes rather than silently changing the logical value.
- **Minimal fix:** Reject embedded NUL bytes before allocation/copy.
- **Regression tests:** `test_embedded_nul_string_bytes_rejected` and `test_embedded_nul_value_rejected`.

### F-04 — Successful deserialization can overwrite existing owned output and leak it

- **File/functions:** `src/protocol/deserializer.c`, `deserialize_query()`; `src/protocol/response_deserializer.c`, `deserialize_response()`
- **Severity:** Medium
- **Status:** Confirmed; fixed
- **Problem:** On success, each parser assigned newly allocated pointers into the caller's output object without checking whether it already held owned memory. Reusing the same output object without freeing it first lost the old pointer and leaked the allocation.
- **Reproduction:** Successfully deserialize a message into an output object, then deserialize another valid message into the same object without freeing it. Before the fix, the second parse succeeded and replaced the owned pointer.
- **Expected behavior:** A parser should not silently overwrite an existing output object and orphan its allocation.
- **Minimal fix:** Reject a non-empty output object with `PROTOCOL_INVALID`; document that output objects must be zero-initialized/empty and freed before reuse.
- **Regression tests:** `test_deserializer_rejects_nonempty_output` and `test_response_deserializer_rejects_nonempty_output`.

### F-05 — Framer init/destroy dereference a null argument

- **File/functions:** `src/protocol/framer.c`, `protocol_framer_init()` and `protocol_framer_destroy()`
- **Severity:** Low
- **Status:** Confirmed; fixed
- **Problem:** The append/extract functions validate their object argument, but init/destroy unconditionally dereferenced it.
- **Reproduction:** Call either function with `NULL`; before the fix, it dereferenced a null pointer.
- **Expected behavior:** Null input is safely ignored, consistent with the defensive behavior of the other framer API functions.
- **Minimal fix:** Add null guards.
- **Regression test:** `framer_init_destroy_null_are_safe_test`.

## Ownership contract clarified

- `query_init()` and `response_init()` store borrowed strings and set ownership flags to false.
- Deserializers allocate owned copies and set ownership flags to true.
- `query_free()` and `response_free()` release only fields marked as owned and clear the pointers/ownership flags.
- Deserializer output objects must be zero-initialized/empty before a parse. Free successfully decoded objects before reusing them.
- Serializer inputs must point to valid NUL-terminated C strings. Embedded NUL bytes in length-delimited wire strings are rejected because the public API cannot represent them without truncation.

## Reviewed behavior that was already correct

- Query deserialization validates message type, declared maximum size, minimum payload size, available frame bytes, operation enum, key/value lengths, and exact payload consumption.
- Response deserialization validates type, maximum/minimum payload size, available bytes, status enum, and value length; the exact payload check is now fixed as described above.
- Framer uses a temporary pointer for `realloc()`, preserving the original allocation on allocation failure; caps retained bytes at one maximum-sized frame; preserves incomplete input; and transfers extracted frames to the caller, which frees them.
- `tcp_send_all()` handles partial sends, retries `EINTR`, and uses `MSG_NOSIGNAL` to avoid `SIGPIPE` on supported Linux/POSIX environments.
- `tcp_recv()` retries `EINTR` and distinguishes orderly disconnect from receive errors.
- The server abandons an incomplete frame when a client disconnects, destroys the framer, closes that client socket, and returns to `accept()`.
- The server processes one connected client at a time. This is existing behavior, not changed in this audit; the supplied README contains no documentation establishing a different concurrency expectation.

## Remaining risks and limitations

- Allocation-failure paths were reviewed, but deterministic allocator-failure injection is not configured in the project, so every allocation-failure branch is not directly exercised by a regression test.
- The API relies on callers passing valid NUL-terminated strings to serializers; C cannot safely validate an arbitrary invalid pointer.
- The server is sequential: a connected client that stays open can delay service to other queued clients. This audit does not redesign connection handling.
- `MSG_NOSIGNAL` is platform-specific; the current project and test environment are Linux-oriented.
- Valgrind was not installed in the audit environment.
- Sanitizer/static-analysis success is evidence for the exercised paths, not proof of complete memory safety.

## Verification after fixes

- Normal GCC/CMake build: passed.
- Normal CTest suite: 10/10 passed.
- Combined AddressSanitizer and UndefinedBehaviorSanitizer build: passed.
- Combined sanitizer CTest suite: 10/10 passed.
- Clang static analyzer: not independently verified during this final validation.
- Valgrind: not run; unavailable in the environment.

No wire-format changes, new dependencies, threads, `epoll`, or unrelated features were added.
