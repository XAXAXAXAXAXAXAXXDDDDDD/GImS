#include "TriangleApp.h"
#include <algorithm>
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

namespace
{

f32m4 getNormalizationTransformation(f32v3 const* const positions, ui32 nPositions)
{
  // zuerst schwerpunkt über Mittelwert der Positionen --> Translation
  (void*)positions;
  (void)nPositions;

  // dann max min der längsten Achse abbilden auf -0.5 ... 0.5 --> Skalierung
  return f32m4(1);
}
} // namespace

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
{
  m_examinerController.setTranslationVector(f32v3(0, 0, 3));
  // m_normalizationTransformation = getNormalizationTransformation(positionsFloatVec, numVertices);

  m_vertices   = reinterpret_cast<f32v3*>(m_cbm.getPositionsPtr());
  m_normals    = reinterpret_cast<f32v3*>(m_cbm.getAttributePtr(0));
  m_textCoords = reinterpret_cast<f32v2*>(m_cbm.getAttributePtr(1));

  m_vertexBufferCPU.resize(m_cbm.getNumVertices());
  for (ui32 i = 0; i < m_cbm.getNumVertices(); i++)
  {
    m_vertexBufferCPU[i] = Vertex {m_vertices[i], m_normals[i], m_textCoords[i]};
  }

  createRootSignature();
  createPipeline();
  createPipelineWireFrame();
  createConstantBuffer();
  createTriangleMesh();
}

MeshViewer::~MeshViewer()
{
}

void MeshViewer::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER parameter = {};
  parameter.InitAsConstantBufferView(0, 0, D3D12_SHADER_VISIBILITY_ALL);

  CD3DX12_ROOT_SIGNATURE_DESC descRootSignature;
  descRootSignature.Init(1, &parameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void MeshViewer::createPipeline()
{
  m_triangleMeshProgram =
      HLSLProgram(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", "VS_main", "PS_main");
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

  psoDesc.InputLayout              = {inputElementDescs, _countof(inputElementDescs)};
  psoDesc.pRootSignature           = m_rootSignature.Get();
  psoDesc.VS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgram.getVertexShader().Get());
  psoDesc.PS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgram.getPixelShader().Get());
  psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  // psoDesc.RasterizerState.FrontCounterClockwise = TRUE;
  psoDesc.RasterizerState.CullMode         = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState                       = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState                = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask                       = UINT_MAX;
  psoDesc.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                 = 1;
  psoDesc.SampleDesc.Count                 = 1;
  psoDesc.RTVFormats[0]                    = getRenderTarget()->GetDesc().Format;
  psoDesc.DSVFormat                        = DXGI_FORMAT_D32_FLOAT; // getDepthStencil()->GetDesc().Format;
  psoDesc.DepthStencilState.DepthEnable    = TRUE;
  psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable  = FALSE;

  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));
}

void MeshViewer::createPipelineWireFrame()
{
  m_triangleMeshProgramWireFrame = HLSLProgram(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl",
                                               "VS_WireFrame_main", "PS_WireFrame_main");
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

  psoDesc.InputLayout              = {inputElementDescs, _countof(inputElementDescs)};
  psoDesc.pRootSignature           = m_rootSignature.Get();
  psoDesc.VS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgramWireFrame.getVertexShader().Get());
  psoDesc.PS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgramWireFrame.getPixelShader().Get());
  psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
  psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
  psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
  psoDesc.BlendState               = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState        = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask               = UINT_MAX;
  psoDesc.PrimitiveTopologyType    = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets         = 1;
  psoDesc.SampleDesc.Count         = 1;
  psoDesc.RTVFormats[0]            = getRenderTarget()->GetDesc().Format;
  psoDesc.DSVFormat                = DXGI_FORMAT_D32_FLOAT; // getDepthStencil()->GetDesc().Format;
  psoDesc.DepthStencilState.DepthEnable    = TRUE;
  psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable  = FALSE;

  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineStateWireFrame)));
}

void MeshViewer::createTriangleMesh(/*std::vector<Vertex> vertices, std::vector<ui32> indices*/)
{
  // std::vector<Vertex> vertexBufferCPU = vertices;
  // std::vector<ui32>   indexBufferCPU  = indices;

  const auto vertexBufferCPUSizeInBytes = m_cbm.getNumVertices() * sizeof(Vertex);
  const auto indexBufferCPUSizeInBytes  = m_cbm.getNumTriangles() * 3 * sizeof(ui32);

  UploadHelper uploadBuffer(getDevice(), std::max(vertexBufferCPUSizeInBytes, indexBufferCPUSizeInBytes));

  const auto vertexBufferDesc      = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferCPUSizeInBytes);
  const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                       D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = (ui32)vertexBufferCPUSizeInBytes;
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);

  uploadBuffer.uploadBuffer(/*reinterpret_cast<f32v3*>(m_cbm.getPositionsPtr())*/ m_vertexBufferCPU.data(),
                            m_vertexBuffer, vertexBufferCPUSizeInBytes, getCommandQueue());

  // uploadBuffer.uploadBuffer(m_vertexBufferCPU.data(), m_vertexBuffer, vertexBufferCPUSizeInBytes, getCommandQueue());

  const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferCPUSizeInBytes);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                       D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&m_indexBuffer));

  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = (ui32)indexBufferCPUSizeInBytes;
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  uploadBuffer.uploadBuffer(/*m_indexBufferCPU.data()*/ m_cbm.getTriangleIndices(), m_indexBuffer,
                            indexBufferCPUSizeInBytes, getCommandQueue());
}

void MeshViewer::createConstantBuffer()
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

void MeshViewer::updateConstantBuffer()
{
  ConstantBuffer cb;
  float          sizeX = ImGui::GetIO().DisplaySize.x;
  float          sizeY = ImGui::GetIO().DisplaySize.y;
  const auto     pM    = glm::perspectiveFovLH_ZO(glm::radians(60.0f), sizeX, sizeY, 0.1f, 5.1f);

  cb.mv                         = m_examinerController.getTransformationMatrix();
  cb.mvp                        = pM * cb.mv;
  cb.ambientColor               = f32v4(0.0f, 0.0f, 0.0f, 0.0f);
  cb.diffuseColor               = f32v4(1.0f, 1.0f, 1.0f, 1.0f);
  cb.specularColor_and_Exponent = f32v4(1.0f, 1.0f, 1.0f, 128.0f);
  cb.wireFrameColor             = f32v4(m_uiData.m_wireFrameColor, 1.0f);
  cb.flags                      = 0x1;

  const auto& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];
  void*       p;
  currentConstantBuffer->Map(0, nullptr, &p);
  memcpy(p, &cb, sizeof(cb));
  currentConstantBuffer->Unmap(0, nullptr);
}

void MeshViewer::onDraw()
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
  // Use this to get the transformation Matrix.
  updateConstantBuffer(/*m_examinerController.getTransformationMatrix()*/);

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

  commandList->SetPipelineState(m_pipelineState.Get());
  commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffers[getFrameIndex()].Get()->GetGPUVirtualAddress());
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->DrawIndexedInstanced(m_cbm.getNumTriangles() * 3, 1, 0, 0, 0);

  if (m_uiData.m_overLayWireFrame)
  {
    commandList->SetPipelineState(m_pipelineStateWireFrame.Get());

    commandList->DrawIndexedInstanced(m_cbm.getNumTriangles() * 3, 1, 0, 0, 0);
  }
}

void MeshViewer::onDrawUI()
{
  const auto imGuiFlags = m_examinerController.active() ? ImGuiWindowFlags_NoInputs : ImGuiWindowFlags_None;
  ImGui::Begin("Information", nullptr, imGuiFlags);
  ImGui::Text("Frametime: %f", 1.0f / ImGui::GetIO().Framerate * 1000.0f);
  ImGui::End();

  ImGui::Begin("Configuration", nullptr, imGuiFlags);
  f32v3 col = f32v3(1.0f, 1.0f, 1.0f);
  ImGui::ColorEdit3("Background Color", &m_uiData.m_backgroundColor.x);
  ImGui::Checkbox("Back-Face Culling", &m_uiData.m_useBackFaceCulling);
  ImGui::Checkbox("Overlay WireFrame", &m_uiData.m_overLayWireFrame);
  ImGui::ColorEdit3("WireFrame Color", &m_uiData.m_wireFrameColor.x);
  ImGui::Checkbox("Two-Sided Lighting", &m_uiData.m_useTwoSidedLighting);
  ImGui::Checkbox("Use Texture", &m_uiData.m_useTexture);
  ImGui::ColorEdit3("Ambient", &m_uiData.m_ambient.x);
  ImGui::ColorEdit3("Diffuse", &m_uiData.m_diffuse.x);
  ImGui::ColorEdit3("Specular", &m_uiData.m_specular.x);
  // ImGui::VSliderFloat("Exponent", &m_uiData.m_specularExponent);
  ImGui::End();
}
