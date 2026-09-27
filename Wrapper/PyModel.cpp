#include "PyModel.hpp"

#include <V3/Model.hpp>
#include <V2/Model.hpp>
#include <Log.hpp>
#include <cstdlib>
#include <cstring>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include <object.h>
#include <string>

using namespace nlohmann;
using namespace Live2D;
using namespace Live2D::Common::Log;

// ---- Check moc file version
static int CheckVersion(const char* jsonPath) {
    std::filesystem::path jp = std::filesystem::u8path(jsonPath);
    std::ifstream jsonFile(jp);
    auto data = json::parse(jsonFile);
    jsonFile.close();


    if (auto modelIt = data["model"]; modelIt.is_string()) {
        auto mocPath = modelIt.get<std::string>();
        auto mp = std::filesystem::u8path(mocPath);
        auto fp = jp.parent_path() / mp;
        std::ifstream mocFile(fp, std::ios::binary);
        if (mocFile) {
            char moc[3];
            mocFile.read(moc, 3);
            if (memcmp(moc, "moc", 3) == 0) {
                return 2;
            } else {
                LOGE("%s is not a valid moc", mocPath.c_str());
            }
        } else {
            LOGE("cannot open %s", mp.c_str());
        }
    } else {
        if (auto frIt = data["FileReferences"]; frIt.is_object()) {
            if (auto mocIt = frIt["Moc"]; mocIt.is_string()) {
                auto mocPath = mocIt.get<std::string>();
                auto mp = std::filesystem::u8path(mocPath);
                auto fp = jp.parent_path() / mp;
                std::ifstream mocFile(fp, std::ios::binary);
                if (mocFile) {
                    char moc[4];
                    mocFile.read(moc, 4);
                    if (memcmp(moc, "MOC3", 4) == 0) {
                        return 3;
                    } else {
                        LOGE("%s is not a valid moc3", mocPath.c_str());
                    }
                } else {
                    LOGE("cannot open %s", mp.c_str());
                }
            }
        }
        LOGE("%s is not a valid json", jsonPath);
    }
    std::abort();
    return -1;
}

// ---- Callback helpers (Python → C++ conversion) ----
static auto MakeMotionCallback(PyObject* cb) -> std::function<void(const std::string&, int)> {
    if (!cb || Py_IsNone(cb) || !PyCallable_Check(cb))
        return nullptr;
    Py_INCREF(cb);
    return [cb](const std::string& g, int n) {
        PyGILState_STATE s = PyGILState_Ensure();
        PyObject* r = PyObject_CallFunction(cb, "si", g.c_str(), n);
        if (r)
            Py_DECREF(r);
        else
            PyErr_Print();
        Py_XDECREF(cb);
        PyGILState_Release(s);
    };
}

static PyObject* PyModel_Init(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model = nullptr;
    return 0;
}
static void PyModel_Dealloc(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    LOGI("deallocate: cpp Model(at=%p)", self->model);
    delete self->model;
    LOGI("deallocate: PyModelObject(at=%p)", self);
    PyObject_Free(self);
}
static PyObject* PyModel_LoadModelJson(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    if (self->model != nullptr) {
        LOGE("model already loaded: %s", self->model->GetModelHomeDir());
        Py_RETURN_NONE;
    }

    const char* modelJsonPath;
    bool createRenderer = true;
    static const char* kwlist[] = {"path", "create_renderer", nullptr};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "s|b", const_cast<char**>(kwlist), &modelJsonPath, &createRenderer)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
        return NULL;
    }
    int version = CheckVersion(modelJsonPath);
    if (version == 2) {
        self->model = new V2::Model();
    } else if (version == 3) {
        self->model = new V3::Model();
    }
    LOGI("allocate: cpp Model(at=%p, version=%d)", self->model, version);

    self->model->LoadModelJson(modelJsonPath, createRenderer);
    Py_RETURN_NONE;
}
static PyObject* PyModel_GetModelHomeDir(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    return Py_BuildValue("s", self->model->GetModelHomeDir());
}

static PyObject* PyModel_Version(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    return PyLong_FromLong(self->model->Version());
}

static PyObject* PyModel_IsV2(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    return PyBool_FromLong(self->model->IsV2() ? 1 : 0);
}

static PyObject* PyModel_IsV3(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    return PyBool_FromLong(self->model->IsV3() ? 1 : 0);
}

static PyObject* PyModel_Update(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    // 可选 dt: 不传 = 内部自计时（哨兵 -1），传入 = delta 驱动
    float deltaTimeSeconds = -1.0f;
    if (!PyArg_ParseTuple(args, "|f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->Update(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_UpdateMotion(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float deltaTimeSeconds;
    if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->UpdateMotion(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_UpdateDrag(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float deltaTimeSeconds;
    if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->UpdateDrag(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_UpdateBreath(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float deltaTimeSeconds;
    if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->UpdateBreath(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_UpdateBlink(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float deltaTimeSeconds;
    if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->UpdateBlink(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_UpdateExpression(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float deltaTimeSeconds;
    if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->UpdateExpression(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_UpdatePhysics(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float deltaTimeSeconds;
    if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->UpdatePhysics(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_UpdatePose(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float deltaTimeSeconds;
    if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
        return NULL;
    }
    self->model->UpdatePose(deltaTimeSeconds);
    Py_RETURN_NONE;
}
static PyObject* PyModel_GetParamIds(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const int count = self->model->GetParameterCount();
    PyObject* list = PyList_New(count);
    int index = 0;
    void* collector[2] = {list, &index};
    self->model->GetParameterIds(collector, [](void* collector, const char* id) {
        PyObject* list = (PyObject*)(((void**)collector)[0]);
        int* index = (int*)(((void**)collector)[1]);
        PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
    });
    return list;
}
static PyObject* PyModel_GetParamCount(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    return Py_BuildValue("i", self->model->GetParameterCount());
}
static PyObject* PyModel_GetParamValueByIndex(PyModelObject* self, PyObject* args,
                                              PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterValue(index));
}
static PyObject* PyModel_GetParamMaxByIndex(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterMaximumValue(index));
}
static PyObject* PyModel_GetParamMinByIndex(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterMinimumValue(index));
}
static PyObject* PyModel_GetParamDefaultByIndex(PyModelObject* self, PyObject* args,
                                                PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterDefaultValue(index));
}
static PyObject* PyModel_SetParamByIndex(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    float value;
    float weight = 1.0f;
    if (!PyArg_ParseTuple(args, "if|f", &index, &value, &weight)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, [float])");
        return NULL;
    }
    self->model->SetParameterValue(index, value, weight);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetParamById(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    float value, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &value, &weight)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, float, [float])");
        return NULL;
    }
    self->model->SetParameterValue(id, value, weight);
    Py_RETURN_NONE;
}
static PyObject* PyModel_AddParamByIndex(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    float value;
    if (!PyArg_ParseTuple(args, "if", &index, &value)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float)");
        return NULL;
    }

    self->model->AddParameterValue(index, value);
    Py_RETURN_NONE;
}
static PyObject* PyModel_AddParamById(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    float value;
    if (!PyArg_ParseTuple(args, "sf", &id, &value)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, float)");
        return NULL;
    }
    self->model->AddParameterValue(id, value);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetSaveParamByIndex(PyModelObject* self, PyObject* args,
                                             PyObject* kwargs) {
    int index;
    float value;
    float weight = 1.0f;
    if (!PyArg_ParseTuple(args, "if|f", &index, &value, &weight)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, [float])");
        return NULL;
    }
    self->model->SetAndSaveParameterValue(index, value, weight);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetSaveParamById(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    float value, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &value, &weight)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, float, [float])");
        return NULL;
    }
    self->model->SetAndSaveParameterValue(id, value, weight);
    Py_RETURN_NONE;
}
static PyObject* PyModel_AddSaveParamByIndex(PyModelObject* self, PyObject* args,
                                             PyObject* kwargs) {
    int index;
    float value;
    if (!PyArg_ParseTuple(args, "if", &index, &value)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float)");
        return NULL;
    }

    self->model->AddAndSaveParameterValue(index, value);
    Py_RETURN_NONE;
}
static PyObject* PyModel_AddSaveParamById(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    float value;
    if (!PyArg_ParseTuple(args, "sf", &id, &value)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, float)");
        return NULL;
    }
    self->model->AddAndSaveParameterValue(id, value);
    Py_RETURN_NONE;
}


static int ParamIndexById(PyModelObject* self, const char* id) {
    const int count = self->model->GetParameterCount();
    for (int i = 0; i < count; i++) {
        if (strcmp(self->model->GetParameterId(i), id) == 0)
            return i;
    }
    return -1;
}

static PyObject* PyModel_GetParamValueById(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
        return NULL;
    }
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterValue(index));
}

static PyObject* PyModel_GetParamMaxById(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
        return NULL;
    }
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterMaximumValue(index));
}

static PyObject* PyModel_GetParamMinById(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
        return NULL;
    }
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterMinimumValue(index));
}

static PyObject* PyModel_GetParamDefaultById(PyModelObject* self, PyObject* args,
                                             PyObject* kwargs) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
        return NULL;
    }
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return NULL;
    }
    return PyFloat_FromDouble(self->model->GetParameterDefaultValue(index));
}
static PyObject* PyModel_LoadParameters(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->LoadParameters();
    Py_RETURN_NONE;
}
static PyObject* PyModel_SaveParameters(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->SaveParameters();
    Py_RETURN_NONE;
}
static PyObject* PyModel_Resize(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int width, height;
    if (!PyArg_ParseTuple(args, "ii", &width, &height)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, int)");
        return NULL;
    }
    self->model->Resize(width, height);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetOffset(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float x, y;
    if (!PyArg_ParseTuple(args, "ff", &x, &y)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (float, float)");
        return NULL;
    }
    self->model->SetOffset(x, y);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetOffsetX(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float x;
    if (!PyArg_ParseTuple(args, "f", &x)) {
        PyErr_SetString(PyExc_TypeError, "argument must be float");
        return NULL;
    }
    self->model->SetOffsetX(x);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetOffsetY(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float y;
    if (!PyArg_ParseTuple(args, "f", &y)) {
        PyErr_SetString(PyExc_TypeError, "argument must be float");
        return NULL;
    }
    self->model->SetOffsetY(y);
    Py_RETURN_NONE;
}
static PyObject* PyModel_Rotate(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float degrees;
    if (!PyArg_ParseTuple(args, "f", &degrees)) {
        PyErr_SetString(PyExc_TypeError, "argument must be float");
        return NULL;
    }
    self->model->Rotate(degrees);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetScale(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float scale;
    if (!PyArg_ParseTuple(args, "f", &scale)) {
        PyErr_SetString(PyExc_TypeError, "argument must be float");
        return NULL;
    }
    self->model->SetScale(scale);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetScaleX(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float scale;
    if (!PyArg_ParseTuple(args, "f", &scale)) {
        PyErr_SetString(PyExc_TypeError, "argument must be float");
        return NULL;
    }
    self->model->SetScaleX(scale);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetScaleY(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float scale;
    if (!PyArg_ParseTuple(args, "f", &scale)) {
        PyErr_SetString(PyExc_TypeError, "argument must be float");
        return NULL;
    }
    self->model->SetScaleY(scale);
    Py_RETURN_NONE;
}
static PyObject* PyModel_GetMvp(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* mvp = PyTuple_New(16);
    const float* mvpArray = self->model->GetMvp();
    for (int i = 0; i < 16; i++) {
        PyTuple_SetItem(mvp, i, PyFloat_FromDouble(mvpArray[i]));
    }
    return mvp;
}
static PyObject* PyModel_StartMotion(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* group;
    int no, priority = 3;
    PyObject* onStartHandler = nullptr;
    PyObject* onFinishHandler = nullptr;
    static const char* kwlist[] = {"group", "no", "priority", "onStart", "onFinish", NULL};
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "si|iOO",
                                     const_cast<char**>(kwlist),
                                     &group,
                                     &no,
                                     &priority,
                                     &onStartHandler,
                                     &onFinishHandler)) {
        PyErr_SetString(PyExc_TypeError,
                        "arguments must be (str, int, [int, [callable, callable]])");
        return NULL;
    }
    self->model->StartMotion(group,
                             no,
                             priority,
                             MakeMotionCallback(onStartHandler),
                             MakeMotionCallback(onFinishHandler));
    Py_RETURN_NONE;
}
static PyObject* PyModel_StartRandomMotion(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* group = nullptr;
    int priority = 3;

    PyObject* onStartHandler = nullptr;
    PyObject* onFinishHandler = nullptr;
    static const char* kwlist[] = {"group", "priority", "onStart", "onFinish", NULL};

    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|ziOO",
                                     const_cast<char**>(kwlist),
                                     &group,
                                     &priority,
                                     &onStartHandler,
                                     &onFinishHandler)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be ([str, [int, [callable, callable]]])");
        return NULL;
    }

    self->model->StartRandomMotion(group ? group : "",
                                   priority,
                                   MakeMotionCallback(onStartHandler),
                                   MakeMotionCallback(onFinishHandler));
    Py_RETURN_NONE;
}
static PyObject* PyModel_IsMotionFinished(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    if (self->model->IsMotionFinished()) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* PyModel_LoadExtraMotion(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char *group, *motionJsonPath;
    if (!PyArg_ParseTuple(args, "ss", &group, &motionJsonPath)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, str)");
        return NULL;
    }

    const int no = self->model->LoadExtraMotion(group, motionJsonPath);
    return Py_BuildValue("i", no);
}
static PyObject* PyModel_GetMotions(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* motions = PyDict_New();
    self->model->GetMotions(
        motions,
        [](void* collector, const char* group, int no, const char* filePath, const char* sound) {
            PyObject* motions = (PyObject*)collector;
            PyObject* motion = PyDict_New();
            PyDict_SetItem(motion, PyUnicode_FromString("File"), PyUnicode_FromString(filePath));
            PyDict_SetItem(motion, PyUnicode_FromString("Sound"), PyUnicode_FromString(sound));
            PyObject* list = PyDict_GetItem(motions, PyUnicode_FromString(group));
            if (list == NULL) {
                list = PyList_New(1);
                PyList_SetItem(list, 0, motion);
                PyDict_SetItem(motions, PyUnicode_FromString(group), list);
            } else {
                PyList_Append(list, motion);
            }
        });
    return motions;
}
static PyObject* PyModel_HitPart(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float x, y;
    bool topOnly = false;
    if (!PyArg_ParseTuple(args, "ff|b", &x, &y, &topOnly)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (float, float, [bool])");
        return NULL;
    }
    PyObject* list = PyList_New(0);
    self->model->HitPart(
        x,
        y,
        list,
        [](void* collector, const char* id) {
            PyList_Append((PyObject*)collector, PyUnicode_FromString(id));
        },
        topOnly);
    return list;
}
static PyObject* PyModel_HitDrawable(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float x, y;
    bool topOnly = false;
    if (!PyArg_ParseTuple(args, "ff|b", &x, &y, &topOnly)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (float, float, [bool])");
        return NULL;
    }
    PyObject* list = PyList_New(0);
    self->model->HitDrawable(
        x,
        y,
        list,
        [](void* collector, const char* id) {
            PyList_Append((PyObject*)collector, PyUnicode_FromString(id));
        },
        topOnly);
    return list;
}
static PyObject* PyModel_Drag(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float x, y;
    if (!PyArg_ParseTuple(args, "ff", &x, &y)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (float, float)");
        return NULL;
    }

    self->model->Drag(x, y);
    Py_RETURN_NONE;
}
static PyObject* PyModel_IsAreaHit(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* areaName;
    float x, y;
    if (!PyArg_ParseTuple(args, "sff", &areaName, &x, &y)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, float, float)");
        return NULL;
    }

    if (self->model->IsAreaHit(areaName, x, y)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* PyModel_IsPartHit(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    float x, y;
    if (!PyArg_ParseTuple(args, "iff", &index, &x, &y)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float)");
        return NULL;
    }
    if (self->model->IsPartHit(index, x, y)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* PyModel_IsDrawableHit(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    float x, y;
    if (!PyArg_ParseTuple(args, "iff", &index, &x, &y)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float)");
        return NULL;
    }

    if (self->model->IsDrawableHit(index, x, y)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}
static PyObject* PyModel_CreateRenderer(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int maskBufferCount = 2;
    if (!PyArg_ParseTuple(args, "|i", &maskBufferCount)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int)");
        return NULL;
    }
    self->model->CreateRenderer(maskBufferCount);
    Py_RETURN_NONE;
}
static PyObject* PyModel_DestroyRenderer(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->DestroyRenderer();
    Py_RETURN_NONE;
}
static PyObject* PyModel_Draw(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->Draw();
    Py_RETURN_NONE;
}
static PyObject* PyModel_GetPartCount(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    return PyLong_FromLong(self->model->GetPartCount());
}
static PyObject* PyModel_GetPartId(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    return PyUnicode_FromString(self->model->GetPartId(index));
}
static PyObject* PyModel_GetPartIds(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const int count = self->model->GetPartCount();
    PyObject* list = PyList_New(count);
    int index = 0;
    void* collector[2] = {list, &index};
    self->model->GetPartIds(collector, [](void* collector, const char* id) {
        PyObject* list = (PyObject*)(((void**)collector)[0]);
        int* index = (int*)(((void**)collector)[1]);

        PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
    });
    return list;
}
static PyObject* PyModel_SetPartOpacity(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    float value;
    if (!PyArg_ParseTuple(args, "if", &index, &value)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float)");
        return NULL;
    }
    self->model->SetPartOpacity(index, value);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetPartScreenColor(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
        return NULL;
    }
    self->model->SetPartScreenColor(index, r, g, b, a);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetPartMultiplyColor(PyModelObject* self, PyObject* args,
                                              PyObject* kwargs) {
    int index;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
        return NULL;
    }
    self->model->SetPartMultiplyColor(index, r, g, b, a);
    Py_RETURN_NONE;
}
static PyObject* PyModel_GetDrawableIds(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const int count = self->model->GetDrawableCount();
    PyObject* list = PyList_New(count);
    int index = 0;
    void* collector[2] = {list, &index};
    self->model->GetDrawableIds(collector, [](void* collector, const char* id) {
        PyObject* list = (PyObject*)(((void**)collector)[0]);
        int* index = (int*)(((void**)collector)[1]);

        PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
    });
    return list;
}

static PyObject* PyModel_GetDrawableVertices(PyModelObject* self, PyObject* args,
                                             PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    const float* verts = self->model->GetDrawableVertices(index);
    const int count = self->model->GetDrawableVertexCount(index) * 2;
    PyObject* list = PyList_New(count);
    for (int i = 0; i < count; i++)
        PyList_SetItem(list, i, PyFloat_FromDouble(verts[i]));
    return list;
}
static PyObject* PyModel_GetDrawableVertexCount(PyModelObject* self, PyObject* args,
                                                PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    return PyLong_FromLong(self->model->GetDrawableVertexCount(index));
}
static PyObject* PyModel_GetDrawableVertexIndexCount(PyModelObject* self, PyObject* args,
                                                     PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    return PyLong_FromLong(self->model->GetDrawableVertexIndexCount(index));
}
static PyObject* PyModel_GetDrawableIndices(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
        return NULL;
    }
    const unsigned short* indices = self->model->GetDrawableIndices(index);
    const int count = self->model->GetDrawableVertexIndexCount(index);
    PyObject* list = PyList_New(count);
    for (int i = 0; i < count; i++)
        PyList_SetItem(list, i, PyLong_FromLong(indices[i]));
    return list;
}
static PyObject* PyModel_GetMotionCount(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* group;
    if (!PyArg_ParseTuple(args, "s", &group)) {
        PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
        return NULL;
    }
    return PyLong_FromLong(self->model->GetMotionCount(group));
}
static PyObject* PyModel_GetMotionGroupCount(PyModelObject* self, PyObject* args,
                                             PyObject* kwargs) {
    return PyLong_FromLong(self->model->GetMotionGroupCount());
}
static PyObject* PyModel_SetDrawableMultiplyColor(PyModelObject* self, PyObject* args,
                                                  PyObject* kwargs) {
    int index;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
        return NULL;
    }
    self->model->SetDrawableMultiColor(index, r, g, b, a);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetDrawableScreenColor(PyModelObject* self, PyObject* args,
                                                PyObject* kwargs) {
    int index;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
        return NULL;
    }
    self->model->SetDrawableScreenColor(index, r, g, b, a);
    Py_RETURN_NONE;
}
static PyObject* PyModel_AddExpression(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* expressionId;
    if (!PyArg_ParseTuple(args, "s", &expressionId)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str)");
        return NULL;
    }
    self->model->AddExpression(expressionId);
    Py_RETURN_NONE;
}
static PyObject* PyModel_RemoveExpression(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* expressionId;
    if (!PyArg_ParseTuple(args, "s", &expressionId)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str)");
        return NULL;
    }
    self->model->RemoveExpression(expressionId);
    Py_RETURN_NONE;
}
static PyObject* PyModel_SetExpression(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* expressionId;
    float fadeoutMs = -1.0f;
    static const char* kwlist[] = {"expressionId", "fadeoutMs", nullptr};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "s|f", const_cast<char**>(kwlist), &expressionId, &fadeoutMs)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, [float])");
        return NULL;
    }
    self->model->SetExpression(expressionId, fadeoutMs);
    Py_RETURN_NONE;
}

static PyObject* PyModel_SetRandomExpression(PyModelObject* self, PyObject* args,
                                             PyObject* kwargs) {
    float fadeoutMs = -1.0f;
    static const char* kwlist[] = {"fadeoutMs", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|f", const_cast<char**>(kwlist), &fadeoutMs)) {
        PyErr_SetString(PyExc_TypeError, "argument must be (float)");
        return NULL;
    }
    const char* expId = self->model->SetRandomExpression(fadeoutMs);
    if (expId != nullptr) {
        return Py_BuildValue("s", expId);
    } else {
        Py_RETURN_NONE;
    }
}

static PyObject* PyModel_ResetExpression(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->ResetExpression();
    Py_RETURN_NONE;
}
static PyObject* PyModel_ResetExpressions(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->ResetExpressions();
    Py_RETURN_NONE;
}
static PyObject* PyModel_LoadExtraExpression(PyModelObject* self, PyObject* args,
                                             PyObject* kwargs) {
    const char* expressionId;
    const char* filePath;
    if (!PyArg_ParseTuple(args, "ss", &expressionId, &filePath)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, str)");
        return NULL;
    }
    self->model->LoadExtraExpression(expressionId, filePath);
    Py_RETURN_NONE;
}
static PyObject* PyModel_GetExpressions(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const int count = self->model->GetExpressionCount();
    PyObject* list = PyList_New(count);
    int index = 0;
    void* collector[2] = {list, &index};
    self->model->GetExpressions(collector, [](void* collector, const char* id, const char* file) {
        PyObject* list = (PyObject*)(((void**)collector)[0]);
        int* index = (int*)(((void**)collector)[1]);

        PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
    });
    return list;
}
static PyObject* PyModel_GetMotionSound(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* group;
    int no;
    if (!PyArg_ParseTuple(args, "si", &group, &no)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (str, int)");
        return NULL;
    }
    return PyUnicode_FromString(self->model->GetMotionSound(group, no));
}
static PyObject* PyModel_GetMotionGroups(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* groups = PyDict_New();
    self->model->GetMotions(
        groups,
        [](void* collector, const char* group, int no, const char* file, const char* sound) {
            PyObject* dict = (PyObject*)collector;
            PyObject* key = PyUnicode_FromString(group);
            PyObject* count = PyDict_GetItem(dict, key);   // borrowed
            if (count == NULL) {
                count = PyLong_FromLong(1);
                PyDict_SetItem(dict, key, count);
                Py_DECREF(count);
            } else {
                PyDict_SetItem(dict, key, PyLong_FromLong(PyLong_AsLong(count) + 1));
            }
            Py_DECREF(key);
        });
    return groups;
}
static PyObject* PyModel_StopAllMotions(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->StopAllMotions();
    Py_RETURN_NONE;
}
static PyObject* PyModel_ResetAllParameters(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->ResetAllParameters();
    Py_RETURN_NONE;
}
static PyObject* PyModel_ResetPose(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    self->model->ResetPose();
    Py_RETURN_NONE;
}

static PyObject* PyModel_GetCanvasSize(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float w, h;
    self->model->GetCanvasSize(w, h);
    return Py_BuildValue("ff", w, h);
}

static PyObject* PyModel_GetCanvasSizePixel(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    float w, h;
    self->model->GetCanvasSizePixel(w, h);
    return Py_BuildValue("ff", w, h);
}

static PyObject* PyModel_GetPixelsPerUnit(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    return Py_BuildValue("f", self->model->GetPixelsPerUnit());
}

static PyObject* PyModel_SetAutoBreath(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    bool on;
    if (!PyArg_ParseTuple(args, "b", &on)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (bool)");
        return NULL;
    }

    self->model->SetAutoBreath(on);

    Py_RETURN_NONE;
}

static PyObject* PyModel_SetAutoBlink(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    bool on;
    if (!PyArg_ParseTuple(args, "b", &on)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (bool)");
        return NULL;
    }

    self->model->SetAutoBlink(on);

    Py_RETURN_NONE;
}

static PyObject* PyModel_GetPartMultiplyColor(PyModelObject* self, PyObject* args,
                                              PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int)");
        return NULL;
    }
    float r, g, b, a;
    self->model->GetPartMultiplyColor(index, r, g, b, a);
    return Py_BuildValue("ffff", r, g, b, a);
}
static PyObject* PyModel_GetPartScreenColor(PyModelObject* self, PyObject* args, PyObject* kwargs) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be (int)");
        return NULL;
    }
    float r, g, b, a;
    self->model->GetPartScreenColor(index, r, g, b, a);
    return Py_BuildValue("ffff", r, g, b, a);
}

static PyObject* PyModel_HasMocConsistencyFromFile(PyModelObject* self, PyObject* args) {
    const char* mocFileName;
    if (!PyArg_ParseTuple(args, "s", &mocFileName)) {
        PyErr_SetString(PyExc_TypeError, "argument must be (str)");
        return NULL;
    }
    if (self->model->HasMocConsistencyFromFile(mocFileName))
        Py_RETURN_TRUE;
    Py_RETURN_FALSE;
}

static PyMethodDef PyModel_Methods[] = {
    {"LoadModelJson", (PyCFunction)PyModel_LoadModelJson, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetModelHomeDir",
     (PyCFunction)PyModel_GetModelHomeDir,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"Version", (PyCFunction)PyModel_Version, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"IsV2", (PyCFunction)PyModel_IsV2, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"IsV3", (PyCFunction)PyModel_IsV3, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Update", (PyCFunction)PyModel_Update, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"UpdateMotion", (PyCFunction)PyModel_UpdateMotion, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"UpdateDrag", (PyCFunction)PyModel_UpdateDrag, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"UpdateBreath", (PyCFunction)PyModel_UpdateBreath, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"UpdateBlink", (PyCFunction)PyModel_UpdateBlink, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"UpdateExpression",
     (PyCFunction)PyModel_UpdateExpression,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"UpdatePhysics", (PyCFunction)PyModel_UpdatePhysics, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"UpdatePose", (PyCFunction)PyModel_UpdatePose, METH_VARARGS | METH_KEYWORDS, nullptr},

    {"GetParamCount", (PyCFunction)PyModel_GetParamCount, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetParamIds", (PyCFunction)PyModel_GetParamIds, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetParamValueByIndex",
     (PyCFunction)PyModel_GetParamValueByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetParamMaxByIndex",
     (PyCFunction)PyModel_GetParamMaxByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetParamMinByIndex",
     (PyCFunction)PyModel_GetParamMinByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetParamDefaultByIndex",
     (PyCFunction)PyModel_GetParamDefaultByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetParamValueById",
     (PyCFunction)PyModel_GetParamValueById,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetParamMaxById",
     (PyCFunction)PyModel_GetParamMaxById,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetParamMinById",
     (PyCFunction)PyModel_GetParamMinById,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetParamDefaultById",
     (PyCFunction)PyModel_GetParamDefaultById,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},

    {"SetParamByIndex",
     (PyCFunction)PyModel_SetParamByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"SetParamById", (PyCFunction)PyModel_SetParamById, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"AddParamByIndex",
     (PyCFunction)PyModel_AddParamByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"AddParamById", (PyCFunction)PyModel_AddParamById, METH_VARARGS | METH_KEYWORDS, nullptr},

    {"SetSaveParamByIndex",
     (PyCFunction)PyModel_SetSaveParamByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"SetSaveParamById",
     (PyCFunction)PyModel_SetSaveParamById,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"AddSaveParamByIndex",
     (PyCFunction)PyModel_AddSaveParamByIndex,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"AddSaveParamById",
     (PyCFunction)PyModel_AddSaveParamById,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},

    {"LoadParameters", (PyCFunction)PyModel_LoadParameters, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SaveParameters", (PyCFunction)PyModel_SaveParameters, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Resize", (PyCFunction)PyModel_Resize, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetOffset", (PyCFunction)PyModel_SetOffset, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetOffsetX", (PyCFunction)PyModel_SetOffsetX, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetOffsetY", (PyCFunction)PyModel_SetOffsetY, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Rotate", (PyCFunction)PyModel_Rotate, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetScale", (PyCFunction)PyModel_SetScale, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetScaleX", (PyCFunction)PyModel_SetScaleX, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetScaleY", (PyCFunction)PyModel_SetScaleY, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetMvp", (PyCFunction)PyModel_GetMvp, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"StartMotion", (PyCFunction)PyModel_StartMotion, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"StartRandomMotion",
     (PyCFunction)PyModel_StartRandomMotion,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"IsMotionFinished",
     (PyCFunction)PyModel_IsMotionFinished,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"LoadExtraMotion",
     (PyCFunction)PyModel_LoadExtraMotion,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetMotions", (PyCFunction)PyModel_GetMotions, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetMotionSound", (PyCFunction)PyModel_GetMotionSound, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetMotionCount", (PyCFunction)PyModel_GetMotionCount, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetMotionGroupCount",
     (PyCFunction)PyModel_GetMotionGroupCount,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetMotionGroups",
     (PyCFunction)PyModel_GetMotionGroups,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"StopAllMotions", (PyCFunction)PyModel_StopAllMotions, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"ResetAllParameters",
     (PyCFunction)PyModel_ResetAllParameters,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"ResetPose", (PyCFunction)PyModel_ResetPose, METH_VARARGS | METH_KEYWORDS, nullptr},

    {"HitPart", (PyCFunction)PyModel_HitPart, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"HitDrawable", (PyCFunction)PyModel_HitDrawable, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"Drag", (PyCFunction)PyModel_Drag, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"IsAreaHit", (PyCFunction)PyModel_IsAreaHit, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"IsPartHit", (PyCFunction)PyModel_IsPartHit, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"IsDrawableHit", (PyCFunction)PyModel_IsDrawableHit, METH_VARARGS | METH_KEYWORDS, nullptr},

    {"CreateRenderer", (PyCFunction)PyModel_CreateRenderer, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"DestroyRenderer",
     (PyCFunction)PyModel_DestroyRenderer,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},

    {"Draw", (PyCFunction)PyModel_Draw, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetPartCount", (PyCFunction)PyModel_GetPartCount, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetPartId", (PyCFunction)PyModel_GetPartId, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetPartIds", (PyCFunction)PyModel_GetPartIds, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetPartOpacity", (PyCFunction)PyModel_SetPartOpacity, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetPartScreenColor",
     (PyCFunction)PyModel_SetPartScreenColor,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"SetPartMultiplyColor",
     (PyCFunction)PyModel_SetPartMultiplyColor,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetPartScreenColor",
     (PyCFunction)PyModel_GetPartScreenColor,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetPartMultiplyColor",
     (PyCFunction)PyModel_GetPartMultiplyColor,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetDrawableIds", (PyCFunction)PyModel_GetDrawableIds, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"GetDrawableVertices",
     (PyCFunction)PyModel_GetDrawableVertices,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetDrawableVertexCount",
     (PyCFunction)PyModel_GetDrawableVertexCount,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetDrawableVertexIndexCount",
     (PyCFunction)PyModel_GetDrawableVertexIndexCount,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"GetDrawableIndices",
     (PyCFunction)PyModel_GetDrawableIndices,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"SetDrawableMultiplyColor",
     (PyCFunction)PyModel_SetDrawableMultiplyColor,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"SetDrawableScreenColor",
     (PyCFunction)PyModel_SetDrawableScreenColor,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},

    {"GetExpressions", (PyCFunction)PyModel_GetExpressions, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"AddExpression", (PyCFunction)PyModel_AddExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"RemoveExpression",
     (PyCFunction)PyModel_RemoveExpression,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"SetExpression", (PyCFunction)PyModel_SetExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
    {"SetRandomExpression",
     (PyCFunction)PyModel_SetRandomExpression,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"ResetExpression",
     (PyCFunction)PyModel_ResetExpression,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"ResetExpressions",
     (PyCFunction)PyModel_ResetExpressions,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},
    {"LoadExtraExpression",
     (PyCFunction)PyModel_LoadExtraExpression,
     METH_VARARGS | METH_KEYWORDS,
     nullptr},

    {"GetCanvasSize", (PyCFunction)PyModel_GetCanvasSize, METH_VARARGS, ""},
    {"GetCanvasSizePixel", (PyCFunction)PyModel_GetCanvasSizePixel, METH_VARARGS, ""},
    {"GetPixelsPerUnit", (PyCFunction)PyModel_GetPixelsPerUnit, METH_VARARGS, ""},

    {"SetAutoBreath", (PyCFunction)PyModel_SetAutoBreath, METH_VARARGS, ""},
    {"SetAutoBlink", (PyCFunction)PyModel_SetAutoBlink, METH_VARARGS, ""},

    {"HasMocConsistencyFromFile", (PyCFunction)PyModel_HasMocConsistencyFromFile, METH_VARARGS, ""},

    {NULL}};
static PyObject* PyModel_New(PyTypeObject* type, PyObject* args, PyObject* kwargs) {
    PyObject* self = (PyObject*)PyObject_Malloc(sizeof(PyModelObject));
    PyObject_Init(self, type);
    return self;
}

static PyType_Slot PyModel_slots[] = {
    {Py_tp_new, (void*)PyModel_New},
    {Py_tp_init, (void*)PyModel_Init},
    {Py_tp_dealloc, (void*)PyModel_Dealloc},
    {Py_tp_methods, (void*)PyModel_Methods},
    {0, 0},
};

PyType_Spec PyModel_Spec = {
    "live2d.Model",
    sizeof(PyModelObject),
    0,
    Py_TPFLAGS_DEFAULT,
    PyModel_slots,
};
