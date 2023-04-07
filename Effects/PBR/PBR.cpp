#include "../../Effect.h"

D3D11BSWrapper g_pBlendState;
D3D11RSWrapper g_pRasterizerState;
D3D11DSSWrapper g_pDepthStencilState;
D3D11InputLayoutWrapper g_pInputLayout;
D3D11VertexShaderWrapper g_pGeometryShader;
D3D11PixelShaderWrapper g_pPBRShader;
D3D11BufferWrapper g_pShadingParamsCB;

struct alignas(16) ShadingParams
{
	XMFLOAT4A ViewDir;
	XMFLOAT4A LightDir;
	XMFLOAT4A Albedo;
	XMFLOAT4A LightColor;
	XMFLOAT4A SmoothnessAndMetalness;
} g_ShadingParams;

bool CreateShaders()
{
	const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT NumElements = sizeof(InputLayoutDesc) / sizeof(InputLayoutDesc[0]);

	TCHAR szShaderFileName[MAX_PATH];
	_tcscpy_s(szShaderFileName, GetExePath());
	_tcscpy_s(szShaderFileName, _T("Shaders\\PBR.hlsl"));
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeometryShader, 
		InputLayoutDesc, NumElements, g_pInputLayout))
		return false;
	if (!CreatePixelShader(szShaderFileName, "PBRShading", EShaderModel::ESM_5, g_pPBRShader))
		return false;
	return true;
}

bool CreateBlendStates()
{
	g_pBlendState = TStaticBlendState<>().GetBlendState();
	return true;
}

bool CreateRasterizerStates()
{
	g_pRasterizerState = TStaticRasterizerState<>().GetRasterizerState();
	return true;
}

bool CreateDepthStencilStates()
{
	g_pDepthStencilState = TStaticDepthStencilState<>().GetDepthStencilState();
	return true;
}

bool CreateConstantBuffers()
{
	D3D11_BUFFER_DESC ShadingParamsDesc;
	ShadingParamsDesc.ByteWidth = sizeof(ShadingParams);
	ShadingParamsDesc.Usage = D3D11_USAGE_DYNAMIC;
	ShadingParamsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ShadingParamsDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ShadingParamsDesc.MiscFlags = 0;
	ShadingParamsDesc.StructureByteStride = 0;
	if (FAILED(g_D3DInterface.m_pDevice->CreateBuffer(&ShadingParamsDesc, nullptr, g_pShadingParamsCB)))
		return false;
	return true;
}

bool LoadResources()
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

	float XPositions[] = { -400.0f, -300.0f, -200.0f, -100.0f, 0.0f, 100.0f, 200.0f, 300.0f, 400.0f };
	float YPositions[] = { -400.0f, -300.0f, -200.0f, -100.0f, 0.0f, 100.0f, 200.0f, 300.0f, 400.0f };
	for (UINT i = 0; i < 9; ++i)
	{
		for (UINT j = 0; j < 9; ++j)
		{
			D3D11_SUBRESOURCE_DATA IndexData;
			IndexData.pSysMem = pIndexBuffer;
			IndexData.SysMemPitch = IndexData.SysMemSlicePitch = 0;
			ID3D11Buffer* pD3DIndexBuffer = nullptr;
			if (FAILED(hr = g_D3DInterface.m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexData, &pD3DIndexBuffer)))
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

	return true;
}



void RenderOneFrame()
{
	float ClearColorFront[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_D3DInterface.m_pDeviceContext->ClearRenderTargetView(g_D3DInterface.m_pMainBackbuffer, ClearColorFront);
	g_D3DInterface.m_pDeviceContext->ClearDepthStencilView(g_D3DInterface.m_pMainDepthbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_D3DInterface.m_pDeviceContext->OMSetRenderTargets(1, g_D3DInterface.m_pMainBackbuffer, g_D3DInterface.m_pMainDepthbuffer);

	float BLEND_FACTOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_D3DInterface.m_pDeviceContext->OMSetBlendState(g_pBlendState, BLEND_FACTOR, 0xffffffff);
	g_D3DInterface.m_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilState, 0);
	g_D3DInterface.m_pDeviceContext->RSSetState(g_pRasterizerState);

	XMVECTOR ViewDir = GetCameraViewLocation();
	XMStoreFloat4A(&g_ShadingParams.ViewDir, ViewDir);
	XMVECTOR LightDir = MakeD3DVECTOR(1.0f, 1.0f, 1.0f);
	LightDir = XMVector3Normalize(LightDir);
	XMStoreFloat4A(&g_ShadingParams.LightDir, LightDir);

	g_D3DInterface.m_pDeviceContext->VSSetShader(g_pGeometryShader, nullptr, 0);
	g_D3DInterface.m_pDeviceContext->IASetInputLayout(g_pInputLayout);
	g_D3DInterface.m_pDeviceContext->PSSetShader(g_pPBRShader, nullptr, 0);

	XMFLOAT4A Albedo(1.0f, 0.71f, 0.29f, 0.5f);
	XMFLOAT4A LightColor(1.0f, 1.0f, 1.0f, 1.0f);
	float Smoothness[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
	float Metalness[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };

	for (UINT i = 0; i < 9; ++i)
	{
		for (UINT j = 0; j < 9; ++j)
		{
			SyncGeomConstantBuffer(GetGeoms()[i * 9 + j].m_WorldMatrix);

			D3D11_MAPPED_SUBRESOURCE SubRc;

			g_ShadingParams.Albedo = Albedo;
			g_ShadingParams.LightColor = LightColor;
			g_ShadingParams.SmoothnessAndMetalness.x = Smoothness[i];
			g_ShadingParams.SmoothnessAndMetalness.y = Metalness[j];
			ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
			g_D3DInterface.m_pDeviceContext->Map(g_pShadingParamsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
			memcpy(SubRc.pData, &g_ShadingParams.ViewDir, sizeof(ShadingParams));
			g_D3DInterface.m_pDeviceContext->Unmap(g_pShadingParamsCB, 0);
			g_D3DInterface.m_pDeviceContext->PSSetConstantBuffers(0, 1, g_pShadingParamsCB);

			DrawOneGeom(GetGeoms()[i * 9 + j]);
		}
	}
}

#ifdef PBR_EXPORT_SYMBOL
	#define PBR_API __declspec(dllexport)
#else
	#define PBR_API
#endif

extern "C" 
{
	PBR_API void InitializeEffectInstance(Effect_Instance* pEffect)
	{
		pEffect->CreateShadersCallback = (RetBoolFunc)CreateShaders;
		pEffect->CreateBlendStatesCallback = (RetBoolFunc)CreateBlendStates;
		pEffect->CreateRasterizerStatesCallback = (RetBoolFunc)CreateRasterizerStates;
		pEffect->CreateDepthStencilStatesCallback = (RetBoolFunc)CreateDepthStencilStates;
		pEffect->LoadResourcesCallback = (RetBoolFunc)LoadResources;
		pEffect->CreateConstantBuffersCallback = (RetBoolFunc)CreateConstantBuffers;
		pEffect->RenderOneFrameCallback = (Func)RenderOneFrame;
	}

	PBR_API void UninitializeEffectInstance()
	{
		g_pBlendState.Reset();
		g_pRasterizerState.Reset();
		g_pDepthStencilState.Reset();
		g_pInputLayout.Reset();
		g_pGeometryShader.Reset();
		g_pPBRShader.Reset();
		g_pShadingParamsCB.Reset();
	}
}