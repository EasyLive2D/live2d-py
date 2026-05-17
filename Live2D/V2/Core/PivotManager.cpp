#include "PivotManager.hpp"
#include "ParamPivots.hpp"
#include "BinaryReader.hpp"
#include "../Model/ModelContext.hpp"
#include "DEF.hpp"
#include <cstdio>
#include <stdexcept>

namespace live2d {

void PivotManager::read(BinaryReader& br) {
    mParamPivotTable = br.readObject<std::vector<ParamPivots*>>();
}

bool PivotManager::checkParamUpdated(ModelContext* modelContext) {
    if (modelContext->requireSetup()) return true;

    int initVersion = modelContext->getInitVersion();
    for (int i = static_cast<int>(mParamPivotTable.size()) - 1; i >= 0; i--) {
        int paramIndex = mParamPivotTable[i]->getParamIndex(initVersion);
        if (paramIndex == ParamPivots::PARAM_INDEX_NOT_INIT) {
            paramIndex = modelContext->getParamIndex(mParamPivotTable[i]->getParamID());
        }
        if (modelContext->isParamUpdated(paramIndex)) return true;
    }
    return false;
}

int PivotManager::calcPivotValues(ModelContext* modelContext, bool& outRet) {
    int paramCount = static_cast<int>(mParamPivotTable.size());
    int initVersion = modelContext->getInitVersion();
    int interpolationCount = 0;

    for (int i = 0; i < paramCount; i++) {
        auto* pp = mParamPivotTable[i];
        int paramIndex = pp->getParamIndex(initVersion);
        if (paramIndex == ParamPivots::PARAM_INDEX_NOT_INIT) {
            paramIndex = modelContext->getParamIndex(pp->getParamID());
            pp->setParamIndex(paramIndex, initVersion);
        }

        float paramValue = (paramIndex < 0) ? 0.0f
                            : modelContext->getParamFloat(paramIndex);
        int pivotCount = pp->getPivotCount();
        auto& pivotValues = pp->getPivotValues();

        int pivotIndex = -1;
        float t = 0.0f;

        if (pivotCount < 1) {
            // nothing
        } else if (pivotCount == 1) {
            float pivotValue = pivotValues[0];
            if (pivotValue - GOSA < paramValue && paramValue < pivotValue + GOSA) {
                pivotIndex = 0;
                t = 0.0f;
            } else {
                pivotIndex = 0;
                outRet = true;
            }
        } else {
            float pivotValue = pivotValues[0];
            if (paramValue < pivotValue - GOSA) {
                pivotIndex = 0;
                outRet = true;
            } else if (paramValue < pivotValue + GOSA) {
                pivotIndex = 0;
            } else {
                bool found = false;
                for (int j = 1; j < pivotCount; j++) {
                    float nextPivotValue = pivotValues[j];
                    if (paramValue < nextPivotValue + GOSA) {
                        if (nextPivotValue - GOSA < paramValue) {
                            pivotIndex = j;
                        } else {
                            pivotIndex = j - 1;
                            t = (paramValue - pivotValue) / (nextPivotValue - pivotValue);
                            interpolationCount++;
                        }
                        found = true;
                        break;
                    }
                    pivotValue = nextPivotValue;
                }
                if (!found) {
                    pivotIndex = pivotCount - 1;
                    t = 0.0f;
                    outRet = true;
                }
            }
        }

        pp->setTmpPivotIndex(pivotIndex);
        pp->setTmpT(t);
    }
    return interpolationCount;
}

void PivotManager::calcPivotIndices(std::vector<int16_t>& indexArray,
                                   std::vector<float>& tArray,
                                   int interpolationCount) {
    int tableSize = 1 << interpolationCount;
    if (tableSize + 1 > PIVOT_TABLE_SIZE) {
        printf("err 23245\n");
    }

    int paramCount = static_cast<int>(mParamPivotTable.size());
    int stride = 1, divisor = 1, tIndex = 0;

    for (int i = 0; i < tableSize; i++) indexArray[i] = 0;

    for (int i = 0; i < paramCount; i++) {
        auto* pp = mParamPivotTable[i];
        if (pp->getTmpT() == 0.0f) {
            int offset = pp->getTmpPivotIndex() * stride;
            for (int j = 0; j < tableSize; j++) indexArray[j] += offset;
        } else {
            int offset1 = stride * pp->getTmpPivotIndex();
            int offset2 = stride * (pp->getTmpPivotIndex() + 1);
            for (int j = 0; j < tableSize; j++)
                indexArray[j] += ((j / divisor) % 2 == 0) ? offset1 : offset2;
            tArray[tIndex] = pp->getTmpT();
            tIndex++;
            divisor *= 2;
        }
        stride *= pp->getPivotCount();
    }
    indexArray[tableSize] = 65535;
    tArray[tIndex] = -1.0f;
}

} // namespace live2d
