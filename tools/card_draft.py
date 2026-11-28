"""Portable visual editor state; no GUI or storage dependencies."""
import logging
from uuid import uuid4
from tools.flashcard_deck import validate_deck, MAX_CARDS

LOG = logging.getLogger(__name__)


class CardDraft:
    def __init__(self):
        self.deck_id = uuid4().hex
        self.cards = []

    def put(self, front, back, index=None):
        try:
            if index is not None and (type(index) is not int or not 0 <= index < len(self.cards)):
                raise ValueError("Select an existing card")
            if index is None and len(self.cards) >= MAX_CARDS:
                raise ValueError("Deck limit reached")
            card = {"card_id": uuid4().hex if index is None else self.cards[index]["card_id"],
                    "front": front, "back": back}
            # Reuse the universal deck validation, even for a single edited card.
            validate_deck({"deck_id": self.deck_id, "version": 1, "title": "Draft", "cards": [card]})
            if index is None:
                self.cards.append(card)
            else:
                self.cards[index] = card
        except ValueError:
            LOG.warning("Draft card rejected")
            raise

    def remove(self, index):
        try:
            if type(index) is not int or not 0 <= index < len(self.cards):
                raise ValueError("Select an existing card")
            self.cards.pop(index)
        except ValueError:
            LOG.warning("Draft removal rejected")
            raise

    def build(self, title):
        return validate_deck({"deck_id": self.deck_id, "version": 1,
                              "title": title, "cards": [dict(c) for c in self.cards]})
