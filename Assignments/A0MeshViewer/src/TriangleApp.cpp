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
  f32m4 translation_mat = glm::transpose(
      glm::translate(f32m4(1), translation_vec)); /* f32m4(f32v4(1.0f, 0.0f, 0.0f, 0.0f), f32v4(0.0f, 1.0f, 0.0f, 0.0f),
                                f32v4(0.0f, 0.0f, 1.0f, 0.0f), f32v4(-translation_vec, 1.0f));*/

  // dann max min der längsten Achse abbilden auf -0.5 ... 0.5 --> Skalierung
  f32 scale_factor = 1.0f / std::max(std::abs(min_x) + std::abs(max_x),
                                     std::max(std::abs(min_y) + std::abs(max_y), std::abs(min_z) + std::abs(max_z)));

  f32m4 scale_mat =
      glm::transpose(glm::scale(f32m4(1), f32v3(scale_factor))); /* (f32v4(scale_factor, 0.0f, 0.0f, 0.0f), f32v4(0.0f,
             scale_factor, 0.0f, 0.0f),
                          f32v4(0.0f, 0.0f, scale_factor, 0.0f), f32v4(0.0f, 0.0f, 0.0f, 1.0f));*/

  f32m4 invertZ_mat = glm::transpose(glm::scale(f32m4(1), f32v3(1, 1, -1)));

  // Scale * Translate
  return /*invertZ_mat **/ scale_mat * translation_mat;
}
} // namespace

MeshViewer::MeshViewer(const DX12AppConfig config)
    : DX12App(config)
    , m_examinerController(true)
{
  m_triangleMeshProgram =
      HLSLProgram(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl", "VS_main", "PS_main");

  m_triangleMeshProgramWireFrame = HLSLProgram(L"../../../Assignments/A0MeshViewer/Shaders/TriangleMesh.hlsl",
                                               "VS_WireFrame_main", "PS_WireFrame_main");
  readMeshData();

  m_examinerController.setTranslationVector(f32v3(0, 0, 3));
  m_normalizationTransformation = getNormalizationTransformation(m_vertices, m_cbm.getNumVertices());

  createRootSignature();

  createPipeline(false, D3D12_CULL_MODE_NONE, &m_pipelineState);
  createPipeline(false, D3D12_CULL_MODE_BACK, &m_pipelineStateBackFaceCulling);
  createPipeline(true, D3D12_CULL_MODE_NONE, &m_pipelineStateWireFrame);
  createPipeline(true, D3D12_CULL_MODE_BACK, &m_pipelineStateWireFrameBackFaceCulling);

  createConstantBuffer();

  createTriangleMesh();
  createTexture();
}

MeshViewer::~MeshViewer()
{
}

#pragma region Texture

void MeshViewer::createTexture()
{
  i32 textureWidth, textureHeight, textureComp;

  stbi_set_flip_vertically_on_load(1);
  std::unique_ptr<ui8, void (*)(ui8*)> image(
      stbi_load("../../../data/bunny.png", &textureWidth, &textureHeight, &textureComp, 4),
      [](ui8* p) { stbi_image_free(p); });

  D3D12_RESOURCE_DESC textureDesc = {};
  textureDesc.MipLevels           = 1;
  textureDesc.Format              = DXGI_FORMAT_R8G8B8A8_UNORM;
  textureDesc.Width               = textureWidth;
  textureDesc.Height              = textureHeight;
  textureDesc.Flags               = D3D12_RESOURCE_FLAG_NONE;
  textureDesc.DepthOrArraySize    = 1;
  textureDesc.SampleDesc.Count    = 1;
  textureDesc.SampleDesc.Quality  = 0;
  textureDesc.Dimension           = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

  const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  throwIfFailed(getDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc,
                                                     D3D12_RESOURCE_STATE_COPY_DEST, nullptr,
                                                     IID_PPV_ARGS(&m_texture)));

  UploadHelper uploadHelper(getDevice(), GetRequiredIntermediateSize(m_texture.Get(), 0, 1));
  uploadHelper.uploadTexture(image.get(), m_texture, textureWidth, textureHeight, getCommandQueue());

  D3D12_DESCRIPTOR_HEAP_DESC desc = {};
  desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  desc.NumDescriptors             = 1;
  desc.NodeMask                   = 0;
  desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  throwIfFailed(getDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_srv)));

  D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = {};
  shaderResourceViewDesc.ViewDimension                   = D3D12_SRV_DIMENSION_TEXTURE2D;
  shaderResourceViewDesc.Shader4ComponentMapping         = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  shaderResourceViewDesc.Format                          = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  shaderResourceViewDesc.Texture2D.MipLevels             = 1;
  shaderResourceViewDesc.Texture2D.MostDetailedMip       = 0;
  shaderResourceViewDesc.Texture2D.ResourceMinLODClamp   = 0.0f;
  getDevice()->CreateShaderResourceView(m_texture.Get(), &shaderResourceViewDesc,
                                        m_srv->GetCPUDescriptorHandleForHeapStart());
}
#pragma endregion

#pragma region Root Signature& Pipeline creation

void MeshViewer::createRootSignature()
{
  CD3DX12_ROOT_PARAMETER parameters[2] {};

  parameters[0].InitAsConstantBufferView(1, 0, D3D12_SHADER_VISIBILITY_ALL);

  CD3DX12_DESCRIPTOR_RANGE range {D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0};
  parameters[1].InitAsDescriptorTable(1, &range);

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
  descRootSignature.Init(2, parameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

  ComPtr<ID3DBlob> rootBlob, errorBlob;
  D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob);

  getDevice()->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                   IID_PPV_ARGS(&m_rootSignature));
}

void MeshViewer::createPipeline(bool isWireFrame, D3D12_CULL_MODE cullMode, ComPtr<ID3D12PipelineState>* pipeLineState)
{
  D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};
  D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

  psoDesc.InputLayout    = {inputElementDescs, _countof(inputElementDescs)};
  psoDesc.pRootSignature = m_rootSignature.Get();
  if (isWireFrame)
  {
    psoDesc.VS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgramWireFrame.getVertexShader().Get());
    psoDesc.PS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgramWireFrame.getPixelShader().Get());
    psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
  }
  else
  {
    psoDesc.VS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgram.getVertexShader().Get());
    psoDesc.PS                       = CD3DX12_SHADER_BYTECODE(m_triangleMeshProgram.getPixelShader().Get());
    psoDesc.RasterizerState          = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
  }

  psoDesc.RasterizerState.CullMode = cullMode;

  psoDesc.BlendState                       = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
  psoDesc.DepthStencilState                = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
  psoDesc.SampleMask                       = UINT_MAX;
  psoDesc.PrimitiveTopologyType            = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  psoDesc.NumRenderTargets                 = 1;
  psoDesc.SampleDesc.Count                 = 1;
  psoDesc.RTVFormats[0]                    = getRenderTarget()->GetDesc().Format;
  psoDesc.DSVFormat                        = DXGI_FORMAT_D32_FLOAT;
  psoDesc.DepthStencilState.DepthEnable    = TRUE;
  psoDesc.DepthStencilState.DepthFunc      = D3D12_COMPARISON_FUNC_LESS;
  psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  psoDesc.DepthStencilState.StencilEnable  = FALSE;

  throwIfFailed(getDevice()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&(*pipeLineState))));
}
#pragma endregion

#pragma region Triangle Mesh

void MeshViewer::readMeshData()
{
  m_vertices   = reinterpret_cast<f32v3*>(m_cbm.getPositionsPtr());
  m_normals    = reinterpret_cast<f32v3*>(m_cbm.getAttributePtr(0));
  m_textCoords = reinterpret_cast<f32v2*>(m_cbm.getAttributePtr(1));

  m_vertexBufferCPU.resize(m_cbm.getNumVertices());
  for (ui32 i = 0; i < m_cbm.getNumVertices(); i++)
  {
    m_vertexBufferCPU[i] = Vertex {m_vertices[i], m_normals[i], m_textCoords[i]};
  }
}

void MeshViewer::createTriangleMesh()
{
  const auto vertexBufferCPUSizeInBytes = m_cbm.getNumVertices() * sizeof(Vertex);
  const auto indexBufferCPUSizeInBytes  = m_cbm.getNumTriangles() * 3 * sizeof(ui32);

  UploadHelper uploadBuffer(getDevice(), std::max(vertexBufferCPUSizeInBytes, indexBufferCPUSizeInBytes));

  const auto vertexBufferDesc      = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferCPUSizeInBytes);
  const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = (ui32)vertexBufferCPUSizeInBytes;
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);

  uploadBuffer.uploadBuffer(m_vertexBufferCPU.data(), m_vertexBuffer, vertexBufferCPUSizeInBytes, getCommandQueue());

  const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(indexBufferCPUSizeInBytes);
  getDevice()->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                       D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));

  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = (ui32)indexBufferCPUSizeInBytes;
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;

  uploadBuffer.uploadBuffer(m_cbm.getTriangleIndices(), m_indexBuffer, indexBufferCPUSizeInBytes, getCommandQueue());
}
#pragma endregion

#pragma region Constant Buffer

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
  // check for sensible values
  ConstantBuffer cb;
  float          sizeX = ImGui::GetIO().DisplaySize.x;
  float          sizeY = ImGui::GetIO().DisplaySize.y;
  sizeX                = sizeX > 0 ? sizeX : 1.0f;
  sizeY                = sizeY > 0 ? sizeY : 1.0f;
  const auto pM =
      glm::perspectiveFovLH_ZO(glm::radians(m_uiData.m_fov), sizeX, sizeY, m_uiData.m_zNear, m_uiData.m_zFar);

  cb.mv                         = m_examinerController.getTransformationMatrix() * m_normalizationTransformation;
  cb.mvp                        = pM * cb.mv;
  cb.mvIT                       = glm::inverseTranspose(cb.mv);
  cb.ambientColor               = f32v4(m_uiData.m_ambient, 1.0f);
  cb.diffuseColor               = f32v4(m_uiData.m_diffuse, 1.0f);
  cb.specularColor_and_Exponent = f32v4(m_uiData.m_specular, m_uiData.m_specularExponent);
  cb.wireFrameColor             = f32v4(m_uiData.m_wireFrameColor, 1.0f);
  cb.lightDirection             = f32v3(m_uiData.m_lightDirection);
  cb.flags                      = (ui32)m_uiData.m_useTwoSidedLighting | (ui32)m_uiData.m_useTexture << 1;

  const auto& currentConstantBuffer = m_constantBuffers[this->getFrameIndex()];
  void*       p;
  currentConstantBuffer->Map(0, nullptr, &p);
  memcpy(p, &cb, sizeof(cb));
  currentConstantBuffer->Unmap(0, nullptr);
}
#pragma endregion

#pragma region Draw

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
  updateConstantBuffer();

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

  commandList->SetGraphicsRootSignature(m_rootSignature.Get());

  commandList->SetGraphicsRootConstantBufferView(0, m_constantBuffers[getFrameIndex()].Get()->GetGPUVirtualAddress());

  const auto srvHandle = m_srv;
  commandList->SetDescriptorHeaps(1, srvHandle.GetAddressOf());
  commandList->SetGraphicsRootDescriptorTable(1, m_srv->GetGPUDescriptorHandleForHeapStart());

  if (m_uiData.m_useBackFaceCulling)
  {
    commandList->SetPipelineState(m_pipelineStateBackFaceCulling.Get());
  }
  else
  {
    commandList->SetPipelineState(m_pipelineState.Get());
  }
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->DrawIndexedInstanced(m_cbm.getNumTriangles() * 3, 1, 0, 0, 0);

  if (m_uiData.m_overLayWireFrame)
  {
    if (m_uiData.m_useBackFaceCulling)
    {
      commandList->SetPipelineState(m_pipelineStateWireFrameBackFaceCulling.Get());
    }
    else
    {
      commandList->SetPipelineState(m_pipelineStateWireFrame.Get());
    }
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
  ImGui::SliderFloat("Exponent", &m_uiData.m_specularExponent, 0.0f, 512.0f);
  ImGui::SliderFloat("FOV", &m_uiData.m_fov, 1.0f, 180.0f);
  ImGui::SliderFloat("Near Plane", &m_uiData.m_zNear, 0.0f, 100.0f);
  ImGui::SliderFloat("Far Plane", &m_uiData.m_zFar, 0.0f, 100.0f);
  ImGui::SliderFloat3("Light Direction", &m_uiData.m_lightDirection.x, -1.0f, 1.0f);
  ImGui::End();
}
#pragma endregion
