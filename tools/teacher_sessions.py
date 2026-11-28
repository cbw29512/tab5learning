"""Process-local sessions for the trusted desktop application."""
import logging
import secrets
import time

LOG = logging.getLogger(__name__)


class TeacherSessions:
    def __init__(self, accounts, ttl_seconds=1800):
        if type(ttl_seconds) is not int or not 1 <= ttl_seconds <= 86400:
            raise ValueError("Invalid session lifetime")
        self.accounts = accounts
        self.ttl = ttl_seconds
        self.sessions = {}

    def login(self, username, password):
        try:
            key = self.accounts.verify(username, password)
            if key is None:
                return None
            now = time.monotonic()
            # Remove expired sessions and replace any prior session for this account.
            self.sessions = {t: v for t, v in self.sessions.items()
                             if v[1] > now and v[0] != key}
            token = secrets.token_urlsafe(32)
            self.sessions[token] = (key, now + self.ttl)
            return token
        except Exception:
            LOG.exception("Local session creation failed")
            raise

    def require(self, token, role=None):
        try:
            record = self.sessions.get(token) if isinstance(token, str) else None
            if not record or record[1] <= time.monotonic():
                self.logout(token)
                raise PermissionError("Session expired or invalid")
            key = record[0]
            actual = self.accounts.role(key)
            if actual is None:
                self.logout(token)
                raise PermissionError("Account disabled")
            if role is not None and actual != role:
                raise PermissionError("Insufficient permissions")
            return key
        except PermissionError:
            LOG.warning("Session authorization denied")
            raise

    def logout(self, token):
        if isinstance(token, str):
            self.sessions.pop(token, None)

    def create_account(self, token, username, password, role):
        actor = self.require(token, "admin")
        self.accounts.create(username, password, role, actor=actor)

    def disable_account(self, token, username):
        actor = self.require(token, "admin")
        self.accounts.disable(actor, username)
