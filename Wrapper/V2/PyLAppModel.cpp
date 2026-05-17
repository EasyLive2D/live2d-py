#include "PyLAppModel.hpp"

PyObject* PyLAppModel_new(PyTypeObject* type, PyObject*, PyObject*) {
    auto* self = (PyLAppModelObject*)PyObject_Malloc(sizeof(PyLAppModelObject));
    if (!self) return nullptr;
    PyObject_Init((PyObject*)self, type);
    return (PyObject*)self;
}

int PyLAppModel_init(PyLAppModelObject* self, PyObject*, PyObject*) {
    self->model = new live2d::LAppModel();
    return 0;
}

void PyLAppModel_dealloc(PyLAppModelObject* self) {
    delete self->model;
    PyObject_Free(self);
}

static PyObject* PyLAppModel_LoadModelJson(PyLAppModelObject* self, PyObject* args) {
    const char* path;
    const char* version = nullptr; int disablePrecision = 0;
    if (!PyArg_ParseTuple(args, "s|sp", &path, &version, &disablePrecision)) return nullptr;
    try {
        bool ok = self->model->loadModelJson(path);
        return PyBool_FromLong(ok ? 1 : 0);
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
        return nullptr;
    }
}

static PyObject* PyLAppModel_Resize(PyLAppModelObject* self, PyObject* args) {
    int w, h;
    if (!PyArg_ParseTuple(args, "ii", &w, &h)) return nullptr;
    self->model->resize(w, h); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Drag(PyLAppModelObject* self, PyObject* args) {
    float x, y;
    if (!PyArg_ParseTuple(args, "ff", &x, &y)) return nullptr;
    self->model->drag(x, y); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_IsMotionFinished(PyLAppModelObject* self, PyObject*) {
    return PyBool_FromLong(self->model->isMotionFinished() ? 1 : 0);
}

static PyObject* PyLAppModel_SetOffset(PyLAppModelObject* self, PyObject* args) {
    float dx, dy;
    if (!PyArg_ParseTuple(args, "ff", &dx, &dy)) return nullptr;
    self->model->setOffset(dx, dy); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetScale(PyLAppModelObject* self, PyObject* args) {
    float s;
    if (!PyArg_ParseTuple(args, "f", &s)) return nullptr;
    self->model->setScale(s); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetParameterValue(PyLAppModelObject* self, PyObject* args) {
    const char* id; float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &val, &weight)) return nullptr;
    self->model->setParameterValue(id, val, weight); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_AddParameterValue(PyLAppModelObject* self, PyObject* args) {
    const char* id; float val, weight = 1.0f;
    if (!PyArg_ParseTuple(args, "sf|f", &id, &val, &weight)) return nullptr;
    self->model->addParameterValue(id, val, weight); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetAutoBreathEnable(PyLAppModelObject* self, PyObject* args) {
    int v;
    if (!PyArg_ParseTuple(args, "p", &v)) return nullptr;
    self->model->setAutoBreathEnable(v != 0); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetAutoBlinkEnable(PyLAppModelObject* self, PyObject* args) {
    int v;
    if (!PyArg_ParseTuple(args, "p", &v)) return nullptr;
    self->model->setAutoBlinkEnable(v != 0); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_GetParameterCount(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->getParameterCount());
}

static PyObject* PyLAppModel_GetPartCount(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->getPartCount());
}

static PyObject* PyLAppModel_GetPartId(PyLAppModelObject* self, PyObject* args) {
    int idx;
    if (!PyArg_ParseTuple(args, "i", &idx)) return nullptr;
    return PyUnicode_FromString(self->model->getPartId(idx).c_str());
}

static PyObject* PyLAppModel_GetPartIds(PyLAppModelObject* self, PyObject*) {
    int n = self->model->getPartCount();
    PyObject* lst = PyList_New(n);
    for (int i = 0; i < n; i++)
        PyList_SetItem(lst, i, PyUnicode_FromString(self->model->getPartId(i).c_str()));
    return lst;
}

static PyObject* PyLAppModel_SetPartOpacity(PyLAppModelObject* self, PyObject* args) {
    int idx; float val;
    if (!PyArg_ParseTuple(args, "if", &idx, &val)) return nullptr;
    self->model->setPartOpacity(idx, val); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Update(PyLAppModelObject* self, PyObject*) {
    self->model->update(); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_Draw(PyLAppModelObject* self, PyObject*) {
    self->model->draw(); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_HitTest(PyLAppModelObject* self, PyObject* args) {
    const char* area; float x, y;
    if (!PyArg_ParseTuple(args, "sff", &area, &x, &y)) return nullptr;
    bool r = self->model->hitTest(area, x, y);
    if (r) return PyUnicode_FromString(area);
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetExpression(PyLAppModelObject* self, PyObject* args) {
    const char* name;
    if (!PyArg_ParseTuple(args, "s", &name)) return nullptr;
    self->model->setExpression(name); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_SetRandomExpression(PyLAppModelObject* self, PyObject*) {
    self->model->setRandomExpression(); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_StartMotion(PyLAppModelObject* self, PyObject* args, PyObject* kwargs) {
    const char* group; int no, priority;
    PyObject* onStart = nullptr; PyObject* onFinish = nullptr;
    static const char* kwlist[] = {"", "", "", "onStartMotionHandler", "onFinishMotionHandler", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "sii|OO", (char**)kwlist,
                                      &group, &no, &priority, &onStart, &onFinish))
        return nullptr;
    self->model->startMotion(group, no, priority);
    // Callbacks ignored for now (motion system is stubbed)
    (void)onStart; (void)onFinish;
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_StartRandomMotion(PyLAppModelObject* self, PyObject* args, PyObject* kwargs) {
    PyObject* nameObj = Py_None; PyObject* prioObj = Py_None;
    PyObject* onStart = nullptr; PyObject* onFinish = nullptr;
    static const char* kwlist[] = {"name", "priority", "onStartMotionHandler", "onFinishMotionHandler", nullptr};
    // Accept either: StartRandomMotion(priority) or StartRandomMotion(name, priority)
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|OOOO", (char**)kwlist,
                                      &nameObj, &prioObj, &onStart, &onFinish))
        return nullptr;

    int priority = 3; // default FORCE
    const char* group = nullptr;
    PyObject* utf8Ref = nullptr;

    if (prioObj != Py_None && PyLong_Check(prioObj)) {
        priority = (int)PyLong_AsLong(prioObj);
    }
    // If first arg is int, treat it as priority (no group name)
    if (nameObj != Py_None) {
        if (PyLong_Check(nameObj)) {
            priority = (int)PyLong_AsLong(nameObj);
        } else {
            utf8Ref = PyUnicode_AsUTF8String(nameObj);
            if (!utf8Ref) return nullptr;
            group = PyBytes_AsString(utf8Ref);
        }
    }
    if (group)
        self->model->startRandomMotion(group, priority);
    else
        self->model->startRandomMotion("", priority);
    Py_XDECREF(utf8Ref);
    (void)onStart; (void)onFinish;
    Py_RETURN_NONE;
}

static PyObject* PyLAppModel_GetCanvasWidth(PyLAppModelObject* self, PyObject*) {
    return PyFloat_FromDouble(self->model->getCanvasWidth());
}

static PyObject* PyLAppModel_GetCanvasHeight(PyLAppModelObject* self, PyObject*) {
    return PyFloat_FromDouble(self->model->getCanvasHeight());
}
static PyObject* PyLAppModel_GetCanvasSize(PyLAppModelObject* self, PyObject*) {
    return Py_BuildValue("(ff)", self->model->getCanvasWidth(), self->model->getCanvasHeight());
}

static PyObject* PyLAppModel_ClearMotions(PyLAppModelObject* self, PyObject*) {
    self->model->clearMotions(); Py_RETURN_NONE;
}
static PyObject* PyLAppModel_StopAllMotions(PyLAppModelObject* self, PyObject*) {
    self->model->stopAllMotions(); Py_RETURN_NONE;
}
static PyObject* PyLAppModel_ResetPose(PyLAppModelObject* self, PyObject*) {
    self->model->resetPose(); Py_RETURN_NONE;
}

static PyObject* PyLAppModel_ResetExpression(PyLAppModelObject* self, PyObject*) {
    self->model->resetExpression(); Py_RETURN_NONE;
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
    if (!PyArg_ParseTuple(args, "i", &index)) return nullptr;
    ensureParamClass();
    if (!sParamClass) Py_RETURN_NONE;
    PyObject* param = PyObject_CallObject(sParamClass, nullptr);
    if (!param) return nullptr;
    PyObject_SetAttrString(param, "id", PyUnicode_FromString(
        self->model->getParameterId(index).c_str()));
    PyObject_SetAttrString(param, "type", PyLong_FromLong(0));
    PyObject_SetAttrString(param, "value", PyFloat_FromDouble(
        self->model->getParameterValue(index)));
    PyObject_SetAttrString(param, "min", PyFloat_FromDouble(
        self->model->getParameterMin(index)));
    PyObject_SetAttrString(param, "max", PyFloat_FromDouble(
        self->model->getParameterMax(index)));
    PyObject_SetAttrString(param, "default", PyFloat_FromDouble(
        self->model->getParameterDefault(index)));
    return param;
}

static PyObject* PyLAppModel_HitPart(PyLAppModelObject* self, PyObject* args) {
    float x, y; int topOnly = 0;
    if (!PyArg_ParseTuple(args, "ff|p", &x, &y, &topOnly)) return nullptr;
    auto ids = self->model->hitPart(x, y, topOnly != 0);
    PyObject* lst = PyList_New(ids.size());
    for (size_t i = 0; i < ids.size(); i++)
        PyList_SetItem(lst, i, PyUnicode_FromString(ids[i].c_str()));
    return lst;
}

static PyObject* PyLAppModel_SetPartScreenColor(PyLAppModelObject* self, PyObject* args) {
    int idx; float r,g,b,a;
    if (!PyArg_ParseTuple(args, "iffff", &idx, &r,&g,&b,&a)) return nullptr;
    self->model->setPartScreenColor(idx, r, g, b, a); Py_RETURN_NONE;
}
static PyObject* PyLAppModel_GetPartScreenColor(PyLAppModelObject* self, PyObject* args) {
    int idx; if (!PyArg_ParseTuple(args, "i", &idx)) return nullptr;
    auto c = self->model->getPartScreenColor(idx);
    return Py_BuildValue("[ffff]", c[0], c[1], c[2], c[3]);
}
static PyObject* PyLAppModel_SetPartMultiplyColor(PyLAppModelObject* self, PyObject* args) {
    int idx; float r,g,b,a;
    if (!PyArg_ParseTuple(args, "iffff", &idx, &r,&g,&b,&a)) return nullptr;
    self->model->setPartMultiplyColor(idx, r, g, b, a); Py_RETURN_NONE;
}
static PyObject* PyLAppModel_GetPartMultiplyColor(PyLAppModelObject* self, PyObject* args) {
    int idx; if (!PyArg_ParseTuple(args, "i", &idx)) return nullptr;
    auto c = self->model->getPartMultiplyColor(idx);
    return Py_BuildValue("[ffff]", c[0], c[1], c[2], c[3]);
}
static PyObject* PyLAppModel_Rotate(PyLAppModelObject* self, PyObject* args) {
    float deg; if (!PyArg_ParseTuple(args, "f", &deg)) return nullptr;
    self->model->rotate(deg); Py_RETURN_NONE;
}
static PyObject* PyLAppModel_GetPixelsPerUnit(PyLAppModelObject* self, PyObject*) {
    return PyLong_FromLong(self->model->getPixelsPerUnit());
}
static PyObject* PyLAppModel_GetCanvasSizePixel(PyLAppModelObject* self, PyObject*) {
    return Py_BuildValue("(ff)", self->model->getCanvasWidth(), self->model->getCanvasHeight());
}

// --- autoBreath property ---
static PyObject* PyLAppModel_getAutoBreath(PyLAppModelObject* self, void*) {
    return PyBool_FromLong(self->model->mAutoBreath ? 1 : 0);
}
static int PyLAppModel_setAutoBreath(PyLAppModelObject* self, PyObject* value, void*) {
    if (!value) { PyErr_SetString(PyExc_TypeError, "Cannot delete attribute"); return -1; }
    int ok = PyObject_IsTrue(value);
    if (ok < 0) return -1;
    self->model->setAutoBreathEnable(ok);
    return 0;
}

// --- autoBlink property ---
static PyObject* PyLAppModel_getAutoBlink(PyLAppModelObject* self, void*) {
    return PyBool_FromLong(self->model->mAutoBlink ? 1 : 0);
}
static int PyLAppModel_setAutoBlink(PyLAppModelObject* self, PyObject* value, void*) {
    if (!value) { PyErr_SetString(PyExc_TypeError, "Cannot delete attribute"); return -1; }
    int ok = PyObject_IsTrue(value);
    if (ok < 0) return -1;
    self->model->setAutoBlinkEnable(ok);
    return 0;
}

static PyGetSetDef PyLAppModel_getset[] = {
    {"autoBreath", (getter)PyLAppModel_getAutoBreath, (setter)PyLAppModel_setAutoBreath, "", nullptr},
    {"autoBlink", (getter)PyLAppModel_getAutoBlink, (setter)PyLAppModel_setAutoBlink, "", nullptr},
    {nullptr}
};

PyMethodDef PyLAppModel_methods[] = {
    {"LoadModelJson", (PyCFunction)PyLAppModel_LoadModelJson, METH_VARARGS, ""},
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
    {"Update", (PyCFunction)PyLAppModel_Update, METH_NOARGS, ""},
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
    {"StartRandomMotion", (PyCFunction)PyLAppModel_StartRandomMotion, METH_VARARGS | METH_KEYWORDS, ""},
    {"GetCanvasWidth", (PyCFunction)PyLAppModel_GetCanvasWidth, METH_NOARGS, ""},
    {"GetCanvasSize", (PyCFunction)PyLAppModel_GetCanvasSize, METH_NOARGS, ""},
    {"GetCanvasWidth", (PyCFunction)PyLAppModel_GetCanvasWidth, METH_NOARGS, ""},
    {"GetCanvasHeight", (PyCFunction)PyLAppModel_GetCanvasHeight, METH_NOARGS, ""},
    {"ClearMotions", (PyCFunction)PyLAppModel_ClearMotions, METH_NOARGS, ""},
    {"StopAllMotions", (PyCFunction)PyLAppModel_StopAllMotions, METH_NOARGS, ""},
    {"ResetPose", (PyCFunction)PyLAppModel_ResetPose, METH_NOARGS, ""},
    {"ResetExpression", (PyCFunction)PyLAppModel_ResetExpression, METH_NOARGS, ""},
    {NULL, NULL, 0, NULL}
};

PyType_Slot PyLAppModel_slots[] = {
    {Py_tp_new, (void*)PyLAppModel_new},
    {Py_tp_init, (void*)PyLAppModel_init},
    {Py_tp_dealloc, (void*)PyLAppModel_dealloc},
    {Py_tp_methods, (void*)PyLAppModel_methods},
    {Py_tp_getset, (void*)PyLAppModel_getset},
    {0, nullptr}
};
