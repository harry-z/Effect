#include "Effect.h"

#ifdef ENVMAP_EFFECT

D3D11InputLayoutWrapper g_pInputLayout;
D3D11VertexShaderWrapper g_pGeometryShader;
D3D11PixelShaderWrapper g_pEnvMapShader;

bool CreateShaders()
{
	const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements = sizeof(InputLayoutDesc) / sizeof(InputLayoutDesc[0]);

	wchar_t szShaderFileName[MAX_PATH];
	wcscpy_s(szShaderFileName, GetExePath());
	wcscat_s(szShaderFileName, L"EnvMap.hlsl");
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

D3D11Texture2DWrapper g_pEnvMap;
D3D11SRVWrapper g_pEnvMapSRV;
bool LoadResources()
{
	wchar_t szTextureFileName[MAX_PATH];
	wcscpy_s(szTextureFileName, GetExePath());
	wcscat_s(szTextureFileName, L"Arches_E_PineTree_8k.jpg");
	return LoadJpegTextureFromFile(szTextureFileName, true, g_pEnvMap, g_pEnvMapSRV);
}

D3D11BufferWrapper g_pVertexBuffer;
void LoadGeoms()
{
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

	g_pd3dDevice->CreateBuffer(&Desc, &SubData, g_pVertexBuffer);
}

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

	g_pImmediateContext->VSSetShader(g_pGeometryShader, nullptr, 0);
	g_pImmediateContext->IASetInputLayout(g_pInputLayout);
	g_pImmediateContext->PSSetShader(g_pEnvMapShader, nullptr, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, g_pEnvMapSRV);
	g_pImmediateContext->PSSetSamplers(0, 1, g_pLinearSamplerState);

	XMMATRIX View = GetCameraMatrixWithoutTranslation();
	XMVECTOR Deter;
	XMMATRIX InvView = XMMatrixInverse(&Deter, View);
	XMMATRIX InvViewProj = XMMatrixMultiply(g_InvProjectionMatrix, InvView);
	XMStoreFloat4x4A(&g_GeomInvBuffer.InvViewProj, InvViewProj);
	D3D11_MAPPED_SUBRESOURCE SubRc;
	ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
	g_pImmediateContext->Map(g_pGeomInvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomInvBuffer, sizeof(GeomInvBuffer));
	g_pImmediateContext->Unmap(g_pGeomInvBuffer, 0);
	// g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pParamsCB);

	if (g_pVertexBuffer.IsValid())
	{
		UINT Stride = sizeof(float) * 2;
		UINT Offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, g_pVertexBuffer, &Stride, &Offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_pImmediateContext->Draw(4, 0);
	}
}

#endif