# fix-cpp-message-framing

**Blocks:** `add-spawn-actors-batch`, `persistent-tcp-connection`.

## Problem

`MCPServerRunnable.cpp:56-77` does one `Recv()` into an 8KB stack buffer, null-terminates, and parses the chunk as a complete JSON command. If a command exceeds 8KB or is fragmented by TCP, the JSON parse fails and the data is **silently discarded** (warning log at `:146`); the Python side then times out and retries the whole command. Likely triggers: large `add_node` `Properties` dicts, future batch commands, and the WSL/remote path where fragmentation is far more likely than on loopback.

The Python side compensates with a "keep recv'ing until the bytes parse as valid JSON" heuristic (`unreal_mcp_server_advanced.py:230-272`) — functional, but an editor hitch mid-response is indistinguishable from a truncated response.

There is already an unused newline-buffered receive path at `MCPServerRunnable.cpp:268` — evidence of an abandoned earlier fix. Remove or finish it.

## Fix

Pick one framing scheme and apply it to **both directions**:

- **Newline-delimited JSON** (simplest): Python appends `\n` to each command; C++ accumulates into an `FString`/`TArray<uint8>` buffer across recvs and processes complete lines. C++ appends `\n` to responses; Python reads until newline instead of parse-until-valid.
- Or 4-byte length prefix if embedded newlines in payloads are a concern (JSON-escaped strings won't contain raw newlines, so newline-delimiting is safe).

~30 lines of change total across `MCPServerRunnable.cpp` and the `UnrealConnection` class.

## Acceptance

- A >8KB command (e.g. a big batch spawn) round-trips correctly.
- Two commands arriving in one TCP segment are both processed (matters once connections are persistent).
