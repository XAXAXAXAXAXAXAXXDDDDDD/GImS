#include "Scene.hpp"
#include <gimslib/contrib/d3dx12/d3dx12.h>
#include <unordered_map>

using namespace gims;

namespace
{
void addToCommandListImpl(Scene& scene, ui32 nodeIdx, f32m4 transformation,
                          const ComPtr<ID3D12GraphicsCommandList>& commandList, ui32 modelViewRootParameterIdx,
                          ui32 materialConstantsRootParameterIdx, ui32 srvRootParameterIdx)
{
  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  Scene::Node currNode = scene.getNode(nodeIdx);
  transformation       = transformation * currNode.transformation;

  for (auto it = currNode.meshIndices.begin(); it != currNode.meshIndices.end(); it++)
  {
    const auto trafo    = transformation;
    void*      trafoPtr = (void*)&trafo;
    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 16, trafoPtr, 0);

    auto material = scene.getMaterial(scene.getMesh(*it).getMaterialIndex());

    commandList->SetGraphicsRootConstantBufferView(
        materialConstantsRootParameterIdx, material.materialConstantBuffer.getResource()->GetGPUVirtualAddress());

    commandList->SetDescriptorHeaps(1, material.srvDescriptorHeap.GetAddressOf());

    auto GPU_base_handle = material.srvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
    commandList->SetGraphicsRootDescriptorTable(srvRootParameterIdx, GPU_base_handle);

    scene.getMesh(*it).addToCommandList(commandList);
  }

  for (auto it = currNode.childIndices.begin(); it != currNode.childIndices.end(); it++)
  {
    addToCommandListImpl(scene, *it, transformation, commandList, modelViewRootParameterIdx,
                         materialConstantsRootParameterIdx, srvRootParameterIdx);
  }
}

void addToCommandListImplBoundingBox(Scene& scene, ui32 nodeIdx, f32m4 transformation,
                                     const ComPtr<ID3D12GraphicsCommandList>& commandList,
                                     ui32                                     modelViewRootParameterIdx)
{
  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  Scene::Node currNode = scene.getNode(nodeIdx);
  transformation       = transformation * currNode.transformation;

  for (auto it = currNode.meshIndices.begin(); it != currNode.meshIndices.end(); it++)
  {
    const auto trafo    = transformation;
    void*      trafoPtr = (void*)&trafo;
    commandList->SetGraphicsRoot32BitConstants(modelViewRootParameterIdx, 16, trafoPtr, 0);

    scene.getMesh(*it).addToCommandListBoundingBox(commandList);
  }

  for (auto it = currNode.childIndices.begin(); it != currNode.childIndices.end(); it++)
  {
    addToCommandListImplBoundingBox(scene, *it, transformation, commandList, modelViewRootParameterIdx);
  }
}
} // namespace

namespace gims
{
const Scene::Node& Scene::getNode(ui32 nodeIdx) const
{
  return m_nodes[nodeIdx];
}

Scene::Node& Scene::getNode(ui32 nodeIdx)
{
  return m_nodes[nodeIdx];
}

const ui32 Scene::getNumberOfNodes() const
{
  return static_cast<ui32>(m_nodes.size());
}

const TriangleMeshD3D12& Scene::getMesh(ui32 meshIdx) const
{
  return m_meshes[meshIdx];
}

const Scene::Material& Scene::getMaterial(ui32 materialIdx) const
{
  return m_materials[materialIdx];
}

const AABB& Scene::getAABB() const
{
  return m_aabb;
}

void Scene::addToCommandList(const ComPtr<ID3D12GraphicsCommandList>& commandList, const f32m4 transformation,
                             ui32 modelViewRootParameterIdx, ui32 materialConstantsRootParameterIdx,
                             ui32 srvRootParameterIdx)
{
  addToCommandListImpl(*this, 0, transformation, commandList, modelViewRootParameterIdx,
                       materialConstantsRootParameterIdx, srvRootParameterIdx);
}

void Scene::addToCommandListBoundingBox(const ComPtr<ID3D12GraphicsCommandList>& commandList,
                                        const f32m4 transformation, ui32 modelViewRootParameterIdx)
{
  addToCommandListImplBoundingBox(*this, 0, transformation, commandList, modelViewRootParameterIdx);
}
} // namespace gims
