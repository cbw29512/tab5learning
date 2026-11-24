"""Bounded text-only deck contract for the first portable MVP slice."""
import csv
import io
import logging
import re

LOG = logging.getLogger(__name__)
ID = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")
MAX_CARDS = 500
MAX_BYTES = 512 * 1024


def validate_deck(deck):
    """Validate untrusted deck data; authorization is a separate transport gate."""
    try:
        if not isinstance(deck, dict) or set(deck) != {"deck_id", "version", "title", "cards"}:
            raise ValueError("Invalid deck fields")
        if not isinstance(deck["deck_id"], str) or not ID.fullmatch(deck["deck_id"]):
            raise ValueError("Invalid deck ID")
        if type(deck["version"]) is not int or not 1 <= deck["version"] <= 2147483647:
            raise ValueError("Invalid version")
        if not isinstance(deck["title"], str) or not 1 <= len(deck["title"].strip()) <= 120:
            raise ValueError("Invalid title")
        cards = deck["cards"]
        if not isinstance(cards, list) or not 1 <= len(cards) <= MAX_CARDS:
            raise ValueError("Invalid card count")
        seen = set()
        for card in cards:
            if not isinstance(card, dict) or set(card) != {"card_id", "front", "back"}:
                raise ValueError("Invalid card fields")
            cid = card["card_id"]
            if not isinstance(cid, str) or not ID.fullmatch(cid) or cid in seen:
                raise ValueError("Invalid or duplicate card ID")
            seen.add(cid)
            for key in ("front", "back"):
                value = card[key]
                if not isinstance(value, str) or not value.strip() or len(value) > 500:
                    raise ValueError("Invalid card text")
        return deck
    except (ValueError, TypeError):
        LOG.warning("Deck validation failed")
        raise


def import_csv(text, deck_id, title, version=1):
    """Import UTF-8 CSV with exact front,back columns; assign stable row IDs."""
    try:
        if not isinstance(text, str) or len(text.encode("utf-8")) > MAX_BYTES:
            raise ValueError("CSV exceeds input limit")
        reader = csv.DictReader(io.StringIO(text.lstrip("\ufeff")), strict=True)
        if reader.fieldnames != ["front", "back"]:
            raise ValueError("Expected front,back CSV header")
        cards = []
        for row in reader:
            if len(cards) >= MAX_CARDS or set(row) != {"front", "back"}:
                raise ValueError("Invalid CSV row or card limit exceeded")
            cards.append({"card_id": f"card-{len(cards)+1}", **row})
        return validate_deck({"deck_id": deck_id, "version": version,
                              "title": title, "cards": cards})
    except (ValueError, TypeError, csv.Error, UnicodeError):
        LOG.warning("CSV deck import rejected")
        raise
