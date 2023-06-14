#pragma once
#include <d3d12.h>
#include <gimslib/types.hpp>
#include <vector>
#include <wrl.h>
using Microsoft::WRL::ComPtr;

namespace gims
{

/// <summary>
/// A D3D12 GPU Bezier Patch.
/// </summary>
class BezierPatchD3D12
{
public:
  /// <summary>
  /// Constructor that creates a D3D12 GPU Triangle mesh from positions, normals, and texture coordiantes.
  /// </summary>
  /// <param name="positions">Array of 3D positions. There must be nVertices elements in this array.</param>
  /// <param name="normals">Array of 3D normal vector. There must be nVertices elements in this array.</param>
  /// <param name="textureCoordinates">Array of 3D texture Coordinates. This class ignores the third component of each
  /// texture coordinate. There must be nVertices elements in this array.</param>
  /// <param name="nVertices">Number of vertices.</param>
  /// <param name="indexBuffer">Index buffer for triangle list. Triples of integer indices form a triangle.</param>
  /// <param name="nIndices">Number of indices (NOT the number triangles!)</param>
  /// <param name="materialIndex">Material index.</param>
  /// <param name="device">Device on which the GPU buffers should be created.</param>
  /// <param name="commandQueue">Command queue used to copy the data from the GPU to the GPU.</param>
  BezierPatchD3D12(const f32v3* positions, const ui32 nVertices, const ui32* indexBuffer, const ui32 nIndices,
                   const ComPtr<ID3D12Device>& device, const ComPtr<ID3D12CommandQueue>& commandQueue);

  /// <summary>
  /// Adds the commands neccessary for rendering this triangle mesh to the provided commandList.
  /// </summary>
  /// <param name="commandList">The command list</param>
  void addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList) const;

  /// <summary>
  /// Returns the input element descriptors required for the pipeline.
  /// </summary>
  /// <returns>The input element descriptor.</returns>
  static const std::vector<D3D12_INPUT_ELEMENT_DESC>& getInputElementDescriptors();

  BezierPatchD3D12();
  BezierPatchD3D12(const BezierPatchD3D12& other)                = default;
  BezierPatchD3D12(BezierPatchD3D12&& other) noexcept            = default;
  BezierPatchD3D12& operator=(const BezierPatchD3D12& other)     = default;
  BezierPatchD3D12& operator=(BezierPatchD3D12&& other) noexcept = default;

private:
  ui32                     m_nVertices;        //!
  ui32                     m_vertexBufferSize; //! Vertex buffer size in bytes.
  ComPtr<ID3D12Resource>   m_vertexBuffer;     //! The vertex buffer on the GPU.
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

  ui32                    m_nIndices;        //! Number of indices in the index buffer.
  ui32                    m_indexBufferSize; //! Index buffer size in bytes.
  ComPtr<ID3D12Resource>  m_indexBuffer;     //! The index buffer on the GPU.
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

  //! Input element descriptor defining the vertex format.
  static const std::vector<D3D12_INPUT_ELEMENT_DESC> m_inputElementDescs;
};

} // namespace gims
