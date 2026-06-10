# persistent-tcp-connection

Do `socket-cleanup-on-disconnect` alongside.

## Problem

Python opens and tears down a TCP connection for **every command**: `_send_command_once` calls `connect()` at the top and `_close_socket_unsafe()` in `finally` (`Python/unreal_mcp_server_advanced.py:350-396`). Meanwhile the C++ accept loop polls `HasPendingConnection` with `FPlatformProcess::Sleep(0.1f)` (`MCPServerRunnable.cpp:188`), so every new connection waits ~50ms average / 100ms worst case just to be accepted. At 800–1000 commands per big build, that's 40–100 seconds of pure handshake/poll overhead.

## Fix

1. **Python:** Keep the socket open across commands. Reconnect only on send/recv failure (the retry/backoff logic in `send_command` already handles this). Remove the `finally: _close_socket_unsafe()`.
2. **C++:** Replace the 0.1s polling accept with a blocking accept or `FSocket::Wait`-based loop so reconnects are picked up promptly.
3. The wire protocol is already newline-framed in both directions, so back-to-back commands sharing one connection/segment are handled.
4. Do `socket-cleanup-on-disconnect` alongside — with persistent connections, a dead-but-unclosed client must not lock out the single-client server.

## Acceptance

- 100 sequential `spawn_actor` calls reuse one TCP connection (verify via log) and show no per-command ~50ms floor.
