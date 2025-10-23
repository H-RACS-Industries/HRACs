# main_app/utils/device_sig.py
from django.http import JsonResponse
from django.core.cache import cache

import time, hashlib, hmac  # only using compare_digest
from decouple import config

from main_app.models import HeatingDevice  # fields: device_id, secret

WINDOW_SECONDS = 120  # allowed clock skew

def _bad(msg, code=401):
    return JsonResponse({"error": msg}, status=code)

def verify_sig(request):
    """
    Verifies X-Device-Id / X-Timestamp / X-Nonce / X-Signature headers.
    Signs exactly: SECRET || METHOD|PATH|TS|NONCE|BODY
    Uses request.path (include leading/trailing slash exactly as Django sees it).
    """
    dev_id = request.headers.get("X-Device-Id")
    ts     = request.headers.get("X-Timestamp")
    nonce  = request.headers.get("X-Nonce", "")
    sig    = request.headers.get("X-Signature")

    if not (dev_id and ts and sig):
        return False, _bad("missing headers")

    # timestamp window
    try:
        ts_int = int(ts)
    except ValueError:
        return False, _bad("bad timestamp")

    if abs(time.time() - ts_int) > WINDOW_SECONDS:
        return False, _bad("stale")

    # replay guard (optional but recommended)
    if nonce:
        key = f"nonce:{dev_id}:{nonce}"
        if cache.get(key):
            return False, _bad("replay")
        cache.set(key, 1, timeout=WINDOW_SECONDS)
    
    body = request.body or b""
    base = (
        config("ESP32_API_SECRET_KEY")
        + request.method.upper()
        + "|"
        + request.path      
        + "|"
        + ts
        + "|"
        + nonce
        + "|"
    ).encode("utf-8") + body
    
    digest = hashlib.sha256(base).hexdigest()  # lowercase hex
    if not hmac.compare_digest(digest, sig):
        return False, _bad("bad signature")

    return True, JsonResponse({"success": "Authorized"}, status=200)