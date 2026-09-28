# test_model_json.py
from __future__ import annotations

import json
import traceback

import live2d
from live2d.utils.model_json import ModelJson, Motion
import os

# ===========================================================================
# Sample data
# ===========================================================================
RESOURCES = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Resources")
)


HARU_V3 = {
    "Version": 3,
    "FileReferences": {
        "Moc": "Haru.moc3",
        "Textures": [
            "Haru.2048/texture_00.png",
            "Haru.2048/texture_01.png",
        ],
        "Physics": "Haru.physics3.json",
        "Pose": "Haru.pose3.json",
        "DisplayInfo": "Haru.cdi3.json",
        "Expressions": [
            {"Name": "F01", "File": "expressions/F01.exp3.json"},
            {"Name": "F02", "File": "expressions/F02.exp3.json"},
        ],
        "Motions": {
            "Idle": [
                {
                    "File": "motions/haru_g_idle.motion3.json",
                    "FadeInTime": 0.5,
                    "FadeOutTime": 0.5,
                },
            ],
            "TapBody": [
                {
                    "File": "motions/haru_g_m26.motion3.json",
                    "FadeInTime": 0.5,
                    "FadeOutTime": 0.5,
                    "Sound": "sounds/haru_talk_13.wav",
                },
            ],
        },
        "UserData": "Haru.userdata3.json",
    },
    "Groups": [
        {
            "Target": "Parameter",
            "Name": "EyeBlink",
            "Ids": ["ParamEyeLOpen", "ParamEyeROpen"],
        },
        {
            "Target": "Parameter",
            "Name": "LipSync",
            "Ids": ["ParamMouthOpenY"],
        },
    ],
    "HitAreas": [
        {"Id": "HitArea", "Name": "Head"},
        {"Id": "HitArea2", "Name": "Body"},
    ],
}

HARU_V2 = {
    "version": "Sample 1.0.0",
    "model": "haru.moc",
    "textures": [
        "haru.1024/texture_00.png",
        "haru.1024/texture_01.png",
        "haru.1024/texture_02.png",
    ],
    "motions": {
        "idle": [
            {"file": "motions/haru_idle_01.mtn"},
            {"file": "motions/haru_idle_02.mtn"},
        ],
        "null": [
            {"file": "motions/haru_m_01.mtn"},
            {"file": "motions/haru_m_02.mtn", "fade_in": 500},
            {
                "file": "motions/haru_m_07.mtn",
                "fade_in": 300,
                "fade_out": 500,
            },
            {
                "file": "motions/haru_normal_01.mtn",
                "sound": "sounds/haru_normal_01.mp3",
            },
        ],
    },
    "expressions": [
        {"name": "f01.exp.json", "file": "expressions/f01.exp.json"},
    ],
    "physics": "haru.physics.json",
    "pose": "haru.pose.json",
}


# ===========================================================================
# Test registry
# ===========================================================================

_TESTS = []


def test(fn):
    _TESTS.append(fn)
    return fn


# ===========================================================================
# Builders
# ===========================================================================

@test
def test_add_motion_creates_group_lazily():
    m = ModelJson(version=3)
    m.add_motion("Idle", Motion("a.motion3.json"))
    m.add_motion("Idle", Motion("b.motion3.json"))
    m.add_motion("TapBody", Motion("c.motion3.json"))
    assert list(m.motions.keys()) == ["Idle", "TapBody"]
    assert [x.file for x in m.motions["Idle"]] == [
        "a.motion3.json",
        "b.motion3.json",
    ]


@test
def test_add_expression_returns_instance():
    m = ModelJson(version=3)
    e = m.add_expression("F01", "expressions/F01.exp3.json")
    assert e in m.expressions
    assert e.name == "F01"


@test
def test_add_group_and_hit_area():
    m = ModelJson(version=3)
    m.add_group("Parameter", "EyeBlink", ["ParamEyeLOpen"])
    m.add_hit_area("HitArea", "Head")
    assert m.groups[0].target == "Parameter"
    assert m.groups[0].ids == ["ParamEyeLOpen"]
    assert m.hit_areas[0].id == "HitArea"


@test
def test_ids_list_is_copied():
    m = ModelJson(version=3)
    src = ["ParamEyeLOpen", "ParamEyeROpen"]
    m.add_group("Parameter", "EyeBlink", src)
    src.append("Intruder")
    assert m.groups[0].ids == ["ParamEyeLOpen", "ParamEyeROpen"]


# ===========================================================================
# v3 serialization
# ===========================================================================

@test
def test_v3_top_level_shape():
    m = ModelJson(version=3)
    m.model = "Haru.moc3"
    m.textures = ["a.png"]
    d = m.to_dict()
    assert d["Version"] == 3
    assert d["FileReferences"]["Moc"] == "Haru.moc3"
    assert d["FileReferences"]["Textures"] == ["a.png"]


@test
def test_v3_groups_and_hit_areas_only_when_present():
    m = ModelJson(version=3)
    assert "Groups" not in m.to_dict()
    assert "HitAreas" not in m.to_dict()
    m.add_group("Parameter", "EyeBlink", ["ParamEyeLOpen"])
    m.add_hit_area("HitArea", "Head")
    d = m.to_dict()
    assert d["Groups"][0]["Name"] == "EyeBlink"
    assert d["HitAreas"][0]["Id"] == "HitArea"


@test
def test_v3_optional_file_refs_omitted_when_none():
    m = ModelJson(version=3)
    m.model = "a.moc3"
    d = m.to_dict()["FileReferences"]
    for key in ("Physics", "Pose", "DisplayInfo", "UserData"):
        assert key not in d


@test
def test_v3_motion_uses_capitalized_keys_and_seconds():
    m = ModelJson(version=3)
    m.add_motion("Idle", Motion("a.motion3.json", fade_in=0.5, fade_out=1.5))
    d = m.to_dict()["FileReferences"]["Motions"]["Idle"][0]
    assert d == {
        "File": "a.motion3.json",
        "FadeInTime": 0.5,
        "FadeOutTime": 1.5,
    }


@test
def test_v3_motion_sound_only_when_set():
    m = ModelJson(version=3)
    m.add_motion("Idle", Motion("a.motion3.json", 0.5, 0.5))
    d = m.to_dict()["FileReferences"]["Motions"]["Idle"][0]
    assert "Sound" not in d


@test
def test_v3_expression_uses_capitalized_keys():
    m = ModelJson(version=3)
    m.add_expression("F01", "expressions/F01.exp3.json")
    d = m.to_dict()["FileReferences"]["Expressions"][0]
    assert d == {"Name": "F01", "File": "expressions/F01.exp3.json"}


# ===========================================================================
# v2 serialization
# ===========================================================================

@test
def test_v2_top_level_shape():
    m = ModelJson(version=2)
    m.model = "haru.moc"
    m.textures = ["a.png"]
    d = m.to_dict()
    assert d["version"] == "Sample 1.0.0"
    assert d["model"] == "haru.moc"
    assert d["textures"] == ["a.png"]
    assert "FileReferences" not in d


@test
def test_v2_v3_only_fields_are_not_emitted():
    m = ModelJson(version=2)
    m.model = "haru.moc"
    m.display_info = "should.be.ignored.json"
    m.user_data = "should.be.ignored.json"
    m.add_group("Parameter", "EyeBlink", ["ParamEyeLOpen"])
    m.add_hit_area("HitArea", "Head")
    d = m.to_dict()
    for key in ("DisplayInfo", "UserData", "Groups", "HitAreas", "FileReferences"):
        assert key not in d


@test
def test_v2_motion_uses_lowercase_keys_and_ms():
    m = ModelJson(version=2)
    m.add_motion("null", Motion("a.mtn", fade_in=500, fade_out=300))
    d = m.to_dict()["motions"]["null"][0]
    assert d == {"file": "a.mtn", "fade_in": 500, "fade_out": 300}


@test
def test_v2_motion_fade_values_are_ints():
    m = ModelJson(version=2)
    m.add_motion("null", Motion("a.mtn", fade_in=500.7))
    d = m.to_dict()["motions"]["null"][0]
    assert isinstance(d["fade_in"], int)
    assert d["fade_in"] == 500


@test
def test_v2_expression_uses_lowercase_keys():
    m = ModelJson(version=2)
    m.add_expression("f01", "expressions/f01.exp.json")
    d = m.to_dict()["expressions"][0]
    assert d == {"name": "f01", "file": "expressions/f01.exp.json"}


# ===========================================================================
# Parsing & auto-detection
# ===========================================================================

@test
def test_from_dict_detects_v3():
    m = ModelJson.from_dict(HARU_V3)
    assert m.version == 3
    assert m.model == "Haru.moc3"
    assert len(m.textures) == 2
    assert m.display_info == "Haru.cdi3.json"
    assert m.user_data == "Haru.userdata3.json"


@test
def test_from_dict_detects_v2():
    m = ModelJson.from_dict(HARU_V2)
    assert m.version == 2
    assert m.model == "haru.moc"
    assert len(m.textures) == 3
    assert m.physics == "haru.physics.json"
    assert m.pose == "haru.pose.json"


@test
def test_v3_groups_and_hit_areas_parsed():
    m = ModelJson.from_dict(HARU_V3)
    assert [g.name for g in m.groups] == ["EyeBlink", "LipSync"]
    assert m.groups[0].ids == ["ParamEyeLOpen", "ParamEyeROpen"]
    assert [(h.id, h.name) for h in m.hit_areas] == [
        ("HitArea", "Head"),
        ("HitArea2", "Body"),
    ]


@test
def test_v3_motions_parsed():
    m = ModelJson.from_dict(HARU_V3)
    assert set(m.motions) == {"Idle", "TapBody"}
    assert m.motions["Idle"][0].fade_in == 0.5
    assert m.motions["TapBody"][0].sound == "sounds/haru_talk_13.wav"


@test
def test_v2_motions_parsed():
    m = ModelJson.from_dict(HARU_V2)
    assert set(m.motions) == {"idle", "null"}
    null = m.motions["null"]
    assert null[1].fade_in == 500
    assert null[2].fade_in == 300 and null[2].fade_out == 500
    assert null[3].sound == "sounds/haru_normal_01.mp3"


@test
def test_from_string():
    m = ModelJson.from_string(json.dumps(HARU_V3))
    assert m.version == 3
    assert m.model == "Haru.moc3"


@test
def test_v3_fade_int_is_coerced():
    d = {
        "Version": 3,
        "FileReferences": {
            "Moc": "a.moc3",
            "Textures": [],
            "Motions": {"Idle": [{"File": "a.motion3.json", "FadeInTime": 1}]},
        },
    }
    m = ModelJson.from_dict(d)
    assert m.motions["Idle"][0].fade_in == 1


# ===========================================================================
# Round-trips
# ===========================================================================

def _round_trip(sample):
    m1 = ModelJson.from_dict(sample)
    text = m1.to_string()
    m2 = ModelJson.from_string(text)
    assert m2.to_dict() == m1.to_dict()


@test
def test_round_trip_v3():
    _round_trip(HARU_V3)


@test
def test_round_trip_v2():
    _round_trip(HARU_V2)


@test
def test_v3_round_trip_preserves_all_fields():
    m1 = ModelJson.from_dict(HARU_V3)
    m2 = ModelJson.from_string(m1.to_string())
    assert m2.version == m1.version
    assert m2.model == m1.model
    assert m2.textures == m1.textures
    assert m2.physics == m1.physics
    assert m2.pose == m1.pose
    assert m2.display_info == m1.display_info
    assert m2.user_data == m1.user_data
    assert [g.to_dict() for g in m2.groups] == [g.to_dict() for g in m1.groups]
    assert [h.to_dict() for h in m2.hit_areas] == [
        h.to_dict() for h in m1.hit_areas
    ]


@test
def test_v2_round_trip_preserves_all_fields():
    m1 = ModelJson.from_dict(HARU_V2)
    m2 = ModelJson.from_string(m1.to_string())
    assert m2.version == 2
    assert m2.model == m1.model
    assert m2.textures == m1.textures
    assert m2.physics == m1.physics
    assert m2.pose == m1.pose
    assert json.loads(m1.to_string())["motions"] == json.loads(m2.to_string())[
        "motions"
    ]


# ===========================================================================
# Output format
# ===========================================================================

@test
def test_to_string_is_indented():
    m = ModelJson.from_dict(HARU_V3)
    assert "\n" in m.to_string()


@test
def test_custom_indent():
    m = ModelJson(version=3)
    assert "\n\t" in m.to_string(indent="\t")


@test
def test_to_string_is_valid_json():
    m = ModelJson.from_dict(HARU_V2)
    json.loads(m.to_string())


@test
def test_unicode_is_not_escaped():
    m = ModelJson(version=3)
    m.model = "ハル.moc3"
    assert "ハル" in m.to_string()


# ===========================================================================
# Edge cases
# ===========================================================================

@test
def test_empty_model_serializes():
    m = ModelJson(version=3)
    d = m.to_dict()
    assert d == {"Version": 3, "FileReferences": {"Moc": "", "Textures": []}}


@test
def test_unknown_keys_are_ignored_on_parse():
    d = dict(HARU_V3)
    d["UnknownFutureKey"] = {"stuff": 1}
    m = ModelJson.from_dict(d)
    assert m.model == "Haru.moc3"
    assert "UnknownFutureKey" not in m.to_dict()


@test
def test_missing_optional_keys_default_to_none():
    d = {
        "Version": 3,
        "FileReferences": {"Moc": "a.moc3", "Textures": []},
    }
    m = ModelJson.from_dict(d)
    assert m.physics is None
    assert m.pose is None
    assert m.display_info is None
    assert m.user_data is None


@test
def test_version_field_default_is_v3():
    assert ModelJson().version == 3


@test
def test_shared_fields_not_shared_between_instances():
    a = ModelJson(version=3)
    b = ModelJson(version=3)
    a.textures.append("x.png")
    a.add_motion("Idle", Motion("a.motion3.json"))
    a.add_group("Parameter", "G", ["P"])
    assert b.textures == []
    assert b.motions == {}
    assert b.groups == []


# ===========================================================================
# End-to-end
# ===========================================================================

@test
def test_build_haru_v3_and_reparse():
    m = ModelJson(version=3)
    m.model = "Haru.moc3"
    m.textures = [
        "Haru.2048/texture_00.png",
        "Haru.2048/texture_01.png",
    ]
    m.physics = "Haru.physics3.json"
    m.pose = "Haru.pose3.json"
    m.display_info = "Haru.cdi3.json"
    m.user_data = "Haru.userdata3.json"

    for i in range(1, 3):
        m.add_expression(f"F{i:02d}", f"expressions/F{i:02d}.exp3.json")

    m.add_motion("Idle", Motion("motions/haru_g_idle.motion3.json", 0.5, 0.5))
    m.add_motion(
        "TapBody",
        Motion(
            "motions/haru_g_m26.motion3.json",
            0.5,
            0.5,
            sound="sounds/haru_talk_13.wav",
        ),
    )

    m.add_group("Parameter", "EyeBlink", ["ParamEyeLOpen", "ParamEyeROpen"])
    m.add_group("Parameter", "LipSync", ["ParamMouthOpenY"])
    m.add_hit_area("HitArea", "Head")
    m.add_hit_area("HitArea2", "Body")

    parsed = ModelJson.from_string(m.to_string())
    assert parsed.version == 3
    assert parsed.model == "Haru.moc3"
    assert parsed.motions["TapBody"][0].sound == "sounds/haru_talk_13.wav"


@test
def test_model_loading_v3():
    m = ModelJson(version=3)
    m.model = "Haru.moc3"
    m.textures = [
        "Haru.2048/texture_00.png",
        "Haru.2048/texture_01.png",
    ]
    m.physics = "Haru.physics3.json"
    m.pose = "Haru.pose3.json"
    m.display_info = "Haru.cdi3.json"
    m.user_data = "Haru.userdata3.json"

    for i in range(1, 3):
        m.add_expression(f"F{i:02d}", f"expressions/F{i:02d}.exp3.json")

    m.add_motion("Idle", Motion("motions/haru_g_idle.motion3.json", 0.5, 0.5))
    m.add_motion(
        "TapBody",
        Motion(
            "motions/haru_g_m26.motion3.json",
            0.5,
            0.5,
            sound="sounds/haru_talk_13.wav",
        ),
    )

    m.add_group("Parameter", "EyeBlink", ["ParamEyeLOpen", "ParamEyeROpen"])
    m.add_group("Parameter", "LipSync", ["ParamMouthOpenY"])
    m.add_hit_area("HitArea", "Head")
    m.add_hit_area("HitArea2", "Body")

    live2d.init()
    model = live2d.Model()
    model.LoadFromJsonString(m.to_string(), root_path=RESOURCES + "/v3/Haru", create_renderer=False)
    del model
    live2d.dispose()


@test
def test_model_loading_v2():
    m = ModelJson(version=2)
    m.model = "haru.moc"
    m.textures = [
        "haru.2048/texture_00.png",
        "haru.2048/texture_01.png",
    ]
    m.physics = "haru.physics.json"
    m.pose = "haru.pose.json"

    for i in range(1, 9):
        m.add_expression(f"f{i:02d}", f"expressions/f{i:02d}.exp.json")

    m.add_motion("idle", Motion("motions/haru_idle_01.mtn"))
    m.add_motion("idle", Motion("motions/haru_idle_02.mtn"))
    m.add_motion("idle", Motion("motions/haru_idle_03.mtn"))

    m.add_motion("null", Motion("motions/haru_m_01.mtn"))
    m.add_motion("null", Motion("motions/haru_m_02.mtn", fade_in=500))
    m.add_motion("null", Motion("motions/haru_m_03.mtn"))
    m.add_motion("null", Motion("motions/haru_m_04.mtn"))
    m.add_motion("null", Motion("motions/haru_m_05.mtn"))
    m.add_motion("null", Motion("motions/haru_m_06.mtn"))
    m.add_motion("null", Motion("motions/haru_m_07.mtn", fade_in=300, fade_out=500))
    m.add_motion("null", Motion("motions/haru_m_08.mtn"))
    m.add_motion("null", Motion("motions/haru_m_09.mtn"))
    m.add_motion("null", Motion("motions/haru_m_10.mtn"))
    m.add_motion(
        "null",
        Motion("motions/haru_normal_01.mtn", sound="sounds/haru_normal_01.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_02.mtn", sound="sounds/haru_normal_02.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_03.mtn", fade_in=500,
            sound="sounds/haru_normal_03.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_04.mtn", sound="sounds/haru_normal_04.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_05.mtn", sound="sounds/haru_normal_05.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_06.mtn", sound="sounds/haru_normal_06.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_07.mtn", fade_in=700,
            sound="sounds/haru_normal_07.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_08.mtn", fade_in=500,
            sound="sounds/haru_normal_08.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_09.mtn", sound="sounds/haru_normal_09.mp3"),
    )
    m.add_motion(
        "null",
        Motion("motions/haru_normal_10.mtn", sound="sounds/haru_normal_10.mp3"),
    )

    live2d.init()
    model = live2d.Model()
    model.LoadFromJsonString(
        m.to_string(),
        root_path=RESOURCES + "/v2/haru",
        create_renderer=False,
    )
    del model
    live2d.dispose()

# ===========================================================================
# Runner
# ===========================================================================

def main() -> int:
    passed = 0
    failed = []

    for fn in _TESTS:
        try:
            fn()
        except Exception:
            failed.append((fn.__name__, traceback.format_exc()))
        else:
            passed += 1
            print(f"PASS  {fn.__name__}")

    print()
    if failed:
        for name, tb in failed:
            print(f"FAIL  {name}")
            print(tb)
        print(f"{passed} passed, {len(failed)} failed "
              f"out of {len(_TESTS)} tests")
        return 1

    print(f"All {passed} tests passed.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())