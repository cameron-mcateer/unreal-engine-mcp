# unify-response-envelope

## Problem

Two response shapes coexist:

- Handler level (`EpicUnrealMCPCommonUtils.cpp:30-48`): `{"success": bool, "data": {...}}` / `{"success": false, "error": "..."}`.
- Bridge level (`EpicUnrealMCPBridge.cpp:252-253, 334-367`): wraps in `{"status": "success"|"error", "result": {...}}`, with sniffing of the inner `success` field at `:348-355`.
- Python (`unreal_mcp_server_advanced.py:383-390`) carries normalization code papering over both (`status == "error"` vs `success is False`).

No structured error codes anywhere — only strings — so callers can't distinguish retryable from fatal errors.

## Fix

- Pick one envelope (suggest the bridge's `{"status", "result"/"error"}` since Python already keys off it) and make `CreateSuccessResponse`/`CreateErrorResponse` emit it directly; delete the bridge-level sniffing and the Python normalization.
- Optionally add an `error_code` field (e.g. `NOT_FOUND`, `INVALID_PARAMS`, `INTERNAL`) so the Python retry loop can stop retrying non-transient failures.

## Acceptance

- Every command returns the same envelope; Python normalization block (`:383-390`) is deleted, not extended.
