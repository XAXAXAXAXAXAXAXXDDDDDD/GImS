struct VertexShaderOutput
{
  float4 position : SV_POSITION;
  float3 viewSpacePosition : POSITION;
  float3 viewSpaceNormal : NORMAL;
  float2 texCoord : TEXCOOD;
};

struct VertexShaderOutput_Wireframe
{
  float4 position : SV_POSITION;
};

cbuffer PerFrameConstants : register(b0)
{
  float4x4 mvp;
  float4x4 mv;
  float4   specularColor_and_Exponent;
  float4   ambientColor;
  float4   diffuseColor;
  float4   wireFrameColor;
  uint1    flags;
}

Texture2D<float3> g_texture : register(t0);
SamplerState      g_sampler : register(s0);

VertexShaderOutput VS_main(float3 position : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD)
{
  VertexShaderOutput output;
  output.position          = mul(mvp, float4(position, 1.0f));
  output.viewSpacePosition = mul(mv, float4(position, 1.0f)).xyz;
  output.viewSpaceNormal   = mul(mv, float4(normal, 0.0f)).xyz;
  output.texCoord          = texCoord;
  return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
  if (false)
  {
    return float4(input.viewSpaceNormal, 1.0f);
  }
  else
  {
    bool twoSidedLighting = flags & 0x1;
    bool useTexture       = flags & 0x2;

    float3 lightDirection = float3(0.0f, 0.0f, -1.0f);

    float3 l = normalize(lightDirection);
    float3 n = normalize(input.viewSpaceNormal);
    if (twoSidedLighting)
    {
      n = n.z < 0.0 ? n : -n;
    }
    float3 v = normalize(-input.viewSpacePosition);
    float3 h = normalize(l + v);

    float f_diffuse  = max(0.0f, dot(n, l));
    float f_specular = pow(max(0.0f, dot(n, h)), specularColor_and_Exponent.w);

    float3 textureColor = useTexture ? g_texture.Sample(g_sampler, input.texCoord, 0) : float3(1.0f, 1.0f, 1.0f);
    // float3 textureColor = float3(1.0f, 1.0f, 1.0f);

    return float4(ambientColor.xyz + f_diffuse * diffuseColor.xyz * textureColor.xyz +
                      f_specular * specularColor_and_Exponent.xyz
                  /*textureColor.xyz*/, 1);
  }
}

VertexShaderOutput_Wireframe VS_WireFrame_main(float3 position
                                               : POSITION, float3 normal
                                               : NORMAL, float2   texCoord
                                               : TEXCOORD)
{
  VertexShaderOutput_Wireframe output;

  output.position = mul(mvp, float4(position, 1.0f));
  return output;
}

float4 PS_WireFrame_main(VertexShaderOutput_Wireframe input)
    : SV_TARGET
{
  return wireFrameColor;
}