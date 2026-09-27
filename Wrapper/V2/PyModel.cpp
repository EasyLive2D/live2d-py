#include "PyModel.hpp"
#include <V2/Model.hpp>
#include "Log.hpp"
#include "Python.hpp"
#include <modsupport.h>
#include <object.h>
#include <pytypedefs.h>

using namespace Live2D::Common::Log;

// ---- Callback helpers (Python → C++ conversion, no live2d dependency) ----
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

PyObject* PyLAppModel_new(PyTypeObject* type, PyObject*, PyObject*) {
    auto* self = (PyLAppModelObject*)PyObject_Malloc(sizeof(PyLAppModelObject));
    if (!self)
        return nullptr;
    PyObject_Init((PyObject*)self, type);
    return (PyObject*)self;
}

int PyLAppModel_init(PyLAppModelObject* self, PyObject*, PyObject*) {
    self->model = new Live2D::V2::Model();
    return 0;
}

void PyLAppModel_dealloc(PyLAppModelObject* self) {
    LOGI("deallocate: cpp LAppModel(at=%p)", self->model);
    delete self->model;
    LOGI("deallocate: PyLAppModelObject(at=%p)", self);
    self->model = nullptr;
    PyObject_Free(self);
}

static PyObject* PyLAppModel_LoadModelJson(PyLAppModelObject* self, PyObject* args,
                                           PyObject* kwargs) {
    const char* path;
    static const char* kwlist[] = {"path", "create_renderer", nullptr};
    bool createRenderer = true;
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "s|b", const_cast<char**>(kwlist), &path, &createRenderer))
        return nullptr;

    self->model->LoadModelJson(path, createRenderer);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Resize(PyLAppModelObject* self, PyObject* args) {
    int w, h;
    if (!PyArg_ParseTuple(args, "ii", &w, &h))
        return nullptr;
    self->model->Resize(w, h);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Drag(PyLAppModelObject* self, PyObject* args) {
    float x, y;
    if (!PyArg_ParseTuple(args, "ff", &x, &y))
        return nullptr;
    self->model->Drag(x, y);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_IsMotionFinished(PyLAppModelObject* self, PyObject*) {
    return PyBool_FromLong(self->model->IsMotionFinished() ? 1 : 0);
}

static PyObject* PyLAppModel_SetOffset(PyLAppModelObject* self, PyObject* args) {
    float dx, dy;
    if (!PyArg_ParseTuple(args, "ff", &dx, &dy))
        return nullptr;
    self->model->SetOffset(dx, dy);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetScale(PyLAppModelObject* self, PyObject* args) {
    float s;
    if (!PyArg_ParseTuple(args, "f", &s))
        return nullptr;
    self->model->SetScale(s);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetParameterValue(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &val, &weight))
        return nullptr;
    self->model->SetParameterValue(id, val, weight);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_AddParameterValue(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &val, &weight))
        return nullptr;
    // 统一接口的 AddParameterValue 无 weight，直接合并进 value（等价 val * w 语义）
    self->model->AddParameterValue(id, val * weight);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetAutoBreathEnable(PyLAppModelObject* self, PyObject* args) {
    int v;
    if (!PyArg_ParseTuple(args, "p", &v))
        return nullptr;
    self->model->SetAutoBreath(v != 0);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetAutoBlinkEnable(PyLAppModelObject* self, PyObject* args) {
    int v;
    if (!PyArg_ParseTuple(args, "p", &v))
        return nullptr;
    self->model->SetAutoBlink(v != 0);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_GetParameterCount(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->GetParameterCount());
}

static PyObject* PyLAppModel_GetPartCount(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->GetPartCount());
}

static PyObject* PyLAppModel_GetPartId(PyLAppModelObject* self, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx))
        return nullptr;
    return PyUnicode_FromString(self->model->GetPartId(idx));
}

static PyObject* PyLAppModel_GetPartIds(PyLAppModelObject* self, PyObject*) {
    int n = self->model->GetPartCount();
    PyObject* lst = PyList_New(n);
    for (int i = 0; i < n; i++)
        PyList_SetItem(lst, i, PyUnicode_FromString(self->model->GetPartId(i)));
    return lst;
}

static PyObject* PyLAppModel_SetPartOpacity(PyLAppModelObject* self, PyObject* args) {
    int idx;
    float val;
    if (!PyArg_ParseTuple(args, "if", &idx, &val))
        return nullptr;
    self->model->SetPartOpacity(idx, val);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Update(PyLAppModelObject* self, PyObject* args) {
    // 可选 dt: 不传 = 墙钟自适配（哨兵 -1），传入 = delta 驱动
    float dt = -1.0f;
    if (!PyArg_ParseTuple(args, "|f", &dt))
        return nullptr;
    self->model->Update(dt);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Draw(PyLAppModelObject* self, PyObject*) {
    self->model->Draw();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_HitTest(PyLAppModelObject* self, PyObject* args) {
    const char* area;
    float x, y;
    if (!PyArg_ParseTuple(args, "sff", &area, &x, &y))
        return nullptr;
    bool r = self->model->IsAreaHit(area, x, y);
    if (r)
        return PyUnicode_FromString(area);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetExpression(PyLAppModelObject* self, PyObject* args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;
    self->model->SetExpression(name);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetRandomExpression(PyLAppModelObject* self, PyObject*) {
    self->model->SetRandomExpression();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_StartMotion(PyLAppModelObject* self, PyObject* args,
                                         PyObject* kwargs) {
    const char* group;
    int no, priority;
    PyObject* onStart = nullptr;
    PyObject* onFinish = nullptr;
    static const char* kwlist[] = {"group", "no", "priority", "onStart", "onFinish", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "sii|OO",
                                     const_cast<char**>(kwlist),
                                     &group,
                                     &no,
                                     &priority,
                                     &onStart,
                                     &onFinish))
        return nullptr;

    self->model->StartMotion(
        group, no, priority, MakeMotionCallback(onStart), MakeMotionCallback(onFinish));
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_StartRandomMotion(PyLAppModelObject* self, PyObject* args,
                                               PyObject* kwargs) {
    PyObject* nameObj = Py_None;
    PyObject* prioObj = Py_None;
    PyObject* onStart = nullptr;
    PyObject* onFinish = nullptr;
    static const char* kwlist[] = {"group", "priority", "onStart", "onFinish", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|OOOO",
                                     const_cast<char**>(kwlist),
                                     &nameObj,
                                     &prioObj,
                                     &onStart,
                                     &onFinish))
        return nullptr;

    int priority = 3;
    const char* group = nullptr;
    PyObject* utf8Ref = nullptr;

    if (prioObj != Py_None && PyLong_Check(prioObj)) {
        priority = (int)PyLong_AsLong(prioObj);
    }
    if (nameObj != Py_None) {
        if (PyLong_Check(nameObj)) {
            priority = (int)PyLong_AsLong(nameObj);
        } else {
            utf8Ref = PyUnicode_AsUTF8String(nameObj);
            if (!utf8Ref)
                return nullptr;
            group = PyBytes_AsString(utf8Ref);
        }
    }

    auto sc = MakeMotionCallback(onStart);
    auto fc = MakeMotionCallback(onFinish);
    self->model->StartRandomMotion(group ? group : "", priority, std::move(sc), std::move(fc));
    Py_XDECREF(utf8Ref);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_GetCanvasWidth(PyLAppModelObject* self, PyObject*) {
    float w, h;
    self->model->GetCanvasSize(w, h);
    return PyFloat_FromDouble(w);
}

static PyObject* PyLAppModel_GetCanvasHeight(PyLAppModelObject* self, PyObject*) {
    float w, h;
    self->model->GetCanvasSize(w, h);
    return PyFloat_FromDouble(h);
}

static PyObject* PyLAppModel_GetCanvasSize(PyLAppModelObject* self, PyObject*) {
    float w, h;
    self->model->GetCanvasSize(w, h);
    return Py_BuildValue("(ff)", w, h);
}

static PyObject* PyLAppModel_ClearMotions(PyLAppModelObject* self, PyObject*) {
    self->model->StopAllMotions();
    Py_RETURN_NONE;
}
static PyObject* PyLAppModel_StopAllMotions(PyLAppModelObject* self, PyObject*) {
    self->model->StopAllMotions();
    Py_RETURN_NONE;
}
static PyObject* PyLAppModel_ResetPose(PyLAppModelObject* self, PyObject*) {
    self->model->ResetPose();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_ResetExpression(PyLAppModelObject* self, PyObject*) {
    self->model->ResetExpression();
    Py_RETURN_NONE;
}
// Global reference to Parameter class (imported from live2d.v2.params)
static PyObject* sParamClass = nullptr;

static void ensureParamClass() {
    if (!sParamClass) {
        auto* mod = PyImport_ImportModule("live2d.v2.params");
        if (mod) {
            sParamClass = PyObject_GetAttrString(mod, "Parameter");
            Py_DECREF(mod);
        }
    }
}

static PyObject* PyLAppModel_GetParameter(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    ensureParamClass();
    if (!sParamClass)
        Py_RETURN_NONE;
    PyObject* param = PyObject_CallObject(sParamClass, nullptr);
    if (!param)
        return nullptr;
    PyObject_SetAttrString(
        param, "id", PyUnicode_FromString(self->model->GetParameterId(index)));
    PyObject_SetAttrString(param, "type", PyLong_FromLong(0));
    PyObject_SetAttrString(
        param, "value", PyFloat_FromDouble(self->model->GetParameterValue(index)));
    PyObject_SetAttrString(
        param, "min", PyFloat_FromDouble(self->model->GetParameterMinimumValue(index)));
    PyObject_SetAttrString(
        param, "max", PyFloat_FromDouble(self->model->GetParameterMaximumValue(index)));
    PyObject_SetAttrString(
        param, "default", PyFloat_FromDouble(self->model->GetParameterDefaultValue(index)));
    return param;
}

static PyObject* PyLAppModel_HitPart(PyLAppModelObject* self, PyObject* args) {
    float x, y;
    int topOnly = 0;
    if (!PyArg_ParseTuple(args, "ff|p", &x, &y, &topOnly))
        return nullptr;
    PyObject* lst = PyList_New(0);
    self->model->HitPart(
        x,
        y,
        lst,
        [](void* collector, const char* id) {
            PyList_Append((PyObject*)collector, PyUnicode_FromString(id));
        },
        topOnly != 0);
    return lst;
}

static PyObject* PyLAppModel_SetPartScreenColor(PyLAppModelObject* self, PyObject* args) {
    int idx;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &idx, &r, &g, &b, &a))
        return nullptr;
    self->model->SetPartScreenColor(idx, r, g, b, a);
    Py_RETURN_NONE;
}
static PyObject* PyLAppModel_GetPartScreenColor(PyLAppModelObject* self, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx))
        return nullptr;
    float r, g, b, a;
    self->model->GetPartScreenColor(idx, r, g, b, a);
    return Py_BuildValue("[ffff]", r, g, b, a);
}
static PyObject* PyLAppModel_SetPartMultiplyColor(PyLAppModelObject* self, PyObject* args) {
    int idx;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &idx, &r, &g, &b, &a))
        return nullptr;
    self->model->SetPartMultiplyColor(idx, r, g, b, a);
    Py_RETURN_NONE;
}
static PyObject* PyLAppModel_GetPartMultiplyColor(PyLAppModelObject* self, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx))
        return nullptr;
    float r, g, b, a;
    self->model->GetPartMultiplyColor(idx, r, g, b, a);
    return Py_BuildValue("[ffff]", r, g, b, a);
}
static PyObject* PyLAppModel_Rotate(PyLAppModelObject* self, PyObject* args) {
    float deg;
    if (!PyArg_ParseTuple(args, "f", &deg))
        return nullptr;
    self->model->Rotate(deg);
    Py_RETURN_NONE;
}
static PyObject* PyLAppModel_GetPixelsPerUnit(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong((long)self->model->GetPixelsPerUnit());
}
static PyObject* PyLAppModel_GetCanvasSizePixel(PyLAppModelObject* self, PyObject*) {
    float w, h;
    self->model->GetCanvasSizePixel(w, h);
    return Py_BuildValue("(ff)", w, h);
}

// --- autoBreath property ---
static PyObject* PyLAppModel_getAutoBreath(PyLAppModelObject* self, void*) {
    return PyBool_FromLong(self->model->AutoBreathEnabled() ? 1 : 0);
}
static int PyLAppModel_setAutoBreath(PyLAppModelObject* self, PyObject* value, void*) {
    if (!value) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute");
        return -1;
    }
    int ok = PyObject_IsTrue(value);
    if (ok < 0)
        return -1;
    self->model->SetAutoBreath(ok != 0);
    return 0;
}

// --- autoBlink property ---
static PyObject* PyLAppModel_getAutoBlink(PyLAppModelObject* self, void*) {
    return PyBool_FromLong(self->model->AutoBlinkEnabled() ? 1 : 0);
}
static int PyLAppModel_setAutoBlink(PyLAppModelObject* self, PyObject* value, void*) {
    if (!value) {
        PyErr_SetString(PyExc_TypeError, "Cannot delete attribute");
        return -1;
    }
    int ok = PyObject_IsTrue(value);
    if (ok < 0)
        return -1;
    self->model->SetAutoBlink(ok != 0);
    return 0;
}

static PyObject* PyLAppModel_CreateRenderer(PyLAppModelObject* self, PyObject* args) {
    int maskBufferCount = 1;
    if (!PyArg_ParseTuple(args, "|i", &maskBufferCount))
        return nullptr;
    self->model->CreateRenderer(maskBufferCount);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_ReleaseRenderer(PyLAppModelObject* self, PyObject*) {
    self->model->DestroyRenderer();
    Py_RETURN_NONE;
}

// ============================================================================
// v3 fine-grained API（IModel 已全部实现，与 live2d.v3.Model 对齐）
// ============================================================================

static PyObject* PyLAppModel_Version(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->Version());
}

static PyObject* PyLAppModel_IsV2(PyLAppModelObject* self, PyObject*) {
    return PyBool_FromLong(self->model->IsV2() ? 1 : 0);
}

static PyObject* PyLAppModel_IsV3(PyLAppModelObject* self, PyObject*) {
    return PyBool_FromLong(self->model->IsV3() ? 1 : 0);
}

static PyObject* PyLAppModel_GetModelHomeDir(PyLAppModelObject* self, PyObject*) {
    return PyUnicode_FromString(self->model->GetModelHomeDir());
}

static PyObject* PyLAppModel_GetParameterIds(PyLAppModelObject* self, PyObject*) {
    const int count = self->model->GetParameterCount();
    PyObject* list = PyList_New(count);
    int index = 0;
    void* collector[2] = {list, &index};
    self->model->GetParameterIds(collector, [](void* collector, const char* id) {
        PyObject* list = (PyObject*)(((void**)collector)[0]);
        int* index = (int*)(((void**)collector)[1]);
        PyList_SetItem(list, (*index)++, PyUnicode_FromString(id));
    });
    return list;
}

static PyObject* PyLAppModel_GetDrawableIds(PyLAppModelObject* self, PyObject*) {
    const int count = self->model->GetDrawableCount();
    PyObject* list = PyList_New(count);
    int index = 0;
    void* collector[2] = {list, &index};
    self->model->GetDrawableIds(collector, [](void* collector, const char* id) {
        PyObject* list = (PyObject*)(((void**)collector)[0]);
        int* index = (int*)(((void**)collector)[1]);
        PyList_SetItem(list, (*index)++, PyUnicode_FromString(id));
    });
    return list;
}

static PyObject* PyLAppModel_GetExpressions(PyLAppModelObject* self, PyObject*) {
    const int count = self->model->GetExpressionCount();
    PyObject* list = PyList_New(count);
    int index = 0;
    void* collector[2] = {list, &index};
    self->model->GetExpressions(collector, [](void* collector, const char* id, const char* file) {
        PyObject* list = (PyObject*)(((void**)collector)[0]);
        int* index = (int*)(((void**)collector)[1]);
        PyList_SetItem(list, (*index)++, PyUnicode_FromString(id));
    });
    return list;
}

static PyObject* PyLAppModel_AddExpression(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id))
        return nullptr;
    self->model->AddExpression(id);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_RemoveExpression(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id))
        return nullptr;
    self->model->RemoveExpression(id);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_ResetExpressions(PyLAppModelObject* self, PyObject*) {
    self->model->ResetExpressions();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_LoadExtraExpression(PyLAppModelObject* self, PyObject* args) {
    const char *id, *path;
    if (!PyArg_ParseTuple(args, "ss", &id, &path))
        return nullptr;
    self->model->LoadExtraExpression(id, path);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_GetMotions(PyLAppModelObject* self, PyObject*) {
    PyObject* motions = PyDict_New();
    self->model->GetMotions(
        motions,
        [](void* collector, const char* group, int no, const char* file, const char* sound) {
            PyObject* motions = (PyObject*)collector;
            PyObject* key = PyUnicode_FromString(group);
            PyObject* list = PyDict_GetItem(motions, key);   // borrowed
            if (list == NULL) {
                list = PyList_New(0);
                PyDict_SetItem(motions, key, list);
                Py_DECREF(list);
            }
            PyObject* motion = PyDict_New();
            PyDict_SetItem(motion, PyUnicode_FromString("File"), PyUnicode_FromString(file));
            PyDict_SetItem(motion, PyUnicode_FromString("Sound"), PyUnicode_FromString(sound));
            PyList_Append(list, motion);
            Py_DECREF(motion);
            Py_DECREF(key);
        });
    return motions;
}

static PyObject* PyLAppModel_LoadExtraMotion(PyLAppModelObject* self, PyObject* args) {
    const char *group, *path;
    if (!PyArg_ParseTuple(args, "ss", &group, &path))
        return nullptr;
    return PyLong_FromLong(self->model->LoadExtraMotion(group, path));
}

static PyObject* PyLAppModel_IsAreaHit(PyLAppModelObject* self, PyObject* args) {
    const char* area;
    float x, y;
    if (!PyArg_ParseTuple(args, "sff", &area, &x, &y))
        return nullptr;
    return PyBool_FromLong(self->model->IsAreaHit(area, x, y) ? 1 : 0);
}

static PyObject* PyLAppModel_HitDrawable(PyLAppModelObject* self, PyObject* args) {
    float x, y;
    int topOnly = 0;
    if (!PyArg_ParseTuple(args, "ff|p", &x, &y, &topOnly))
        return nullptr;
    PyObject* lst = PyList_New(0);
    self->model->HitDrawable(
        x,
        y,
        lst,
        [](void* collector, const char* id) {
            PyList_Append((PyObject*)collector, PyUnicode_FromString(id));
        },
        topOnly != 0);
    return lst;
}

static PyObject* PyLAppModel_LoadParameters(PyLAppModelObject* self, PyObject*) {
    self->model->LoadParameters();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SaveParameters(PyLAppModelObject* self, PyObject*) {
    self->model->SaveParameters();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_UpdateMotion(PyLAppModelObject* self, PyObject* args) {
    float dt;
    if (!PyArg_ParseTuple(args, "f", &dt))
        return nullptr;
    return PyBool_FromLong(self->model->UpdateMotion(dt) ? 1 : 0);
}

static PyObject* PyLAppModel_UpdateDrag(PyLAppModelObject* self, PyObject* args) {
    float dt;
    if (!PyArg_ParseTuple(args, "f", &dt))
        return nullptr;
    self->model->UpdateDrag(dt);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_UpdateBreath(PyLAppModelObject* self, PyObject* args) {
    float dt;
    if (!PyArg_ParseTuple(args, "f", &dt))
        return nullptr;
    self->model->UpdateBreath(dt);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_UpdateBlink(PyLAppModelObject* self, PyObject* args) {
    float dt;
    if (!PyArg_ParseTuple(args, "f", &dt))
        return nullptr;
    self->model->UpdateBlink(dt);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_UpdateExpression(PyLAppModelObject* self, PyObject* args) {
    float dt;
    if (!PyArg_ParseTuple(args, "f", &dt))
        return nullptr;
    self->model->UpdateExpression(dt);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_UpdatePhysics(PyLAppModelObject* self, PyObject* args) {
    float dt;
    if (!PyArg_ParseTuple(args, "f", &dt))
        return nullptr;
    self->model->UpdatePhysics(dt);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_UpdatePose(PyLAppModelObject* self, PyObject* args) {
    float dt;
    if (!PyArg_ParseTuple(args, "f", &dt))
        return nullptr;
    self->model->UpdatePose(dt);
    Py_RETURN_NONE;
}

static PyGetSetDef PyLAppModel_getset[] = {
    {"autoBreath",
     (getter)PyLAppModel_getAutoBreath,
     (setter)PyLAppModel_setAutoBreath,
     "",
     nullptr},
    {"autoBlink", (getter)PyLAppModel_getAutoBlink, (setter)PyLAppModel_setAutoBlink, "", nullptr},
    {nullptr}};

PyMethodDef PyLAppModel_methods[] = {
    {"LoadModelJson", (PyCFunction)PyLAppModel_LoadModelJson, METH_VARARGS | METH_KEYWORDS, ""},
    {"Resize", (PyCFunction)PyLAppModel_Resize, METH_VARARGS, ""},
    {"Drag", (PyCFunction)PyLAppModel_Drag, METH_VARARGS, ""},
    {"IsMotionFinished", (PyCFunction)PyLAppModel_IsMotionFinished, METH_NOARGS, ""},
    {"SetOffset", (PyCFunction)PyLAppModel_SetOffset, METH_VARARGS, ""},
    {"SetScale", (PyCFunction)PyLAppModel_SetScale, METH_VARARGS, ""},
    {"SetParameterValue", (PyCFunction)PyLAppModel_SetParameterValue, METH_VARARGS, ""},
    {"AddParameterValue", (PyCFunction)PyLAppModel_AddParameterValue, METH_VARARGS, ""},
    {"SetAutoBreathEnable", (PyCFunction)PyLAppModel_SetAutoBreathEnable, METH_VARARGS, ""},
    {"SetAutoBlinkEnable", (PyCFunction)PyLAppModel_SetAutoBlinkEnable, METH_VARARGS, ""},
    {"GetParameterCount", (PyCFunction)PyLAppModel_GetParameterCount, METH_NOARGS, ""},
    {"GetPartCount", (PyCFunction)PyLAppModel_GetPartCount, METH_NOARGS, ""},
    {"GetPartId", (PyCFunction)PyLAppModel_GetPartId, METH_VARARGS, ""},
    {"GetPartIds", (PyCFunction)PyLAppModel_GetPartIds, METH_NOARGS, ""},
    {"SetPartOpacity", (PyCFunction)PyLAppModel_SetPartOpacity, METH_VARARGS, ""},
    {"Update", (PyCFunction)PyLAppModel_Update, METH_VARARGS, ""},
    {"Draw", (PyCFunction)PyLAppModel_Draw, METH_NOARGS, ""},
    {"HitTest", (PyCFunction)PyLAppModel_HitTest, METH_VARARGS, ""},
    {"HitPart", (PyCFunction)PyLAppModel_HitPart, METH_VARARGS, ""},
    {"GetParameter", (PyCFunction)PyLAppModel_GetParameter, METH_VARARGS, ""},
    {"SetPartScreenColor", (PyCFunction)PyLAppModel_SetPartScreenColor, METH_VARARGS, ""},
    {"setPartScreenColor", (PyCFunction)PyLAppModel_SetPartScreenColor, METH_VARARGS, ""},
    {"GetPartScreenColor", (PyCFunction)PyLAppModel_GetPartScreenColor, METH_VARARGS, ""},
    {"SetPartMultiplyColor", (PyCFunction)PyLAppModel_SetPartMultiplyColor, METH_VARARGS, ""},
    {"GetPartMultiplyColor", (PyCFunction)PyLAppModel_GetPartMultiplyColor, METH_VARARGS, ""},
    {"Rotate", (PyCFunction)PyLAppModel_Rotate, METH_VARARGS, ""},
    {"GetPixelsPerUnit", (PyCFunction)PyLAppModel_GetPixelsPerUnit, METH_NOARGS, ""},
    {"GetCanvasSizePixel", (PyCFunction)PyLAppModel_GetCanvasSizePixel, METH_NOARGS, ""},
    {"SetExpression", (PyCFunction)PyLAppModel_SetExpression, METH_VARARGS, ""},
    {"SetRandomExpression", (PyCFunction)PyLAppModel_SetRandomExpression, METH_NOARGS, ""},
    {"StartMotion", (PyCFunction)PyLAppModel_StartMotion, METH_VARARGS | METH_KEYWORDS, ""},
    {"StartRandomMotion",
     (PyCFunction)PyLAppModel_StartRandomMotion,
     METH_VARARGS | METH_KEYWORDS,
     ""},
    {"GetCanvasWidth", (PyCFunction)PyLAppModel_GetCanvasWidth, METH_NOARGS, ""},
    {"GetCanvasHeight", (PyCFunction)PyLAppModel_GetCanvasHeight, METH_NOARGS, ""},
    {"GetCanvasSize", (PyCFunction)PyLAppModel_GetCanvasSize, METH_NOARGS, ""},
    {"ClearMotions", (PyCFunction)PyLAppModel_ClearMotions, METH_NOARGS, ""},
    {"StopAllMotions", (PyCFunction)PyLAppModel_StopAllMotions, METH_NOARGS, ""},
    {"ResetPose", (PyCFunction)PyLAppModel_ResetPose, METH_NOARGS, ""},
    {"ResetExpression", (PyCFunction)PyLAppModel_ResetExpression, METH_NOARGS, ""},
    {"CreateRenderer", (PyCFunction)PyLAppModel_CreateRenderer, METH_VARARGS, ""},
    {"ReleaseRenderer", (PyCFunction)PyLAppModel_ReleaseRenderer, METH_NOARGS, ""},

    // ---- v3 fine-grained API ----
    {"Version", (PyCFunction)PyLAppModel_Version, METH_NOARGS, ""},
    {"IsV2", (PyCFunction)PyLAppModel_IsV2, METH_NOARGS, ""},
    {"IsV3", (PyCFunction)PyLAppModel_IsV3, METH_NOARGS, ""},
    {"GetModelHomeDir", (PyCFunction)PyLAppModel_GetModelHomeDir, METH_NOARGS, ""},
    {"GetParameterIds", (PyCFunction)PyLAppModel_GetParameterIds, METH_NOARGS, ""},
    {"GetDrawableIds", (PyCFunction)PyLAppModel_GetDrawableIds, METH_NOARGS, ""},
    {"GetExpressions", (PyCFunction)PyLAppModel_GetExpressions, METH_NOARGS, ""},
    {"AddExpression", (PyCFunction)PyLAppModel_AddExpression, METH_VARARGS, ""},
    {"RemoveExpression", (PyCFunction)PyLAppModel_RemoveExpression, METH_VARARGS, ""},
    {"ResetExpressions", (PyCFunction)PyLAppModel_ResetExpressions, METH_NOARGS, ""},
    {"LoadExtraExpression", (PyCFunction)PyLAppModel_LoadExtraExpression, METH_VARARGS, ""},
    {"GetMotions", (PyCFunction)PyLAppModel_GetMotions, METH_NOARGS, ""},
    {"LoadExtraMotion", (PyCFunction)PyLAppModel_LoadExtraMotion, METH_VARARGS, ""},
    {"IsAreaHit", (PyCFunction)PyLAppModel_IsAreaHit, METH_VARARGS, ""},
    {"HitDrawable", (PyCFunction)PyLAppModel_HitDrawable, METH_VARARGS, ""},
    {"LoadParameters", (PyCFunction)PyLAppModel_LoadParameters, METH_NOARGS, ""},
    {"SaveParameters", (PyCFunction)PyLAppModel_SaveParameters, METH_NOARGS, ""},
    {"UpdateMotion", (PyCFunction)PyLAppModel_UpdateMotion, METH_VARARGS, ""},
    {"UpdateDrag", (PyCFunction)PyLAppModel_UpdateDrag, METH_VARARGS, ""},
    {"UpdateBreath", (PyCFunction)PyLAppModel_UpdateBreath, METH_VARARGS, ""},
    {"UpdateBlink", (PyCFunction)PyLAppModel_UpdateBlink, METH_VARARGS, ""},
    {"UpdateExpression", (PyCFunction)PyLAppModel_UpdateExpression, METH_VARARGS, ""},
    {"UpdatePhysics", (PyCFunction)PyLAppModel_UpdatePhysics, METH_VARARGS, ""},
    {"UpdatePose", (PyCFunction)PyLAppModel_UpdatePose, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL}};

PyType_Slot PyLAppModel_slots[] = {{Py_tp_new, (void*)PyLAppModel_new},
                                   {Py_tp_init, (void*)PyLAppModel_init},
                                   {Py_tp_dealloc, (void*)PyLAppModel_dealloc},
                                   {Py_tp_methods, (void*)PyLAppModel_methods},
                                   {Py_tp_getset, (void*)PyLAppModel_getset},
                                   {0, nullptr}};
