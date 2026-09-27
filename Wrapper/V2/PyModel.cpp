#include "PyModel.hpp"
#include <V2/Model.hpp>
#include "Log.hpp"
#include "Python.hpp"
#include <modsupport.h>
#include <object.h>
#include <pytypedefs.h>

using namespace Live2D::Common::Log;

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

static PyObject* PyLAppModel_GetModelHomeDir(PyLAppModelObject* self, PyObject*) {
    return PyUnicode_FromString(self->model->GetModelHomeDir());
}

static PyObject* PyLAppModel_Version(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->Version());
}

static PyObject* PyLAppModel_IsV2(PyLAppModelObject* self, PyObject*) {
    return PyBool_FromLong(self->model->IsV2() ? 1 : 0);
}

static PyObject* PyLAppModel_IsV3(PyLAppModelObject* self, PyObject*) {
    return PyBool_FromLong(self->model->IsV3() ? 1 : 0);
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

static PyObject* PyLAppModel_SetOffsetX(PyLAppModelObject* self, PyObject* args) {
    float x;
    if (!PyArg_ParseTuple(args, "f", &x))
        return nullptr;
    self->model->SetOffsetX(x);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetOffsetY(PyLAppModelObject* self, PyObject* args) {
    float y;
    if (!PyArg_ParseTuple(args, "f", &y))
        return nullptr;
    self->model->SetOffsetY(y);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetScale(PyLAppModelObject* self, PyObject* args) {
    float s;
    if (!PyArg_ParseTuple(args, "f", &s))
        return nullptr;
    self->model->SetScale(s);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetScaleX(PyLAppModelObject* self, PyObject* args) {
    float s;
    if (!PyArg_ParseTuple(args, "f", &s))
        return nullptr;
    self->model->SetScaleX(s);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetScaleY(PyLAppModelObject* self, PyObject* args) {
    float s;
    if (!PyArg_ParseTuple(args, "f", &s))
        return nullptr;
    self->model->SetScaleY(s);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Rotate(PyLAppModelObject* self, PyObject* args) {
    float deg;
    if (!PyArg_ParseTuple(args, "f", &deg))
        return nullptr;
    self->model->Rotate(deg);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_GetMvp(PyLAppModelObject* self, PyObject*) {
    const float* mvp = self->model->GetMvp();
    PyObject* tuple = PyTuple_New(16);
    for (int i = 0; i < 16; i++)
        PyTuple_SetItem(tuple, i, PyFloat_FromDouble(mvp[i]));
    return tuple;
}

// ---- 参数（统一命名: 按 index 为主，ById 为 id 版本） ----

static PyObject* PyLAppModel_SetParamByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "if|f", &index, &val, &weight))
        return nullptr;
    self->model->SetParameterValue(index, val, weight);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetParamById(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &val, &weight))
        return nullptr;
    self->model->SetParameterValue(id, val, weight);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_AddParamByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    float val;
    if (!PyArg_ParseTuple(args, "if", &index, &val))
        return nullptr;
    self->model->AddParameterValue(index, val);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_AddParamById(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    float val;
    if (!PyArg_ParseTuple(args, "sf", &id, &val))
        return nullptr;
    self->model->AddParameterValue(id, val);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetSaveParamByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "if|f", &index, &val, &weight))
        return nullptr;
    self->model->SetAndSaveParameterValue(index, val, weight);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetSaveParamById(PyLAppModelObject* self,
                                                           PyObject* args) {
    const char* id;
    float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &val, &weight))
        return nullptr;
    self->model->SetAndSaveParameterValue(id, val, weight);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_AddSaveParamByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    float val;
    if (!PyArg_ParseTuple(args, "if", &index, &val))
        return nullptr;
    self->model->AddAndSaveParameterValue(index, val);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_AddSaveParamById(PyLAppModelObject* self,
                                                           PyObject* args) {
    const char* id;
    float val;
    if (!PyArg_ParseTuple(args, "sf", &id, &val))
        return nullptr;
    self->model->AddAndSaveParameterValue(id, val);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_GetParamCount(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->GetParameterCount());
}

static PyObject* PyLAppModel_GetParamIds(PyLAppModelObject* self, PyObject*) {
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

static PyObject* PyLAppModel_GetParamValueByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    return PyFloat_FromDouble(self->model->GetParameterValue(index));
}

static PyObject* PyLAppModel_GetParamMaxByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    return PyFloat_FromDouble(self->model->GetParameterMaximumValue(index));
}

static PyObject* PyLAppModel_GetParamMinByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    return PyFloat_FromDouble(self->model->GetParameterMinimumValue(index));
}

static PyObject* PyLAppModel_GetParamDefaultByIndex(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    return PyFloat_FromDouble(self->model->GetParameterDefaultValue(index));
}


static int ParamIndexById(PyLAppModelObject* self, const char* id) {
    const int count = self->model->GetParameterCount();
    for (int i = 0; i < count; i++) {
        if (strcmp(self->model->GetParameterId(i), id) == 0)
            return i;
    }
    return -1;
}

static PyObject* PyLAppModel_GetParamValueById(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id))
        return nullptr;
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return nullptr;
    }
    return PyFloat_FromDouble(self->model->GetParameterValue(index));
}

static PyObject* PyLAppModel_GetParamMaxById(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id))
        return nullptr;
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return nullptr;
    }
    return PyFloat_FromDouble(self->model->GetParameterMaximumValue(index));
}

static PyObject* PyLAppModel_GetParamMinById(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id))
        return nullptr;
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return nullptr;
    }
    return PyFloat_FromDouble(self->model->GetParameterMinimumValue(index));
}

static PyObject* PyLAppModel_GetParamDefaultById(PyLAppModelObject* self, PyObject* args) {
    const char* id;
    if (!PyArg_ParseTuple(args, "s", &id))
        return nullptr;
    const int index = ParamIndexById(self, id);
    if (index < 0) {
        PyErr_Format(PyExc_ValueError, "parameter not found: %s", id);
        return nullptr;
    }
    return PyFloat_FromDouble(self->model->GetParameterDefaultValue(index));
}
static PyObject* PyLAppModel_LoadParameters(PyLAppModelObject* self, PyObject*) {
    self->model->LoadParameters();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SaveParameters(PyLAppModelObject* self, PyObject*) {
    self->model->SaveParameters();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_ResetAllParameters(PyLAppModelObject* self, PyObject*) {
    self->model->ResetAllParameters();
    Py_RETURN_NONE;
}

// ---- 部件 ----

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

// ---- drawable ----

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

static PyObject* PyLAppModel_GetDrawableVertices(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    const float* verts = self->model->GetDrawableVertices(index);
    const int count = self->model->GetDrawableVertexCount(index) * 2;
    PyObject* list = PyList_New(count);
    for (int i = 0; i < count; i++)
        PyList_SetItem(list, i, PyFloat_FromDouble(verts[i]));
    return list;
}

static PyObject* PyLAppModel_GetDrawableVertexCount(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    return PyLong_FromLong(self->model->GetDrawableVertexCount(index));
}

static PyObject* PyLAppModel_GetDrawableVertexIndexCount(PyLAppModelObject* self,
                                                         PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    return PyLong_FromLong(self->model->GetDrawableVertexIndexCount(index));
}

static PyObject* PyLAppModel_GetDrawableIndices(PyLAppModelObject* self, PyObject* args) {
    int index;
    if (!PyArg_ParseTuple(args, "i", &index))
        return nullptr;
    const unsigned short* indices = self->model->GetDrawableIndices(index);
    const int count = self->model->GetDrawableVertexIndexCount(index);
    PyObject* list = PyList_New(count);
    for (int i = 0; i < count; i++)
        PyList_SetItem(list, i, PyLong_FromLong(indices[i]));
    return list;
}

static PyObject* PyLAppModel_SetDrawableMultiplyColor(PyLAppModelObject* self, PyObject* args) {
    int idx;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &idx, &r, &g, &b, &a))
        return nullptr;
    self->model->SetDrawableMultiColor(idx, r, g, b, a);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetDrawableScreenColor(PyLAppModelObject* self, PyObject* args) {
    int idx;
    float r, g, b, a;
    if (!PyArg_ParseTuple(args, "iffff", &idx, &r, &g, &b, &a))
        return nullptr;
    self->model->SetDrawableScreenColor(idx, r, g, b, a);
    Py_RETURN_NONE;
}

// ---- 动作 ----

static PyObject* PyLAppModel_StartMotion(PyLAppModelObject* self, PyObject* args,
                                         PyObject* kwargs) {
    const char* group;
    int no, priority = 3;
    PyObject* onStart = nullptr;
    PyObject* onFinish = nullptr;
    static const char* kwlist[] = {"group", "no", "priority", "onStart", "onFinish", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "si|iOO",
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
    const char* group = nullptr;
    int priority = 3;
    PyObject* onStart = nullptr;
    PyObject* onFinish = nullptr;
    static const char* kwlist[] = {"group", "priority", "onStart", "onFinish", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args,
                                     kwargs,
                                     "|ziOO",
                                     const_cast<char**>(kwlist),
                                     &group,
                                     &priority,
                                     &onStart,
                                     &onFinish))
        return nullptr;

    self->model->StartRandomMotion(group ? group : "",
                                   priority,
                                   MakeMotionCallback(onStart),
                                   MakeMotionCallback(onFinish));
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_StopAllMotions(PyLAppModelObject* self, PyObject*) {
    self->model->StopAllMotions();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_LoadExtraMotion(PyLAppModelObject* self, PyObject* args) {
    const char *group, *path;
    if (!PyArg_ParseTuple(args, "ss", &group, &path))
        return nullptr;
    return PyLong_FromLong(self->model->LoadExtraMotion(group, path));
}

static PyObject* PyLAppModel_GetMotionGroupCount(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->GetMotionGroupCount());
}

static PyObject* PyLAppModel_GetMotionCount(PyLAppModelObject* self, PyObject* args) {
    const char* group;
    if (!PyArg_ParseTuple(args, "s", &group))
        return nullptr;
    return PyLong_FromLong(self->model->GetMotionCount(group));
}

static PyObject* PyLAppModel_GetMotionSound(PyLAppModelObject* self, PyObject* args) {
    const char* group;
    int no;
    if (!PyArg_ParseTuple(args, "si", &group, &no))
        return nullptr;
    return PyUnicode_FromString(self->model->GetMotionSound(group, no));
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

static PyObject* PyLAppModel_GetMotionGroups(PyLAppModelObject* self, PyObject*) {
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

// ---- 表情 ----

static PyObject* PyLAppModel_SetExpression(PyLAppModelObject* self, PyObject* args,
                                              PyObject* kwargs) {
    const char* name;
    float fadeoutMs = -1.0f;
    static const char* kwlist[] = {"expressionId", "fadeoutMs", nullptr};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "s|f", const_cast<char**>(kwlist), &name, &fadeoutMs))
        return nullptr;
    self->model->SetExpression(name, fadeoutMs);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetRandomExpression(PyLAppModelObject* self, PyObject* args,
                                                      PyObject* kwargs) {
    float fadeoutMs = -1.0f;
    static const char* kwlist[] = {"fadeoutMs", nullptr};
    if (!PyArg_ParseTupleAndKeywords(
            args, kwargs, "|f", const_cast<char**>(kwlist), &fadeoutMs))
        return nullptr;
    const char* id = self->model->SetRandomExpression(fadeoutMs);
    if (id)
        return PyUnicode_FromString(id);
    Py_RETURN_NONE;
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

static PyObject* PyLAppModel_ResetExpression(PyLAppModelObject* self, PyObject*) {
    self->model->ResetExpression();
    Py_RETURN_NONE;
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

static PyObject* PyLAppModel_LoadExtraExpression(PyLAppModelObject* self, PyObject* args) {
    const char *id, *path;
    if (!PyArg_ParseTuple(args, "ss", &id, &path))
        return nullptr;
    self->model->LoadExtraExpression(id, path);
    Py_RETURN_NONE;
}

// ---- 命中测试 ----

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

static PyObject* PyLAppModel_IsAreaHit(PyLAppModelObject* self, PyObject* args) {
    const char* area;
    float x, y;
    if (!PyArg_ParseTuple(args, "sff", &area, &x, &y))
        return nullptr;
    return PyBool_FromLong(self->model->IsAreaHit(area, x, y) ? 1 : 0);
}

static PyObject* PyLAppModel_IsPartHit(PyLAppModelObject* self, PyObject* args) {
    int index;
    float x, y;
    if (!PyArg_ParseTuple(args, "iff", &index, &x, &y))
        return nullptr;
    return PyBool_FromLong(self->model->IsPartHit(index, x, y) ? 1 : 0);
}

static PyObject* PyLAppModel_IsDrawableHit(PyLAppModelObject* self, PyObject* args) {
    int index;
    float x, y;
    if (!PyArg_ParseTuple(args, "iff", &index, &x, &y))
        return nullptr;
    return PyBool_FromLong(self->model->IsDrawableHit(index, x, y) ? 1 : 0);
}

// ---- 画布 ----

static PyObject* PyLAppModel_GetCanvasSize(PyLAppModelObject* self, PyObject*) {
    float w, h;
    self->model->GetCanvasSize(w, h);
    return Py_BuildValue("(ff)", w, h);
}

static PyObject* PyLAppModel_GetCanvasSizePixel(PyLAppModelObject* self, PyObject*) {
    float w, h;
    self->model->GetCanvasSizePixel(w, h);
    return Py_BuildValue("(ff)", w, h);
}

static PyObject* PyLAppModel_GetPixelsPerUnit(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong((long)self->model->GetPixelsPerUnit());
}

// ---- 自动机制 ----

static PyObject* PyLAppModel_SetAutoBreath(PyLAppModelObject* self, PyObject* args) {
    int v;
    if (!PyArg_ParseTuple(args, "p", &v))
        return nullptr;
    self->model->SetAutoBreath(v != 0);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetAutoBlink(PyLAppModelObject* self, PyObject* args) {
    int v;
    if (!PyArg_ParseTuple(args, "p", &v))
        return nullptr;
    self->model->SetAutoBlink(v != 0);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_HasMocConsistencyFromFile(PyLAppModelObject* self, PyObject* args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name))
        return nullptr;
    return PyBool_FromLong(self->model->HasMocConsistencyFromFile(name) ? 1 : 0);
}

// ---- 渲染 ----

static PyObject* PyLAppModel_CreateRenderer(PyLAppModelObject* self, PyObject* args) {
    int maskBufferCount = 1;
    if (!PyArg_ParseTuple(args, "|i", &maskBufferCount))
        return nullptr;
    self->model->CreateRenderer(maskBufferCount);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_DestroyRenderer(PyLAppModelObject* self, PyObject*) {
    self->model->DestroyRenderer();
    Py_RETURN_NONE;
}

// ---- 更新 ----

static PyObject* PyLAppModel_Update(PyLAppModelObject* self, PyObject* args) {
    // 可选 dt: 不传 = 墙钟自适配（哨兵 -1），传入 = delta 驱动
    float dt = -1.0f;
    if (!PyArg_ParseTuple(args, "|f", &dt))
        return nullptr;
    self->model->Update(dt);
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

static PyObject* PyLAppModel_ResetPose(PyLAppModelObject* self, PyObject*) {
    self->model->ResetPose();
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Draw(PyLAppModelObject* self, PyObject*) {
    self->model->Draw();
    Py_RETURN_NONE;
}

PyMethodDef PyLAppModel_methods[] = {
    {"LoadModelJson", (PyCFunction)PyLAppModel_LoadModelJson, METH_VARARGS | METH_KEYWORDS, ""},
    {"GetModelHomeDir", (PyCFunction)PyLAppModel_GetModelHomeDir, METH_NOARGS, ""},
    {"Version", (PyCFunction)PyLAppModel_Version, METH_NOARGS, ""},
    {"IsV2", (PyCFunction)PyLAppModel_IsV2, METH_NOARGS, ""},
    {"IsV3", (PyCFunction)PyLAppModel_IsV3, METH_NOARGS, ""},
    {"Resize", (PyCFunction)PyLAppModel_Resize, METH_VARARGS, ""},
    {"Drag", (PyCFunction)PyLAppModel_Drag, METH_VARARGS, ""},
    {"IsMotionFinished", (PyCFunction)PyLAppModel_IsMotionFinished, METH_NOARGS, ""},
    {"SetOffset", (PyCFunction)PyLAppModel_SetOffset, METH_VARARGS, ""},
    {"SetOffsetX", (PyCFunction)PyLAppModel_SetOffsetX, METH_VARARGS, ""},
    {"SetOffsetY", (PyCFunction)PyLAppModel_SetOffsetY, METH_VARARGS, ""},
    {"SetScale", (PyCFunction)PyLAppModel_SetScale, METH_VARARGS, ""},
    {"SetScaleX", (PyCFunction)PyLAppModel_SetScaleX, METH_VARARGS, ""},
    {"SetScaleY", (PyCFunction)PyLAppModel_SetScaleY, METH_VARARGS, ""},
    {"Rotate", (PyCFunction)PyLAppModel_Rotate, METH_VARARGS, ""},
    {"GetMvp", (PyCFunction)PyLAppModel_GetMvp, METH_NOARGS, ""},
    {"SetParamByIndex", (PyCFunction)PyLAppModel_SetParamByIndex, METH_VARARGS, ""},
    {"SetParamById", (PyCFunction)PyLAppModel_SetParamById, METH_VARARGS, ""},
    {"AddParamByIndex", (PyCFunction)PyLAppModel_AddParamByIndex, METH_VARARGS, ""},
    {"AddParamById", (PyCFunction)PyLAppModel_AddParamById, METH_VARARGS, ""},
    {"SetSaveParamByIndex", (PyCFunction)PyLAppModel_SetSaveParamByIndex, METH_VARARGS, ""},
    {"SetSaveParamById",
     (PyCFunction)PyLAppModel_SetSaveParamById,
     METH_VARARGS,
     ""},
    {"AddSaveParamByIndex", (PyCFunction)PyLAppModel_AddSaveParamByIndex, METH_VARARGS, ""},
    {"AddSaveParamById",
     (PyCFunction)PyLAppModel_AddSaveParamById,
     METH_VARARGS,
     ""},
    {"GetParamCount", (PyCFunction)PyLAppModel_GetParamCount, METH_NOARGS, ""},
    {"GetParamIds", (PyCFunction)PyLAppModel_GetParamIds, METH_NOARGS, ""},
    {"GetParamValueByIndex", (PyCFunction)PyLAppModel_GetParamValueByIndex, METH_VARARGS, ""},
    {"GetParamMaxByIndex", (PyCFunction)PyLAppModel_GetParamMaxByIndex, METH_VARARGS, ""},
    {"GetParamMinByIndex", (PyCFunction)PyLAppModel_GetParamMinByIndex, METH_VARARGS, ""},
    {"GetParamDefaultByIndex", (PyCFunction)PyLAppModel_GetParamDefaultByIndex, METH_VARARGS, ""},
    {"GetParamValueById", (PyCFunction)PyLAppModel_GetParamValueById, METH_VARARGS, ""},
    {"GetParamMaxById", (PyCFunction)PyLAppModel_GetParamMaxById, METH_VARARGS, ""},
    {"GetParamMinById", (PyCFunction)PyLAppModel_GetParamMinById, METH_VARARGS, ""},
    {"GetParamDefaultById", (PyCFunction)PyLAppModel_GetParamDefaultById, METH_VARARGS, ""},

    {"LoadParameters", (PyCFunction)PyLAppModel_LoadParameters, METH_NOARGS, ""},
    {"SaveParameters", (PyCFunction)PyLAppModel_SaveParameters, METH_NOARGS, ""},
    {"ResetAllParameters", (PyCFunction)PyLAppModel_ResetAllParameters, METH_NOARGS, ""},
    {"GetPartCount", (PyCFunction)PyLAppModel_GetPartCount, METH_NOARGS, ""},
    {"GetPartId", (PyCFunction)PyLAppModel_GetPartId, METH_VARARGS, ""},
    {"GetPartIds", (PyCFunction)PyLAppModel_GetPartIds, METH_NOARGS, ""},
    {"SetPartOpacity", (PyCFunction)PyLAppModel_SetPartOpacity, METH_VARARGS, ""},
    {"SetPartScreenColor", (PyCFunction)PyLAppModel_SetPartScreenColor, METH_VARARGS, ""},
    {"GetPartScreenColor", (PyCFunction)PyLAppModel_GetPartScreenColor, METH_VARARGS, ""},
    {"SetPartMultiplyColor", (PyCFunction)PyLAppModel_SetPartMultiplyColor, METH_VARARGS, ""},
    {"GetPartMultiplyColor", (PyCFunction)PyLAppModel_GetPartMultiplyColor, METH_VARARGS, ""},
    {"GetDrawableIds", (PyCFunction)PyLAppModel_GetDrawableIds, METH_NOARGS, ""},
    {"GetDrawableVertices", (PyCFunction)PyLAppModel_GetDrawableVertices, METH_VARARGS, ""},
    {"GetDrawableVertexCount", (PyCFunction)PyLAppModel_GetDrawableVertexCount, METH_VARARGS, ""},
    {"GetDrawableVertexIndexCount",
     (PyCFunction)PyLAppModel_GetDrawableVertexIndexCount,
     METH_VARARGS,
     ""},
    {"GetDrawableIndices", (PyCFunction)PyLAppModel_GetDrawableIndices, METH_VARARGS, ""},
    {"SetDrawableMultiplyColor", (PyCFunction)PyLAppModel_SetDrawableMultiplyColor, METH_VARARGS, ""},
    {"SetDrawableScreenColor", (PyCFunction)PyLAppModel_SetDrawableScreenColor, METH_VARARGS, ""},
    {"StartMotion", (PyCFunction)PyLAppModel_StartMotion, METH_VARARGS | METH_KEYWORDS, ""},
    {"StartRandomMotion", (PyCFunction)PyLAppModel_StartRandomMotion, METH_VARARGS | METH_KEYWORDS, ""},
    {"StopAllMotions", (PyCFunction)PyLAppModel_StopAllMotions, METH_NOARGS, ""},
    {"LoadExtraMotion", (PyCFunction)PyLAppModel_LoadExtraMotion, METH_VARARGS, ""},
    {"GetMotionGroupCount", (PyCFunction)PyLAppModel_GetMotionGroupCount, METH_NOARGS, ""},
    {"GetMotionCount", (PyCFunction)PyLAppModel_GetMotionCount, METH_VARARGS, ""},
    {"GetMotionSound", (PyCFunction)PyLAppModel_GetMotionSound, METH_VARARGS, ""},
    {"GetMotions", (PyCFunction)PyLAppModel_GetMotions, METH_NOARGS, ""},
    {"GetMotionGroups", (PyCFunction)PyLAppModel_GetMotionGroups, METH_NOARGS, ""},
    {"SetExpression", (PyCFunction)PyLAppModel_SetExpression, METH_VARARGS | METH_KEYWORDS, ""},
    {"SetRandomExpression", (PyCFunction)PyLAppModel_SetRandomExpression, METH_VARARGS | METH_KEYWORDS, ""},
    {"AddExpression", (PyCFunction)PyLAppModel_AddExpression, METH_VARARGS, ""},
    {"RemoveExpression", (PyCFunction)PyLAppModel_RemoveExpression, METH_VARARGS, ""},
    {"ResetExpressions", (PyCFunction)PyLAppModel_ResetExpressions, METH_NOARGS, ""},
    {"ResetExpression", (PyCFunction)PyLAppModel_ResetExpression, METH_NOARGS, ""},
    {"GetExpressions", (PyCFunction)PyLAppModel_GetExpressions, METH_NOARGS, ""},
    {"LoadExtraExpression", (PyCFunction)PyLAppModel_LoadExtraExpression, METH_VARARGS, ""},
    {"HitPart", (PyCFunction)PyLAppModel_HitPart, METH_VARARGS, ""},
    {"HitDrawable", (PyCFunction)PyLAppModel_HitDrawable, METH_VARARGS, ""},
    {"IsAreaHit", (PyCFunction)PyLAppModel_IsAreaHit, METH_VARARGS, ""},
    {"IsPartHit", (PyCFunction)PyLAppModel_IsPartHit, METH_VARARGS, ""},
    {"IsDrawableHit", (PyCFunction)PyLAppModel_IsDrawableHit, METH_VARARGS, ""},
    {"GetCanvasSize", (PyCFunction)PyLAppModel_GetCanvasSize, METH_NOARGS, ""},
    {"GetCanvasSizePixel", (PyCFunction)PyLAppModel_GetCanvasSizePixel, METH_NOARGS, ""},
    {"GetPixelsPerUnit", (PyCFunction)PyLAppModel_GetPixelsPerUnit, METH_NOARGS, ""},
    {"SetAutoBreath", (PyCFunction)PyLAppModel_SetAutoBreath, METH_VARARGS, ""},
    {"SetAutoBlink", (PyCFunction)PyLAppModel_SetAutoBlink, METH_VARARGS, ""},
    {"HasMocConsistencyFromFile", (PyCFunction)PyLAppModel_HasMocConsistencyFromFile, METH_VARARGS, ""},
    {"CreateRenderer", (PyCFunction)PyLAppModel_CreateRenderer, METH_VARARGS, ""},
    {"DestroyRenderer", (PyCFunction)PyLAppModel_DestroyRenderer, METH_NOARGS, ""},
    {"Update", (PyCFunction)PyLAppModel_Update, METH_VARARGS, ""},
    {"UpdateMotion", (PyCFunction)PyLAppModel_UpdateMotion, METH_VARARGS, ""},
    {"UpdateDrag", (PyCFunction)PyLAppModel_UpdateDrag, METH_VARARGS, ""},
    {"UpdateBreath", (PyCFunction)PyLAppModel_UpdateBreath, METH_VARARGS, ""},
    {"UpdateBlink", (PyCFunction)PyLAppModel_UpdateBlink, METH_VARARGS, ""},
    {"UpdateExpression", (PyCFunction)PyLAppModel_UpdateExpression, METH_VARARGS, ""},
    {"UpdatePhysics", (PyCFunction)PyLAppModel_UpdatePhysics, METH_VARARGS, ""},
    {"UpdatePose", (PyCFunction)PyLAppModel_UpdatePose, METH_VARARGS, ""},
    {"ResetPose", (PyCFunction)PyLAppModel_ResetPose, METH_NOARGS, ""},
    {"Draw", (PyCFunction)PyLAppModel_Draw, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL}};

PyType_Slot PyLAppModel_slots[] = {{Py_tp_new, (void*)PyLAppModel_new},
                                   {Py_tp_init, (void*)PyLAppModel_init},
                                   {Py_tp_dealloc, (void*)PyLAppModel_dealloc},
                                   {Py_tp_methods, (void*)PyLAppModel_methods},
                                   {0, nullptr}};
