#include "PatchApp.h"

void PatchApp::createPipeline(bool isSolid, bool isQuad)
{
  HLSLProgram program;
  if (isQuad)
  {
    program = m_hlslProgramWireframe;
  }
  else
  {
    program = m_hlslProgramSolid;
  }
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {};
  psoDesc.pRootSignature                     = m_rootSignature.Get();
  psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(program.getVertexShader().Get());
  psoDesc.HS                                 = CD3DX12_SHADER_BYTECODE(program.getHullShader().Get());
  psoDesc.DS                                 = CD3DX12_SHADER_BYTECODE(program.getDomainShader().Get());
  psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(program.getPixelShader().Get());
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  if (!isSolid)
  {
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
  }
  else
  {
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  }
  psoDesc.BlendState                      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState.DepthEnable   = FALSE;
  psoDesc.DepthStencilState.StencilEnable = FALSE;
  psoDesc.SampleMask                      = UINT_MAX;
  psoDesc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
  psoDesc.NumRenderTargets                = 1;
  psoDesc.SampleDesc.Count                = 1;
  psoDesc.RTVFormats[0]                   = getRenderTarget()->GetDesc().Format;
  psoDesc.DSVFormat                       = getDepthStencil()->GetDesc().Format;
  if (isSolid && isQuad)
  {
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateSolid)));
  }
  else if (isSolid && !isQuad)
  {
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateSolidTriangle)));
  }
  else if (!isSolid && isQuad)
  {
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateWireframe)));
  }
  else
  {
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateWireframeTriangle)));
  }
}

void PatchApp::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER parameters[1] {};

  parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
  descRootSignature.Init(1, parameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void PatchApp::createConstantBuffer()
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

void PatchApp::updateConstantBuffer()
{
  // check for sensible values
  ConstantBuffer cb;
  cb.innerTessFactor = m_uiData.innerTessFactor;
  cb.outerTessFactor = m_uiData.outerTessFactor;

  const auto& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];
  void*       p;
  currentConstantBuffer->Map(0, nullptr, &p);
  memcpy(p, &cb, sizeof(cb));
  currentConstantBuffer->Unmap(0, nullptr);
}

void PatchApp::onDraw()
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

  commandList->SetGraphicsRootSignature(m_rootSignature.Get());
  commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffers[getFrameIndex()].Get()->GetGPUVirtualAddress());

  if (m_uiData.showQuadDomain)
  {
    if (m_uiData.showSolid)
    {
      commandList->SetPipelineState(m_pipelineStateSolid.Get());
    }
    else
    {
      commandList->SetPipelineState(m_pipelineStateWireframe.Get());
    }
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);
    commandList->DrawInstanced(4, 1, 0, 0);
  }
  else
  {
    if (m_uiData.showSolid)
    {
      commandList->SetPipelineState(m_pipelineStateSolidTriangle.Get());
    }
    else
    {
      commandList->SetPipelineState(m_pipelineStateWireframeTriangle.Get());
    }
    commandList->SetGraphicsRootSignature(m_rootSignature.Get());
    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
    commandList->DrawInstanced(3, 1, 0, 0);
  }
}

void PatchApp::onDrawUI()
{
  ImGui::Begin("Information", nullptr);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();

  ImGui::Begin("Configuration", nullptr);
  ImGui::Checkbox("Show Solid", &m_uiData.showSolid);
  ImGui::Checkbox("Switch to Quad Domain", &m_uiData.showQuadDomain);
  ImGui::SliderFloat("Outer tessellation factor", &m_uiData.outerTessFactor, 1.0f, 32.0f);
  ImGui::SliderFloat("Inner tessellation factor", &m_uiData.innerTessFactor, 1.0f, 32.0f);
  ImGui::End();
}