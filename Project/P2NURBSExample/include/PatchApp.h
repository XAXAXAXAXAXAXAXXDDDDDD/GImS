#include "MorbiusStrip.h"
#include "NURBSPatchD3D12.h"
#include "Teapot.h"
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/HLSLProgram.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <imgui.h>
#include <iostream>

namespace
{

f32m4 getNormalizationTransformation(f32v3 const* const positions, ui32 nPositions)
{
  f32v3 allVals = f32v3(0.0f, 0.0f, 0.0f);
  f32   max_x   = FLT_MIN;
  f32   max_y   = FLT_MIN;
  f32   max_z   = FLT_MIN;

  f32 min_x = FLT_MAX;
  f32 min_y = FLT_MAX;
  f32 min_z = FLT_MAX;

  // zuerst schwerpunkt über Mittelwert der Positionen --> Translation
  for (ui32 i = 0; i < nPositions; i++)
  {
    allVals += positions[i];

    if (positions[i].x < min_x)
      min_x = positions[i].x;
    if (positions[i].y < min_y)
      min_y = positions[i].y;
    if (positions[i].z < min_z)
      min_z = positions[i].z;

    if (positions[i].x > max_x)
      max_x = positions[i].x;
    if (positions[i].y > max_y)
      max_y = positions[i].y;
    if (positions[i].z > max_z)
      max_z = positions[i].z;
  }
  f32v3 translation_vec = allVals / nPositions;
  f32m4 translation_mat =
      glm::translate(f32m4(1), translation_vec); /* f32m4(f32v4(1.0f, 0.0f, 0.0f, 0.0f), f32v4(0.0f, 1.0f, 0.0f, 0.0f),
                                f32v4(0.0f, 0.0f, 1.0f, 0.0f), f32v4(-translation_vec, 1.0f));*/

  // dann max min der längsten Achse abbilden auf -0.5 ... 0.5 --> Skalierung
  f32 scale_factor = 1.0f / std::max(std::abs(min_x) + std::abs(max_x),
                                     std::max(std::abs(min_y) + std::abs(max_y), std::abs(min_z) + std::abs(max_z)));

  f32m4 scale_mat =
      /*glm::transpose*/ (
          glm::scale(f32m4(1), f32v3(scale_factor))); /* (f32v4(scale_factor, 0.0f, 0.0f, 0.0f), f32v4(0.0f,
scale_factor, 0.0f, 0.0f),
           f32v4(0.0f, 0.0f, scale_factor, 0.0f), f32v4(0.0f, 0.0f, 0.0f, 1.0f));*/

  f32m4 invertZ_mat = glm::transpose(glm::scale(f32m4(1), f32v3(1, 1, -1)));

  // Scale * Translate
  return /*invertZ_mat **/ /*glm::transpose*/ scale_mat * translation_mat;
}
} // namespace

using namespace gims;

class PatchApp : public DX12App
{
private:
  struct UiData
  {
    f32  tessFactor = 4.0f;
    bool showSolid  = false;
  };
  UiData m_uiData;

  gims::ExaminerController m_examinerController;

  gims::HLSLProgram m_hlslProgramSolid;
  gims::HLSLProgram m_hlslProgramWireframe;

  ComPtr<ID3D12PipelineState> m_pipelineStateSolid;
  ComPtr<ID3D12PipelineState> m_pipelineStateWireframe;
  void                        createPipeline(bool isSolid);

  ComPtr<ID3D12RootSignature> m_rootSignature;
  void                        createRootSignature();

  NURBSPatchD3D12 m_NURBSPatch;
  f32m4           m_normalizationTransformation;

  struct ConstantBuffer
  {
    f32m4 mv;
    f32m4 p;
    f32m4 pIT;
    f32   tessFactor;
  };
  std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;
  void                                createConstantBuffer();
  void                                updateConstantBuffer();

public:
  PatchApp(const DX12AppConfig createInfo)
      : DX12App(createInfo)
      , m_examinerController(true)
  {
    m_hlslProgramSolid = HLSLProgram(L"../../../Project/P2NURBSExample/Shaders/NURBSPatch.hlsl", "VS_main", "PS_main",
                                     "HS_main", "DS_main");
    m_hlslProgramWireframe = HLSLProgram(L"../../../Project/P2NURBSExample/Shaders/NURBSPatch.hlsl", "VS_main",
                                         "PS_main_Wireframe", "HS_main", "DS_main");

    fillCorrPatch();

    m_NURBSPatch = NURBSPatchD3D12(teapotVertices, kTeapotNumVertices, teapotPatchesCorr, 16 * kTeapotNumPatches,
                                   getDevice(), getCommandQueue());
    /*m_NURBSPatch = NURBSPatchD3D12(mobiusStripVertices, kMobiusNumVertices, mobiusStripPatches, 16 * kMobiusNumPatches,
                                   getDevice(), getCommandQueue());*/

    m_examinerController.setTranslationVector(f32v3(0, 0, 3));
    m_normalizationTransformation = getNormalizationTransformation(teapotVertices, kTeapotNumVertices);
    // glm::identity<f32m4>();

    createRootSignature();

    createPipeline(false);
    createPipeline(true);

    createConstantBuffer();
  }

  virtual void onDraw();
  virtual void onDrawUI();
};