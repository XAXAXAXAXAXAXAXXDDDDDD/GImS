#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/HLSLProgram.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>
#include <gimslib/d3d/HLSLProgram.hpp>
#include <gimslib/d3d/DX12App.hpp>
#include <imgui.h>
#include <iostream>
using namespace gims;

class TriangleApp : public DX12App
{
private:
  gims::HLSLProgram           m_hlslProgram;
  ComPtr<ID3D12PipelineState> m_pipelineState;
  ComPtr<ID3D12RootSignature> m_rootSignature;

  void createRootSignature();

  void createPipeline();

public:
  TriangleApp(const DX12AppConfig createInfo)
      : DX12App(createInfo)
  {
    createRootSignature();
    createPipeline();
  }

  virtual void onDraw();
  virtual void onDrawUI();
};