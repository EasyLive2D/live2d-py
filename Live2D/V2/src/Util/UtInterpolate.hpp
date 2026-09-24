#pragma once

#include <vector>

namespace live2d {

class ModelContext;
class PivotManager;

class UtInterpolate {
public:
    static float interpolateFloat(ModelContext* mdc, PivotManager* pivotMgr, bool& ret,
                                  const std::vector<float>& values);
    static int interpolateInt(ModelContext* mdc, PivotManager* pivotMgr, bool& ret,
                              const std::vector<int>& values);
    static void interpolatePoints(ModelContext* mdc, PivotManager* pivotMgr, bool& ret,
                                  int pointCount,
                                  const std::vector<std::vector<float>>& pivotPoints,
                                  std::vector<float>& dstPoints, int ptOffset, int ptStep);
};

} // namespace live2d
