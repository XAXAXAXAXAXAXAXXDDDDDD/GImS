#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/HLSLProgram.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <imgui.h>
#include <iostream>
using namespace gims;

class PatchApp : public DX12App
{
private:
  struct UiData
  {
    f32  outerTessFactor = 5.0f;
    f32  innerTessFactor = 3.0f;
    bool showSolid       = false;
    bool showQuadDomain  = false;
  };
  UiData m_uiData;

  gims::HLSLProgram m_hlslProgramSolid;
  gims::HLSLProgram m_hlslProgramWireframe;

  ComPtr<ID3D12PipelineState> m_pipelineStateSolid;
  ComPtr<ID3D12PipelineState> m_pipelineStateSolidTriangle;
  ComPtr<ID3D12PipelineState> m_pipelineStateWireframe;
  ComPtr<ID3D12PipelineState> m_pipelineStateWireframeTriangle;
  void                        createPipeline(bool isSolid, bool isQuad);

  ComPtr<ID3D12RootSignature> m_rootSignature;
  void                        createRootSignature();

  struct ConstantBuffer
  {
    f32 outerTessFactor;
    f32 innerTessFactor;
  };
  std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;
  void                                createConstantBuffer();
  void                                updateConstantBuffer();

public:
  PatchApp(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
    m_hlslProgramSolid = HLSLProgram(L"../../../Project/P0TessellationExample/Shaders/TrianglePatch.hlsl", "VS_main",
                                        "PS_main", "HS_main", "DS_main");
    m_hlslProgramWireframe     = HLSLProgram(L"../../../Project/P0TessellationExample/Shaders/QuadPatch.hlsl", "VS_main",
                                        "PS_main", "HS_main", "DS_main");

    createRootSignature();

    createPipeline(false, false);
    createPipeline(false, true);
    createPipeline(true, true);
    createPipeline(true, false);

    createConstantBuffer();
  }

  virtual void onDraw();
  virtual void onDrawUI();
};