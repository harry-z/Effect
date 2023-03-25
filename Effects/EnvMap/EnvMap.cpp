#include "../../Effect.h"

D3D11BSWrapper g_pBlendState;
D3D11RSWrapper g_pRasterizerState;
D3D11DSSWrapper g_pDepthStencilState;
D3D11SSWrapper g_pSamplerState;
D3D11InputLayoutWrapper g_pInputLayout;
D3D11VertexShaderWrapper g_pGeometryShader;
D3D11PixelShaderWrapper g_pEnvMapShader;
D3D11Texture2DWrapper g_pEnvMap;
D3D11SRVWrapper g_pEnvMapSRV;
D3D11BufferWrapper g_pVertexBuffer;

bool CreateShaders()
{
	const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = sizeof(InputLayoutDesc) / sizeof(InputLayoutDesc[0]);

	TCHAR szShaderFileName[MAX_PATH];
	_tcscpy_s(szShaderFileName, GetExePath());
	_tcscpy_s(szShaderFileName, _T("Shaders\\EnvMap.hlsl"));
#ifdef DEBUG_SHADER
	UINT CompileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT CompileFlag = 0;
#endif
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeometryShader, 
		InputLayoutDesc, NumElements, g_pInputLayout))
		return false;
	if (!CreatePixelShader(szShaderFileName, "EnvMapPS", EShaderModel::ESM_5, g_pEnvMapShader))
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
	g_pDepthStencilState = TStaticDepthStencilState<TRUE, D3D11_COMPARISON_LESS_EQUAL>().GetDepthStencilState();
	return true;
}

bool CreateSamplerStates()
{
	g_pSamplerState = TStaticSamplerState<D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT>().GetSamplerState();
	return true;
}

bool LoadResources()
{
	TCHAR szTextureFileName[MAX_PATH];
	_tcscpy_s(szTextureFileName, GetExePath());
	_tcscpy_s(szTextureFileName, _T("Resources\\Chelsea_Stairs_8k.jpg"));
	if (!LoadJpegTextureFromFile(szTextureFileName, true, g_pEnvMap, g_pEnvMapSRV))
		return false;

	float VB[] = {
		-1.0f, 1.0f,
		-1.0f, -1.0f,
		1.0f, 1.0f,
		1.0f, -1.0f
	};

	D3D11_BUFFER_DESC Desc;
	Desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	Desc.ByteWidth = sizeof(float) * 2 * 4;
	Desc.CPUAccessFlags = 0;
	Desc.MiscFlags = 0;
	Desc.StructureByteStride = sizeof(float) * 2;
	Desc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA SubData;
	SubData.pSysMem = VB;
	SubData.SysMemPitch = SubData.SysMemSlicePitch = 0;

	g_D3DInterface.m_pDevice->CreateBuffer(&Desc, &SubData, g_pVertexBuffer);

	return true;
}

void RenderOneFrame()
{
	float ClearBackBufferColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_D3DInterface.m_pDeviceContext->ClearRenderTargetView(g_D3DInterface.m_pMainBackbuffer, ClearBackBufferColor);
	g_D3DInterface.m_pDeviceContext->ClearDepthStencilView(g_D3DInterface.m_pMainDepthbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_D3DInterface.m_pDeviceContext->OMSetRenderTargets(1, g_D3DInterface.m_pMainBackbuffer, g_D3DInterface.m_pMainDepthbuffer);

	float BLEND_FACTOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_D3DInterface.m_pDeviceContext->OMSetBlendState(g_pBlendState, BLEND_FACTOR, 0xffffffff);
	g_D3DInterface.m_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilState, 0);
	g_D3DInterface.m_pDeviceContext->RSSetState(g_pRasterizerState);

	g_D3DInterface.m_pDeviceContext->VSSetShader(g_pGeometryShader, nullptr, 0);
	g_D3DInterface.m_pDeviceContext->IASetInputLayout(g_pInputLayout);
	g_D3DInterface.m_pDeviceContext->PSSetShader(g_pEnvMapShader, nullptr, 0);
	g_D3DInterface.m_pDeviceContext->PSSetShaderResources(0, 1, g_pEnvMapSRV);
	g_D3DInterface.m_pDeviceContext->PSSetSamplers(0, 1, g_pSamplerState);

	XMVECTOR Deter;
	XMMATRIX Proj = GetPerspectiveProjectionMatrix();
	XMMATRIX InvProj = XMMatrixInverse(&Deter, Proj);
	XMStoreFloat4x4A(&g_GeomInvBuffer.InvProj, InvProj);
		
	XMMATRIX ViewProj = XMMatrixMultiply(GetCameraMatrixWithoutTranslation(), Proj);
	XMMATRIX InvViewProj = XMMatrixInverse(&Deter, ViewProj);
	XMStoreFloat4x4A(&g_GeomInvBuffer.InvViewProj, InvViewProj);

	D3D11_MAPPED_SUBRESOURCE SubRc;
	ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
	g_D3DInterface.m_pDeviceContext->Map(g_pGeomInvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomInvBuffer, sizeof(GeomInvBuffer));
	g_D3DInterface.m_pDeviceContext->Unmap(g_pGeomInvBuffer, 0);

	g_D3DInterface.m_pDeviceContext->VSSetConstantBuffers(1, 1, g_pGeomInvBuffer);

	if (g_pVertexBuffer.IsValid())
	{
		UINT Stride = sizeof(float) * 2;
		UINT Offset = 0;
		g_D3DInterface.m_pDeviceContext->IASetVertexBuffers(0, 1, g_pVertexBuffer, &Stride, &Offset);
		g_D3DInterface.m_pDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_D3DInterface.m_pDeviceContext->Draw(4, 0);
	}
}

#ifdef ENV_MAP_EXPORT_SYMBOL
	#define ENV_MAP_API __declspec(dllexport)
#else
	#define ENV_MAP_API
#endif

extern "C" 
{
	ENV_MAP_API void InitializeEffectInstance(Effect_Instance* pEffect)
	{
		pEffect->CreateShadersCallback = (RetBoolFunc)CreateShaders;
		pEffect->CreateBlendStatesCallback = (RetBoolFunc)CreateBlendStates;
		pEffect->CreateRasterizerStatesCallback = (RetBoolFunc)CreateRasterizerStates;
		pEffect->CreateDepthStencilStatesCallback = (RetBoolFunc)CreateDepthStencilStates;
		pEffect->CreateSamplerStatesCallback = (RetBoolFunc)CreateSamplerStates;
		pEffect->LoadResourcesCallback = (RetBoolFunc)LoadResources;
		pEffect->RenderOneFrameCallback = (Func)RenderOneFrame;
	}

	ENV_MAP_API void UninitializeEffectInstance()
	{
		g_pBlendState.Reset();
		g_pRasterizerState.Reset();
		g_pDepthStencilState.Reset();
		g_pSamplerState.Reset();
		g_pInputLayout.Reset();
		g_pGeometryShader.Reset();
		g_pEnvMapShader.Reset();
		g_pEnvMap.Reset();
		g_pEnvMapSRV.Reset();
		g_pVertexBuffer.Reset();
	}
}
