#include "ShaderHeader.h"

struct Geometry_In
{
    float4 position : position;
};

struct Geometry_Out
{
    float4 HPosition : SV_Position;
};

Geometry_Out GeometryVS ( Geometry_In IN )
{
    Geometry_Out OUT;
    OUT.HPosition = mul(IN.position, WorldViewProj);
    return OUT;
}

float4 ColorPS () : SV_Target0
{
	return float4(0.5f, 0.5f, 0.5f, 1.0f);
}