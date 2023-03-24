#pragma pack_matrix(row_major)

cbuffer GeomParams : register(b0)
{
	float4x4 World;
	float4x4 WorldView;
	float4x4 WorldViewProj;
};

cbuffer GeomInvParams : register(b1)
{
	float4x4 InvProj;
	float4x4 InvViewProj;
};

cbuffer GeomITParams : register(b2)
{
	float4x4 WorldIT; // Inverse Transposed
	float4x4 WorldViewIT;
};

#define PI 3.1415926f