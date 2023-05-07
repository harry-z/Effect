#define PI 3.1415926f

float4 FullScreenQuad(float2 InPosition : POSITION) : SV_Position
{
    return float4(InPosition, 0.0f, 1.0f);
}

float RadicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}

float2 Hammersley2d(uint i, uint N) {
    return float2(float(i)/float(N), RadicalInverse_VdC(i));
}

float Random2DTo1D(float2 Value, float a, float b)
{
    float2 SmallValue = sin(Value);
	float Random = dot(SmallValue, b);
	Random = frac(sin(Random) * a);
	return Random;
}

float2 Random2DTo2D(float2 Value)
{
    return float2(
		Random2DTo1D(Value, 14375.5964f, float2(15.637f, 76.243f)),
		Random2DTo1D(Value, 14684.6034f, float2(45.366f, 23.168f))
	);
}