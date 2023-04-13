#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/HLSLProgram.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/types.hpp>
#include <iostream>
using namespace gims;

class RootConstantBufferApp : public DX12App
{
private:
  ComPtr<ID3D12PipelineState> m_pipelineState;
  gims::HLSLProgram           m_triangleMeshProgram;
  ComPtr<ID3D12RootSignature> m_rootSignature;
  ui32                        m_counter;

  void createRootSignature()
  {
    CD3DX12_ROOT_PARAMETER parameter = {};
    parameter.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

    CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
    descRootSignature.Init(1, &parameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);
    // descRootSignature.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

    getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                     IID_PPV_ARGS(&m_rootSignature));
  }

  void createPipeline()
  {
    m_triangleMeshProgram =
        HLSLProgram(L"../../../Tutorials/T6RootConstantBuffer/Shaders/RootConstantBuffer.hlsl", "VS_main", "PS_main");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                        = {};
    psoDesc.pRootSignature                     = m_rootSignature.Get();
    psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgram.getVertexShader().Get());
    psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgram.getPixelShader().Get());
    psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
    psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState                  = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.SampleMask                         = UINT_MAX;
    psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                   = 1;
    psoDesc.SampleDesc.Count                   = 1;
    psoDesc.RTVFormats[0]                      = getRenderTarget()->GetDesc().Format;
    psoDesc.DSVFormat                          = getDepthStencil()->GetDesc().Format;
    psoDesc.DepthStencilState.DepthEnable      = FALSE;
    psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_ALWAYS;
    psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.StencilEnable    = FALSE;

    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
  }

  struct ConstantBuffer
  {
    f32m4 rotation;
  };
  // Byte alignment könnte anders sein ALIGN 4 / 16 / ... etc. zwischen C++ und HLSL

  std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;

  void createConstantBuffer()
  {
    const auto frameCount = getDX12AppConfig().frameCount;
    m_constantBuffers.resize(frameCount);
    for (ui32 i = 0; i < frameCount; i++)
    {
      static const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
      static const auto constantBufferDesc   = CD3DX12_RESOURCE_DESC::Buffer(sizeof(ConstantBuffer));
      getDevice()->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &constantBufferDesc,
                                           D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
                                           IID_PPV_ARGS(&m_constantBuffers[i]));
    }
  }

  void updateConstantBuffer()
  {
    ConstantBuffer cb;
    cb.rotation = glm::eulerAngleZ(glm::radians(
        (m_counter / 60.0f) * 360.0f)); // glm::axisAngleMatrix(f32v3()); // glm::rotateZ(glm::radians(15));

    const auto& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];
    void*       p;
    currentConstantBuffer->Map(0, nullptr, &p);
    // PCIExpress Kopie, CPU -> GPU, dazu Map und Unmap
    memcpy(p, &cb, sizeof(cb));
    currentConstantBuffer->Unmap(0, nullptr);
  }

public:
  RootConstantBufferApp(const DX12AppConfig createInfo)
      : DX12App(createInfo)
      , m_counter(0)
  {
    createRootSignature();
    createPipeline();
    createConstantBuffer();
  }

  virtual void onDraw()
  {
    updateConstantBuffer();
    const auto commandList = getCommandList();
    const auto rtvHandle   = getRTVHandle();
    const auto dsvHandle   = getDSVHandle();
    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

    f32v4 clearColor = {0.0f, 0.0f, 0.0f, 1.0f};
    commandList->ClearRenderTargetView(rtvHandle, &clearColor.x, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

    commandList->RSSetViewports(1, &getViewport());
    commandList->RSSetScissorRects(1, &getRectScissor());

    commandList->SetPipelineState(m_pipelineState.Get());
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffers[getFrameIndex()].Get()->GetGPUVirtualAddress());

    commandList->DrawInstanced(3, 1, 0, 0);
    m_counter++;
  }
};

int main(int /* argc*/, char /* **argv */)
{
  gims::DX12AppConfig config;
  config.title    = L"Tutorial 6 Root Constant Buffer";
  config.useVSync = true;
  try
  {
    RootConstantBufferApp app(config);
    app.run();
  }
  catch (const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << "\n";
  }
  if (config.debug)
  {
    DX12Util::reportLiveObjects();
  }
  return 0;
}
