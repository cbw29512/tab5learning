"""Local append-only award history. Authorization is a caller responsibility."""
import json
import logging
import sqlite3
from contextlib import closing
from tools.competency_graph import valid_id

LOG = logging.getLogger(__name__)


class AwardLedger:
    def __init__(self, path, learner_id):
        self.path, self.learner_id = path, learner_id
        try:
            if not valid_id(learner_id):
                raise ValueError("Invalid learner")
            with closing(self.connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                db.execute("CREATE TABLE IF NOT EXISTS award_owner (slot INTEGER PRIMARY KEY CHECK(slot=1), learner TEXT NOT NULL)")
                db.execute("CREATE TABLE IF NOT EXISTS awards (sequence INTEGER PRIMARY KEY, kind TEXT, award_id TEXT, version INTEGER, body TEXT NOT NULL, UNIQUE(kind,award_id,version))")
                db.execute("INSERT OR IGNORE INTO award_owner VALUES (1,?)", (learner_id,))
                self.check_owner(db)
        except (sqlite3.Error, ValueError):
            LOG.exception("Award ledger setup failed")
            raise

    def connect(self):
        return sqlite3.connect(self.path, timeout=5)

    def check_owner(self, db):
        if db.execute("SELECT learner FROM award_owner WHERE slot=1").fetchone() != (self.learner_id,):
            raise ValueError("Wrong learner ledger")

    def issue(self, candidate, version):
        """Issue only after trusted policy evaluation; exact replay is harmless."""
        try:
            if set(candidate) != {"award_id", "kind", "evidence_ids"}:
                raise ValueError("Invalid award fields")
            key, kind, evidence = candidate["award_id"], candidate["kind"], candidate["evidence_ids"]
            if not valid_id(key) or kind not in ("badge", "certificate") or type(version) is not int or not 1 <= version <= 2**31-1:
                raise ValueError("Invalid award identity")
            if not isinstance(evidence, list) or not evidence or len(evidence)>10000 or any(not valid_id(k) for k in evidence):
                raise ValueError("Invalid award evidence")
            body = json.dumps(sorted(set(evidence)))
            with closing(self.connect()) as db, db:
                db.execute("BEGIN IMMEDIATE")
                self.check_owner(db)
                existing = db.execute("SELECT sequence,body FROM awards WHERE kind=? AND award_id=? AND version=?", (kind,key,version)).fetchone()
                if existing:
                    if existing[1] != body:
                        raise ValueError("Conflicting award replay")
                    result = (existing[0], False)
                else:
                    seq = db.execute("INSERT INTO awards(kind,award_id,version,body) VALUES (?,?,?,?)", (kind,key,version,body)).lastrowid
                    result = (seq, True)
            return result
        except (sqlite3.Error, ValueError, TypeError, KeyError):
            LOG.warning("Award issuance rejected")
            raise

    def portfolio(self):
        try:
            with closing(self.connect()) as db:
                self.check_owner(db)
                return [{"sequence": seq, "kind": kind, "award_id": key,
                         "version": ver, "evidence_ids": json.loads(body)}
                        for seq,kind,key,ver,body in db.execute("SELECT sequence,kind,award_id,version,body FROM awards ORDER BY sequence")]
        except (sqlite3.Error, ValueError):
            LOG.warning("Award portfolio read failed")
            raise
