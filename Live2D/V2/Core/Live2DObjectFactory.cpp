#include "Live2DObjectFactory.hpp"
#include "ISerializable.hpp"
#include "ModelImpl.hpp"
#include "Avatar.hpp"
#include "PartsData.hpp"
#include "ParamDefFloat.hpp"
#include "ParamDefSet.hpp"
#include "ParamPivots.hpp"
#include "PivotManager.hpp"
#include "../Deformer/Deformer.hpp"
#include "../Deformer/RotationDeformer.hpp"
#include "../Deformer/WarpDeformer.hpp"
#include "../Deformer/AffineEnt.hpp"
#include "../Draw/Mesh.hpp"
#include <stdexcept>
#include <string>

namespace live2d {

std::unique_ptr<ISerializable> Live2DObjectFactory::create(int clsNo) {
    if (clsNo < 100) {
        switch (clsNo) {
            case 65: return std::make_unique<WarpDeformer>();
            case 66: return std::make_unique<PivotManager>();
            case 67: return std::make_unique<ParamPivots>();
            case 68: return std::make_unique<RotationDeformer>();
            case 69: return std::make_unique<AffineEnt>();
            case 70: return std::make_unique<Mesh>();
            default: break;
        }
    } else if (clsNo < 150) {
        switch (clsNo) {
            case 131: return std::make_unique<ParamDefFloat>();
            case 133: return std::make_unique<PartsData>();
            case 136: return std::make_unique<ModelImpl>();
            case 137: return std::make_unique<ParamDefSet>();
            case 142: return std::make_unique<Avatar>();
            default: break;
        }
    }

    throw std::runtime_error("Unknown class ID: " + std::to_string(clsNo));
}

} // namespace live2d
