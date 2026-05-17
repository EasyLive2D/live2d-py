#pragma once
#define Py_LIMITED_API
#include <Python.h>
#ifndef Py_IsNone
#define Py_IsNone(o) (o == Py_None)
#endif
