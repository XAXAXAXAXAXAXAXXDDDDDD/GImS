#include "PatchApp.h"

void PatchApp::createPipeline(bool isSolid)
{
  HLSLProgram program;
  if (isSolid)
  {
    program = m_hlslProgramSolid;
  }
  else
  {
    program = m_hlslProgramWireframe;
  }
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {m_bezierPatch.getInputElementDescriptors().data(),
                                                (ui32)(m_bezierPatch.getInputElementDescriptors().size())};
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
  psoDesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                       = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState                = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask                       = UINT_MAX;
  psoDesc.DepthStencilState.DepthEnable    = TRUE;
  psoDesc.DepthStencilState.StencilEnable  = FALSE;
  psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
  psoDesc.NumRenderTargets                 = 1;
  psoDesc.SampleDesc.Count                 = 1;
  psoDesc.RTVFormats[0]                    = getRenderTarget()->GetDesc().Format;
  psoDesc.DSVFormat                        = DXGI_FORMAT_D32_FLOAT;
  if (isSolid)
  {
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateSolid)));
  }
  else
  {
    throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateWireframe)));
  }
}

void PatchApp::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER parameters[1] {};

  parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
  descRootSignature.Init(1, parameters, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

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
  cb.tessFactor = m_uiData.tessFactor;
  float sizeX   = ImGui::GetIO().DisplaySize.x;
  float sizeY   = ImGui::GetIO().DisplaySize.y;
  sizeX         = sizeX > 0 ? sizeX : 1.0f;
  sizeY         = sizeY > 0 ? sizeY : 1.0f;
  const auto pM = glm::perspectiveFovLH_ZO(glm::radians(30.0f), sizeX, sizeY, 0.1f, 20.0f);

  cb.mvp = glm::transpose(pM * m_examinerController.getTransformationMatrix() * m_normalizationTransformation);
  // /*m_normalizationTransformation; */
  // glm::identity<f32m4>();
  const auto& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];
  void*       p;
  currentConstantBuffer->Map(0, nullptr, &p);
  memcpy(p, &cb, sizeof(cb));
  currentConstantBuffer->Unmap(0, nullptr);
}

void PatchApp::onDraw()
{
  if (!ImGui::GetIO().WantCaptureMouse)
  {
    bool pressed  = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseClicked(ImGuiMouseButton_Right);
    bool released = ImGui::IsMouseReleased(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Right);
    if (pressed || released)
    {
      bool left = ImGui::IsMouseClicked(ImGuiMouseButton_Left) || ImGui::IsMouseReleased(ImGuiMouseButton_Left);
      m_examinerController.click(pressed, left == true ? 1 : 2,
                                 ImGui::IsKeyDown(ImGuiKey_LeftCtrl) || ImGui::IsKeyDown(ImGuiKey_RightCtrl),
                                 getNormalizedMouseCoordinates());
    }
    else
    {
      m_examinerController.move(getNormalizedMouseCoordinates());
    }
  }

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

  if (m_uiData.showSolid)
  {
    commandList->SetPipelineState(m_pipelineStateSolid.Get());
  }
  else
  {
    commandList->SetPipelineState(m_pipelineStateWireframe.Get());
  }

  m_bezierPatch.addToCommandList(commandList);
  // here draw bezier patch
}

void PatchApp::onDrawUI()
{
  ImGui::Begin("Information", nullptr);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();

  ImGui::Begin("Configuration", nullptr);
  ImGui::Checkbox("Show Solid", &m_uiData.showSolid);
  ImGui::SliderFloat("Tessellation factor", &m_uiData.tessFactor, 1.0f, 32.0f);
  ImGui::End();
}