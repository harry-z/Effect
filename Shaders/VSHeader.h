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

struct VS_In_P
{
	float3 Position			: POSITION;
};

struct VS_In_PN
{
	float3 Position			: POSITION;
	float3 Normal			: NORMAL;
};

struct VS_In_PNTUV0
{
	float3 Position			: POSITION;
	float3 Normal			: NORMAL;
	float3 Tangent			: TANGENT;
	float2 UV				: TEXCOORD0;
};

struct VS_Out_P
{
	float4 HPosition			: SV_Position;
	float3 WorldPosition		: TEXCOORD0;
};

struct VS_Out_PN
{
	float4 HPosition		: SV_Position;
	float3 WorldPosition	: TEXCOORD0;
	float3 WorldNormal		: TEXCOORD1;
};

struct VS_Out_PNTUV0
{
	float4 HPosition		: SV_Position;
	float3 WorldPosition	: TEXCOORD0;
	float3 WorldNormal		: TEXCOORD1;
	float3 WorldTangent		: TEXCOORD2;
	float3 WorldBinormal	: TEXCOORD3;
	float2 UV				: TEXCOORD4;
};