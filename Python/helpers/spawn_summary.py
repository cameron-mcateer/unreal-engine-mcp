"""
Spawned-Actor Result Summaries

Builder tools spawn hundreds of actors; returning the full per-actor list
into the MCP client's context wastes tens of kilobytes per call. This module
condenses a spawned-actor list into a compact summary: count, a few sample
names, the shared name prefix, and a bounding box.
"""

from typing import Any, Dict, List, Optional

SAMPLE_NAME_COUNT = 5


def summarize_spawned_actors(
    actors: Optional[List[Any]],
    name_prefix: Optional[str] = None,
    sample_size: int = SAMPLE_NAME_COUNT,
) -> Dict[str, Any]:
    """Condense a list of spawned actors into a compact summary dict.

    Accepts either raw spawn_actor responses ({"status": ..., "result": {...}})
    or bare actor result dicts ({"name": ..., "location": [x, y, z], ...}),
    since builder code accumulates both shapes.
    """
    names: List[str] = []
    locations: List[List[float]] = []

    for item in actors or []:
        if not isinstance(item, dict):
            continue
        info = item.get("result") if isinstance(item.get("result"), dict) else item
        name = info.get("final_name") or info.get("name")
        if isinstance(name, str) and name:
            names.append(name)
        loc = info.get("location")
        if isinstance(loc, (list, tuple)) and len(loc) == 3:
            try:
                locations.append([float(v) for v in loc])
            except (TypeError, ValueError):
                pass

    summary: Dict[str, Any] = {
        "total_actors": len(actors or []),
        "sample_actor_names": names[:sample_size],
    }
    if name_prefix:
        summary["name_prefix"] = name_prefix
        summary["hint"] = (
            f"Actor names start with '{name_prefix}'. "
            "Use find_actors_by_name to list individual actors."
        )
    if locations:
        summary["bounds"] = {
            "min": [round(min(loc[i] for loc in locations), 1) for i in range(3)],
            "max": [round(max(loc[i] for loc in locations), 1) for i in range(3)],
        }
    return summary
