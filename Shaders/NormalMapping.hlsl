#include "Shaders/VSHeader.h"
#include "Shaders/PSHeader.h"


VS_Out_PNTUV0 GeometryVS(VS_In_PNTUV0 IN)
{
	VS_Out_PNTUV0 OUT;
	OUT.HPosition = mul(float4(IN.Position, 1.0f), WorldViewProj);
    OUT.WorldPosition = mul(float4(IN.Position, 1.0f), World).xyz;
    OUT.WorldNormal = mul(float4(IN.Normal, 0.0f), WorldIT).xyz;
    OUT.WorldTangent = mul(float4(IN.Tangent, 0.0f), World).xyz;
    OUT.WorldBinormal = cross(OUT.WorldNormal, OUT.WorldTangent);
    OUT.UV = IN.UV;
	return OUT;
}

Texture2D ColorMap : register(t0);
SamplerState ColorMapSampler : register(s0);
Texture2D NormalMap : register(t1);
SamplerState NormalMapSampler : register(s1);

float4 NormalMappingPS(VS_Out_PNTUV0 InData) : SV_Target0
{
    float3 Color = ColorMap.Sample(ColorMapSampler, InData.UV).xyz;
    float3 n = NormalMap.Sample(NormalMapSampler, InData.UV).xyz;
    n = n * 2.0f - 1.0f;
    float3x3 TBN = {
        InData.WorldTangent,
        InData.WorldBinormal,
        InData.WorldNormal
    };
    n = mul(n, TBN);

    float3 ViewDir = normalize(ViewLocation - InData.WorldPosition);
	BrdfContext Context;
    InitBrdfContext(Context, n, LightDir.xyz, ViewDir.xyz);
	float3 Result = PI * (DiffuseColor(BaseColor.xyz * Color) + SpecularGGX(Context, BaseColor.xyz, BaseColor.w, SmoothAndMetallic)) * Context.NoL * LightColor.xyz;
	return float4(saturate(pow(Result, 1.0f / 2.4f)), 1.0f);
}