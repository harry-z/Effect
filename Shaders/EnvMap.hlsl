#include "Shaders/ShaderHeader.h"

struct Geometry_VSOut
{
	float4 HPosition : SV_Position;
	float3 ViewDir : TEXCOORD0;
};

Geometry_VSOut GeometryVS(float2 InPos : POSITION)
{
	Geometry_VSOut OUT;
	OUT.HPosition = float4(InPos, 1.0f, 1.0f);
	// 转换到世界空间方向
	OUT.ViewDir = mul(float4(InPos, 1.0f, 1.0f), InvViewProj);
	return OUT;
}

Texture2D EnvMap : register(t0);
SamplerState EnvMapSampler : register(s0);

float4 EnvMapPS(Geometry_VSOut InData) : SV_Target0
{
	float3 NormalView = normalize(InData.ViewDir);
	// 直角坐标系转换到球面坐标系
	float theta = acos(NormalView.z);
	float phi = atan2(NormalView.y, NormalView.x);
	float Inv2PI = 0.5f / PI;
	float InvPI = 1.0f / PI;
	return float4(pow(EnvMap.Sample(EnvMapSampler, float2((phi + PI) * Inv2PI, (theta) * InvPI), float2(0.0f, 0.0f)).xyz, 1.0f / 2.4f), 1.0f);
}