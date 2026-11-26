"""Synthetic text-deck archive codec; integrity only, not authentication."""
import hashlib
import hmac
import json
import logging

from tools.flashcard_deck import ID, validate_deck

LOG = logging.getLogger(__name__)
MAGIC = b"PLTA\x01"
MAX_BYTES = 16 * 1024 * 1024
MAX_VERSIONS = 2000


def _unique(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError("Duplicate JSON key")
        result[key] = value
    return result


def validate(archive):
    try:
        if not isinstance(archive, dict) or set(archive) != {"owner", "decks"}:
            raise ValueError("Invalid archive fields")
        owner, decks = archive["owner"], archive["decks"]
        if not isinstance(owner, str) or not ID.fullmatch(owner):
            raise ValueError("Invalid archive owner")
        if not isinstance(decks, list) or len(decks) > MAX_VERSIONS:
            raise ValueError("Invalid archive size")
        versions = {}
        for deck in decks:
            validate_deck(deck)
            ident = deck["deck_id"]
            if deck["version"] != versions.get(ident, 0) + 1:
                raise ValueError("Missing, duplicate or unordered deck version")
            versions[ident] = deck["version"]
        return archive
    except (ValueError, TypeError):
        LOG.warning("Teaching archive validation failed")
        raise


def encode(archive):
    try:
        body = json.dumps(validate(archive), ensure_ascii=False, sort_keys=True,
                          separators=(",", ":")).encode("utf-8")
        framed = MAGIC + body
        if len(framed) + 32 > MAX_BYTES:
            raise ValueError("Archive exceeds preview capacity")
        return framed + hashlib.sha256(framed).digest()
    except (ValueError, TypeError, UnicodeError):
        LOG.warning("Teaching archive encoding rejected")
        raise


def decode(raw):
    try:
        if not isinstance(raw, bytes) or not len(MAGIC)+32 < len(raw) <= MAX_BYTES:
            raise ValueError("Invalid archive length")
        if not raw.startswith(MAGIC) or not hmac.compare_digest(
                hashlib.sha256(raw[:-32]).digest(), raw[-32:]):
            raise ValueError("Archive integrity check failed")
        archive = json.loads(raw[len(MAGIC):-32].decode("utf-8"), object_pairs_hook=_unique)
        return validate(archive)
    except (ValueError, TypeError, UnicodeError, RecursionError):
        LOG.warning("Teaching archive decoding rejected")
        raise ValueError("Invalid teaching archive") from None
