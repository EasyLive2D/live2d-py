#include "PyModel.hpp"

#include <Log.hpp>
#include <pytypedefs.h>

static void MotionStartCallback(ACubismMotion *motion)
{
	void* callee = motion->GetBeganMotionCustomData();
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

static void MotionFinishCallback(ACubismMotion *motion)
{
	void* callee = motion->GetFinishedMotionCustomData();
	if (callee == nullptr)
	{
		return;
	}
	PyGILState_STATE state = PyGILState_Ensure();
	PyObject *f_call = (PyObject *)callee;
	PyObject *result = PyObject_CallFunction(f_call, "si", motion->group.c_str(), motion->no);
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

static PyObject *PyModel_Init(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model = new Model();
	Info("allocate: cpp Model(at=%p)", self->model);
	return 0;
}
static void PyModel_Dealloc(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	delete self->model;
	Info("deallocate: cpp Model(at=%p)", self->model);
	Info("deallocate: PyModelObject(at=%p)", self);
	PyObject_Free(self);
}
static PyObject *PyModel_LoadModelJson(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *modelJsonPath;
	if (!PyArg_ParseTuple(args, "s", &modelJsonPath))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be str");
		return NULL;
	}

	self->model->LoadModelJson(modelJsonPath);
	Py_RETURN_NONE;
}
static PyObject *PyModel_GetModelHomeDir(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	return Py_BuildValue("s", self->model->GetModelHomeDir());
}

static PyObject *PyModel_Update(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->Update(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_UpdateMotion(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->UpdateMotion(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_UpdateDrag(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->UpdateDrag(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_UpdateBreath(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->UpdateBreath(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_UpdateBlink(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->UpdateBlink(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_UpdateExpression(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->UpdateExpression(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_UpdatePhysics(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->UpdatePhysics(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_UpdatePose(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float deltaTimeSeconds;
	if (!PyArg_ParseTuple(args, "f", &deltaTimeSeconds))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be float");
		return NULL;
	}
	self->model->UpdatePose(deltaTimeSeconds);
	Py_RETURN_NONE;
}
static PyObject *PyModel_GetParameterIds(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const int count = self->model->GetParameterCount();
	PyObject *list = PyList_New(count);
	int index = 0;
	void *collector[2] = {list, &index};
	self->model->GetParameterIds(collector,
								 [](void *collector, const char *id)
								 {
									 PyObject *list = (PyObject *)(((void **)collector)[0]);
									 int *index = (int *)(((void **)collector)[1]);
									 PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
								 });
	return list;
}
static PyObject *PyModel_GetParameterValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
		return NULL;
	}
	return PyFloat_FromDouble(self->model->GetParameterValue(index));
}
static PyObject *PyModel_GetParameterMaximumValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
		return NULL;
	}
	return PyFloat_FromDouble(self->model->GetParameterMaximumValue(index));
}
static PyObject *PyModel_GetParameterMinimumValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
		return NULL;
	}
	return PyFloat_FromDouble(self->model->GetParameterMinimumValue(index));
}
static PyObject *PyModel_GetParameterDefaultValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	if (!PyArg_ParseTuple(args, "i", &index))
	{
		PyErr_SetString(PyExc_TypeError, "argument 1 must be int");
		return NULL;
	}
	return PyFloat_FromDouble(self->model->GetParameterDefaultValue(index));
}
static PyObject *PyModel_SetParameterValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float value;
	float weight = 1.0f;
	if (!PyArg_ParseTuple(args, "if|f", &index, &value, &weight))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, [float])");
		return NULL;
	}
	self->model->SetParameterValue(index, value, weight);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetParameterValueById(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *id;
	float value, weight = 1.0f;
	if (!PyArg_ParseTuple(args, "sf|f", &id, &value, &weight))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, float, [float])");
		return NULL;
	}
	self->model->SetParameterValue(id, value, weight);
	Py_RETURN_NONE;
}
static PyObject *PyModel_AddParameterValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float value;
	if (!PyArg_ParseTuple(args, "if", &index, &value))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float)");
		return NULL;
	}

	self->model->AddParameterValue(index, value);
	Py_RETURN_NONE;
}
static PyObject *PyModel_AddParameterValueById(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *id;
	float value;
	if (!PyArg_ParseTuple(args, "sf", &id, &value))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, float)");
		return NULL;
	}
	self->model->AddParameterValue(id, value);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetAndSaveParameterValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float value;
	float weight = 1.0f;
	if (!PyArg_ParseTuple(args, "if|f", &index, &value, &weight))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, [float])");
		return NULL;
	}
	self->model->SetAndSaveParameterValue(index, value, weight);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetAndSaveParameterValueById(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *id;
	float value, weight = 1.0f;
	if (!PyArg_ParseTuple(args, "sf|f", &id, &value, &weight))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, float, [float])");
		return NULL;
	}
	self->model->SetAndSaveParameterValue(id, value, weight);
	Py_RETURN_NONE;
}
static PyObject *PyModel_AddAndSaveParameterValue(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float value;
	if (!PyArg_ParseTuple(args, "if", &index, &value))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float)");
		return NULL;
	}

	self->model->AddAndSaveParameterValue(index, value);
	Py_RETURN_NONE;
}
static PyObject *PyModel_AddAndSaveParameterValueById(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *id;
	float value;
	if (!PyArg_ParseTuple(args, "sf", &id, &value))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, float)");
		return NULL;
	}
	self->model->AddAndSaveParameterValue(id, value);
	Py_RETURN_NONE;
}

static PyObject *PyModel_LoadParameters(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->LoadParameters();
	Py_RETURN_NONE;
}
static PyObject *PyModel_SaveParameters(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->SaveParameters();
	Py_RETURN_NONE;
}
static PyObject *PyModel_Resize(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int width, height;
	if (!PyArg_ParseTuple(args, "ii", &width, &height))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, int)");
		return NULL;
	}
	self->model->Resize(width, height);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetOffset(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float x, y;
	if (!PyArg_ParseTuple(args, "ff", &x, &y))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (float, float)");
		return NULL;
	}
	self->model->SetOffset(x, y);
	Py_RETURN_NONE;
}
static PyObject *PyModel_Rotate(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float degrees;
	if (!PyArg_ParseTuple(args, "f", &degrees))
	{
		PyErr_SetString(PyExc_TypeError, "argument must be float");
		return NULL;
	}
	self->model->Rotate(degrees);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetScale(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float scale;
	if (!PyArg_ParseTuple(args, "f", &scale))
	{
		PyErr_SetString(PyExc_TypeError, "argument must be float");
		return NULL;
	}
	self->model->SetScale(scale);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetScaleX(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float scale;
	if (!PyArg_ParseTuple(args, "f", &scale))
	{
		PyErr_SetString(PyExc_TypeError, "argument must be float");
		return NULL;
	}
	self->model->SetScaleX(scale);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetScaleY(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float scale;
	if (!PyArg_ParseTuple(args, "f", &scale))
	{
		PyErr_SetString(PyExc_TypeError, "argument must be float");
		return NULL;
	}
	self->model->SetScaleY(scale);
	Py_RETURN_NONE;
}
static PyObject *PyModel_GetMvp(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *mvp = PyTuple_New(16);
	const float *mvpArray = self->model->GetMvp();
	for (int i = 0; i < 16; i++)
	{
		PyTuple_SetItem(mvp, i, PyFloat_FromDouble(mvpArray[i]));
	}
	return mvp;
}
static PyObject *PyModel_StartMotion(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *group;
	int no, priority = 3;
	PyObject *onStartHandler = nullptr;
	PyObject *onFinishHandler = nullptr;
	static char *kwlist[] = {
		(char *)"group", (char *)"no", (char *)"priority", (char *)"onStart", (char *)"onFinish",
		NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "si|iOO", kwlist, &group, &no, &priority, &onStartHandler, &onFinishHandler))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, int, [int, [callable, callable]])");
		return NULL;
	}
	self->model->StartMotion(group, no, priority,
							 MakeCallee(onStartHandler), MotionStartCallback,
							 MakeCallee(onFinishHandler), MotionFinishCallback);
	Py_RETURN_NONE;
}
static PyObject *PyModel_StartRandomMotion(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *group = nullptr;
	int priority = 3;

	PyObject *onStartHandler = nullptr;
	PyObject *onFinishHandler = nullptr;
	static char *kwlist[] = {(char *)"group", (char *)"priority", (char *)"onStart", (char *)"onFinish", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|siOO", kwlist, &group, &priority, &onStartHandler, &onFinishHandler))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, [int, [callable, callable]])");
		return NULL;
	}

	self->model->StartRandomMotion(group, priority,
								   MakeCallee(onStartHandler), MotionStartCallback,
								   MakeCallee(onFinishHandler), MotionFinishCallback);
	Py_RETURN_NONE;
}
static PyObject *PyModel_IsMotionFinished(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	if (self->model->IsMotionFinished())
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
static PyObject *PyModel_LoadExtraMotion(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *group, *motionJsonPath;
	if (!PyArg_ParseTuple(args, "ss", &group, &motionJsonPath))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, str)");
		return NULL;
	}

	const int no = self->model->LoadExtraMotion(group, motionJsonPath);
	return Py_BuildValue("i", no);
}
static PyObject *PyModel_GetMotions(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	PyObject *motions = PyDict_New();
	int *counts = new int[self->model->GetMotionGroupCount()];
	self->model->GetMotions(motions,
							[](void *collector, const char *group, int no, const char *filePath, const char *sound)
							{
								PyObject *motions = (PyObject *)collector;
								PyObject *motion = PyDict_New();
								PyDict_SetItem(motion, PyUnicode_FromString("File"), PyUnicode_FromString(filePath));
								PyDict_SetItem(motion, PyUnicode_FromString("Sound"), PyUnicode_FromString(sound));
								PyObject *list = PyDict_GetItem(motions, PyUnicode_FromString(group));
								if (list == NULL)
								{
									list = PyList_New(1);
									PyList_SetItem(list, 0, motion);
									PyDict_SetItem(motions, PyUnicode_FromString(group), list);
								}
								else
								{
									PyList_Append(list, motion);
								}
							});
	return motions;
}
static PyObject *PyModel_HitPart(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float x, y;
	bool topOnly = false;
	if (!PyArg_ParseTuple(args, "ff|b", &x, &y, &topOnly))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (float, float, [bool])");
		return NULL;
	}
	PyObject *list = PyList_New(0);
	self->model->HitPart(x, y, list, [](void *collector, const char *id)
						 { PyList_Append((PyObject *)collector, PyUnicode_FromString(id)); }, topOnly);
	return list;
}
static PyObject *PyModel_HitDrawable(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float x, y;
	bool topOnly = false;
	if (!PyArg_ParseTuple(args, "ff|b", &x, &y, &topOnly))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (float, float, [bool])");
		return NULL;
	}
	PyObject *list = PyList_New(0);
	self->model->HitDrawable(x, y, list, [](void *collector, const char *id)
							 { PyList_Append((PyObject *)collector, PyUnicode_FromString(id)); }, topOnly);
	return list;
}
static PyObject *PyModel_Drag(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	float x, y;
	if (!PyArg_ParseTuple(args, "ff", &x, &y))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (float, float)");
		return NULL;
	}

	self->model->Drag(x, y);
	Py_RETURN_NONE;
}
static PyObject *PyModel_IsAreaHit(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *areaName;
	float x, y;
	if (!PyArg_ParseTuple(args, "sff", &areaName, &x, &y))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, float, float)");
		return NULL;
	}

	if (self->model->IsAreaHit(areaName, x, y))
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
static PyObject *PyModel_IsPartHit(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float x, y;
	if (!PyArg_ParseTuple(args, "iff", &index, &x, &y))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float)");
		return NULL;
	}
	if (self->model->IsPartHit(index, x, y))
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
static PyObject *PyModel_IsDrawableHit(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float x, y;
	if (!PyArg_ParseTuple(args, "iff", &index, &x, &y))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float)");
		return NULL;
	}

	if (self->model->IsDrawableHit(index, x, y))
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}
static PyObject *PyModel_CreateRenderer(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int maskBufferCount = 2;
	if (!PyArg_ParseTuple(args, "|i", &maskBufferCount))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int)");
		return NULL;
	}
	self->model->CreateRenderer(maskBufferCount);
	Py_RETURN_NONE;
}
static PyObject *PyModel_DestroyRenderer(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->DeleteRenderer();
	Py_RETURN_NONE;
}
static PyObject *PyModel_Draw(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->Draw();
	Py_RETURN_NONE;
}
static PyObject *PyModel_GetPartIds(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const int count = self->model->GetPartCount();
	PyObject *list = PyList_New(count);
	int index = 0;
	void *collector[2] = {list, &index};
	self->model->GetPartIds(collector,
							[](void *collector, const char *id)
							{
								PyObject *list = (PyObject *)(((void **)collector)[0]);
								int *index = (int *)(((void **)collector)[1]);

								PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
							});
	return list;
}
static PyObject *PyModel_SetPartOpacity(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float value;
	if (!PyArg_ParseTuple(args, "if", &index, &value))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float)");
		return NULL;
	}
	self->model->SetPartOpacity(index, value);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetPartScreenColor(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float r, g, b, a;
	if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
		return NULL;
	}
	self->model->SetPartScreenColor(index, r, g, b, a);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetPartMultiplyColor(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float r, g, b, a;
	if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
		return NULL;
	}
	self->model->SetPartMultiplyColor(index, r, g, b, a);
	Py_RETURN_NONE;
}
static PyObject *PyModel_GetDrawableIds(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const int count = self->model->GetDrawableCount();
	PyObject *list = PyList_New(count);
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
static PyObject *PyModel_SetDrawableMultiplyColor(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float r, g, b, a;
	if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
		return NULL;
	}
	self->model->SetDrawableMultiColor(index, r, g, b, a);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetDrawableScreenColor(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	int index;
	float r, g, b, a;
	if (!PyArg_ParseTuple(args, "iffff", &index, &r, &g, &b, &a))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (int, float, float, float, float)");
		return NULL;
	}
	self->model->SetDrawableScreenColor(index, r, g, b, a);
	Py_RETURN_NONE;
}
static PyObject *PyModel_AddExpression(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *expressionId;
	if (!PyArg_ParseTuple(args, "s", &expressionId))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str)");
		return NULL;
	}
	self->model->AddExpression(expressionId);
	Py_RETURN_NONE;
}
static PyObject *PyModel_RemoveExpression(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *expressionId;
	if (!PyArg_ParseTuple(args, "s", &expressionId))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str)");
		return NULL;
	}
	self->model->RemoveExpression(expressionId);
	Py_RETURN_NONE;
}
static PyObject *PyModel_SetExpression(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char *expressionId;
	if (!PyArg_ParseTuple(args, "s", &expressionId))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str)");
		return NULL;
	}
	self->model->SetExpression(expressionId);
	Py_RETURN_NONE;
}

static PyObject *PyModel_SetRandomExpression(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const char* expId = self->model->SetRandomExpression();
	if (expId != nullptr)
	{
		return Py_BuildValue("s", expId);
	}
	else 
	{
		Py_RETURN_NONE;
	}
}

static PyObject *PyModel_ResetExpression(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->ResetExpression();
	Py_RETURN_NONE;
}
static PyObject *PyModel_ResetExpressions(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->ResetExpressions();
	Py_RETURN_NONE;
}
static PyObject *PyModel_LoadExtraExpression(PyModelObject *self, PyObject *args, PyObject *kwargs) 
{
	const char *expressionId;
	const char *filePath;
	if (!PyArg_ParseTuple(args, "ss", &expressionId, &filePath))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (str, str)");
		return NULL;
	}
	self->model->LoadExtraExpression(expressionId, filePath);
	Py_RETURN_NONE;
}
static PyObject *PyModel_GetExpressions(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	const int count = self->model->GetExpressionCount();
	PyObject *list = PyList_New(count);
	int index = 0;
	void *collector[2] = {list, &index};
	self->model->GetExpressions(collector,
								[](void *collector, const char *id, const char *file)
								{
									PyObject *list = (PyObject *)(((void **)collector)[0]);
									int *index = (int *)(((void **)collector)[1]);

									PyList_SetItem(list, index[0]++, PyUnicode_FromString(id));
								});
	return list;
}
static PyObject *PyModel_StopAllMotions(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->StopAllMotions();
	Py_RETURN_NONE;
}
static PyObject *PyModel_ResetAllParameters(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->ResetAllParameters();
	Py_RETURN_NONE;
}
static PyObject *PyModel_ResetPose(PyModelObject *self, PyObject *args, PyObject *kwargs)
{
	self->model->ResetPose();
	Py_RETURN_NONE;
}

static PyObject* PyModel_GetCanvasSize(PyModelObject* self, PyObject* args, PyObject* kwargs)
{
    float w, h;
    self->model->GetCanvasSize(w, h);
    return Py_BuildValue("ff", w, h);
}

static PyObject* PyModel_GetCanvasSizePixel(PyModelObject* self, PyObject* args, PyObject* kwargs)
{
    float w, h;
    self->model->GetCanvasSizePixel(w, h);
    return Py_BuildValue("ff", w, h);
}

static PyObject* PyModel_GetPixelsPerUnit(PyModelObject* self, PyObject* args, PyObject* kwargs)
{
    return Py_BuildValue("f", self->model->GetPixelsPerUnit());
}

static PyObject* PyModel_SetAutoBreath(PyModelObject* self, PyObject* args, PyObject* kwargs)
{
	bool on;
	if (!PyArg_ParseTuple(args, "b", &on))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (bool)");
		return NULL;
	}

	self->model->SetAutoBreath(on);

	Py_RETURN_NONE;
}

static PyObject* PyModel_SetAutoBlink(PyModelObject* self, PyObject* args, PyObject* kwargs)
{
	bool on;
	if (!PyArg_ParseTuple(args, "b", &on))
	{
		PyErr_SetString(PyExc_TypeError, "arguments must be (bool)");
		return NULL;
	}

	self->model->SetAutoBlink(on);

	Py_RETURN_NONE;
}

static PyMethodDef PyModel_Methods[] = {
	{"LoadModelJson", (PyCFunction)PyModel_LoadModelJson, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetModelHomeDir", (PyCFunction)PyModel_GetModelHomeDir, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"Update", (PyCFunction)PyModel_Update, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"UpdateMotion", (PyCFunction)PyModel_UpdateMotion, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"UpdateDrag", (PyCFunction)PyModel_UpdateDrag, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"UpdateBreath", (PyCFunction)PyModel_UpdateBreath, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"UpdateBlink", (PyCFunction)PyModel_UpdateBlink, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"UpdateExpression", (PyCFunction)PyModel_UpdateExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"UpdatePhysics", (PyCFunction)PyModel_UpdatePhysics, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"UpdatePose", (PyCFunction)PyModel_UpdatePose, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetParameterIds", (PyCFunction)PyModel_GetParameterIds, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetParameterValue", (PyCFunction)PyModel_GetParameterValue, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetParameterMaximumValue", (PyCFunction)PyModel_GetParameterMaximumValue, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetParameterMinimumValue", (PyCFunction)PyModel_GetParameterMinimumValue, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetParameterDefaultValue", (PyCFunction)PyModel_GetParameterDefaultValue, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"SetParameterValue", (PyCFunction)PyModel_SetParameterValue, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetParameterValueById", (PyCFunction)PyModel_SetParameterValueById, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"AddParameterValue", (PyCFunction)PyModel_AddParameterValue, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"AddParameterValueById", (PyCFunction)PyModel_AddParameterValueById, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"SetAndSaveParameterValue", (PyCFunction)PyModel_SetAndSaveParameterValue, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetAndSaveParameterValueById", (PyCFunction)PyModel_SetAndSaveParameterValueById, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"AddAndSaveParameterValue", (PyCFunction)PyModel_AddAndSaveParameterValue, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"AddAndSaveParameterValueById", (PyCFunction)PyModel_AddAndSaveParameterValueById, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"LoadParameters", (PyCFunction)PyModel_LoadParameters, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SaveParameters", (PyCFunction)PyModel_SaveParameters, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"Resize", (PyCFunction)PyModel_Resize, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetOffset", (PyCFunction)PyModel_SetOffset, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"Rotate", (PyCFunction)PyModel_Rotate, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetScale", (PyCFunction)PyModel_SetScale, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetScaleX", (PyCFunction)PyModel_SetScaleX, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetScaleY", (PyCFunction)PyModel_SetScaleY, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetMvp", (PyCFunction)PyModel_GetMvp, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"StartMotion", (PyCFunction)PyModel_StartMotion, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"StartRandomMotion", (PyCFunction)PyModel_StartRandomMotion, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"IsMotionFinished", (PyCFunction)PyModel_IsMotionFinished, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"LoadExtraMotion", (PyCFunction)PyModel_LoadExtraMotion, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetMotions", (PyCFunction)PyModel_GetMotions, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"StopAllMotions", (PyCFunction)PyModel_StopAllMotions, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"ResetAllParameters", (PyCFunction)PyModel_ResetAllParameters, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"ResetPose", (PyCFunction)PyModel_ResetPose, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"HitPart", (PyCFunction)PyModel_HitPart, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"HitDrawable", (PyCFunction)PyModel_HitDrawable, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"Drag", (PyCFunction)PyModel_Drag, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"IsAreaHit", (PyCFunction)PyModel_IsAreaHit, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"IsPartHit", (PyCFunction)PyModel_IsPartHit, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"IsDrawableHit", (PyCFunction)PyModel_IsDrawableHit, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"CreateRenderer", (PyCFunction)PyModel_CreateRenderer, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"DestroyRenderer", (PyCFunction)PyModel_DestroyRenderer, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"Draw", (PyCFunction)PyModel_Draw, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetPartIds", (PyCFunction)PyModel_GetPartIds, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetPartOpacity", (PyCFunction)PyModel_SetPartOpacity, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetPartScreenColor", (PyCFunction)PyModel_SetPartScreenColor, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetPartMultiplyColor", (PyCFunction)PyModel_SetPartMultiplyColor, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"GetDrawableIds", (PyCFunction)PyModel_GetDrawableIds, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetDrawableMultiplyColor", (PyCFunction)PyModel_SetDrawableMultiplyColor, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetDrawableScreenColor", (PyCFunction)PyModel_SetDrawableScreenColor, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"GetExpressions", (PyCFunction)PyModel_GetExpressions, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"AddExpression", (PyCFunction)PyModel_AddExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"RemoveExpression", (PyCFunction)PyModel_RemoveExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetExpression", (PyCFunction)PyModel_SetExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"SetRandomExpression", (PyCFunction)PyModel_SetRandomExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"ResetExpression", (PyCFunction)PyModel_ResetExpression, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"ResetExpressions", (PyCFunction)PyModel_ResetExpressions, METH_VARARGS | METH_KEYWORDS, nullptr},
	{"LoadExtraExpression", (PyCFunction)PyModel_LoadExtraExpression, METH_VARARGS | METH_KEYWORDS, nullptr},

	{"GetCanvasSize", (PyCFunction)PyModel_GetCanvasSize, METH_VARARGS, ""},
    {"GetCanvasSizePixel", (PyCFunction)PyModel_GetCanvasSizePixel, METH_VARARGS, ""},
    {"GetPixelsPerUnit", (PyCFunction)PyModel_GetPixelsPerUnit, METH_VARARGS, ""},

    {"SetAutoBreath", (PyCFunction)PyModel_SetAutoBreath, METH_VARARGS, ""},
    {"SetAutoBlink", (PyCFunction)PyModel_SetAutoBlink, METH_VARARGS, ""},

	{NULL}};
static PyObject *PyModel_New(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
	PyObject *self = (PyObject *)PyObject_Malloc(sizeof(PyModelObject));
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
