#include "TriangleMeshD3D12.hpp"
#include <gimslib/contrib/d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <iostream>
namespace
{
struct Vertex
{
  gims::f32v3 position;
  gims::f32v3 normal;
  gims::f32v2 textureCoordinate;
};

struct VertexWireframe
{
  gims::f32v3 position;
};
} // namespace

namespace gims
{
const std::vector<D3D12_INPUT_ELEMENT_DESC> TriangleMeshD3D12::m_inputElementDescs = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

TriangleMeshD3D12::TriangleMeshD3D12(f32v3 const* const positions, f32v3 const* const normals,
                                     f32v3 const* const textureCoordinates, ui32 nVertices,
                                     ui32v3 const* const indexBuffer, ui32 nIndices, ui32 materialIndex,
                                     const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue)
    : m_nIndices(nIndices)
    , m_vertexBufferSize(static_cast<ui32>(nVertices * sizeof(Vertex)))
    , m_indexBufferSize(static_cast<ui32>(nIndices * sizeof(ui32)))
    , m_aabb(positions, nVertices)
    , m_materialIndex(materialIndex)
{
  // create vertex buffer on cpu
  std::vector<Vertex> m_vertexBufferCPU;
  m_vertexBufferCPU.resize(nVertices);

  for (ui32 i = 0; i < nVertices; i++)
  {
    if (!textureCoordinates)
    {
      m_vertexBufferCPU[i] = Vertex {positions[i], normals[i], f32v2(0.0f, 0.0f)};
    }
    else
    {
      m_vertexBufferCPU[i] = Vertex {positions[i], normals[i], textureCoordinates[i]};
    }
  }

  // create upload helper
  UploadHelper uploadBuffer(device, std::max(m_vertexBufferSize, m_indexBufferSize));
  // create gpu vertex buffer
  const auto vertexBufferDesc      = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);
  const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                  D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBuffer));

  m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
  m_vertexBufferView.SizeInBytes    = (ui32)m_vertexBufferSize;
  m_vertexBufferView.StrideInBytes  = sizeof(Vertex);
  // upload vertex buffer
  uploadBuffer.uploadBuffer(m_vertexBufferCPU.data(), m_vertexBuffer, m_vertexBufferSize, commandQueue);

  // create gpu index buffer
  const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize);
  device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                  D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBuffer));

  m_indexBufferView.BufferLocation = m_indexBuffer->GetGPUVirtualAddress();
  m_indexBufferView.SizeInBytes    = (ui32)m_indexBufferSize;
  m_indexBufferView.Format         = DXGI_FORMAT_R32_UINT;
  // upload index buffer
  uploadBuffer.uploadBuffer(indexBuffer, m_indexBuffer, m_indexBufferSize, commandQueue);

  createVertexAndIndexBufferBoundingBox(device, commandQueue);
}

void TriangleMeshD3D12::addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

const AABB& TriangleMeshD3D12::getAABB() const
{
  return m_aabb;
}

const ui32 TriangleMeshD3D12::getMaterialIndex() const
{
  return m_materialIndex;
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& TriangleMeshD3D12::getInputElementDescriptors()
{
  return m_inputElementDescs;
}

TriangleMeshD3D12::TriangleMeshD3D12()
    : m_nIndices(0)
    , m_vertexBufferSize(0)
    , m_indexBufferSize(0)
    , m_materialIndex((ui32)-1)
    , m_vertexBufferView()
    , m_indexBufferView()
    , m_indexBufferViewBoundingBox()
    , m_vertexBufferViewBoundingBox()
{
}

#pragma region Wireframe

const std::vector<D3D12_INPUT_ELEMENT_DESC> TriangleMeshD3D12::m_inputElementDescsBoundingBox = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

const std::vector<D3D12_INPUT_ELEMENT_DESC>& TriangleMeshD3D12::getInputElementDescriptorsBoundingBox()
{
  return m_inputElementDescsBoundingBox;
}

void TriangleMeshD3D12::createVertexAndIndexBufferBoundingBox(const ComPtr<ID3D12Device>&       device,
                                                              const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  f32v3 p0 = f32v3(getAABB().getUpperRightTop().x, getAABB().getUpperRightTop().y, getAABB().getUpperRightTop().z);
  f32v3 p1 = f32v3(getAABB().getUpperRightTop().x, getAABB().getLowerLeftBottom().y, getAABB().getUpperRightTop().z);
  f32v3 p2 = f32v3(getAABB().getLowerLeftBottom().x, getAABB().getLowerLeftBottom().y, getAABB().getUpperRightTop().z);
  f32v3 p3 = f32v3(getAABB().getLowerLeftBottom().x, getAABB().getUpperRightTop().y, getAABB().getUpperRightTop().z);

  f32v3 p4 = f32v3(getAABB().getUpperRightTop().x, getAABB().getUpperRightTop().y, getAABB().getLowerLeftBottom().z);
  f32v3 p5 = f32v3(getAABB().getUpperRightTop().x, getAABB().getLowerLeftBottom().y, getAABB().getLowerLeftBottom().z);
  f32v3 p6 =
      f32v3(getAABB().getLowerLeftBottom().x, getAABB().getLowerLeftBottom().y, getAABB().getLowerLeftBottom().z);
  f32v3 p7 = f32v3(getAABB().getLowerLeftBottom().x, getAABB().getUpperRightTop().y, getAABB().getLowerLeftBottom().z);

  f32v3 positions[] = {p0, p1, p2, p3, p4, p5, p6, p7};

  ui32 m_vertexBufferSize = static_cast<ui32>(8 * sizeof(VertexWireframe));
  ui32 m_indexBufferSize  = static_cast<ui32>(24 * sizeof(ui32));

  // create vertex buffer on cpu
  std::vector<VertexWireframe> m_vertexBufferCPU;
  m_vertexBufferCPU.resize(8);

  for (ui32 i = 0; i < 8; i++)
  {
    m_vertexBufferCPU[i] = VertexWireframe {positions[i]};
  }

  // create upload helper
  UploadHelper uploadBuffer(device, std::max(m_vertexBufferSize, m_indexBufferSize));
  // create gpu vertex buffer
  const auto vertexBufferDesc      = CD3DX12_RESOURCE_DESC::Buffer(m_vertexBufferSize);
  const auto defaultHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
  device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &vertexBufferDesc,
                                  D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_vertexBufferBoundingBox));

  m_vertexBufferViewBoundingBox.BufferLocation = m_vertexBufferBoundingBox->GetGPUVirtualAddress();
  m_vertexBufferViewBoundingBox.SizeInBytes    = (ui32)m_vertexBufferSize;
  m_vertexBufferViewBoundingBox.StrideInBytes  = sizeof(VertexWireframe);
  // upload vertex buffer
  uploadBuffer.uploadBuffer(m_vertexBufferCPU.data(), m_vertexBufferBoundingBox, m_vertexBufferSize, commandQueue);

  ui32 indexBuffer[] = {0, 1, 1, 2, 2, 3, 3, 0, 4, 0, 4, 5, 5, 1, 5, 6, 2, 6, 6, 7, 3, 7, 4, 7};

  // create gpu index buffer
  const auto indexBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(m_indexBufferSize);
  device->CreateCommittedResource(&defaultHeapProperties, D3D12_HEAP_FLAG_NONE, &indexBufferDesc,
                                  D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&m_indexBufferBoundingBox));

  m_indexBufferViewBoundingBox.BufferLocation = m_indexBufferBoundingBox->GetGPUVirtualAddress();
  m_indexBufferViewBoundingBox.SizeInBytes    = (ui32)m_indexBufferSize;
  m_indexBufferViewBoundingBox.Format         = DXGI_FORMAT_R32_UINT;
  // upload index buffer
  uploadBuffer.uploadBuffer(indexBuffer, m_indexBufferBoundingBox, m_indexBufferSize, commandQueue);
}

void TriangleMeshD3D12::addToCommandListBoundingBox(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferViewBoundingBox);
  commandList->IASetIndexBuffer(&m_indexBufferViewBoundingBox);

  commandList->DrawIndexedInstanced(24, 1, 0, 0, 0);
}

#pragma endregion

} // namespace gims