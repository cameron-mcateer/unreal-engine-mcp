# socket-cleanup-on-disconnect

Becomes important once `persistent-tcp-connection` lands — do them together.

## Problem

The C++ server is single-client: after `Accept` (`MCPServerRunnable.cpp:45`) it services that one socket until disconnect and cannot accept another connection meanwhile (`:36-189`). On client disconnect, `ClientSocket` is never explicitly closed/reset before the loop re-polls — cleanup leans on the shared-pointer destructor and OS. Today the Python per-command connect/close masks all this; with persistent connections, a crashed-but-not-closed client (or a half-open socket after WSL network blips) could lock the server out indefinitely. Shutdown also uses `ServerThread->Kill(true)` (`EpicUnrealMCPBridge.cpp:192`), which can leave sockets half-open if the thread is blocked in `Recv`.

## Fix

- On inner-loop exit, explicitly `Close()` and reset `ClientSocket` before returning to the accept loop.
- Add an idle/receive timeout (or `FSocket::Wait` with timeout) so a dead client is detected and dropped, letting a reconnecting client in.
- Optionally: on a new pending connection while a client is "connected", drop the old socket and accept the new one — the Python side is the only legitimate client, and its reconnect logic already handles being dropped.
- For shutdown, signal `bRunning = false` and close the sockets to unblock `Recv` before joining the thread, instead of `Kill(true)`.

## Acceptance

- Kill the Python process mid-session without closing the socket; a new Python server instance can connect within a few seconds without restarting Unreal.
