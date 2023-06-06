struct VertexShaderOutput
{
    float4 clipSpacePosition : SV_POSITION;
    float3 viewSpacePosition : POSITION;
    float3 viewSpaceNormal : NORMAL;
    float2 texCoord : TEXCOOD;
};

/// <summary>
/// Constants that can change every frame.
/// </summary>
cbuffer PerFrameConstants : register(b0)
{
    float4x4 projectionMatrix;
}

/// <summary>
/// Constants that can change per Mesh/Drawcall
/// </summary>
cbuffer PerMeshConstants : register(b1)
{
    float4x4 modelViewMatrix;
}

/// <summary>
/// Consttants that are really constant for the entire scene.
/// </summary>
cbuffer Material : register(b2)
{
    float4 ambientColor;
    float4 diffuseColor;
    float4 specularColorAndExponent;
}

Texture2D<float3> g_textureAmbient : register(t0);
Texture2D<float3> g_textureDiffuse : register(t1);
Texture2D<float3> g_textureSpecular : register(t2);
Texture2D<float3> g_textureEmissive : register(t3);
Texture2D<float3> g_textureNormal : register(t4);

SamplerState g_sampler : register(s0);

VertexShaderOutput VS_main(float3 position : POSITION, float3 normal : NORMAL, float2 texCoord : TEXCOORD)
{
    VertexShaderOutput output;

    //float4x4 modelViewMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };

    float4 p4 = mul(modelViewMatrix, float4(position, 1.0f));
    output.viewSpacePosition = p4.xyz;
    output.viewSpaceNormal = mul(modelViewMatrix, float4(normal, 0.0f)).xyz;
    output.clipSpacePosition = mul(projectionMatrix, p4);
    output.texCoord = texCoord;

    return output;
}

float4 PS_main(VertexShaderOutput input)
    : SV_TARGET
{
    float3 l = normalize(float3(-1.0f, 0.0f, 0.0f));
    float3 n = normalize(input.viewSpaceNormal);

    float3 v = normalize(-input.viewSpacePosition);
    float3 h = normalize(l + v);

    float f_diffuse = max(0.0f, dot(n, l));
    float f_specular = pow(max(0.0f, dot(n, h)), specularColorAndExponent.w);
    
    float3 textureColorAmbient = g_textureAmbient.Sample(g_sampler, input.texCoord, 0);
    float3 textureColorDiffuse = g_textureDiffuse.Sample(g_sampler, input.texCoord, 0);
    float3 textureColorSpecular = g_textureSpecular.Sample(g_sampler, input.texCoord, 0);
    float3 textureColorEmissive = g_textureEmissive.Sample(g_sampler, input.texCoord, 0);
    float3 textureColorHeight = g_textureNormal.Sample(g_sampler, input.texCoord, 0);
    
    // float3 textureColor = float3(1.0f, 1.0f, 1.0f);

    return float4(ambientColor.xyz * textureColorAmbient.xyz + textureColorEmissive.xyz + /*textureColorHeight +*//*f_diffuse **/textureColorDiffuse.xyz * diffuseColor.xyz +
                      f_specular * textureColorSpecular * specularColorAndExponent.xyz, 1);
    
    //return float4(textureColorHeight, 1);
    
 /*   return float4(textureColorSpecular, 1.0f);*/ /*diffuseColor;*/ /*float4(input.texCoord.x, input.texCoord.y, 0.0f, 1.0f);*/

}

struct VertexShaderOutput_BoundingBox
{
    float4 position : SV_POSITION;
};

VertexShaderOutput_BoundingBox VS_BoundingBox_main(float3 position
                                               : POSITION)
{
    VertexShaderOutput_BoundingBox output;

    float4 p4 = mul(projectionMatrix, mul(modelViewMatrix, float4(position, 1.0f)));
    
    output.position = p4;
    return output;
}

float4 PS_BoundingBox_main(VertexShaderOutput_BoundingBox input)
    : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}