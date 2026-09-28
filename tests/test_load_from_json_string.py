"""
LoadFromJsonString: 从内存 model json 字符串加载模型（v2/v3 统一 live2d.Model）

覆盖:
  - v3 / v2 正常加载（create_renderer=False，无需 OpenGL 上下文）
  - json 内绝对路径: 不传 root_path 直接读取（核心新语义）
  - 相对路径 + root_path（无尾部分隔符自动补 '/'）
  - 非 ASCII root_path
  - 非法 json / 空串 / 非模型 json / moc 缺失 → ValueError（不 abort 不崩溃）
  - 重复加载守卫

运行（repo 根目录）:
  python tests/test_load_from_json_string.py
"""
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "package"))

import live2d

RESOURCES = os.path.abspath(
    os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Resources")
)


def read_text(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def load(json_path, *args, **kwargs):
    model = live2d.Model()
    model.LoadFromJsonString(read_text(json_path), *args, **kwargs)
    return model


def make_v3_paths_absolute(data, home):
    """把 v3 model3.json 里所有资源路径改成绝对路径"""
    fr = data.get("FileReferences", {})
    if isinstance(fr.get("Moc"), str):
        fr["Moc"] = os.path.join(home, fr["Moc"])
    if isinstance(fr.get("Textures"), list):
        fr["Textures"] = [os.path.join(home, t) for t in fr["Textures"]]
    for key in ("Physics", "Pose", "UserData", "DisplayInfo"):
        if isinstance(fr.get(key), str):
            fr[key] = os.path.join(home, fr[key])
    for exp in fr.get("Expressions", []):
        if isinstance(exp.get("File"), str):
            exp["File"] = os.path.join(home, exp["File"])
    for group, motions in fr.get("Motions", {}).items():
        for m in motions:
            if isinstance(m.get("File"), str):
                m["File"] = os.path.join(home, m["File"])
    return data


def make_v2_paths_absolute(data, home):
    """把 v2 model.json 里所有资源路径改成绝对路径"""
    if isinstance(data.get("model"), str):
        data["model"] = os.path.join(home, data["model"])
    if isinstance(data.get("textures"), list):
        data["textures"] = [os.path.join(home, t) for t in data["textures"]]
    for key in ("physics", "pose"):
        if isinstance(data.get(key), str):
            data[key] = os.path.join(home, data[key])
    for group, motions in data.get("motions", {}).items():
        for m in motions:
            if isinstance(m.get("file"), str):
                m["file"] = os.path.join(home, m["file"])
    for exp in data.get("expressions", []):
        if isinstance(exp.get("file"), str):
            exp["file"] = os.path.join(home, exp["file"])
    return data


def test_v3_relative():
    home = os.path.join(RESOURCES, "v3", "Haru")
    model = load(os.path.join(home, "Haru.model3.json"), create_renderer=False, root_path=home)
    assert model.Version() == 3 and model.IsV3()
    assert model.GetParamCount() > 0
    assert len(model.GetMotions()) > 0, model.GetMotions()
    assert len(model.GetExpressions()) > 0
    print("[v3] params=%d motions=%d expressions=%d home=%s"
          % (model.GetParamCount(), len(model.GetMotions()),
             len(model.GetExpressions()), model.GetModelHomeDir()))


def test_v2_relative():
    home = os.path.join(RESOURCES, "v2", "kasumi2")
    # 第三个参数按位置传，验证 create_renderer 位置/关键字两种形式
    model = load(os.path.join(home, "kasumi2.model.json"), False, root_path=home)
    assert model.Version() == 2 and model.IsV2()
    assert model.GetParamCount() > 0
    assert len(model.GetMotions()) > 0, model.GetMotions()
    print("[v2] params=%d motions=%d home=%s"
          % (model.GetParamCount(), len(model.GetMotions()), model.GetModelHomeDir()))


def test_v3_absolute_paths():
    """json 内绝对路径: 不传 root_path 直接读取"""
    home = os.path.join(RESOURCES, "v3", "Haru")
    data = make_v3_paths_absolute(
        json.loads(read_text(os.path.join(home, "Haru.model3.json"))), home)
    model = live2d.Model()
    # ensure_ascii=False: CubismJson 解析器不支持 \uXXXX 转义
    model.LoadFromJsonString(json.dumps(data, ensure_ascii=False, indent=4), create_renderer=False)
    assert model.Version() == 3 and model.IsV3()
    assert model.GetParamCount() > 0
    assert len(model.GetMotions()) > 0
    print("[v3-abs] loaded with absolute paths, no root_path")


def test_v2_absolute_paths():
    home = os.path.join(RESOURCES, "v2", "kasumi2")
    data = make_v2_paths_absolute(
        json.loads(read_text(os.path.join(home, "kasumi2.model.json"))), home)
    model = live2d.Model()
    model.LoadFromJsonString(json.dumps(data, ensure_ascii=False), create_renderer=False)
    assert model.Version() == 2 and model.IsV2()
    assert model.GetParamCount() > 0
    assert len(model.GetMotions()) > 0
    print("[v2-abs] loaded with absolute paths, no root_path")


def test_home_dir_trailing_separator():
    home = os.path.join(RESOURCES, "v3", "Haru").replace("\\", "/")
    assert not home.endswith("/")
    model = load(os.path.join(home, "Haru.model3.json"), create_renderer=False, root_path=home)
    assert model.GetParamCount() > 0
    assert model.GetModelHomeDir().endswith("/")
    print("[v3] trailing '/' auto-appended:", model.GetModelHomeDir())


def test_non_ascii_root_path():
    home = os.path.join(RESOURCES, "v2", "托尔")
    model = load(os.path.join(home, "model0.json"), create_renderer=False, root_path=home)
    assert model.Version() == 2
    assert model.GetParamCount() > 0
    print("[v2] non-ascii root ok:", model.GetModelHomeDir())


def expect_value_error(json_text, root_path, label):
    model = live2d.Model()
    try:
        model.LoadFromJsonString(json_text, root_path=root_path)
    except ValueError as e:
        print("[error] %s -> ValueError: %s" % (label, e))
        return
    raise AssertionError("expected ValueError for %s" % label)


def test_errors():
    expect_value_error("{ not json", os.path.join(RESOURCES, "v3", "Haru"), "malformed json")
    expect_value_error("", RESOURCES, "empty json")
    expect_value_error('{"hello": "world"}', RESOURCES, "non-model json")
    expect_value_error(
        read_text(os.path.join(RESOURCES, "v3", "Haru", "Haru.model3.json")),
        os.path.join(RESOURCES, "v2", "kasumi2"),
        "moc missing under root_path",
    )


def test_double_load_guard():
    home = os.path.join(RESOURCES, "v3", "Haru")
    model = load(os.path.join(home, "Haru.model3.json"), create_renderer=False, root_path=home)
    before = model.GetParamCount()
    model.LoadFromJsonString(read_text(os.path.join(home, "Haru.model3.json")), home,
                             create_renderer=False)
    assert model.GetParamCount() == before
    print("[guard] second LoadFromJsonString ignored")


def main():
    live2d.init()
    test_v3_relative()
    test_v2_relative()
    test_v3_absolute_paths()
    test_v2_absolute_paths()
    test_home_dir_trailing_separator()
    test_non_ascii_root_path()
    test_errors()
    test_double_load_guard()
    print("all LoadFromJsonString tests passed")


if __name__ == "__main__":
    main()
