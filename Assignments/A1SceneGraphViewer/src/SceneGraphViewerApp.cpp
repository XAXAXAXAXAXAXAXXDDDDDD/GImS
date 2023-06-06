#include "SceneGraphViewerApp.hpp"
#include "SceneFactory.hpp"
#include <gimslib/contrib/d3dx12/d3dx12.h>
#include <gimslib/contrib/stb/stb_image.h>
#include <gimslib/d3d/DX12Util.hpp>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/sys/Event.hpp>
#include <imgui.h>
#include <iostream>
#include <vector>
using namespace gims;

SceneGraphViewerApp::SceneGraphViewerApp(const DX12AppConfig config, const std::filesystem::path pathToScene)
    : DX12App(config)
    , m_examinerController(true)
    , m_scene(SceneGraphFactory::createFromAssImpScene(pathToScene, getDevice(), getCommandQueue()))
{
  m_examinerController.setTranslationVector(f32v3(0, -0.25f, 1.5));
  m_shader = HLSLProgram(L"../../../Assignments/A1SceneGraphViewer/Shaders/TriangleMesh.hlsl", "VS_main", "PS_main");
  createRootSignature();
  createSceneConstantBuffer();
  // createMeshConstantBuffer();
  createPipeline();
  createPipelineBoundingBox();
}

void SceneGraphViewerApp::onDraw()
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

  const auto commandList = getCommandList();
  const auto rtvHandle   = getRTVHandle();
  const auto dsvHandle   = getDSVHandle();

  commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);

  const float clearColor[] = {m_uiData.m_backgroundColor.x, m_uiData.m_backgroundColor.y, m_uiData.m_backgroundColor.z,
                              1.0f};
  commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
  commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

  commandList->RSSetViewports(1, &getViewport());
  commandList->RSSetScissorRects(1, &getRectScissor());

  drawScene(commandList);

  // now draw wireframe
  if (m_uiData.m_showBoundingBox)
  {
    drawSceneBoundingBox(commandList);
  }
}

void SceneGraphViewerApp::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();
  ImGui::Begin("Configuration", nullptr, imGuiFlags);
  ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor[0]);
  ImGui::Checkbox("Show Bounding Boxes", &m_uiData.m_showBoundingBox);
  ImGui::End();
}

void SceneGraphViewerApp::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER parameters[4] {};

  CD3DX12_DESCRIPTOR_RANGE range1 {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0};
  // parameters[3].InitAsDescriptorTable(1, &range);

  CD3DX12_DESCRIPTOR_RANGE range2 {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1};
  // parameters[4].InitAsDescriptorTable(1, &range2);

  CD3DX12_DESCRIPTOR_RANGE range3 {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2};
  // parameters[5].InitAsDescriptorTable(1, &range3);

  CD3DX12_DESCRIPTOR_RANGE range4 {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 3};
  // parameters[6].InitAsDescriptorTable(1, &range4);

  CD3DX12_DESCRIPTOR_RANGE range5 {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 4};
  /*parameters[7].InitAsDescriptorTable(1, &range5);*/

  CD3DX12_DESCRIPTOR_RANGE ranges[5] = {range1, range2, range3, range4, range5};

  parameters[3].InitAsDescriptorTable(5, ranges);

  parameters[0].InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);
  parameters[1].InitAsConstants(16, 1, D3D12_SHADER_VISIBILITY_ALL);
  parameters[2].InitAsConstantBufferView(2, 0, D3D12_SHADER_VISIBILITY_ALL);

  D3D12_STATIC_SAMPLER_DESC sampler = {};
  sampler.Filter                    = D3D12_FILTER_MIN_MAG_MIP_POINT;
  sampler.AddressU                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressV                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.AddressW                  = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  sampler.MipLODBias                = 0;
  sampler.MaxAnisotropy             = 0;
  sampler.ComparisonFunc            = D3D12_COMPARISON_FUNC_NEVER;
  sampler.BorderColor               = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
  sampler.MinLOD                    = 0.0f;
  sampler.MaxLOD                    = D3D12_FLOAT32_MAX;
  sampler.ShaderRegister            = 0;
  sampler.RegisterSpace             = 0;
  sampler.ShaderVisibility          = D3D12_SHADER_VISIBILITY_ALL;

  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
  descRootSignature.Init(/*8*/ 4, parameters, 1, &sampler,
                         D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void SceneGraphViewerApp::createPipeline()
{
  waitForGPU();
  const auto inputElementDescs = TriangleMeshD3D12::getInputElementDescriptors();

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {inputElementDescs.data(), (ui32)inputElementDescs.size()};
  psoDesc.pRootSignature                     = m_rootSignature.Get();
  psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(m_shader.getVertexShader().Get());
  psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(m_shader.getPixelShader().Get());
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode           = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
  psoDesc.DepthStencilState.DepthEnable      = TRUE;
  psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable    = FALSE;
  psoDesc.SampleMask                         = UINT_MAX;
  psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                   = 1;
  psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
  psoDesc.SampleDesc.Count                   = 1;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void SceneGraphViewerApp::drawScene(const ComPtr<ID3D12GraphicsCommandList>& cmdLst)
{
  updateSceneConstantBuffer();

  // updateMeshConstantBuffer();

  // Assignment 2: Uncomment after successfull implementation.
  const auto cb = m_constantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  // const auto cb_mesh      = m_constantBuffers_Mesh[getFrameIndex()].getResource()->GetGPUVirtualAddress();
  const auto cameraMatrix = m_examinerController.getTransformationMatrix();

  // Assignment 6

  cmdLst->SetPipelineState(m_pipelineState.Get());

  // Assignment 2: Uncomment after successfull implementation.
  cmdLst->SetGraphicsRootSignature(m_rootSignature.Get());
  cmdLst->SetGraphicsRootConstantBufferView(0, cb);
  // cmdLst->SetGraphicsRootConstantBufferView(1, cb_mesh);

  f32m4 transformation = cameraMatrix * m_scene.getAABB().getNormalizationTransformation();

  m_scene.addToCommandList(cmdLst, transformation, 1, 2, 3);
}

#pragma region Bounding Box

void SceneGraphViewerApp::drawSceneBoundingBox(const ComPtr<ID3D12GraphicsCommandList>& commandList)
{
  updateSceneConstantBuffer();

  const auto cb = m_constantBuffers[getFrameIndex()].getResource()->GetGPUVirtualAddress();

  const auto cameraMatrix = m_examinerController.getTransformationMatrix();

  commandList->SetPipelineState(m_pipelineStateBoundingBox.Get());

  commandList->SetGraphicsRootSignature(m_rootSignature.Get());
  commandList->SetGraphicsRootConstantBufferView(0, cb);

  f32m4 transformation = cameraMatrix * m_scene.getAABB().getNormalizationTransformation();

  m_scene.addToCommandListBoundingBox(commandList, transformation, 1);
}

void SceneGraphViewerApp::createPipelineBoundingBox()
{
  HLSLProgram shaderBoundingBox = HLSLProgram(L"../../../Assignments/A1SceneGraphViewer/Shaders/TriangleMesh.hlsl",
                                            "VS_BoundingBox_main", "PS_BoundingBox_main");

  waitForGPU();
  const auto inputElementDescs = TriangleMeshD3D12::getInputElementDescriptorsBoundingBox();

  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
  psoDesc.InputLayout                        = {inputElementDescs.data(), (ui32)inputElementDescs.size()};
  psoDesc.pRootSignature                     = m_rootSignature.Get();
  psoDesc.VS                                 = CD3DX12_SHADER_BYTECODE(shaderBoundingBox.getVertexShader().Get());
  psoDesc.PS                                 = CD3DX12_SHADER_BYTECODE(shaderBoundingBox.getPixelShader().Get());
  psoDesc.RasterizerState                    = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode           = D3D12_FILL_MODE_SOLID;
  psoDesc.RasterizerState.CullMode           = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                         = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DSVFormat                          = getDX12AppConfig().depthBufferFormat;
  psoDesc.DepthStencilState.DepthEnable      = TRUE;
  psoDesc.DepthStencilState.DepthFunc        = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask   = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable    = FALSE;
  psoDesc.SampleMask                         = UINT_MAX;
  psoDesc.PrimitiveTopologyType              = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  psoDesc.NumRenderTargets                   = 1;
  psoDesc.RTVFormats[0]                      = getDX12AppConfig().renderTargetFormat;
  psoDesc.SampleDesc.Count                   = 1;
  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateBoundingBox)));
}

#pragma endregion

namespace PerSceneConstants
{
struct ConstantBuffer
{
  f32m4 projectionMatrix;
};

} // namespace PerSceneConstants

// namespace PerMeshConstants
//{
// struct ConstantBuffer
//{
//   f32m4 modelViewMatrix;
// };
//
// } // namespace PerMeshConstants

void SceneGraphViewerApp::createSceneConstantBuffer()
{
  const PerSceneConstants::ConstantBuffer cb         = {};
  const auto                              frameCount = getDX12AppConfig().frameCount;
  m_constantBuffers.resize(frameCount);
  for (ui32 i = 0; i < frameCount; i++)
  {
    m_constantBuffers[i] = ConstantBufferD3D12(cb, getDevice());
  }
}

void SceneGraphViewerApp::updateSceneConstantBuffer()
{
  PerSceneConstants::ConstantBuffer cb {};
  cb.projectionMatrix =
      glm::perspectiveFovLH_ZO<f32>(glm::radians(45.0f), (f32)getWidth(), (f32)getHeight(), 1.0f / 256.0f, 256.0f);
  m_constantBuffers[getFrameIndex()].upload(&cb);
}

// void SceneGraphViewerApp::createMeshConstantBuffer()
//{
//   const PerMeshConstants::ConstantBuffer cb         = {};
//   const auto                             frameCount = getDX12AppConfig().frameCount;
//   m_constantBuffers_Mesh.resize(frameCount);
//   for (ui32 i = 0; i < frameCount; i++)
//   {
//     m_constantBuffers_Mesh[i] = ConstantBufferD3D12(cb, getDevice());
//   }
// }
//
// void SceneGraphViewerApp::updateMeshConstantBuffer()
//{
//   PerMeshConstants::ConstantBuffer cb {};
//   cb.modelViewMatrix =
//       m_examinerController.getTransformationMatrix() * m_scene.getMesh(1).getAABB().getNormalizationTransformation();
//   m_constantBuffers_Mesh[getFrameIndex()].upload(&cb);
// }