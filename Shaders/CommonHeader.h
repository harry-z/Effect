#define PI 3.1415926f

struct FullScreenQuadOutput
{
    float4 HPosition : SV_Position;
};

float4 FullScreenQuad(float2 InPosition : POSITION) : SV_Position
{
    return float4(InPosition, 0.0f, 1.0f);
}