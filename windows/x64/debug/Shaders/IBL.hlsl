#include "ShaderHeader.h"

cbuffer PreprocessData : register(b3)
{
	float4 MipLevel;
};

cbuffer ShadingData : register(b4)
{
    float4 g_viewDir;
    float4 g_albedo;
	float3 g_Smooth_Metallic_MaxMip;
};

struct FullScreenQuadVSOut
{
	float4 Position : SV_Position;
	float2 uv       : TEXCOORD0;
};

struct Geometry_VSIn
{
    float4 position : position;
    float3 normal   : normal;
};

struct Geometry_VSOut
{
    float4 HPosition   : SV_Position;
    float3 Normal      : TexCoord;
};

struct Env_VSOut
{
	float4 HPosition    : SV_Position;
	float3 ViewDir      : TEXCOORD0;
};

Texture2D EnvMap                            : register(t0);
ByteAddressBuffer IrradianceSampleData      : register(t1);
Texture2D IrradianceMap                     : register(t1);
Texture2D SpecularPreFilterMap              : register(t2);
Texture2D LUTMap                            : register(t3);

SamplerState EnvMapSampler                  : register(s0);

FullScreenQuadVSOut FullScreenQuad(float2 InPosition : position)
{
	FullScreenQuadVSOut OUT;
	OUT.Position = float4(InPosition, 0.0f, 1.0f);
	float2 Sign = float2(1.0f, -1.0f);
	OUT.uv = (InPosition + Sign) * Sign * 0.5f;
	return OUT;
}

Geometry_VSOut GeometryVS ( Geometry_VSIn IN )
{
    Geometry_VSOut OUT;
    OUT.HPosition = mul(IN.position, WorldViewProj);
    OUT.Normal = normalize(mul(IN.normal, (float3x3)WorldIT).xyz);
    return OUT;
}

Env_VSOut EnvVS(float2 InPos : POSITION)
{
	Env_VSOut OUT;
	OUT.HPosition = float4(InPos, 1.0f, 1.0f);
	// 转换到世界空间方向
	OUT.ViewDir = mul(float4(InPos, 1.0f, 1.0f), InvViewProj).xyz;
	return OUT;
}

float4 EnvMapPS(Env_VSOut InData) : SV_Target0
{
	float3 NormalView = normalize(InData.ViewDir);
	float ViewDirProjOnXY = sqrt(NormalView.x * NormalView.x + NormalView.y * NormalView.y);
	// 直角坐标系转换到球面坐标系
	float theta = acos(NormalView.z);
	float phi = atan2(NormalView.y, NormalView.x);
	float Inv2PI = 0.5f / PI;
	float InvPI = 1.0f / PI;
	return float4(pow(EnvMap.Sample(EnvMapSampler, float2((phi + PI) * Inv2PI, (theta) * InvPI), float2(0.0f, 0.0f)).xyz, 1.0f / 2.4f), 1.0f);
}

float RadicalInverse_VdC(uint bits) 
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley2d(uint i, uint N) 
{
    return float2(float(i)/float(N), RadicalInverse_VdC(i));
}

float2 ToSphereCoordinatesUV(float3 L)
{
	float ViewDirProjOnXY = sqrt(L.x * L.x + L.y * L.y);
	// 直角坐标系转换到球面坐标系
	float theta = acos(L.z);
	float phi = atan2(L.y, L.x);
	float Inv2PI = 0.5f / PI;
	float InvPI = 1.0f / PI;
    return float2((phi + PI) * Inv2PI, (theta) * InvPI);
}

uint Float3ColorToUint(float3 fColor)
{
    uint3 iColor = (uint3)(fColor * 255.0f);
    return iColor.b | (iColor.g << 8) | (iColor.r << 16) | 0xFF000000u;
}

float3 UintToFloat3Color(uint iColor)
{
    return float3((float)((iColor & 0x00FF0000u) >> 16) / 255.0f, 
        (float)((iColor & 0x0000FF00u) >> 8) / 255.0f,
        (float)(iColor & 0x000000FFu) / 255.0f);
}

/////////////////////////////////////////////////////////
// Diffuse Part

float3 ImportanceSampleIrradiance(float2 Xi, float3 N)
{
    // float Phi = 2 * PI * Xi.x;
    float CosTheta = cos(Xi.y);
    float SinTheta = sqrt(1.0f - CosTheta * CosTheta);
    float3 H;
    H.x = SinTheta * cos( Xi.x );
    H.y = SinTheta * sin( Xi.x );
    H.z = CosTheta;
    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize( cross( UpVector, N ) );
    float3 TangentY = cross( N, TangentX );
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

float4 GenIrradianceMap(FullScreenQuadVSOut In) : SV_Target0
{
    float phi = In.uv.x * 2.0f * PI;
	float theta = In.uv.y * PI;
	float3 N = normalize(float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)));
    float3 IrradianceLighting = 0.0f;

    uint SampleCount = 0;

    uint BufferDim;
    IrradianceSampleData.GetDimensions(BufferDim);
    BufferDim /= 8;

    for (uint i = 0; i < BufferDim; ++i)
    {
        uint2 Value = IrradianceSampleData.Load2(i * 8);
        float2 fValue = float2(asfloat(Value.x), asfloat(Value.y));
        float3 H = ImportanceSampleIrradiance(fValue, N);
        float2 uv = ToSphereCoordinatesUV(H);
        IrradianceLighting += EnvMap.Sample(EnvMapSampler, uv).rgb * cos(fValue.y) * sin(fValue.y);
        ++SampleCount;
    }

    float3 RetColor = PI * IrradianceLighting / SampleCount;
    return float4(RetColor, 1.0f);
}

/////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////
// Specular Part

float3 ImportanceSampleGGX(float2 Xi, float Roughness, float3 N)
{
	float a = Roughness * Roughness;
    // 将2D采样点映射到半球面上
    float Phi = 2 * PI * Xi.x;
    // CosTheta要使用NDF推导出
    float CosTheta = sqrt( (1 - Xi.y) / ( 1 + (a*a - 1) * Xi.y ) );
    float SinTheta = sqrt( 1 - CosTheta * CosTheta );
    // 得到Half向量，球面坐标系转换回笛卡尔坐标系，此时方向在切空间
    float3 H;
    H.x = SinTheta * cos( Phi );
    H.y = SinTheta * sin( Phi );
    H.z = CosTheta;
    // 计算切空间坐标系
    float3 UpVector = abs(N.z) < 0.999 ? float3(0,0,1) : float3(1,0,0);
    float3 TangentX = normalize( cross( UpVector, N ) );
    float3 TangentY = cross( N, TangentX );
    // 切空间回到世界空间
    return TangentX * H.x + TangentY * H.y + N * H.z;
}

float3 PreFilter(float Roughness, float3 N)
{
	float3 V = N;
	float3 SpecularLighting = 0.0f;
    float TotalWeight = 0.0f;
    // 采样数量
    const uint NumSamples = 1024;
    for( uint i = 0; i < NumSamples; i++ )
    {
        // 生成Hammersley点集
        float2 Xi = Hammersley2d( i, NumSamples );
        // 计算每个采样点对应的H
        float3 H = ImportanceSampleGGX( Xi, Roughness, N );
        // 相当于Reflect函数，反求L
        float3 L = 2 * dot( V, H ) * H - V;
        float NoL = saturate( dot( N, L ) );
        // Li(l) * NoL
        SpecularLighting += EnvMap.Sample(EnvMapSampler, ToSphereCoordinatesUV(L)).rgb * NoL;
        TotalWeight += NoL;
    }
    return SpecularLighting / TotalWeight;
}

float4 CopyResource(FullScreenQuadVSOut In) : SV_Target0
{
    return EnvMap.Sample(EnvMapSampler, In.uv);
}

float4 GenPreFilterEnvMap(FullScreenQuadVSOut In) : SV_Target0
{
    float phi = In.uv.x * 2.0f * PI + PI;
	float theta = In.uv.y * PI;

	float3 N = normalize(float3(cos(phi) * sin(theta), sin(phi) * sin(theta), cos(theta)));
	return float4(PreFilter(MipLevel.x, N), 1.0f);	
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

float4 GenerateLUT(FullScreenQuadVSOut In) : SV_Target0
{
    float NoV = In.uv.x == 0.0f ? 0.001f : In.uv.x;
    float Roughness = In.uv.y; 

    float3 V;
    V.x = sqrt( 1.0f - NoV * NoV ); // sin
    V.y = 0;
    V.z = NoV; // cos

    float3 N = float3(0.f, 0.f, 1.f);

    float A = 0;
    float B = 0;
    const uint NumSamples = 1024;
    for( uint i = 0; i < NumSamples; i++ )
    {
        float2 Xi = Hammersley2d( i, NumSamples );
        float3 H = ImportanceSampleGGX( Xi, Roughness, N );
        float3 L = 2 * dot( V, H ) * H - V;
        float NoL = saturate( L.z );
        float NoH = saturate( H.z );
        float VoH = saturate( dot( V, H ) );
        float G = G_Smith( Roughness, NoV, NoL );
        float G_Vis = G * VoH / (NoH * NoV);
        float Fc = pow( 1 - VoH, 5 );
        A += (1 - Fc) * G_Vis;
        B += Fc * G_Vis;
    }

    return float4(float2( A, B ) / NumSamples, 0.0f, 0.0f);
}

float DielectricSpecularToF0(float Specular)
{
	return 0.08f * Specular;
}

float3 ComputeF0(float3 Albedo, float Specular, float Metallic)
{
	return lerp(DielectricSpecularToF0(Specular).xxx, Albedo, Metallic.xxx);
}

float4 IBLShading( Geometry_VSOut IN ) : SV_Target0
{
    float3 Normal = normalize(IN.Normal);
    float4 DiffuseColor = g_albedo - g_albedo * g_Smooth_Metallic_MaxMip.y;
    float3 r = reflect(-g_viewDir.xyz, Normal);
    float2 uv = ToSphereCoordinatesUV(r);

    float3 IrradianceFloatColor = IrradianceMap.Sample(EnvMapSampler, uv).rgb;

    float Roughness = 1.0f - g_Smooth_Metallic_MaxMip.x;
    float NoV = saturate(dot(r, Normal));

    float lod = Roughness * g_Smooth_Metallic_MaxMip.z;
    float lodf = floor(lod);
    float lodc = ceil(lod);
    float3 PreFilterFloorColor = SpecularPreFilterMap.SampleLevel(EnvMapSampler, uv, lodf).rgb;
    float3 PreFilterCeilingColor = SpecularPreFilterMap.SampleLevel(EnvMapSampler, uv, lodc).rgb; 
    float3 SpecularFloatColor = lerp(PreFilterFloorColor, PreFilterCeilingColor, lod - lodf);

    float2 LUTFloatColor = LUTMap.Sample(EnvMapSampler, float2(NoV, Roughness)).rg;

    float3 FinalColor = IrradianceFloatColor * DiffuseColor.xyz + SpecularFloatColor * (ComputeF0(g_albedo.xyz, g_albedo.w, g_Smooth_Metallic_MaxMip.y) * LUTFloatColor.x + LUTFloatColor.y);
    return float4(pow(FinalColor, 1.0f / 2.2f), 1.0f);
}