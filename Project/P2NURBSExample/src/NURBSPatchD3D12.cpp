#include "NURBSPatchD3D12.h"
#include <gimslib/contrib/d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <iostream>
namespace
{
struct Vertex
{
  gims::f32v3 position;
};

} // namespace

namespace gims
{
const std::vector<D3D12_INPUT_ELEMENT_DESC> BezierPatchD3D12::m_inputElementDescs = {
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

NURBSPatchD3D12::NURBSPatchD3D12(const f32v3* positions, const ui32 nVertices, const ui32* indexBuffer,
                                   const ui32 nIndices, const ComPtr<ID3D12Device>& device,
                                   const ComPtr<ID3D12CommandQueue>& commandQueue)
    : m_vertexBufferSize(static_cast<ui32>(nVertices * sizeof(Vertex)))
    , m_indexBufferSize(static_cast<ui32>(nIndices * sizeof(ui32)))
    , m_nVertices(nVertices)
    , m_nIndices(nIndices)
{
  // create vertex buffer on cpu
  std::vector<Vertex> m_vertexBufferCPU;
  m_vertexBufferCPU.resize(nVertices);

  for (ui32 i = 0; i < nVertices; i++)
  {
    m_vertexBufferCPU[i] = Vertex {positions[i]};
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
}

void NURBSPatchD3D12::addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList) const
{
  commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST);
  commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);
  commandList->IASetIndexBuffer(&m_indexBufferView);

  commandList->DrawIndexedInstanced(m_nIndices, 1, 0, 0, 0);
}

const std::vector<D3D12_INPUT_ELEMENT_DESC>& BezierPatchD3D12::getInputElementDescriptors()
{
  return m_inputElementDescs;
}

NURBSPatchD3D12::BezieNURBSPatchD3D12rPatchD3D12()
    : m_vertexBufferSize(0)
    , m_vertexBufferView()
    , m_nVertices(0)
{
}

} // namespace gims