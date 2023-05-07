#include "Shaders/VSHeader.h"

struct VS_PopulateDepth
{
    float4 HPosition            : SV_Position;
    float3 ViewPos              : TEXCOORD0;
    float3 ViewNormal           : TEXCOORD1;
    float3 ViewTangent          : TEXCOORD2;
};

VS_PopulateDepth OutputViewSpaceDepthVS(VS_In_PNTUV0 IN)
{
    VS_PopulateDepth PopulateDepth;
    PopulateDepth.HPosition = mul(float4(IN.Position, 1.0f), WorldViewProj);
    PopulateDepth.ViewPos = mul(float4(IN.Position, 1.0f), WorldView);
    PopulateDepth.ViewNormal = mul(float4(IN.Normal, 0.0f), WorldViewIT);
    PopulateDepth.ViewTangent = mul(float4(IN.Tangent, 0.0f), WorldViewIT);
    return PopulateDepth;
}

struct PS_PopulateDepth
{
    float4 ViewPos              : SV_Target0;
    float4 ViewNormal           : SV_Target1;
    float4 ViewTangent          : SV_Target2;
};

PS_PopulateDepth OutputViewSpaceDepthPS(VS_PopulateDepth InData)
{
    PS_PopulateDepth PopulateDepthBuffer;
    PopulateDepthBuffer.ViewPos = float4(InData.ViewPos, 0.0f);
    PopulateDepthBuffer.ViewNormal = float4(InData.ViewNormal, 0.0f);
    PopulateDepthBuffer.ViewTangent = float4(InData.ViewTangent, 0.0f);
    return PopulateDepthBuffer;
}

cbuffer AOParams : register(b0)
{
    float ViewSpaceSampleRadius;
    float2 FocalLen;
    float2 WindowParams;
};

Texture2D <float4> ViewPosMap                : register(t0);
Texture2D <float4> ViewNormalMap             : register(t1);
Texture2D <float4> ViewTangentMap            : register(t2);
SamplerState TextureSampler                  : register(s0);

float4 HBAO(float4 HPosition : SV_Position) : SV_Target0;
{
    float2 SampleRadius = 0.5f * ViewSpaceSampleRadius * FocalLen / HPosition.w;
    float PixelRadius = SampleRadius.x * WindowParams.x;

    float AOResult = 1.0f;
    [branch] if (PixelRadius > 1.0f)
    {

    }
}