#include "Effect.h"

#ifdef PBR_EFFECT

D3D11InputLayoutWrapper g_pInputLayout;
D3D11VertexShaderWrapper g_pGeometryShader;
D3D11PixelShaderWrapper g_pPBRShader;
bool CreateShaders()
{
	const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT NumElements = sizeof(InputLayoutDesc) / sizeof(InputLayoutDesc[0]);

	wchar_t szShaderFileName[MAX_PATH];
	wcscpy_s(szShaderFileName, GetExePath());
	wcscat_s(szShaderFileName, L"PBR.hlsl");
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeometryShader, 
		InputLayoutDesc, NumElements, g_pInputLayout))
		return false;
	if (!CreatePixelShader(szShaderFileName, "PBRShading", EShaderModel::ESM_5, g_pPBRShader))
		return false;
	return true;
}

bool CreateBlendStates()
{
	return true;
}

bool CreateRasterizerStates()
{
	return true;
}

bool CreateDepthStencilStates()
{
	return true;
}

bool CreateSamplerStates()
{
	return true;
}

bool CreateRenderTargets()
{
	return true;
}

D3D11BufferWrapper g_pShadingParamsCB;
bool CreateConstantBuffers()
{
	D3D11_BUFFER_DESC ShadingParamsDesc;
	ShadingParamsDesc.ByteWidth = sizeof(float) * 16;
	ShadingParamsDesc.Usage = D3D11_USAGE_DYNAMIC;
	ShadingParamsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ShadingParamsDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ShadingParamsDesc.MiscFlags = 0;
	ShadingParamsDesc.StructureByteStride = 0;
	if (FAILED(g_pd3dDevice->CreateBuffer(&ShadingParamsDesc, nullptr, g_pShadingParamsCB)))
		return false;
	return true;
}

bool LoadResources()
{
	return true;
}

void LoadGeoms()
{
	constexpr UINT LatStep = 36; 
	constexpr UINT LonStep = 72;
	constexpr float Radius = 30.0f;

	float LatStepAngle = PI / LatStep;
	float LonStepAngle = PI * 2.0f / LonStep;
	UINT LatStride = LatStep + 1;
	UINT NumVertices = LatStride * LonStep;
	UINT NumIndices = LatStep * LonStep * 6;

	XMVECTOR RadiusVec = MakeD3DVECTOR(Radius, Radius, Radius);
	XMVECTOR LonDir = MakeD3DVECTOR(1.0f, 0.0f, 0.0f);
	XMVECTOR LatDir = MakeD3DVECTOR(0.0f, 0.0f, 1.0f);
	XMVECTOR UpVector = MakeD3DVECTOR(0.0f, 0.0f, 1.0f);

	float* pPositionBuffer = new float[NumVertices * 3];
	float* pNormalBuffer = new float[NumVertices * 3];
	UINT* pIndexBuffer = new UINT[NumIndices];

	UINT Sentinel = 0;

	for (UINT lon = 0; lon < LonStep; ++lon)
	{
		XMVECTOR LonRot = XMQuaternionRotationAxis(UpVector, LonStepAngle * lon);
		XMVECTOR LonVec = XMVector3Rotate(LonDir, LonRot);
		XMVECTOR LonRight = XMVector3Cross(UpVector, LonVec);
		LonRight = XMVector3Normalize(LonRight);
		for (UINT lat = 0; lat <= LatStep; ++lat)
		{
			XMVECTOR LatRot = XMQuaternionRotationAxis(LonRight, LatStepAngle * lat);
			XMVECTOR SphereDir = XMVector3Rotate(LatDir, LatRot);
			XMVECTOR SpherePoint = XMVectorMultiply(SphereDir, RadiusVec);
			XMFLOAT3A SpherePointFloat3;
			XMStoreFloat3A(&SpherePointFloat3, SpherePoint);
			pPositionBuffer[Sentinel] = SpherePointFloat3.x;
			pPositionBuffer[Sentinel + 1] = SpherePointFloat3.y;
			pPositionBuffer[Sentinel + 2] = SpherePointFloat3.z;
			XMStoreFloat3A(&SpherePointFloat3, SphereDir);
			pNormalBuffer[Sentinel] = SpherePointFloat3.x;
			pNormalBuffer[Sentinel + 1] = SpherePointFloat3.y;
			pNormalBuffer[Sentinel + 2] = SpherePointFloat3.z;
			Sentinel += 3;
		}
	}

	Sentinel = 0;

	for (UINT lon = 0; lon < LonStep; ++lon)
	{
		for (UINT lat = 0; lat < LatStep; ++lat)
		{
			pIndexBuffer[Sentinel] = lon * LatStride + lat;
			pIndexBuffer[Sentinel + 1] = ((lon + 1) % LonStep) * LatStride + lat;
			pIndexBuffer[Sentinel + 2] = ((lon + 1) % LonStep) * LatStride + lat + 1;
			pIndexBuffer[Sentinel + 3] = lon * LatStride + lat;
			pIndexBuffer[Sentinel + 4] = ((lon + 1) % LonStep) * LatStride + lat + 1;
			pIndexBuffer[Sentinel + 5] = lon * LatStride + lat + 1;
			Sentinel += 6;
		}
	}

	D3D11_BUFFER_DESC IndexBufferDesc;
	IndexBufferDesc.ByteWidth = sizeof(UINT) * NumIndices;
	IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexBufferDesc.CPUAccessFlags = 0;
	IndexBufferDesc.MiscFlags = 0;
	IndexBufferDesc.StructureByteStride = sizeof(UINT);
	
	HRESULT hr;

	float XPositions[] = { -500.0f, -400.0f, -300.0f, -200.0f, -100.0f, 0.0f, 100.0f, 200.0f, 300.0f, 400.0f, 500.0f };
	float YPositions[] = { -500.0f, -400.0f, -300.0f, -200.0f, -100.0f, 0.0f, 100.0f, 200.0f, 300.0f, 400.0f, 500.0f };
	for (UINT i = 0; i < 11; ++i)
	{
		for (UINT j = 0; j < 11; ++j)
		{
			D3D11_SUBRESOURCE_DATA IndexData;
			IndexData.pSysMem = pIndexBuffer;
			IndexData.SysMemPitch = IndexData.SysMemSlicePitch = 0;
			ID3D11Buffer* pD3DIndexBuffer = nullptr;
			if (FAILED(hr = g_pd3dDevice->CreateBuffer(&IndexBufferDesc, &IndexData, &pD3DIndexBuffer)))
				continue;
			VB_PositionNormal* pPositionNormal = new VB_PositionNormal;
			if (pPositionNormal->Initialize(pPositionBuffer, pNormalBuffer, NumVertices))
				AddGeom(pPositionNormal, pD3DIndexBuffer, NumIndices, XMMATRIX(
					1.0f, 0.0f, 0.0f, 0.0f, 
					0.0f, 1.0f, 0.0f, 0.0f,
					0.0f, 0.0f, 1.0f, 0.0f,
					XPositions[i], YPositions[j], 0.0f, 1.0f
				));
		}
	}

	delete[] pPositionBuffer;
	delete[] pNormalBuffer;
	delete[] pIndexBuffer;
}

struct alignas(16) ShadingParams
{
	XMFLOAT4A ViewDir;
	XMFLOAT4A LightDir;
	XMFLOAT4A Albedo;
	XMFLOAT4A SmoothnessAndMetalness;
} g_ShadingParams;

void RenderOneFrame()
{
	float ClearColorFront[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pOrigRTV, ClearColorFront);
	g_pImmediateContext->ClearDepthStencilView(g_pOrigDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_pImmediateContext->OMSetRenderTargets(1, g_pOrigRTV, g_pOrigDSV);

	float BLEND_FACTOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_pImmediateContext->OMSetBlendState(g_pOverwriteBS, BLEND_FACTOR, 0xffffffff);
	g_pImmediateContext->OMSetDepthStencilState(g_pLessEqualDS, 0);
	g_pImmediateContext->RSSetState(g_pBackCullRS);

	XMStoreFloat4A(&g_ShadingParams.ViewDir, XMVectorNegate(GetCameraViewDirection()));
	XMVECTOR LightDir = MakeD3DVECTOR(-0.7f, 1.0f, 0.5f);
	LightDir = XMVector3Normalize(LightDir);
	XMStoreFloat4A(&g_ShadingParams.LightDir, LightDir);

	g_pImmediateContext->VSSetShader(g_pGeometryShader, nullptr, 0);
	g_pImmediateContext->IASetInputLayout(g_pInputLayout);
	g_pImmediateContext->PSSetShader(g_pPBRShader, nullptr, 0);

	XMFLOAT4A Albedo(1.0f, 0.0f, 0.0f, 0.5f);
	float Smoothness[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	float Metalness[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	// float Metalness = 0.5f;

	for (UINT i = 0; i < 11; ++i)
	{
		for (UINT j = 0; j < 11; ++j)
		{
			SyncGeomConstantBuffer(GetGeoms()[i * 11 + j].m_WorldMatrix);

			D3D11_MAPPED_SUBRESOURCE SubRc;

			g_ShadingParams.Albedo = Albedo;
			g_ShadingParams.SmoothnessAndMetalness.x = Smoothness[i];
			g_ShadingParams.SmoothnessAndMetalness.y = Metalness[j];
			ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
			g_pImmediateContext->Map(g_pShadingParamsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
			memcpy(SubRc.pData, &g_ShadingParams.ViewDir, sizeof(ShadingParams));
			g_pImmediateContext->Unmap(g_pShadingParamsCB, 0);
			g_pImmediateContext->PSSetConstantBuffers(3, 1, g_pShadingParamsCB);

			DrawOneGeom(GetGeoms()[i * 11 + j]);
		}
	}
}

#endif