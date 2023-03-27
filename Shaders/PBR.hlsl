#include "Shaders/ShaderHeader.h"

cbuffer ShadingConstants : register(b3)
{
	float4 g_viewDir;
	float4 g_lightDir;
	float4 g_albedo;
	float4 g_lightColor;
	float2 g_SmoothAndMetallic;
};

struct Geometry_VSIn
{
    float4 position : position;
    float3 normal   : normal;
};

struct Geometry_VSOut
{
    float4 HPosition            : SV_Position;
    float3 Normal      : TexCoord;
};

Geometry_VSOut GeometryVS ( Geometry_VSIn IN )
{
    Geometry_VSOut OUT;
    OUT.HPosition = mul(IN.position, WorldViewProj);
    // OUT.Normal = normalize(mul(IN.normal, (float3x3)WorldIT).xyz);
	OUT.Normal = normalize(mul(float4(IN.normal, 0.0f), World).xyz);
    return OUT;
}

// UE4中对Metallic的定义
// 金属度越高，Diffuse越接近于黑色
float3 GetAlbedoColor(float3 Albedo, float Metallic)
{
	return Albedo - Albedo * Metallic;
}

// ShadingParams float4: NDotL NDotV NDotH HDotL

// 最简单的Lambert的Diffuse和Burley的模型几乎无差别，故使用Lambert模型
// 注释掉的是Burley模型
// Disney Model
// [Burley 2012, "Physically-Based Shading at Disney"]
float3 DiffuseColor( float4 ShadingParams )
{
	return GetAlbedoColor(g_albedo.xyz, g_SmoothAndMetallic.y);
	// 1 - smoothness = roughness
	// 0.5 + 2 * Roughness * HoL * HoL
	// float FD90 = 0.5f + 2 * (1.0f - g_SmoothAndMetallic.x) * pow(ShadingParams.w, 2.0f);
	// FD90 -= 1.0f;
	// return g_albedo.xyz * (1.0f + FD90 * pow(1.0f - ShadingParams.x, 5.0f)) * (1.0f + FD90 * pow(1.0f - ShadingParams.y, 5.0f));
}

// UE4中计算F0的方式
// 半导体的F0通常为0.04，所以UE4Specular的默认值为0.5
float DielectricSpecularToF0(float Specular)
{
	return 0.08f * Specular;
}

float3 ComputeF0(float3 Albedo, float Specular, float Metallic)
{
	return lerp(DielectricSpecularToF0(Specular).xxx, Albedo, Metallic.xxx);
}

float NDF(float Roughness, float NoH)
{
	float a = Roughness * Roughness;
	float a2 = a * a;
	float denom = (a2 - 1.0f) * NoH * NoH + 1.0f;
	return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float NoV, float Roughness)
{
    float a = Roughness + 1.0f;
    float k = (a * a) / 8.0f;

    float nom   = NoV;
    float denom = NoV * (1.0f - k) + k;

    return nom / denom;
}

float G_Smith(float Roughness, float NoV, float NoL)
{
    return GeometrySchlickGGX(NoV, Roughness) * GeometrySchlickGGX(NoL, Roughness);
}

float3 SpecularColor( float4 ShadingParams )
{
	float Roughness = 1.0f - g_SmoothAndMetallic.x;
	float3 F0 = ComputeF0(g_albedo.xyz, g_albedo.w, g_SmoothAndMetallic.y);
	float3 Fresnel = F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * pow(1.0f - ShadingParams.z, 5.0f);

	float G_GGX = G_Smith(Roughness, ShadingParams.y, ShadingParams.x);
	float D_GGX = NDF(Roughness, ShadingParams.z);
	return Fresnel * G_GGX * D_GGX / (4.0f * ShadingParams.x * ShadingParams.y + 1e-5);
}

float4 PBRShading( Geometry_VSOut IN ) : SV_Target
{
	float3 Half = normalize(g_viewDir.xyz + g_lightDir.xyz);
	float3 Normal = normalize(IN.Normal);
	float4 ShadingParams = float4(
		saturate(dot(Normal, g_lightDir.xyz)), // NoL
		saturate(dot(Normal, g_viewDir.xyz)), // NoV
		saturate(dot(Normal, Half)), // NoH
		saturate(dot(Half, g_lightDir.xyz))) // HoL
		;
	float3 Result = (DiffuseColor(ShadingParams) + PI * SpecularColor(ShadingParams)) * ShadingParams.x * g_lightColor.xyz;
	return float4(saturate(pow(Result, 1.0f / 2.4f)), 1.0f);
}