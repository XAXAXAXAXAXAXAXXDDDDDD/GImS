static const float3 vertices[] = { { -0.25f, -0.25f, 0.0f }, { -0.25f, 0.25f, 0.0f }, { 0.25f, 0.25f, 0.0f }, { 0.25f, -0.25f, 0.0f } };

cbuffer PerFrameConstants : register(b0)
{
    float outerTessFactor;
    float innerTessFactor;
}

struct VS_OUTPUT_HS_INPUT
{
    float4 vPosWS : SV_POSITION;
};

// VERTEX SHADER
VS_OUTPUT_HS_INPUT VS_main(uint i : SV_VertexID)
{
    VS_OUTPUT_HS_INPUT output;
    output.vPosWS = float4(vertices[i], 1.0f);
    return output;
}

static const float g_TessellationFactor = 5.0f;
struct HS_CONTROL_POINT_OUTPUT
{
    float3 vWorldPos : QUADPOS;
};

struct HS_CONSTANT_DATA_OUTPUT
{
    float Edges[4] : SV_TessFactor;
    float Inside[2] : SV_InsideTessFactor;
};

// HULL SHADER
// Called once per control point
[domain("quad")] // indicates a quad patch (4 verts)
[partitioning("fractional_odd")] // fractional avoids popping
// vertex ordering for the output triangles
[outputtopology("triangle_ccw")]
[outputcontrolpoints(4)]
// name of the patch constant hull shader
[patchconstantfunc("ConstantsHS")]
[maxtessfactor(32.0)] //hint to the driver – the lower the better
// Pass in the input patch and an index for the control point
HS_CONTROL_POINT_OUTPUT HS_main(
        InputPatch<VS_OUTPUT_HS_INPUT, 4> inputPatch,
        uint uCPID : SV_OutputControlPointID)
{
    HS_CONTROL_POINT_OUTPUT Out;
    // Copy inputs to outputs – “pass through” shaders are optimal
    Out.vWorldPos = inputPatch[uCPID].vPosWS.xyz;
    
    return Out;
}

//Called once per patch. The patch and an index to the patch (patch
// ID) are passed in
HS_CONSTANT_DATA_OUTPUT ConstantsHS(InputPatch<VS_OUTPUT_HS_INPUT, 4>
p, uint PatchID : SV_PrimitiveID)
{
    HS_CONSTANT_DATA_OUTPUT Out;
    // Assign tessellation factors – in this case use a global
    // tessellation factor for all edges and the inside. These are
    // constant for the whole mesh.
    Out.Edges[0] = outerTessFactor;
    Out.Edges[1] = outerTessFactor;
    Out.Edges[2] = outerTessFactor;
    Out.Edges[3] = outerTessFactor;
    
    Out.Inside[0] = innerTessFactor;
    Out.Inside[1] = innerTessFactor;
    
    return Out;
}

struct DS_VS_OUTPUT_PS_INPUT
{
    float4 vPosCS : SV_POSITION;
};

// DOMAIN SHADER 
// Called once per tessellated vertex
[domain("quad")] // indicates that quad patches were used
// The original patch is passed in, along with the vertex position in barycentric coordinates, and the patch constant phase hull shader output(tessellation factors)
DS_VS_OUTPUT_PS_INPUT DS_main(
        HS_CONSTANT_DATA_OUTPUT input,
        float2 UV : SV_DomainLocation,
        const OutputPatch<HS_CONTROL_POINT_OUTPUT, 4> QuadPatch)
{
    DS_VS_OUTPUT_PS_INPUT Out;
    // lerp world space position with uv-coordinates
    float3 vWorldPos = float3(
        0.5f * UV.x + QuadPatch[0].vWorldPos.x,
        0.5f * UV.y + QuadPatch[0].vWorldPos.y,
        0.0f);
    
    // transform to clip space
    Out.vPosCS = float4(vWorldPos, 1.0f);
    return Out;
}

// PIXEL SHADER
float4 PS_main(DS_VS_OUTPUT_PS_INPUT input) : SV_TARGET
{
    return float4(1, 1, 0, 1);
}
