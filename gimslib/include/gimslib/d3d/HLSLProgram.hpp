#pragma once
#include <d3d12.h>
#include <filesystem>
#include <wrl.h>
using Microsoft::WRL::ComPtr;
namespace gims
{
class HLSLProgram
{
public:
  HLSLProgram();
  HLSLProgram(std::filesystem::path pathToShader, char const* const vertexShaderMain, char const* const pixelShaderMain,
              char const* const hullShaderMain = nullptr, char const* const domainShaderMain = nullptr);

  const ComPtr<ID3DBlob>& getVertexShader() const;
  const ComPtr<ID3DBlob>& getPixelShader() const;

  const ComPtr<ID3DBlob>& getHullShader() const;
  const ComPtr<ID3DBlob>& getDomainShader() const;

private:
  ComPtr<ID3DBlob> m_vertexShader;
  ComPtr<ID3DBlob> m_pixelShader;

  ComPtr<ID3DBlob> m_hullShader;
  ComPtr<ID3DBlob> m_domainShader;
};
} // namespace gims