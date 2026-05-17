#ifndef HACKPROPERTIES_H
#define HACKPROPERTIES_H
#include <string>

#define __ADDITIONAL_PROPERTIES__ \
    std::string group; \
    int no;

#define __ADDITIONAL_METHODS__ \
    void AddAndSaveParameterValue(const Csm::CubismId* parameterId, float value, float weight = 1.0f)\
    {\
        const csmInt32 index = GetParameterIndex(parameterId);\
        AddParameterValue(index, value, weight);\
        _savedParameters[index] = _parameterValues[index];\
    }\
    void SetAndSaveParameterValue(const Csm::CubismId* parameterId, float value, float weight = 1.0f)\
    {\
        const csmInt32 index = GetParameterIndex(parameterId);\
        SetParameterValue(index, value, weight);\
        _savedParameters[index] = _parameterValues[index];\
    }\
    void AddAndSaveParameterValue(int index, float value, float weight = 1.0f)\
    {\
        AddParameterValue(index, value, weight);\
        _savedParameters[index] = _parameterValues[index];\
    }\
    void SetAndSaveParameterValue(int index, float value, float weight = 1.0f)\
    {\
        SetParameterValue(index, value, weight);\
        _savedParameters[index] = _parameterValues[index];\
    }\

#endif // HACKPROPERTIES_H
