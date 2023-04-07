#include "Shaders/VSHeader.h"

VS_Out_P GeometryVS ( VS_In_P IN )
{
    VS_Out_P OUT;
    OUT.HPosition = mul(float4(IN.Position, 1.0f), WorldViewProj);
    OUT.WorldPosition = mul(float4(IN.Position, 1.0f), World).xyz;
    return OUT;
}

float4 ColorPS () : SV_Target0
{
	return float4(0.5f, 0.5f, 0.5f, 1.0f);
}