#include "Shaders/CommonHeader.h"

cbuffer PBRParams : register(b0)
{
	float4 ViewLocation;
	float4 LightDir;
	float4 BaseColor; // w for specular
	float4 LightColor;
	float2 SmoothAndMetallic;
};

// UE4中对Metallic的定义
// 金属度越高，Diffuse越接近于黑色
float3 GetAlbedoColor(float3 Albedo, float Metallic)
{
	return Albedo - Albedo * Metallic;
}

// 最简单的Lambert的Diffuse和Burley的模型几乎无差别，故使用Lambert模型
// 注释掉的是Burley模型
// Disney Model
// [Burley 2012, "Physically-Based Shading at Disney"]
float3 DiffuseColor(float3 Albedo)
{
    // Lambert diffuse
	return GetAlbedoColor(Albedo, SmoothAndMetallic.y) / PI;
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

float3 ComputeF0(float3 BaseColor, float Specular, float Metallic)
{
	return lerp(DielectricSpecularToF0(Specular).xxx, BaseColor, Metallic.xxx);
}

float3 SchlickFresnel(float3 F0, float VoH)
{
    return F0 + (float3(1.0f, 1.0f, 1.0f) - F0) * pow(saturate(1.0f - VoH), 5.0f);
}

float NDF(float Roughness, float NoH)
{
	float a = Roughness * Roughness;
	float a2 = a * a;
	float denom = (a2 - 1.0f) * NoH * NoH + 1.0f;
	return a2 / (PI * denom * denom);
}

float GeometrySchlickGGX(float Dot, float Roughness)
{
    float a = Roughness + 1.0f;
    float k = (a * a) / 8.0f;

    float nom   = Dot;
    float denom = Dot * (1.0f - k) + k;

    return nom / denom;
}

float GSmith(float Roughness, float NoV, float NoL)
{
    return GeometrySchlickGGX(NoV, Roughness) * GeometrySchlickGGX(NoL, Roughness);
}

struct BrdfContext
{
    float NoV;
    float NoL;
    float VoL;
    float NoH;
    float VoH;
};

void InitBrdfContext(inout BrdfContext Context, float3 N, float3 L, float3 V)
{
    Context.NoL = saturate( dot(N, L) );
	Context.NoV = saturate( dot(N, V) );
	Context.VoL = saturate( dot(V, L) );
    float3 H = normalize(L + V);
	// float InvLenH = rsqrt( 2 + 2 * Context.VoL );
	Context.NoH = saturate( dot(N, H) );
	Context.VoH = saturate( dot(V, H) );
}

float3 SpecularGGX(BrdfContext Context, float3 BaseColor, float Specular, float2 SmoothMetallic)
{
    float3 Fresnel = SchlickFresnel(ComputeF0(BaseColor, Specular, SmoothMetallic.y), Context.VoH);
    float Roughness = saturate(1.0f - SmoothMetallic.x);
    float Vis = GSmith(Roughness, Context.NoV, Context.NoL);
    float G = NDF(Roughness, Context.NoH);
    return Fresnel * Vis * G / (4.0f * Context.NoL * Context.NoV + 1e-5);
}