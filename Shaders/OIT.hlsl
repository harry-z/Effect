#include "ShaderHeader.h"

cbuffer ShadingConstants : register(b3)
{
    float4 g_dir;
    float4 g_color;
}

struct Geometry_VSIn
{
    float4 position : position;
    float3 normal   : normal;
};

struct Geometry_VSOut
{
    float4 HPosition            : SV_Position;
    centroid float3 Normal      : TexCoord;
};

Geometry_VSOut GeometryVS ( Geometry_VSIn IN )
{
    Geometry_VSOut OUT;
    OUT.HPosition = mul(IN.position, WorldViewProj);
    OUT.Normal = normalize(mul(IN.normal, (float3x3)WorldIT).xyz);
    return OUT;
}

float4 ShadeFragment( float3 Normal )
{
    // return float4(g_color.rgb * abs(Normal.z), g_color.a);
    return float4(g_color.rgb * clamp(dot(g_dir.xyz, normalize(Normal)), 0.0f, 1.0f), g_color.a);
}

struct FullscreenVSOut
{
    float4 pos : SV_Position;
    float2 tex : TEXCOORD;
};

FullscreenVSOut FullScreenTriangleVS( uint id : SV_VertexID )
{
    FullscreenVSOut output = (FullscreenVSOut)0.0f;
    output.tex = float2( (id << 1) & 2, id & 2 );
    output.pos = float4( output.tex * float2( 2.0f, -2.0f ) + float2( -1.0f, 1.0f), 0.0f, 1.0f );
    return output;
}

Texture2D<float2> tDepthBlender : register(t0);
Texture2D<float4> tFrontBlender : register(t1);
Texture2D<float4> tBackBlender  : register(t2);

#define MAX_DEPTH_FLOAT 1.0f

struct DDPOutputMRT
{
    float2 Depths       : SV_Target0;
    float4 FrontColor   : SV_Target1;
    float4 BackColor    : SV_Target2;
};

float4 DDPFirstPassPS ( Geometry_VSOut IN ) : SV_TARGET
{
    float z = IN.HPosition.z;
    // Using -z to get nearest z, MAX blend state
    return float4(-z, z, 0, 0);
}

DDPOutputMRT DDPDepthPeelPS ( Geometry_VSOut IN )
{
    DDPOutputMRT OUT = (DDPOutputMRT)0;

    // Window-space depth interpolated linearly in screen space
    float fragDepth = IN.HPosition.z;

    OUT.Depths.xy = tDepthBlender.Load( int3( IN.HPosition.xy, 0 ) ).xy;
    float nearestDepth = -OUT.Depths.x;
    float farthestDepth = OUT.Depths.y;

    if (fragDepth < nearestDepth || fragDepth > farthestDepth) {
        // Skip this depth in the peeling algorithm
        OUT.Depths.xy = -MAX_DEPTH_FLOAT;
        return OUT;
    }
    
    if (fragDepth > nearestDepth && fragDepth < farthestDepth) {
        // This fragment needs to be peeled again
        OUT.Depths.xy = float2(-fragDepth, fragDepth);
        return OUT;
    }
    
    // If we made it here, this fragment is on the peeled layer from last pass
    // therefore, we need to shade it, and make sure it is not peeled any farther
    float4 color = ShadeFragment(IN.Normal);
    OUT.Depths.xy = -MAX_DEPTH_FLOAT;

    // Front to back blending, Under Blending
    // C_dest = A_dest * (A_src * C_src) + C_dest
    // A_dest = (1 - A_src) * A_dest
    // With more peeling pass, A_dest is getting smaller. The larger the depth is,
    // the smaller the A_dest will be.
    if (fragDepth == nearestDepth) 
    {
        // A_src * C_src
        color.rgb *= color.a;
        OUT.FrontColor = color;
    }
    // Back to front blending, ie. regular translucent blending
    // C_dest = A_src * C_src + (1 - A_src) * C_dest
    else
    {
        OUT.BackColor = color;
    }
    return OUT;
}

float3 DDPFinalPS ( FullscreenVSOut IN ) : SV_TARGET
{
    float4 frontColor = tFrontBlender.Load( int3( IN.pos.xy, 0 ) );
    float3 backColor = tBackBlender.Load( int3( IN.pos.xy, 0 ) ).xyz;
    return frontColor.xyz + backColor * frontColor.w;
}