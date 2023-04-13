#pragma once
#include <d3d12.h>
#include <wrl.h>
#include <filesystem>
using Microsoft::WRL::ComPtr;
namespace gims
{
class HLSLProgram
{
public:
  HLSLProgram();
  HLSLProgram(std::filesystem::path pathToShader, char const* const vertexShaderMain, char const* const pixelShaderMain);
  const ComPtr<ID3DBlob>& getVertexShader() const;
  const ComPtr<ID3DBlob>& getPixelShader() const;

private:
  ComPtr<ID3DBlob> m_vertexShader;
  ComPtr<ID3DBlob> m_pixelShader;
};
} // namespace gims