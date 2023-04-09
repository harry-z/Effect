#include "Shaders/VSHeader.h"
#include "Shaders/PSHeader.h"

VS_Out_PN GeometryVS ( VS_In_PN IN )
{
    VS_Out_PN OUT;
    OUT.HPosition = mul(float4(IN.Position, 1.0f), WorldViewProj);
	OUT.WorldPosition = mul(float4(IN.Position, 1.0f), World).xyz;
	OUT.WorldNormal = mul(float4(IN.Normal, 0.0f), WorldIT).xyz;
    return OUT;
}

float4 PBRShading( VS_Out_PN IN ) : SV_Target
{
	float3 ViewDir = normalize(ViewLocation - IN.WorldPosition);
	BrdfContext Context;
    InitBrdfContext(Context, normalize(IN.WorldNormal), LightDir.xyz, ViewDir.xyz);
	float3 Result = PI * (DiffuseColor(BaseColor.xyz) + SpecularGGX(Context, BaseColor.xyz, BaseColor.w, SmoothAndMetallic)) * Context.NoL * LightColor.xyz;
	return float4(saturate(pow(Result, 1.0f / 2.4f)), 1.0f);
}