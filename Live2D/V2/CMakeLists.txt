# ---- V2: Live2D Cubism 2.x C++ port ----
set(V2_TARGET V2)

add_library(${V2_TARGET} STATIC
    Core/Id.cpp
    Core/BinaryReader.cpp
    Core/Live2DObjectFactory.cpp
    Core/ParamDefFloat.cpp
    Core/ParamDefSet.cpp
    Core/PartsData.cpp
    Core/PartsDataContext.cpp
    Core/Avatar.cpp
    Core/ModelImpl.cpp
    Core/ParamPivots.cpp
    Core/PivotManager.cpp
    Model/ModelContext.cpp
    Model/ALive2DModel.cpp
    Model/Live2DModelOpenGL.cpp
    Deformer/Deformer.cpp
    Deformer/AffineEnt.cpp
    Deformer/RotationDeformer.cpp
    Deformer/RotationContext.cpp
    Deformer/WarpDeformer.cpp
    Deformer/WarpContext.cpp
    Draw/IDrawData.cpp
    Draw/Mesh.cpp
    Graphics/DrawParamOpenGL.cpp
    Graphics/ClippingManagerOpenGL.cpp
    Graphics/ClipContext.cpp
    Framework/L2DMatrix44.cpp
    Framework/L2DModelMatrix.cpp
    Framework/L2DTargetPoint.cpp
    Framework/L2DEyeBlink.cpp
    Framework/L2DPose.cpp
    Framework/L2DPhysics.cpp
    Framework/L2DExpressionMotion.cpp
    Framework/L2DMotionManager.cpp
    Framework/L2DBaseModel.cpp
    Framework/LAppModel.cpp
    Framework/MatrixManager.cpp
    Motion/Live2DMotion.cpp
    Util/UtInterpolate.cpp
    Util/UtSystem.cpp
    Util/stb_impl.cpp
    ${CMAKE_SOURCE_DIR}/Live2D/Common/Log.cpp
)

target_include_directories(${V2_TARGET}
    PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/Live2D/Glad/include
    PRIVATE ${CMAKE_SOURCE_DIR}/Live2D/Common
)
target_link_libraries(${V2_TARGET} PRIVATE glad)
target_compile_features(${V2_TARGET} PUBLIC cxx_std_17)

if(MSVC)
    target_compile_options(${V2_TARGET} PRIVATE "/utf-8" "/wd4018" "/wd4244" "/wd4996")
endif()
