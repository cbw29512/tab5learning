"""Portable prerequisite graph reference, independent of grade and hardware."""
import logging
import re
from collections import deque

LOG = logging.getLogger(__name__)
IDENTIFIER = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")
MAX_SKILLS = 10000


def valid_id(value):
    return isinstance(value, str) and IDENTIFIER.fullmatch(value) is not None


def validate_catalog(catalog):
    """Return deterministic topological order; reject missing edges and cycles."""
    try:
        if not isinstance(catalog, dict) or set(catalog) != {"catalog_id", "version", "skills"}:
            raise ValueError("Invalid catalog envelope")
        if not valid_id(catalog["catalog_id"]) or type(catalog["version"]) is not int or not 1 <= catalog["version"] <= 2**31-1:
            raise ValueError("Invalid catalog identity")
        skills = catalog["skills"]
        if not isinstance(skills, list) or not 1 <= len(skills) <= MAX_SKILLS:
            raise ValueError("Invalid skill count")
        graph = {}
        for skill in skills:
            if not isinstance(skill, dict) or set(skill) != {"skill_id", "subject", "level_label", "prerequisites"}:
                raise ValueError("Invalid skill envelope")
            key = skill["skill_id"]
            if not valid_id(key) or key in graph or not valid_id(skill["subject"]):
                raise ValueError("Invalid or duplicate skill identity")
            label = skill["level_label"]
            deps = skill["prerequisites"]
            if not isinstance(label, str) or not label.strip() or len(label) > 80:
                raise ValueError("Invalid level label")
            if not isinstance(deps, list) or len(deps) > 128 or any(not valid_id(d) for d in deps) or len(set(deps)) != len(deps):
                raise ValueError("Invalid prerequisites")
            graph[key] = tuple(deps)
        children = {key: [] for key in graph}
        degree = {}
        for key, deps in graph.items():
            degree[key] = len(deps)
            for dep in deps:
                if dep not in graph:
                    raise ValueError("Unknown prerequisite")
                children[dep].append(key)
        ready = deque(sorted(k for k, n in degree.items() if n == 0))
        order = []
        while ready:
            key = ready.popleft()
            order.append(key)
            for child in sorted(children[key]):
                degree[child] -= 1
                if degree[child] == 0:
                    ready.append(child)
        if len(order) != len(graph):
            raise ValueError("Prerequisite cycle")
        return order
    except (ValueError, TypeError):
        LOG.warning("Competency catalog rejected")
        raise


def eligible_skills(catalog, approved_mastery):
    """Only approved, prerequisite-consistent mastery unlocks later skills.

    Inputs must come from an authorized version-matched policy projection.
    This function neither assesses mastery nor authorizes advancement.
    """
    try:
        order = validate_catalog(catalog)
        if not isinstance(approved_mastery, (set, frozenset)) or any(not valid_id(k) for k in approved_mastery):
            raise ValueError("Expected approved skill ID set")
        graph = {s["skill_id"]: set(s["prerequisites"]) for s in catalog["skills"]}
        if not approved_mastery <= graph.keys():
            raise ValueError("Unknown mastered skill")
        # Invalidate downstream approvals if a prerequisite was revoked.
        effective = set()
        for key in order:
            if key in approved_mastery and graph[key] <= effective:
                effective.add(key)
        return {
            "effective_mastery": sorted(effective),
            "needs_review": sorted(approved_mastery - effective),
            "available": [k for k in order if k not in effective and graph[k] <= effective],
            "blocked": {k: sorted(graph[k] - effective) for k in order
                        if k not in effective and not graph[k] <= effective},
        }
    except (ValueError, TypeError):
        LOG.warning("Competency eligibility rejected")
        raise
