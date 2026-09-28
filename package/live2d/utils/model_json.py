from __future__ import annotations

import json
from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional


@dataclass
class Expression:
    name: str = ""
    file: str = ""

    def to_dict(self, v3: bool) -> Dict[str, Any]:
        if v3:
            return {"Name": self.name, "File": self.file}
        return {"name": self.name, "file": self.file}

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "Expression":
        # v3 uses "Name"/"File", v2 uses "name"/"file"
        return cls(
            name=d.get("Name", d.get("name", "")),
            file=d.get("File", d.get("file", "")),
        )


@dataclass
class Motion:
    file: str = ""
    # v3: seconds (FadeInTime/FadeOutTime), v2: milliseconds (fade_in/fade_out)
    fade_in: Optional[float] = None
    fade_out: Optional[float] = None
    sound: Optional[str] = None

    def to_dict(self, v3: bool) -> Dict[str, Any]:
        if v3:
            d: Dict[str, Any] = {"File": self.file}
            if self.fade_in is not None:
                d["FadeInTime"] = self.fade_in
            if self.fade_out is not None:
                d["FadeOutTime"] = self.fade_out
            if self.sound:
                d["Sound"] = self.sound
            return d
        d = {"file": self.file}
        if self.fade_in is not None:
            d["fade_in"] = int(self.fade_in)
        if self.fade_out is not None:
            d["fade_out"] = int(self.fade_out)
        if self.sound:
            d["sound"] = self.sound
        return d

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "Motion":
        return cls(
            file=d.get("File", d.get("file", "")),
            fade_in=d.get("FadeInTime", d.get("fade_in")),
            fade_out=d.get("FadeOutTime", d.get("fade_out")),
            sound=d.get("Sound", d.get("sound")),
        )


@dataclass
class Group:
    target: str = "Parameter"
    name: str = ""
    ids: List[str] = field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return {"Target": self.target, "Name": self.name, "Ids": list(self.ids)}

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "Group":
        return cls(
            target=d.get("Target", "Parameter"),
            name=d.get("Name", ""),
            ids=list(d.get("Ids", [])),
        )


@dataclass
class HitArea:
    id: str = ""
    name: str = ""

    def to_dict(self) -> Dict[str, Any]:
        return {"Id": self.id, "Name": self.name}

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "HitArea":
        return cls(id=d.get("Id", ""), name=d.get("Name", ""))


@dataclass
class ModelJson:
    # 2 = Cubism 2 (.model.json), 3 = Cubism 3/4 (.model3.json)
    version: int = 3

    # shared
    model: str = ""                             # "Moc" (v3) / "model" (v2)
    textures: List[str] = field(default_factory=list)
    physics: Optional[str] = None
    pose: Optional[str] = None
    expressions: List[Expression] = field(default_factory=list)
    motions: Dict[str, List[Motion]] = field(default_factory=dict)

    # v3-only
    display_info: Optional[str] = None
    user_data: Optional[str] = None
    groups: List[Group] = field(default_factory=list)
    hit_areas: List[HitArea] = field(default_factory=list)

    # -- conveniences -------------------------------------------------------

    def add_expression(self, name: str, file: str) -> "Expression":
        e = Expression(name=name, file=file)
        self.expressions.append(e)
        return e

    def add_motion(self, group: str, motion: Motion) -> Motion:
        self.motions.setdefault(group, []).append(motion)
        return motion

    def add_group(self, target: str, name: str, ids: List[str]) -> "Group":
        g = Group(target=target, name=name, ids=list(ids))
        self.groups.append(g)
        return g

    def add_hit_area(self, id_: str, name: str) -> "HitArea":
        h = HitArea(id=id_, name=name)
        self.hit_areas.append(h)
        return h

    # -- serialization ------------------------------------------------------

    def to_dict(self) -> Dict[str, Any]:
        v3 = self.version >= 3
        if v3:
            fr: Dict[str, Any] = {
                "Moc": self.model,
                "Textures": list(self.textures),
            }
            for key, val in (
                ("Physics", self.physics),
                ("Pose", self.pose),
                ("DisplayInfo", self.display_info),
                ("UserData", self.user_data),
            ):
                if val:
                    fr[key] = val
            if self.expressions:
                fr["Expressions"] = [e.to_dict(True) for e in self.expressions]
            if self.motions:
                fr["Motions"] = {
                    g: [m.to_dict(True) for m in ms]
                    for g, ms in self.motions.items()
                }
            d: Dict[str, Any] = {"Version": 3, "FileReferences": fr}
            if self.groups:
                d["Groups"] = [g.to_dict() for g in self.groups]
            if self.hit_areas:
                d["HitAreas"] = [h.to_dict() for h in self.hit_areas]
            return d

        # ---- v2 ----
        d = {
            "version": "Sample 1.0.0",
            "model": self.model,
            "textures": list(self.textures),
        }
        if self.motions:
            d["motions"] = {
                g: [m.to_dict(False) for m in ms]
                for g, ms in self.motions.items()
            }
        if self.expressions:
            d["expressions"] = [e.to_dict(False) for e in self.expressions]
        if self.physics:
            d["physics"] = self.physics
        if self.pose:
            d["pose"] = self.pose
        return d

    def to_string(self, indent: int = 2) -> str:
        """Indented on purpose — CubismJson mis-handles minified input."""
        return json.dumps(self.to_dict(), indent=indent, ensure_ascii=False)

    # -- parsing ------------------------------------------------------------

    @classmethod
    def from_dict(cls, d: Dict[str, Any]) -> "ModelJson":
        # Auto-detect by key casing: "Version"/"FileReferences" → v3.
        if "Version" in d or "FileReferences" in d:
            fr = d.get("FileReferences", {})
            return cls(
                version=3,
                model=fr.get("Moc", ""),
                textures=list(fr.get("Textures", [])),
                physics=fr.get("Physics"),
                pose=fr.get("Pose"),
                display_info=fr.get("DisplayInfo"),
                user_data=fr.get("UserData"),
                expressions=[Expression.from_dict(e) for e in fr.get("Expressions", [])],
                motions={
                    g: [Motion.from_dict(m) for m in ms]
                    for g, ms in fr.get("Motions", {}).items()
                },
                groups=[Group.from_dict(g) for g in d.get("Groups", [])],
                hit_areas=[HitArea.from_dict(h) for h in d.get("HitAreas", [])],
            )

        # ---- v2 ----
        return cls(
            version=2,
            model=d.get("model", ""),
            textures=list(d.get("textures", [])),
            physics=d.get("physics"),
            pose=d.get("pose"),
            expressions=[Expression.from_dict(e) for e in d.get("expressions", [])],
            motions={
                g: [Motion.from_dict(m) for m in ms]
                for g, ms in d.get("motions", {}).items()
            },
        )

    @classmethod
    def from_string(cls, s: str) -> "ModelJson":
        return cls.from_dict(json.loads(s))