#include "Shaders/VSHeader.h"
#include "Shaders/PSHeader.h"

struct VS_PopulateDepth
{
    float4 HPosition            : SV_Position;
    float3 ViewPos              : TEXCOORD0;
};

VS_PopulateDepth OutputViewSpaceDepthVS(float3 InPosition : POSITION)
{
    VS_PopulateDepth PopulateDepth;
    PopulateDepth.HPosition = mul(float4(InPosition, 1.0f), WorldViewProj);
    PopulateDepth.ViewPos = mul(float4(InPosition, 1.0f), WorldView);
    return PopulateDepth;
}

float4 OutputViewSpaceDepthPS(VS_PopulateDepth InData) : SV_Target0
{
    return float4(InData.ViewPos, 0.0f);
}

Texture2D ViewPosMap : register(t0);
SamplerState ViewPosMapSampler : register(s0);

float4 HBAO(FullScreenQuadOutput InData) : SV_TARGET0
{
    
}