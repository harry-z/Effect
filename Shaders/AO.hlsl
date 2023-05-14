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
    float4 WindowParams;
};

Texture2D <float4> ViewPosMap                : register(t0);
Texture2D <float4> ViewNormalMap             : register(t1);
Texture2D <float4> ViewTangentMap            : register(t2);
SamplerState TextureSampler                  : register(s0);

const int NumSamples = 3;
const int NumDirections = 6;
const float PixelMaxRadius = 50.0f;

void ComputeStepSize(out float2 StepSizeUV, out float NumSteps, float PixelRadius)
{
    NumSteps = min(PixelRadius, NumSamples);
    float PixelStepSize = PixelRadius / NumSteps;
    float MaxNumSteps = PixelMaxRadius / PixelStepSize;
    NumSteps = MaxNumSteps < NumSteps ? max(1, floor(MaxNumSteps + 0.5f)) : NumSteps;
    PixelStepSize = MaxNumSteps < NumSteps ? PixelMaxRadius / NumSteps : PixelStepSize;
    StepSizeUV = PixelStepSize * WindowParams.zw;
}

float2 RotateDirection(float2 Direction, float2 RandomDirection)
{
    return float2(Direction.x * RandomDirection.x - Direction.x * RandomDirection.y, 
                    Direction.y * RandomDirection.y + Direction.x * RandomDirection.x);
}

float Tan(float3 V)
{
    return V.z / sqrt(dot(V.xy, V.xy));
}

float TanToSin(float Tan)
{
    return Tan / sqrt(1.0f + Tan * Tan);
}

float Occlusion(float2 DeltaUV, float2 SampleUV, float RandStep, float NumSteps)
{
    float AO = 0.0f;
    float3 Position = ViewPosMap.Load(int3(SampleUV, 0)).xyz;
    float3 Tangent = ViewTangentMap.Load(int3(SampleUV, 0)).xyz;
    float TanH = Tan(Tangent);
    float SinH = TanToSin(TanH);
    for (int i = 1; i <= NumSteps; ++i)
    {
        SampleUV += DeltaUV;
        float3 SamplePosition = ViewPosMap.Load(int3(SampleUV, 0)).xyz;
        TanS = Tan(SamplePosition - Position);
        float Dis = length(SamplePosition - Position);
        if (Dis < ViewSpaceSampleRadius && TanS > TanH)
        {
            float SinS = TanToSin(TanS);
            AO += SinS - SinT;
            TanH = TanS;
        }
    }
    return AO;
}

float4 HBAO(float4 HPosition : SV_Position) : SV_Target0;
{
    // 得到NDC空间下的采样半径
    float2 SampleRadius = 0.5f * ViewSpaceSampleRadius * FocalLen / HPosition.w;
    // 以像素为单位的采样半径
    float PixelRadius = SampleRadius.x * WindowParams.x;

    float AOResult = 1.0f;
    [branch] if (PixelRadius > 1.0f)
    {
        AOResult = 0.0f;
        float2 StepSizeUV;
        float NumSteps;
        ComputeStepSize(StepSizeUV, NumSteps, PixelRadius);
        float Angle = 2 * PI / NumDirections;
        for (int i = 0; i < NumDirections; ++i)
        {
            float Theta = i * Angle;
            float RandNum = Random2DTo1D(HPosition.xy, 14375.5964f, float2(15.637f, 76.243f));
            // 在采样方向基础上做随机扰动
            float2 Direction = RotateDirection(float2(cos(Theta), sin(Theta)), float2(cos(RandNum), sin(RandNum)));
            float2 DeltaUV = Direction * StepSizeUV;
            AOResult += Occlusion(DeltaUV, HPosition.xy, RandNum, NumSteps);
        }
        AOResult = 1.0f - AOResult / NumDirections;
    }
    return float4(AOResult);
}