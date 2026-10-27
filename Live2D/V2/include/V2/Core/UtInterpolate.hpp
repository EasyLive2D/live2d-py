#pragma once

#include <vector>

namespace Live2D {
namespace V2 {

class Moc;
class PivotManager;

class UtInterpolate {
public:
    static float InterpolateFloat(Moc* moc, PivotManager* pivotMgr, bool& ret,
                                  const std::vector<float>& values);
    static int InterpolateInt(Moc* moc, PivotManager* pivotMgr, bool& ret,
                              const std::vector<int>& values);
    static void InterpolatePoints(Moc* moc, PivotManager* pivotMgr, bool& ret, int pointCount,
                                  const std::vector<std::vector<float>>& pivotPoints,
                                  std::vector<float>& dstPoints, int ptOffset, int ptStep);
};

}   // namespace V2
}   // namespace Live2D