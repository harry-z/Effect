#include "../../Effect.h"


D3D11BSWrapper g_pBlendState;
D3D11RSWrapper g_pRasterizerState;
D3D11DSSWrapper g_pDepthStencilState;
D3D11InputLayoutWrapper g_pInputLayout;
D3D11VertexShaderWrapper g_pGeomShader;
D3D11PixelShaderWrapper g_pColorShader;

bool CreateShaders()
{
	const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = sizeof(InputLayoutDesc) / sizeof(InputLayoutDesc[0]);

	TCHAR szShaderFileName[MAX_PATH];
	_tcscpy_s(szShaderFileName, GetExePath());
	_tcscpy_s(szShaderFileName, _T("Shaders\\SimpleHLSL.hlsl"));
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeomShader, 
		InputLayoutDesc, NumElements, g_pInputLayout))
		return false;
	if (!CreatePixelShader(szShaderFileName, "ColorPS", EShaderModel::ESM_5, g_pColorShader))
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

bool LoadResources()
{
	VB_Position* pPositionVB = new VB_Position;
	float VBData[] = {
		0.0f, -100.0f, 100.0f,
		0.0f, -100.0f, -100.0f,
		0.0f, 100.0f, 100.0f,
		0.0f, 100.0f, -100.0f
	};
	if (pPositionVB->Initialize(VBData, 4))
		AddGeom(pPositionVB, nullptr, 0, XMMatrixIdentity());
	else
		delete pPositionVB;

	LoadMesh(_T("Resources\\vbt5gh.fbx"));

	return true;
}

void RenderOneFrame()
{
	XMMATRIX World(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	SyncGeomConstantBuffer(World);

	float ClearBackBufferColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_D3DInterface.m_pDeviceContext->ClearRenderTargetView(g_D3DInterface.m_pMainBackbuffer, ClearBackBufferColor);
	g_D3DInterface.m_pDeviceContext->ClearDepthStencilView(g_D3DInterface.m_pMainDepthbuffer, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_D3DInterface.m_pDeviceContext->OMSetRenderTargets(1, g_D3DInterface.m_pMainBackbuffer, g_D3DInterface.m_pMainDepthbuffer);

	g_D3DInterface.m_pDeviceContext->RSSetState(g_pRasterizerState);
	float BlendFactor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_D3DInterface.m_pDeviceContext->OMSetBlendState(g_pBlendState, BlendFactor, 0xffffffff);
	g_D3DInterface.m_pDeviceContext->OMSetDepthStencilState(g_pDepthStencilState, 0);

	g_D3DInterface.m_pDeviceContext->VSSetShader(g_pGeomShader, nullptr, 0);
	g_D3DInterface.m_pDeviceContext->PSSetShader(g_pColorShader, nullptr, 0);

	g_D3DInterface.m_pDeviceContext->IASetInputLayout(g_pInputLayout);
	for (const auto& Geom : GetGeoms())
		DrawOneGeom(Geom);
}

#ifdef SIMPLE_HLSL_EXPORT_SYMBOL
	#define SIMPLE_HLSL_API __declspec(dllexport)
#else
	#define SIMPLE_HLSL_API
#endif

extern "C" 
{
	SIMPLE_HLSL_API void InitializeEffectInstance(Effect_Instance* pEffect)
	{
		pEffect->CreateShadersCallback = (RetBoolFunc)CreateShaders;
		pEffect->CreateBlendStatesCallback = (RetBoolFunc)CreateBlendStates;
		pEffect->CreateRasterizerStatesCallback = (RetBoolFunc)CreateRasterizerStates;
		pEffect->CreateDepthStencilStatesCallback = (RetBoolFunc)CreateDepthStencilStates;
		pEffect->LoadResourcesCallback = (RetBoolFunc)LoadResources;
		pEffect->RenderOneFrameCallback = (Func)RenderOneFrame;
	}

	SIMPLE_HLSL_API void UninitializeEffectInstance()
	{
		g_pBlendState.Reset();
		g_pRasterizerState.Reset();
		g_pDepthStencilState.Reset();
		g_pInputLayout.Reset();
		g_pGeomShader.Reset();
		g_pColorShader.Reset();
	}
}