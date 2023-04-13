#pragma once
#include <gimslib/d3d/DX12App.hpp>
#include <gimslib/d3d/HLSLProgram.hpp>
#include <gimslib/io/CograBinaryMeshFile.hpp>
#include <gimslib/types.hpp>
#include <gimslib/ui/ExaminerController.hpp>

using namespace gims;
class MeshViewer : public gims::DX12App
{
public:
  MeshViewer(const DX12AppConfig config);

  ~MeshViewer();

  virtual void onDraw();
  virtual void onDrawUI();

private:
  struct UiData
  {
    f32v3 m_backgroundColor     = f32v3(0.25f, 0.25f, 0.25f);
    f32v3 m_wireFrameColor      = f32v3(0.25f, 0.25f, 0.25f);
    bool  m_useBackFaceCulling  = false;
    bool  m_overLayWireFrame    = false;
    bool  m_useTwoSidedLighting = false;
    bool  m_useTexture          = false;
    f32v3 m_ambient             = f32v3(0.25f, 0.25f, 0.25f);
    f32v3 m_diffuse             = f32v3(0.25f, 0.25f, 0.25f);
    f32v3 m_specular            = f32v3(0.25f, 0.25f, 0.25f);
    f32   m_specularExponent    = 128.0f;
  };

  UiData m_uiData;

  gims::ExaminerController m_examinerController;
  f32m4                    m_normalizationTransformation;

  CograBinaryMeshFile m_cbm = CograBinaryMeshFile("../../../data/bunny.cbm");

  ComPtr<ID3D12PipelineState> m_pipelineState;
  ComPtr<ID3D12PipelineState> m_pipelineStateWireFrame;

  gims::HLSLProgram           m_triangleMeshProgram;
  gims::HLSLProgram           m_triangleMeshProgramWireFrame;
  ComPtr<ID3D12RootSignature> m_rootSignature;

  ComPtr<ID3D12Resource>   m_vertexBuffer;
  D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

  ComPtr<ID3D12Resource>  m_indexBuffer;
  D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

  ComPtr<ID3D12Resource>       m_texture;
  ComPtr<ID3D12DescriptorHeap> m_srv;

  f32v3* m_vertices;
  f32v3* m_normals;
  f32v2* m_textCoords;

  ui32 m_numVertices;
  ui32 m_numTriangles;

  struct Vertex
  {
    f32v3 position;
    f32v3 normal;
    f32v2 texCoord;
  };

  std::vector<Vertex> m_vertexBufferCPU;
  std::vector<ui32>   m_indexBufferCPU;

  struct ConstantBuffer
  {
    f32m4 mvp;
    f32m4 mv;
    f32v4 specularColor_and_Exponent;
    f32v4 ambientColor;
    f32v4 diffuseColor;
    f32v4 wireFrameColor;
    ui32  flags;
  };
  std::vector<ComPtr<ID3D12Resource>> m_constantBuffers;
  void                                createConstantBuffer();
  void                                updateConstantBuffer(/*f32m4 mvFromMouseInput*/);

  void createRootSignature();
  void createPipeline();
  void createPipelineWireFrame();
  void createTriangleMesh();
  void createTexture();

  void configurePipeline();
};
