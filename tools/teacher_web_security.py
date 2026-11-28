"""Local web request checks; missing Origin never substitutes for a CSRF token."""
import hashlib
import hmac
import re
import secrets
import time
from urllib.parse import urlsplit

LOGIN_COOKIE = "pocket_teacher_login"
LOGIN_TTL = 600


def browser_origin_ok(env, origin):
    # Different ports are different origins, even on the same loopback host.
    if env.get("HTTP_SEC_FETCH_SITE") in {"cross-site", "same-site"}:
        return False
    supplied = env.get("HTTP_ORIGIN")
    if supplied not in (None, "", "null", origin):
        return False
    referrer = env.get("HTTP_REFERER")
    if referrer:
        try:
            parsed = urlsplit(referrer)
            if f"{parsed.scheme}://{parsed.netloc}" != origin:
                return False
        except ValueError:
            return False
    # Caller MUST also check the session or login challenge CSRF token.
    return True


def login_challenge(key):
    nonce = f"{int(time.time())}.{secrets.token_hex(32)}"
    proof = hmac.new(key, ("login:" + nonce).encode(), hashlib.sha256).hexdigest()
    return nonce, proof


def valid_login_challenge(key, nonce, proof):
    if not isinstance(nonce, str) or not re.fullmatch(r"[0-9]{1,12}\.[0-9a-f]{64}", nonce):
        return False
    if not isinstance(proof, str) or not re.fullmatch(r"[0-9a-f]{64}", proof):
        return False
    issued = int(nonce.split(".", 1)[0])
    if not 0 <= time.time() - issued <= LOGIN_TTL:
        return False
    expected = hmac.new(key, ("login:" + nonce).encode(), hashlib.sha256).hexdigest()
    return hmac.compare_digest(expected, proof)
