#include "../../Effect.h"



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

	wchar_t szShaderFileName[MAX_PATH];
	wcscpy_s(szShaderFileName, GetExePath());
	wcscat_s(szShaderFileName, L"SimpleHLSL.hlsl");
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeomShader, 
		InputLayoutDesc, NumElements, g_pInputLayout))
		return false;
	if (!CreatePixelShader(szShaderFileName, "ColorPS", EShaderModel::ESM_5, g_pColorShader))
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

bool CreateConstantBuffers()
{
	return true;
}

bool LoadResources()
{
	return true;
}

void LoadGeoms()
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
	g_pImmediateContext->ClearRenderTargetView(g_pOrigRTV, ClearBackBufferColor);
	g_pImmediateContext->ClearDepthStencilView(g_pOrigDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_pImmediateContext->OMSetRenderTargets(1, g_pOrigRTV, g_pOrigDSV);

	g_pImmediateContext->RSSetState(g_pBackCullRS);
	float BlendFactor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_pImmediateContext->OMSetBlendState(g_pOverwriteBS, BlendFactor, 0xffffffff);
	g_pImmediateContext->OMSetDepthStencilState(g_pLessEqualDS, 0);

	g_pImmediateContext->VSSetShader(g_pGeomShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pColorShader, nullptr, 0);

	g_pImmediateContext->IASetInputLayout(g_pInputLayout);
	for (const auto& Geom : GetGeoms())
		DrawOneGeom(Geom);
}

#ifdef SIMPLE_HLSL_API
	#define EXPORT_API __declspec(dllexport)
#else
	#define EXPORT_API
#endif

extern "C" 
{
	EXPORT_API void InitializeEffectInstance(Effect_Instance* pEffect)
	{
		pEffect->CreateShadersCallback = (RetBoolFunc)CreateShaders;
		pEffect->LoadResourcesCallback = (RetBoolFunc)LoadResources;
		pEffect->RenderOneFrameCallback = (Func)RenderOneFrame;
	}
}