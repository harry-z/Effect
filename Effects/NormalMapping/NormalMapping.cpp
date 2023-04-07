#include "../../Effect.h"

D3D11BSWrapper g_pBlendState;
D3D11RSWrapper g_pRasterizerState;
D3D11DSSWrapper g_pDepthStencilState;
D3D11InputLayoutWrapper g_pInputLayout;
D3D11VertexShaderWrapper g_pGeometryShader;
D3D11PixelShaderWrapper g_pNormalMappingShader;
D3D11BufferWrapper g_pShadingParamsCB;
D3D11Texture2DWrapper g_pBaseColorTexture;
D3D11SRVWrapper g_pBaseColorSRV;
D3D11Texture2DWrapper g_pNRMTexture;
D3D11SRVWrapper g_pNRMSRV;

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
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = sizeof(InputLayoutDesc) / sizeof(InputLayoutDesc[0]);

	TCHAR szShaderFileName[MAX_PATH];
	_tcscpy_s(szShaderFileName, GetExePath());
	_tcscpy_s(szShaderFileName, _T("Shaders\\NormalMapping.hlsl"));
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeometryShader, 
		InputLayoutDesc, NumElements, g_pInputLayout))
		return false;
	if (!CreatePixelShader(szShaderFileName, "NormalMappingPS", EShaderModel::ESM_5, g_pNormalMappingShader))
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
	if (!LoadMesh(_T("Resources\\vbt5gh.fbx")))
		return false;
	if (!LoadJpegTextureFromFile(_T("Resources\\BaseColor.png"), true, g_pBaseColorTexture, g_pBaseColorSRV))
		return false;
	if (!LoadJpegTextureFromFile(_T("Resources\\BaseColor_NRM.jpg"), false, g_pNRMTexture, g_pNRMSRV))
		return false;
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
	g_D3DInterface.m_pDeviceContext->PSSetShader(g_pNormalMappingShader, nullptr, 0);

	XMFLOAT4A Albedo(1.0f, 0.71f, 0.29f, 0.5f);
	XMFLOAT4A LightColor(1.0f, 1.0f, 1.0f, 1.0f);
	float Smoothness[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };
	float Metalness[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f };

	XMMATRIX World(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	SyncGeomConstantBuffer(World);

	D3D11_MAPPED_SUBRESOURCE SubRc;

	g_ShadingParams.Albedo = Albedo;
	g_ShadingParams.LightColor = LightColor;
	g_ShadingParams.SmoothnessAndMetalness.x = 0.7f;
	g_ShadingParams.SmoothnessAndMetalness.y = 0.5f;
	ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
	g_D3DInterface.m_pDeviceContext->Map(g_pShadingParamsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_ShadingParams.ViewDir, sizeof(ShadingParams));
	g_D3DInterface.m_pDeviceContext->Unmap(g_pShadingParamsCB, 0);
	g_D3DInterface.m_pDeviceContext->PSSetConstantBuffers(0, 1, g_pShadingParamsCB);
	g_D3DInterface.m_pDeviceContext->PSSetShaderResources(0, 1, g_pBaseColorSRV);
	g_D3DInterface.m_pDeviceContext->PSSetShaderResources(1, 1, g_pNRMSRV);

	for (const auto& Geom : GetGeoms())	
		DrawOneGeom(Geom);
}

#ifdef NORMAL_MAPPING_EXPORT_SYMBOL
	#define NORMAL_MAPPING_API __declspec(dllexport)
#else
	#define NORMAL_MAPPING_API
#endif

extern "C" 
{
	NORMAL_MAPPING_API void InitializeEffectInstance(Effect_Instance* pEffect)
	{
		pEffect->CreateShadersCallback = (RetBoolFunc)CreateShaders;
		pEffect->CreateBlendStatesCallback = (RetBoolFunc)CreateBlendStates;
		pEffect->CreateRasterizerStatesCallback = (RetBoolFunc)CreateRasterizerStates;
		pEffect->CreateDepthStencilStatesCallback = (RetBoolFunc)CreateDepthStencilStates;
		pEffect->LoadResourcesCallback = (RetBoolFunc)LoadResources;
		pEffect->CreateConstantBuffersCallback = (RetBoolFunc)CreateConstantBuffers;
		pEffect->RenderOneFrameCallback = (Func)RenderOneFrame;
	}

	NORMAL_MAPPING_API void UninitializeEffectInstance()
	{
		g_pBlendState.Reset();
		g_pRasterizerState.Reset();
		g_pDepthStencilState.Reset();
		g_pInputLayout.Reset();
		g_pGeometryShader.Reset();
		g_pNormalMappingShader.Reset();
		g_pShadingParamsCB.Reset();
		g_pBaseColorTexture.Reset();
		g_pBaseColorSRV.Reset();
		g_pNRMTexture.Reset();
		g_pNRMSRV.Reset();
	}
}