#include "TriangleApp.h"

void TriangleApp::createPipeline()
{
  m_hlslProgram = HLSLProgram(L"../../../Project/P0TessellationExample/Shaders/TriangleMesh.hlsl", "VS_main", "PS_main",
                              "HS_main", "DS_main");

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {};
  psoDesc.pRootSignature                     = m_rootSignature.Get();
  psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(m_hlslProgram.getVertexShader().Get());
  psoDesc.HS                                 = CD3DX12_SHADER_BYTECODE(m_hlslProgram.getHullShader().Get());
  psoDesc.DS                                 = CD3DX12_SHADER_BYTECODE(m_hlslProgram.getDomainShader().Get());
  psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(m_hlslProgram.getPixelShader().Get());
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode           = D3D12_FILL_MODE_WIREFRAME;
  psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState.DepthEnable      = FALSE;
  psoDesc.DepthStencilState.StencilEnable    = FALSE;
  psoDesc.SampleMask                         = UINT_MAX;
  psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
  psoDesc.NumRenderTargets                   = 1;
  psoDesc.SampleDesc.Count                   = 1;
  psoDesc.RTVFormats[0]                      = getRenderTarget()->GetDesc().Format;
  psoDesc.DSVFormat                          = getDepthStencil()->GetDesc().Format;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void TriangleApp::createRootSignature()
{
  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
  descRootSignature.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_NONE);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void TriangleApp::onDraw()
{
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
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_3_CONTROL_POINT_PATCHLIST);
  commandList->DrawInstanced(3, 1, 0, 0);
}

void TriangleApp::onDrawUI()
{
  // const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  ImGui::Begin("Information", nullptr);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();

  // ImGui::Begin("Configuration", nullptr, imGuiFlags);
  // f32v3 col = f32v3(1.0f, 1.0f, 1.0f);
  // ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor.x);
  // ImGui::Checkbox("Back-Face Culling", &m_uiData.m_useBackFaceCulling);
  // ImGui::Checkbox("Overlay WireFrame", &m_uiData.m_overLayWireFrame);
  // ImGui::ColorEdit3("WireFrame Color", &m_uiData.m_wireFrameColor.x);
  // ImGui::Checkbox("Two-Sided Lighting", &m_uiData.m_useTwoSidedLighting);
  // ImGui::Checkbox("Use Texture", &m_uiData.m_useTexture);
  // ImGui::ColorEdit3("Ambient", &m_uiData.m_ambient.x);
  // ImGui::ColorEdit3("Diffuse", &m_uiData.m_diffuse.x);
  // ImGui::ColorEdit3("Specular", &m_uiData.m_specular.x);
  // ImGui::SliderFloat("Exponent", &m_uiData.m_specularExponent, 0.0f, 512.0f);
  // ImGui::SliderFloat("FOV", &m_uiData.m_fov, 1.0f, 180.0f);
  // ImGui::SliderFloat("Near Plane", &m_uiData.m_zNear, 0.0f, 100.0f);
  // ImGui::SliderFloat("Far Plane", &m_uiData.m_zFar, 0.0f, 100.0f);
  // ImGui::SliderFloat3("Light Direction", &m_uiData.m_lightDirection.x, -1.0f, 1.0f);
  // ImGui::End();
}