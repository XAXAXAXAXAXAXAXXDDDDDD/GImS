#include "SceneFactory.hpp"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <gimslib/contrib/d3dx12/d3dx12.h>
#include <gimslib/d3d/UploadHelper.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <numeric>

using namespace gims;

namespace
{
/// <summary>
/// Converts the index buffer required for D3D12 renndering from an aiMesh.
/// </summary>
/// <param name="mesh">The ai mesh containing an index buffer.</param>
/// <returns></returns>
std::vector<ui32v3> getTriangleIndicesFromAiMesh(aiMesh const* const mesh)
{
  std::vector<ui32v3> result;
  result.reserve(mesh->mNumFaces);
  for (ui32 fIdx = 0; fIdx < mesh->mNumFaces; fIdx++)
  {
    result.emplace_back(mesh->mFaces[fIdx].mIndices[0], mesh->mFaces[fIdx].mIndices[1], mesh->mFaces[fIdx].mIndices[2]);
  }
  return result;
}

void addTextureToDescriptorHeap(const ComPtr<ID3D12Device>& device, aiTextureType aiTextureTypeValue,
                                i32 offsetInDescriptors, aiMaterial const* const inputMaterial,
                                const std::vector<Texture2DD3D12>& m_textures, Scene::Material& material,
                                std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex,
                                ui32                                            defaultTextureIndex)
{
  if (inputMaterial->GetTextureCount(aiTextureTypeValue) == 0)
  {
    m_textures[defaultTextureIndex].addToDescriptorHeap(device, material.srvDescriptorHeap, offsetInDescriptors);
  }
  else
  {
    aiString path;
    inputMaterial->GetTexture((aiTextureType)aiTextureTypeValue, 0, &path);
    m_textures[textureFileNameToTextureIndex[path.C_Str()]].addToDescriptorHeap(device, material.srvDescriptorHeap,
                                                                                offsetInDescriptors);
  }
}

std::unordered_map<std::filesystem::path, ui32> textureFilenameToIndex(aiScene const* const inputScene)
{
  std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex;

  ui32 textureIdx = 3;
  for (ui32 mIdx = 0; mIdx < inputScene->mNumMaterials; mIdx++)
  {
    for (ui32 textureType = aiTextureType_NONE; textureType < aiTextureType_UNKNOWN; textureType++)
    {
      for (ui32 i = 0; i < inputScene->mMaterials[mIdx]->GetTextureCount((aiTextureType)textureType); i++)
      {
        aiString path;
        inputScene->mMaterials[mIdx]->GetTexture((aiTextureType)textureType, i, &path);

        const auto texturePathCstr = path.C_Str();
        const auto textureIter     = textureFileNameToTextureIndex.find(texturePathCstr);
        if (textureIter == textureFileNameToTextureIndex.end())
        {
          textureFileNameToTextureIndex.emplace(texturePathCstr, static_cast<ui32>(textureIdx));
          textureIdx++;
        }
      }
    }
  }
  return textureFileNameToTextureIndex;
}
/// <summary>
/// Reads the color from the Asset Importer specific (pKey, type, idx) triple.
/// Use the Asset Importer Macros AI_MATKEY_COLOR_AMBIENT, AI_MATKEY_COLOR_DIFFUSE, etc. which map to these arguments
/// correctly.
///
/// If that key does not exist a null vector is returned.
/// </summary>
/// <param name="pKey">Asset importer specific parameter</param>
/// <param name="type"></param>
/// <param name="idx"></param>
/// <param name="material">The material from which we wish to extract the color.</param>
/// <returns>Color or 0 vector if no color exists.</returns>
f32v4 getColor(char const* const pKey, unsigned int type, unsigned int idx, aiMaterial const* const material)
{
  aiColor3D color;
  if (material->Get(pKey, type, idx, color) == aiReturn_SUCCESS)
  {
    return f32v4(color.r, color.g, color.b, 0.0f);
  }
  else
  {
    return f32v4(0.0f);
  }
}

} // namespace

namespace gims
{
Scene SceneGraphFactory::createFromAssImpScene(const std::filesystem::path       pathToScene,
                                               const ComPtr<ID3D12Device>&       device,
                                               const ComPtr<ID3D12CommandQueue>& commandQueue)
{
  Scene outputScene;
  f32v3 vec[]        = {f32v3(0.0f, 0.0f, 0.0f)};
  outputScene.m_aabb = AABB(vec, 1);

  const auto absolutePath = std::filesystem::weakly_canonical(pathToScene);
  if (!std::filesystem::exists(absolutePath))
  {
    throw std::exception((absolutePath.string() + std::string(" does not exist.")).c_str());
  }

  const auto arguments = aiPostProcessSteps::aiProcess_Triangulate | aiProcess_GenSmoothNormals |
                         aiProcess_GenUVCoords | aiProcess_ConvertToLeftHanded | aiProcess_OptimizeMeshes |
                         aiProcess_RemoveRedundantMaterials | aiProcess_ImproveCacheLocality |
                         aiProcess_FindInvalidData | aiProcess_FindDegenerates;

  Assimp::Importer imp;
  auto             inputScene = imp.ReadFile(absolutePath.string(), arguments);
  if (!inputScene)
  {
    throw std::exception((absolutePath.string() + std::string(" can't be loaded. with Assimp.")).c_str());
  }
  const auto textureFileNameToTextureIndex = textureFilenameToIndex(inputScene);

  createMeshes(inputScene, device, commandQueue, outputScene);

  createNodes(inputScene, outputScene, inputScene->mRootNode);

  computeSceneAABB(outputScene, outputScene.m_aabb, 0, glm::identity<f32m4>());
  createTextures(textureFileNameToTextureIndex, absolutePath.parent_path(), device, commandQueue, outputScene);
  createMaterials(inputScene, textureFileNameToTextureIndex, device, outputScene);

  return outputScene;
}

void SceneGraphFactory::createMeshes(aiScene const* const inputScene, const ComPtr<ID3D12Device>& device,
                                     const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{

  for (ui32 mIdx = 0; mIdx < inputScene->mNumMeshes; mIdx++)
  {
    const auto mesh        = inputScene->mMeshes[mIdx];
    const auto indexBuffer = getTriangleIndicesFromAiMesh(mesh);
    outputScene.m_meshes.emplace_back(
        reinterpret_cast<f32v3*>(mesh->mVertices), reinterpret_cast<f32v3*>(mesh->mNormals),
        reinterpret_cast<f32v3*>(mesh->mTextureCoords[0]), mesh->mNumVertices, indexBuffer.data(), mesh->mNumFaces * 3,
        mesh->mMaterialIndex, device, commandQueue);
  }
}

ui32 SceneGraphFactory::createNodes(aiScene const* const inputScene, Scene& outputScene, aiNode const* const inputNode)
{
  ui32 currNodeIdx = outputScene.getNumberOfNodes();

  const auto  trafo = inputNode->mTransformation;
  Scene::Node n     = Scene::Node {
      glm::transpose(f32m4(trafo.a1, trafo.a2, trafo.a3, trafo.a4, trafo.b1, trafo.b2, trafo.b3, trafo.b4, trafo.c1,
                               trafo.c2, trafo.c3, trafo.c4, trafo.d1, trafo.d2, trafo.d3, trafo.d4)),
      std::vector<ui32>(inputNode->mMeshes, inputNode->mMeshes + inputNode->mNumMeshes), std::vector<ui32>()};
  outputScene.m_nodes.push_back(n);

  for (ui32 i = 0; i < inputNode->mNumChildren; i++)
  {
    n.childIndices.push_back(createNodes(inputScene, outputScene, inputNode->mChildren[i]));
  }
  outputScene.m_nodes[currNodeIdx] = n;

  // Assignment 4
  return currNodeIdx;
}

void SceneGraphFactory::computeSceneAABB(Scene& scene, AABB& aabb, ui32 nodeIdx, f32m4 transformation)
{
  if (nodeIdx >= scene.getNumberOfNodes())
  {
    return;
  }

  Scene::Node currNode = scene.getNode(nodeIdx);
  transformation       = transformation * currNode.transformation;

  for (auto it = currNode.meshIndices.begin(); it != currNode.meshIndices.end(); it++)
  {
    const auto aabbMesh            = scene.getMesh(*it).getAABB();
    AABB       transformedMeshAABB = aabbMesh.getTransformed(transformation);

    aabb = aabb.getUnion(transformedMeshAABB);
  }

  for (auto it = currNode.childIndices.begin(); it != currNode.childIndices.end(); it++)
  {
    computeSceneAABB(scene, aabb, *it, transformation);
  }
  // Assignment 5
}

enum defaultTextures
{
  defaultTextureWhite,
  defaultTextureBlack,
  defaultTextureBlue
};

void SceneGraphFactory::createTextures(
    const std::unordered_map<std::filesystem::path, ui32>& textureFileNameToTextureIndex,
    std::filesystem::path parentPath, const ComPtr<ID3D12Device>& device,
    const ComPtr<ID3D12CommandQueue>& commandQueue, Scene& outputScene)
{
  ui8v4 white(255, 255, 255, 1);
  ui8v4 black(0, 0, 0, 1);
  ui8v4 blue(0, 0, 255, 1);

  auto              value_selector = [](auto pair) { return pair.second; };
  std::vector<ui32> values(textureFileNameToTextureIndex.size());
  transform(textureFileNameToTextureIndex.begin(), textureFileNameToTextureIndex.end(), values.begin(), value_selector);
  auto it = max_element(std::begin(values), std::end(values));

  outputScene.m_textures.resize((*it) + 1);
  Texture2DD3D12 defaultTextureWhite(&white, 1, 1, device, commandQueue);
  outputScene.m_textures[defaultTextures::defaultTextureWhite] = defaultTextureWhite;

  Texture2DD3D12 defaultTextureBlack(&black, 1, 1, device, commandQueue);
  outputScene.m_textures[defaultTextures::defaultTextureBlack] = defaultTextureBlack;

  Texture2DD3D12 defaultTextureBlue(&blue, 1, 1, device, commandQueue);
  outputScene.m_textures[defaultTextures::defaultTextureBlue] = defaultTextureBlue;

  for (auto& keyValuePair : textureFileNameToTextureIndex)
  {
    Texture2DD3D12 currTexture(parentPath / keyValuePair.first, device, commandQueue);
    outputScene.m_textures[keyValuePair.second] = currTexture;
  }
}

void SceneGraphFactory::createMaterials(aiScene const* const                            inputScene,
                                        std::unordered_map<std::filesystem::path, ui32> textureFileNameToTextureIndex,
                                        const ComPtr<ID3D12Device>& device, Scene& outputScene)
{
  outputScene.m_materials.resize(inputScene->mNumMaterials);
  for (ui32 i = 0; i < inputScene->mNumMaterials; i++)
  {
    ComPtr<ID3D12DescriptorHeap> srv;

    D3D12_DESCRIPTOR_HEAP_DESC desc = {};
    desc.Type                       = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    desc.NumDescriptors             = 5;
    desc.NodeMask                   = 0;
    desc.Flags                      = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    throwIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&srv)));

    const auto material = inputScene->mMaterials[i];

    aiColor3D color_diffuse(0.f, 0.f, 0.f);
    material->Get(AI_MATKEY_COLOR_DIFFUSE, color_diffuse);

    aiColor4D color_specular(0.f, 0.f, 0.f, 0.f);
    material->Get(AI_MATKEY_COLOR_SPECULAR, color_specular);

    aiColor3D color_ambient(0.f, 0.f, 0.f);
    material->Get(AI_MATKEY_COLOR_AMBIENT, color_ambient);

    Scene::Material m;
    // set descriptor heap for textures
    m.srvDescriptorHeap = srv;

    // create material constant buffer
    Scene::MaterialConstantBuffer mCB = {};
    mCB.ambientColor                  = f32v4(color_ambient.r, color_ambient.g, color_ambient.b, 1.0f);
    mCB.diffuseColor                  = f32v4(color_diffuse.r, color_diffuse.g, color_diffuse.b, 1.0f);
    mCB.specularColorAndExponent      = f32v4(color_specular.r, color_specular.g, color_specular.b, color_specular.a);

    // set constant buffer
    m.materialConstantBuffer = ConstantBufferD3D12(mCB, device);

    addTextureToDescriptorHeap(device, aiTextureType_AMBIENT, 0, material, outputScene.m_textures, m,
                               textureFileNameToTextureIndex, defaultTextures::defaultTextureBlack);
    addTextureToDescriptorHeap(device, aiTextureType_DIFFUSE, 1, material, outputScene.m_textures, m,
                               textureFileNameToTextureIndex, defaultTextures::defaultTextureBlack);
    addTextureToDescriptorHeap(device, aiTextureType_SPECULAR, 2, material, outputScene.m_textures, m,
                               textureFileNameToTextureIndex, defaultTextures::defaultTextureWhite);
    addTextureToDescriptorHeap(device, aiTextureType_EMISSIVE, 3, material, outputScene.m_textures, m,
                               textureFileNameToTextureIndex, defaultTextures::defaultTextureBlack);
    addTextureToDescriptorHeap(device, aiTextureType_HEIGHT, 4, material, outputScene.m_textures, m,
                               textureFileNameToTextureIndex, defaultTextures::defaultTextureBlue);

    outputScene.m_materials[i] = m;
  }
  // Assignment 9
}
} // namespace gims