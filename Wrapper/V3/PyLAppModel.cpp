#include "PyLAppModel.hpp"

#include <Log.hpp>
#include <unordered_map>
#include <mutex>
#include <chrono>

// LAppModel()
static int PyLAppModel_init(PyLAppModelObject *self, PyObject *args, PyObject *kwds)
{
    self->model = new LAppModel();
    // 结构体绕过了构造函数，
    // 其底层char数组指针可能未指向可用空间，导致访问出错
    new (&self->lastExpression) std::string("");
    self->expStartedAt = -1;
    self->fadeout = -1;
    Info("[M] allocate cpp LAppModel(at=%p)", self->model);
    return 0;
}

static void PyLAppModel_dealloc(PyLAppModelObject *self)
{
    Info("[M] deallocate: cpp LAppModel(at=%p)", self->model);
    self->lastExpression.~basic_string();
    delete self->model;
    Info("[M] deallocate: PyLAppModelObject(at=%p)", self);
    PyObject_Free(self);
}

// LAppModel->LoadAssets
static PyObject *PyLAppModel_LoadModelJson(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    const char *fileName;
    int maskBufferCount = 2;
    static char* kwlist[] = {(char*)"modelJsonPath", (char*)"maskBufferCount", nullptr};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|i", kwlist, &fileName, &maskBufferCount))
    {
        return NULL;
    }

    self->model->LoadModelJson(fileName, maskBufferCount);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_Resize(PyLAppModelObject *self, PyObject *args)
{
    int ww, wh;
    if (!PyArg_ParseTuple(args, "ii", &ww, &wh))
    {
        PyErr_SetString(PyExc_TypeError, "invalid params.");
        return NULL;
    }

    self->model->Resize(ww, wh);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_Draw(PyLAppModelObject *self, PyObject *args)
{
    self->model->Draw();
    Py_RETURN_NONE;
}

typedef Live2D::Cubism::Framework::ACubismMotion ACubismMotion;

void OnMotionStartedCallback(ACubismMotion *motion)
{
    void *callee = motion->GetBeganMotionCustomData();
    if (callee == nullptr)
    {
        return;
    }
    PyGILState_STATE state = PyGILState_Ensure();
    PyObject *s_call = (PyObject *)callee;
    PyObject *result = PyObject_CallFunction(s_call, "si", motion->group.c_str(), motion->no);
    if (result != nullptr)
        Py_XDECREF(result);
    Py_XDECREF(s_call);
    PyGILState_Release(state);
}

void OnMotionFinishedCallback(ACubismMotion *motion)
{
    void *callee = motion->GetFinishedMotionCustomData();
    if (callee == nullptr)
    {
        return;
    }
    PyGILState_STATE state = PyGILState_Ensure();
    PyObject *f_call = (PyObject *)callee;
    PyObject *result = PyObject_CallFunction(f_call, nullptr);
    if (result != nullptr)
        Py_XDECREF(result);
    Py_XDECREF(f_call);
    PyGILState_Release(state);
}

static PyObject *MakeCallee(PyObject *callback)
{
    if (callback == nullptr)
        return nullptr;

    if (Py_IsNone(callback))
    {
        return nullptr;
    }

    if (!PyCallable_Check(callback))
    {
        PyErr_SetString(PyExc_TypeError, "handler must be callable or None");
        return NULL;
    }

    Py_XINCREF(callback);

    return callback;
}

static PyObject *PyLAppModel_StartMotion(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    const char *group;
    int no, priority;
    PyObject *onStartHandler = nullptr;
    PyObject *onFinishHandler = nullptr;

    static char *kwlist[] = {
        (char *)"group", (char *)"no", (char *)"priority", (char *)"onStartMotionHandler", (char *)"onFinishMotionHandler",
        NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "sii|OO", kwlist, &group, &no, &priority, &onStartHandler,
                                      &onFinishHandler)))
    {
        return NULL;
    }

    Csm::CubismMotionQueueEntryHandle _ = self->model->StartMotion(group, no, priority,
                                                                   MakeCallee(onStartHandler),
                                                                   OnMotionStartedCallback,
                                                                   MakeCallee(onFinishHandler),
                                                                   OnMotionFinishedCallback);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_StartRandomMotion(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    const char *group = nullptr;
    int priority = 3;

    PyObject *onStartHandler = nullptr;
    PyObject *onFinishHandler = nullptr;

    static char *kwlist[] = {
        (char *)"group", (char *)"priority", (char *)"onStartMotionHandler", (char *)"onFinishMotionHandler", NULL};
    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|siOO", kwlist, &group, &priority, &onStartHandler,
                                      &onFinishHandler)))
    {
        return NULL;
    }

    self->model->StartRandomMotion(group, priority,
                                   MakeCallee(onStartHandler),
                                   OnMotionStartedCallback,
                                   MakeCallee(onFinishHandler),
                                   OnMotionFinishedCallback);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetExpression(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    const char *expressionID;
    int fadeout = -1;

    static char *kwlist[] = {
        (char *)"expressionId", (char *)"fadeout", NULL};

    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "s|i", kwlist, &expressionID, &fadeout)))
    {
        return NULL;
    }

    if (fadeout >= 0)
    {
        auto now = std::chrono::system_clock::now();
        self->expStartedAt = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    }
    else
    {
        self->lastExpression = std::string(expressionID);
        Info("set default expression: %s", expressionID);
    }

    self->fadeout = fadeout;
    self->model->SetExpression(expressionID);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_ResetExpression(PyLAppModelObject *self, PyObject *args)
{
    self->fadeout = -1;
    self->expStartedAt = -1;
    if (self->lastExpression != "")
    {
        self->lastExpression = "";
    }

    self->model->ResetExpression();
    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetRandomExpression(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    int fadeout = -1;
    char *kwlist[] = {(char *)"fadeout", NULL};

    if (!(PyArg_ParseTupleAndKeywords(args, kwargs, "|i", kwlist, &fadeout)))
    {
        return NULL;
    }

    self->fadeout = fadeout;

    const std::string expId = self->model->SetRandomExpression();

    PyObject *pyExpIdStr = Py_BuildValue("s", expId.c_str());

    if (self->fadeout >= 0)
    {
        auto now = std::chrono::system_clock::now();
        self->expStartedAt = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
    }
    else
    {
        self->lastExpression = expId;
        Info("set default expression: %s", expId.c_str());
    }

    return pyExpIdStr;
}

typedef Live2D::Cubism::Framework::csmString csmString;

static PyObject *PyLAppModel_HitTest(PyLAppModelObject *self, PyObject *args)
{
    const char *hitAreaName;
    float x, y;
    if (!(PyArg_ParseTuple(args, "sff", &hitAreaName, &x, &y)))
    {
        return NULL;
    }

    if (self->model->HitTest(hitAreaName, x, y))
    {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}

static PyObject *PyLAppModel_HasMocConsistencyFromFile(PyLAppModelObject *self, PyObject *args)
{
    const char *mocFileName;
    if (!(PyArg_ParseTuple(args, "s", &mocFileName)))
    {
        return NULL;
    }

    bool result = self->model->HasMocConsistencyFromFile(mocFileName);

    if (result)
        Py_RETURN_TRUE;

    Py_RETURN_FALSE;
}

static PyObject *PyLAppModel_Drag(PyLAppModelObject *self, PyObject *args)
{
    float mx, my;
    if (!(PyArg_ParseTuple(args, "ff", &mx, &my)))
    {
        return NULL;
    }

    self->model->Drag(mx, my);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_IsMotionFinished(PyLAppModelObject *self, PyObject *args)
{
    if (self->model->IsMotionFinished())
    {
        Py_RETURN_TRUE;
    }

    Py_RETURN_FALSE;
}

static PyObject *PyLAppModel_SetOffset(PyLAppModelObject *self, PyObject *args)
{
    float dx, dy;

    if (PyArg_ParseTuple(args, "ff", &dx, &dy) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Missing param 'float dx, float dy'");
        return NULL;
    }

    self->model->SetOffset(dx, dy);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetScale(PyLAppModelObject *self, PyObject *args)
{
    float scale;

    if (PyArg_ParseTuple(args, "f", &scale) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Missing param 'float scale'");
        return NULL;
    }

    self->model->SetScale(scale);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetScaleX(PyLAppModelObject *self, PyObject *args)
{
    float scale;

    if (PyArg_ParseTuple(args, "f", &scale) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Missing param 'float scale'");
        return NULL;
    }

    self->model->SetScaleX(scale);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetScaleY(PyLAppModelObject *self, PyObject *args)
{
    float scale;

    if (PyArg_ParseTuple(args, "f", &scale) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Missing param 'float scale'");
        return NULL;
    }

    self->model->SetScaleY(scale);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_Rotate(PyLAppModelObject *self, PyObject *args)
{
    float deg;

    if (PyArg_ParseTuple(args, "f", &deg) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Missing param 'float deg'");
        return NULL;
    }

    self->model->Rotate(deg);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetParameterValue(PyLAppModelObject *self, PyObject *args)
{
    const char *paramId;
    float value, weight = 1.0f;

    if (PyArg_ParseTuple(args, "sf|f", &paramId, &value, &weight) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid params (str, float, float)");
        return NULL;
    }

    self->model->SetParameterValue(paramId, value, weight);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetIndexParamValue(PyLAppModelObject *self, PyObject *args)
{
    int index;
    float value, weight = 1.0f;

    if (PyArg_ParseTuple(args, "if|f", &index, &value, &weight) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid params (int, float, float)");
        return NULL;
    }

    self->model->SetIndexParamValue(index, value, weight);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_AddParameterValue(PyLAppModelObject *self, PyObject *args)
{
    const char *paramId;
    float value;

    if (PyArg_ParseTuple(args, "sf", &paramId, &value) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid params (str, float)");
        return NULL;
    }

    self->model->AddParameterValue(paramId, value);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_AddIndexParamValue(PyLAppModelObject *self, PyObject *args)
{
    int index;
    float value;

    if (PyArg_ParseTuple(args, "sf", &index, &value) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid params (str, float)");
        return NULL;
    }

    self->model->AddIndexParamValue(index, value);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_Update(PyLAppModelObject *self, PyObject *args)
{
    if (self->fadeout >= 0)
    {
        auto now = std::chrono::system_clock::now();
        auto value = std::chrono::time_point_cast<std::chrono::milliseconds>(now).time_since_epoch().count();
        time_t elapsed = value - self->expStartedAt;
        if (elapsed >= self->fadeout)
        {
            if (self->lastExpression != "")
            {
                self->model->SetExpression(self->lastExpression.c_str());
                Info("reset expression %s", self->lastExpression.c_str());
            }
            else
            {
                self->model->ResetExpression();
                Info("clear expression");
            }
            self->fadeout = -1;
        }
    }

    self->model->Update();

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetAutoBreathEnable(PyLAppModelObject *self, PyObject *args)
{
    bool enable;

    if (PyArg_ParseTuple(args, "b", &enable) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    self->model->SetAutoBreathEnable(enable);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetAutoBlinkEnable(PyLAppModelObject *self, PyObject *args)
{
    bool enable;

    if (PyArg_ParseTuple(args, "b", &enable) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    self->model->SetAutoBlinkEnable(enable);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_GetParameterCount(PyLAppModelObject *self, PyObject *args)
{
    return PyLong_FromLong(self->model->GetParameterCount());
}

extern PyObject *typeobject_live2d_v3_parameter;

static PyObject *CreatePyParameter(const char *id, int type, float value, float maxValue, float minValue,
                                   float defaultValue)
{
    PyObject *instance = PyObject_CallObject(typeobject_live2d_v3_parameter, NULL);
    if (instance == NULL)
    {
        PyErr_Print();
        return NULL;
    }
    PyObject *py_id = PyUnicode_FromString(id);
    PyObject *py_type = PyLong_FromLong(type);
    PyObject *py_val = PyLong_FromLong(value);
    PyObject *py_max = PyLong_FromLong(maxValue);
    PyObject *py_min = PyLong_FromLong(minValue);
    PyObject *py_def = PyFloat_FromDouble(defaultValue);

    PyObject_SetAttrString(instance, "id", py_id);
    PyObject_SetAttrString(instance, "type", py_type);
    PyObject_SetAttrString(instance, "value", py_val);
    PyObject_SetAttrString(instance, "max", py_max);
    PyObject_SetAttrString(instance, "min", py_min);
    PyObject_SetAttrString(instance, "default", py_def);

    Py_DECREF(py_id);
    Py_DECREF(py_type);
    Py_DECREF(py_val);
    Py_DECREF(py_max);
    Py_DECREF(py_min);
    Py_DECREF(py_def);
    return instance;
}

static PyObject *PyLAppModel_GetParameter(PyLAppModelObject *self, PyObject *args)
{
    int index;
    if (PyArg_ParseTuple(args, "i", &index) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    const char *id;
    int type;
    float value, maxValue, minValue, defaultValue;
    self->model->GetParameter(index, id, type, value, maxValue, minValue, defaultValue);

    return CreatePyParameter(id, type, value, maxValue, minValue, defaultValue);
}

static PyObject *PyLAppModel_GetParamIds(PyLAppModelObject *self, PyObject *args)
{
    const int size = self->model->GetParameterCount();
    PyObject *list = PyList_New(size);
    const char *id;
    int type;
    float value, maxValue, minValue, defaultValue;
    for (int i = 0; i < size; i++)
    {
        self->model->GetParameter(i, id, type, value, maxValue, minValue, defaultValue);
        PyObject *str = Py_BuildValue("s", id);
        PyList_SetItem(list, i, str);
    }
    return list;
}

static PyObject *PyLAppModel_GetParameterValue(PyLAppModelObject *self, PyObject *args)
{
    int index;
    if (PyArg_ParseTuple(args, "i", &index) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    return PyFloat_FromDouble(self->model->GetParameterValue(index));
}

// GetPartCount() -> int
static PyObject *PyLAppModel_GetPartCount(PyLAppModelObject *self, PyObject *args)
{
    return PyLong_FromLong(self->model->GetPartCount());
}

// GetPartId(index: int) -> str
static PyObject *PyLAppModel_GetPartId(PyLAppModelObject *self, PyObject *args)
{
    int index;
    if (PyArg_ParseTuple(args, "i", &index) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    return PyUnicode_FromString(self->model->GetPartId(index).GetRawString());
}

// GetPartIds() -> tuple[str]
static PyObject *PyLAppModel_GetPartIds(PyLAppModelObject *self, PyObject *args)
{
    const int size = self->model->GetPartCount();

    PyObject *list = PyList_New(size);

    for (int i = 0; i < size; ++i)
    {
        PyList_SetItem(list, i, PyUnicode_FromString(self->model->GetPartId(i).GetRawString()));
    }

    return list;
}

// SetPartOpacity(id: str, opacity: float) -> None
static PyObject *PyLAppModel_SetPartOpacity(PyLAppModelObject *self, PyObject *args)
{
    int index;
    float opacity;
    if (PyArg_ParseTuple(args, "if", &index, &opacity) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    self->model->SetPartOpacity(index, opacity);
    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_HitPart(PyLAppModelObject *self, PyObject *args)
{
    float x, y;
    bool topOnly = false;
    if (PyArg_ParseTuple(args, "ff|b", &x, &y, &topOnly) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    PyObject *list = PyList_New(0);
    self->model->HitPart(x, y, topOnly, list, [](void *collector, const char *paramId)
                         { PyList_Append((PyObject *)collector, PyUnicode_FromString(paramId)); });

    return list;
}

static PyObject *PyLAppModel_SetPartMultiplyColor(PyLAppModelObject *self, PyObject *args)
{
    int index;
    float r, g, b, a;
    if (PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }
    self->model->SetPartMultiplyColor(index, r, g, b, a);
    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_GetPartMultiplyColor(PyLAppModelObject *self, PyObject *args)
{
    int index;
    if (PyArg_ParseTuple(args, "i", &index) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    float r, g, b, a;
    self->model->GetPartMultiplyColor(index, r, g, b, a);
    return Py_BuildValue("ffff", r, g, b, a);
}

static PyObject *PyLAppModel_SetPartScreenColor(PyLAppModelObject *self, PyObject *args)
{
    int index;
    float r, g, b, a;
    if (PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }
    self->model->SetPartScreenColor(index, r, g, b, a);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_GetPartScreenColor(PyLAppModelObject *self, PyObject *args)
{
    int index;

    if (PyArg_ParseTuple(args, "i", &index) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }

    float r, g, b, a;
    self->model->GetPartScreenColor(index, r, g, b, a);

    return Py_BuildValue("ffff", r, g, b, a);
}

static PyObject *PyLAppModel_GetDrawableIds(PyLAppModelObject *self, PyObject *args)
{
    const int size = self->model->GetDrawableCount();
    PyObject *list = PyList_New(size);
    int index = 0;
    void *collector[2] = {list, &index};
    self->model->GetDrawableIds(collector,
                                [](void *collector, const char *id)
                                {
                                    PyObject *list = (PyObject *)(((void **)collector)[0]);
                                    int *index = (int *)(((void **)collector)[1]);
                                    PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
                                });
    return list;
}

static PyObject *PyLAppModel_SetDrawableMultiplyColor(PyLAppModelObject *self, PyObject *args)
{
    int index;
    float r, g, b, a;
    if (PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }
    self->model->SetDrawableMultiplyColor(index, r, g, b, a);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_SetDrawableScreenColor(PyLAppModelObject *self, PyObject *args)
{
    int index;
    float r, g, b, a;
    if (PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }
    self->model->SetDrawableScreenColor(index, r, g, b, a);

    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_StopAllMotions(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    self->model->StopAllMotions();
    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_ResetParameters(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    self->model->ResetParameters();
    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_ResetPose(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    self->model->ResetPose();
    Py_RETURN_NONE;
}

static PyObject *PyLAppModel_GetExpressionIds(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *list = PyList_New(0);
    self->model->GetExpressionIds(list, [](void *collector, const char *expId)
                                  {
        PyObject* list = (PyObject*) collector;
        PyList_Append(list, Py_BuildValue("s", expId)); });
    return list;
}

static PyObject *PyLAppModel_GetMotionGroups(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *dict = PyDict_New();
    self->model->GetMotionGroups(dict, [](void *collector, const char *group, int count)
                                 {
        PyObject* dict = (PyObject*) collector;
        PyDict_SetItem(dict, Py_BuildValue("s", group), Py_BuildValue("i", count)); });
    return dict;
}

static PyObject *PyLAppModel_GetSoundPath(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    const char *group;
    int index;
    if (PyArg_ParseTuple(args, "si", &group, &index) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "Invalid param");
        return NULL;
    }
    return Py_BuildValue("s", self->model->GetSoundPath(group, index));
}

static PyObject *PyLAppModel_GetCanvasSize(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    float w, h;
    self->model->GetCanvasSize(w, h);
    return Py_BuildValue("ff", w, h);
}

static PyObject *PyLAppModel_GetCanvasSizePixel(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    float w, h;
    self->model->GetCanvasSizePixel(w, h);
    return Py_BuildValue("ff", w, h);
}

static PyObject *PyLAppModel_GetPixelsPerUnit(PyLAppModelObject *self, PyObject *args, PyObject *kwargs)
{
    return Py_BuildValue("f", self->model->GetPixelsPerUnit());
}

static PyObject *PyLAppModel_AddExpression(PyLAppModelObject* self, PyObject *args, PyObject* kwargs)
{
    const char* expId = nullptr;
    if (PyArg_ParseTuple(args, "s", &expId) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "argument must be (str)");
        return nullptr;
    }
    
    self->model->AddExpression(expId);
    Py_RETURN_NONE;
}
static PyObject *PyLAppModel_RemoveExpression(PyLAppModelObject* self, PyObject *args, PyObject* kwargs)
{
    const char* expId = nullptr;
    if (PyArg_ParseTuple(args, "s", &expId) < 0)
    {
        PyErr_SetString(PyExc_TypeError, "argument must be (str)");
        return nullptr;
    }
    
    self->model->RemoveExpression(expId);

    Py_RETURN_NONE;
}
static PyObject *PyLAppModel_ResetExpressions(PyLAppModelObject* self, PyObject *args, PyObject* kwargs)
{
    self->fadeout = -1;
    self->expStartedAt = -1;
    if (!self->lastExpression.empty())
    {
        self->lastExpression = "";
    }

    self->model->ResetExpressions();
    Py_RETURN_NONE;
}

// 包装模块方法的方法列表
static PyMethodDef PyLAppModel_methods[] = {
    {"LoadModelJson", (PyCFunction)PyLAppModel_LoadModelJson, METH_VARARGS | METH_KEYWORDS, ""},
    {"Resize", (PyCFunction)PyLAppModel_Resize, METH_VARARGS, ""},
    {"Draw", (PyCFunction)PyLAppModel_Draw, METH_VARARGS, ""},
    {"StartMotion", (PyCFunction)PyLAppModel_StartMotion, METH_VARARGS | METH_KEYWORDS, ""},
    {"StartRandomMotion", (PyCFunction)PyLAppModel_StartRandomMotion, METH_VARARGS | METH_KEYWORDS, ""},

    {"SetExpression", (PyCFunction)PyLAppModel_SetExpression, METH_VARARGS | METH_KEYWORDS, ""},
    {"SetRandomExpression", (PyCFunction)PyLAppModel_SetRandomExpression, METH_VARARGS | METH_KEYWORDS, ""},
    {"ResetExpression", (PyCFunction)PyLAppModel_ResetExpression, METH_VARARGS, ""},

    {"HitTest", (PyCFunction)PyLAppModel_HitTest, METH_VARARGS, "Get the name of the area being hit."},
    {"HasMocConsistencyFromFile", (PyCFunction)PyLAppModel_HasMocConsistencyFromFile, METH_VARARGS, ""},
    {"Drag", (PyCFunction)PyLAppModel_Drag, METH_VARARGS, ""},
    {"IsMotionFinished", (PyCFunction)PyLAppModel_IsMotionFinished, METH_VARARGS, ""},
    {"SetOffset", (PyCFunction)PyLAppModel_SetOffset, METH_VARARGS, ""},
    {"SetScale", (PyCFunction)PyLAppModel_SetScale, METH_VARARGS, ""},
    {"SetScaleX", (PyCFunction)PyLAppModel_SetScaleX, METH_VARARGS, ""},
    {"SetScaleY", (PyCFunction)PyLAppModel_SetScaleY, METH_VARARGS, ""},
    {"Rotate", (PyCFunction)PyLAppModel_Rotate, METH_VARARGS, ""},
    {"Update", (PyCFunction)PyLAppModel_Update, METH_VARARGS, ""},

    {"SetAutoBreathEnable", (PyCFunction)PyLAppModel_SetAutoBreathEnable, METH_VARARGS, ""},
    {"SetAutoBlinkEnable", (PyCFunction)PyLAppModel_SetAutoBlinkEnable, METH_VARARGS, ""},

    {"SetParameterValue", (PyCFunction)PyLAppModel_SetParameterValue, METH_VARARGS, ""},
    {"SetIndexParamValue", (PyCFunction)PyLAppModel_SetIndexParamValue, METH_VARARGS, ""},
    {"AddParameterValue", (PyCFunction)PyLAppModel_AddParameterValue, METH_VARARGS, ""},
    {"AddIndexParamValue", (PyCFunction)PyLAppModel_AddIndexParamValue, METH_VARARGS, ""},
    {"GetParameterCount", (PyCFunction)PyLAppModel_GetParameterCount, METH_VARARGS, ""},
    {"GetParameter", (PyCFunction)PyLAppModel_GetParameter, METH_VARARGS, ""},
    {"GetParamIds", (PyCFunction)PyLAppModel_GetParamIds, METH_VARARGS, ""},
    {"GetParameterValue", (PyCFunction)PyLAppModel_GetParameterValue, METH_VARARGS, ""},

    {"GetPartCount", (PyCFunction)PyLAppModel_GetPartCount, METH_VARARGS, ""},
    {"GetPartId", (PyCFunction)PyLAppModel_GetPartId, METH_VARARGS, ""},
    {"GetPartIds", (PyCFunction)PyLAppModel_GetPartIds, METH_VARARGS, ""},
    {"SetPartOpacity", (PyCFunction)PyLAppModel_SetPartOpacity, METH_VARARGS, ""},
    {"HitPart", (PyCFunction)PyLAppModel_HitPart, METH_VARARGS, ""},

    {"SetPartMultiplyColor", (PyCFunction)PyLAppModel_SetPartMultiplyColor, METH_VARARGS, ""},
    {"GetPartMultiplyColor", (PyCFunction)PyLAppModel_GetPartMultiplyColor, METH_VARARGS, ""},

    {"SetPartScreenColor", (PyCFunction)PyLAppModel_SetPartScreenColor, METH_VARARGS, ""},
    {"GetPartScreenColor", (PyCFunction)PyLAppModel_GetPartScreenColor, METH_VARARGS, ""},

    {"GetDrawableIds", (PyCFunction)PyLAppModel_GetDrawableIds, METH_VARARGS, ""},
    {"SetDrawableMultiplyColor", (PyCFunction)PyLAppModel_SetDrawableMultiplyColor, METH_VARARGS, ""},
    {"SetDrawableScreenColor", (PyCFunction)PyLAppModel_SetDrawableScreenColor, METH_VARARGS, ""},

    // 复位
    {"StopAllMotions", (PyCFunction)PyLAppModel_StopAllMotions, METH_VARARGS | METH_KEYWORDS, ""},
    {"ResetParameters", (PyCFunction)PyLAppModel_ResetParameters, METH_VARARGS | METH_KEYWORDS, ""},
    {"ResetPose", (PyCFunction)PyLAppModel_ResetPose, METH_VARARGS | METH_KEYWORDS, ""},

    {"GetExpressionIds", (PyCFunction)PyLAppModel_GetExpressionIds, METH_VARARGS | METH_KEYWORDS, ""},
    {"GetMotionGroups", (PyCFunction)PyLAppModel_GetMotionGroups, METH_VARARGS | METH_KEYWORDS, ""},

    {"GetSoundPath", (PyCFunction)PyLAppModel_GetSoundPath, METH_VARARGS | METH_KEYWORDS, ""},

    {"GetCanvasSize", (PyCFunction)PyLAppModel_GetCanvasSize, METH_VARARGS, ""},
    {"GetCanvasSizePixel", (PyCFunction)PyLAppModel_GetCanvasSizePixel, METH_VARARGS, ""},
    {"GetPixelsPerUnit", (PyCFunction)PyLAppModel_GetPixelsPerUnit, METH_VARARGS, ""},

    {"AddExpression", (PyCFunction)PyLAppModel_AddExpression, METH_VARARGS, ""},
    {"RemoveExpression", (PyCFunction)PyLAppModel_RemoveExpression, METH_VARARGS, ""},
    {"ResetExpressions", (PyCFunction)PyLAppModel_ResetExpressions, METH_VARARGS, ""},

    {NULL} // 方法列表结束的标志
};

static PyObject *PyLAppModel_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    PyObject *self = (PyObject *)PyObject_Malloc(sizeof(PyLAppModelObject));
    PyObject_Init(self, type);
    return self;
}

static PyType_Slot PyLAppModel_slots[] = {
    {Py_tp_new, (void *)PyLAppModel_new},
    {Py_tp_init, (void *)PyLAppModel_init},
    {Py_tp_dealloc, (void *)PyLAppModel_dealloc},
    {Py_tp_methods, (void *)PyLAppModel_methods},
    {0, NULL}};

PyType_Spec PyLAppModel_spec = {
    "live2d.LAppModel",
    sizeof(PyLAppModelObject),
    0,
    Py_TPFLAGS_DEFAULT,
    PyLAppModel_slots,
};
