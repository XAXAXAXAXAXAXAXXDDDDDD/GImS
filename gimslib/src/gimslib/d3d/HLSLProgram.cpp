#include <D3Dcompiler.h>
#include <filesystem>
#include <gimslib/d3d/HLSLProgram.hpp>
#include <gimslib/dbg/HrException.hpp>
#include <iostream>
using namespace gims;
namespace
{
ComPtr<ID3DBlob> compileShader(std::filesystem::path pathToShader, char const* const shaderMain,
                               char const* const shaderModel)
{
#ifdef _DEBUG
  const auto compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  const auto compileFlags = D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif
  ComPtr<ID3DBlob> shader;
  ComPtr<ID3DBlob> shaderErrorBlob;
  auto             compileResults = D3DCompileFromFile(pathToShader.c_str(), nullptr, nullptr, shaderMain, shaderModel,
                                                       compileFlags, 0, &shader, &shaderErrorBlob);
  if (FAILED(compileResults))
  {
    if (nullptr != shaderErrorBlob.Get())
    {
      OutputDebugStringA((char*)shaderErrorBlob->GetBufferPointer());
      throw std::exception((char*)shaderErrorBlob->GetBufferPointer());
    }
    throw HrException(compileResults);
  }
  return shader;
}
} // namespace

namespace gims
{

HLSLProgram::HLSLProgram()
{
}

HLSLProgram::HLSLProgram(std::filesystem::path pathToShader, char const* const vertexShaderMain,
                         char const* const pixelShaderMain, char const* const hullShaderMain,
                         char const* const domainShaderMain)
{
  const auto absolutePath = std::filesystem::weakly_canonical(pathToShader);

  if (!std::filesystem::exists(absolutePath))
  {
    throw std::exception((absolutePath.string() + std::string(" does not exist.")).c_str());
  }

  m_vertexShader = ::compileShader(absolutePath, vertexShaderMain, "vs_5_1");
  m_pixelShader  = ::compileShader(absolutePath, pixelShaderMain, "ps_5_1");

  if (hullShaderMain)
  {
    m_hullShader = ::compileShader(absolutePath, hullShaderMain, "hs_5_1");
  }

  if (domainShaderMain)
  {
    m_domainShader = ::compileShader(absolutePath, domainShaderMain, "ds_5_1");
  }
}

const ComPtr<ID3DBlob>& HLSLProgram::getVertexShader() const
{
  return m_vertexShader;
}
const ComPtr<ID3DBlob>& HLSLProgram::getPixelShader() const
{
  return m_pixelShader;
}

const ComPtr<ID3DBlob>& HLSLProgram::getHullShader() const
{
  return m_hullShader;
}
const ComPtr<ID3DBlob>& HLSLProgram::getDomainShader() const
{
  return m_domainShader;
}
} // namespace gims