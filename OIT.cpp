#include "Effect.h"

#ifdef OIT_EFFECT

D3D11InputLayoutWrapper g_pInputLayout;
D3D11VertexShaderWrapper g_pGeomShader;
D3D11VertexShaderWrapper g_pFullScreenRectShader;
D3D11PixelShaderWrapper g_pDDPFirstPassShader;
D3D11PixelShaderWrapper g_pDDPDepthPeelShader;
D3D11PixelShaderWrapper g_pDDPFinalShader;

bool CreateShaders()
{
	HRESULT hr;

	struct ScopedGuard {
		ID3D11InputLayout* m_pInputLayout = nullptr;
		ID3D11VertexShader* m_pGeomShader = nullptr;
		ID3D11VertexShader* m_pFullScreenRectShader = nullptr;
		ID3D11PixelShader* m_pDDPFirstPassShader = nullptr;
		ID3D11PixelShader* m_pDDPDepthPeelShader = nullptr;
		ID3D11PixelShader* m_pDDPFinalShader = nullptr;
		ID3DBlob* m_pBlob = nullptr;
		~ScopedGuard() {
			SAFE_RELEASE(m_pInputLayout);
			SAFE_RELEASE(m_pGeomShader);
			SAFE_RELEASE(m_pFullScreenRectShader);
			SAFE_RELEASE(m_pDDPFirstPassShader);
			SAFE_RELEASE(m_pDDPDepthPeelShader);
			SAFE_RELEASE(m_pDDPFinalShader);
			SAFE_RELEASE(m_pBlob);
		}
	} Guard;

	const D3D11_INPUT_ELEMENT_DESC InputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	UINT NumElements = sizeof(InputLayoutDesc) / sizeof(InputLayoutDesc[0]);

	wchar_t szShaderFileName[MAX_PATH];
	wcscpy_s(szShaderFileName, GetExePath());
	wcscat_s(szShaderFileName, L"OIT.hlsl");
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeomShader, 
		InputLayoutDesc, NumElements, g_pInputLayout))
		return false;
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "FullScreenTriangleVS", EShaderModel::ESM_5, g_pFullScreenRectShader, 
		nullptr, 0, nullptr))
		return false;
	if (!CreatePixelShader(szShaderFileName, "DDPFirstPassPS", EShaderModel::ESM_5, g_pDDPFirstPassShader))
		return false;
	if (!CreatePixelShader(szShaderFileName, "DDPDepthPeelPS", EShaderModel::ESM_5, g_pDDPDepthPeelShader))
		return false;
	if (!CreatePixelShader(szShaderFileName, "DDPFinalPS", EShaderModel::ESM_5, g_pDDPFinalShader))
		return false;

	return true;
}

D3D11BSWrapper g_pDDPBS;
D3D11BSWrapper g_pMaxBS;
D3D11BSWrapper g_pNoBlendBS;

bool CreateBlendStates()
{
	D3D11_BLEND_DESC blendState;
	blendState.AlphaToCoverageEnable = FALSE;
	blendState.IndependentBlendEnable = TRUE;
	for (int i = 0; i < 3; ++i)
	{
		blendState.RenderTarget[i].BlendEnable = TRUE;
		blendState.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}
	for (int i = 3; i < 8; ++i)
	{
		blendState.RenderTarget[i].BlendEnable = FALSE;
		blendState.RenderTarget[i].RenderTargetWriteMask = 0;
	}

	// Max blending
	blendState.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendState.RenderTarget[0].DestBlend = D3D11_BLEND_ONE;
	blendState.RenderTarget[0].BlendOp = D3D11_BLEND_OP_MAX;
	blendState.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendState.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendState.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_MAX;

	// Front-to-back blending
	blendState.RenderTarget[1].SrcBlend = D3D11_BLEND_DEST_ALPHA;
	blendState.RenderTarget[1].DestBlend = D3D11_BLEND_ONE;
	blendState.RenderTarget[1].BlendOp = D3D11_BLEND_OP_ADD;
	blendState.RenderTarget[1].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendState.RenderTarget[1].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[1].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	// Back-to-front blending
	blendState.RenderTarget[2].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendState.RenderTarget[2].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendState.RenderTarget[2].BlendOp = D3D11_BLEND_OP_ADD;
	blendState.RenderTarget[2].SrcBlendAlpha = D3D11_BLEND_ZERO;
	blendState.RenderTarget[2].DestBlendAlpha = D3D11_BLEND_ONE;
	blendState.RenderTarget[2].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	if (FAILED(g_pd3dDevice->CreateBlendState(&blendState, g_pDDPBS)))
		return false;

	// Max blending
	for (int i = 0; i < 3; ++i)
	{
		blendState.RenderTarget[i].BlendEnable = TRUE;
		blendState.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		blendState.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
		blendState.RenderTarget[i].DestBlend = D3D11_BLEND_ONE;
		blendState.RenderTarget[i].BlendOp = D3D11_BLEND_OP_MAX;
		blendState.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		blendState.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ONE;
		blendState.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_MAX;
	}
	if (FAILED(g_pd3dDevice->CreateBlendState(&blendState, g_pMaxBS)))
		return false;

	// No blending
	blendState.IndependentBlendEnable = FALSE;
	for (int i = 0; i < 8; ++i)
	{
		blendState.RenderTarget[i].BlendEnable = FALSE;
		blendState.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}
	if (FAILED(g_pd3dDevice->CreateBlendState(&blendState, g_pNoBlendBS)))
		return false;
	return true;
}

D3D11RSWrapper g_pNoCullRS;
bool CreateRasterizerStates()
{
	D3D11_RASTERIZER_DESC rasterizerState;
	rasterizerState.FillMode = D3D11_FILL_SOLID;
	rasterizerState.CullMode = D3D11_CULL_NONE;
	rasterizerState.FrontCounterClockwise = FALSE;
	rasterizerState.DepthBias = FALSE;
	rasterizerState.DepthBiasClamp = 0;
	rasterizerState.SlopeScaledDepthBias = 0;
	rasterizerState.DepthClipEnable = FALSE;
	rasterizerState.ScissorEnable = FALSE;
	rasterizerState.MultisampleEnable = FALSE;
	rasterizerState.AntialiasedLineEnable = FALSE;
	if (FAILED(g_pd3dDevice->CreateRasterizerState(&rasterizerState, g_pNoCullRS)))
		return false;
	return true;
}

D3D11DSSWrapper g_pNoDepthNoStencilDS;
bool CreateDepthStencilStates()
{
	D3D11_DEPTH_STENCIL_DESC depthstencilState;
	depthstencilState.DepthEnable = FALSE;
	depthstencilState.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthstencilState.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	depthstencilState.StencilEnable = FALSE;
	if (FAILED(g_pd3dDevice->CreateDepthStencilState(&depthstencilState, g_pNoDepthNoStencilDS)))
		return false;
	return true;
}

bool CreateSamplerStates()
{
	return true;
}

SimpleRT m_MinMaxZRenderTargets[2];
SimpleRT m_FrontBlenderRenderTarget;
SimpleRT m_BackBlenderRenderTarget;

bool CreateRenderTargets()
{
	RECT rect;
	GetClientRect(GetHWnd(), &rect);

	D3D11_TEXTURE2D_DESC texDesc;
	texDesc.Width = rect.right - rect.left;
	texDesc.Height = rect.bottom - rect.top;
	texDesc.ArraySize = 1;
	texDesc.MiscFlags = 0;
	texDesc.MipLevels = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	texDesc.Usage = D3D11_USAGE_DEFAULT;
	texDesc.CPUAccessFlags = 0;

	for (int i = 0; i < 2; ++i)
	{
		if (!m_MinMaxZRenderTargets[i].Initialize(g_pd3dDevice, &texDesc, DXGI_FORMAT_R32G32_FLOAT))
			return false;
	}

	if (!m_FrontBlenderRenderTarget.Initialize(g_pd3dDevice, &texDesc, DXGI_FORMAT_R8G8B8A8_UNORM))
		return false;
	if (!m_BackBlenderRenderTarget.Initialize(g_pd3dDevice, &texDesc, DXGI_FORMAT_R8G8B8A8_UNORM))
		return false;

	return true;
}

D3D11BufferWrapper g_pShadingParamsCB;

bool CreateConstantBuffers()
{
	D3D11_BUFFER_DESC ShadingParamsDesc;
	ShadingParamsDesc.ByteWidth = sizeof(float) * 8;
	ShadingParamsDesc.Usage = D3D11_USAGE_DYNAMIC;
	ShadingParamsDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	ShadingParamsDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ShadingParamsDesc.MiscFlags = 0;
	ShadingParamsDesc.StructureByteStride = 0;
	if (FAILED(g_pd3dDevice->CreateBuffer(&ShadingParamsDesc, nullptr, g_pShadingParamsCB)))
		return false;
	return true;
}

template <class Type>
Type ReadPODTypeAndMove(byte*& pSentinel)
{
	Type Ret = *((Type*)pSentinel);
	pSentinel += sizeof(Type);
	return Ret;
}

void LoadGeom(wchar_t* pszFileName) {
	FILE* pFile;
	if (_wfopen_s(&pFile, pszFileName, L"rb") == 0) {
		long s = ftell(pFile);
		fseek(pFile, 0, SEEK_END);
		long e = ftell(pFile);
		e -= s;
		fseek(pFile, 0, SEEK_SET);

		byte* pMem = (byte*)malloc(e);
		fread(pMem, sizeof(byte), e, pFile);
		fclose(pFile);

		byte* pSentinel = pMem;
		pSentinel += sizeof(int); // Skip version
		int NumSection = ReadPODTypeAndMove<int>(pSentinel);
		int NumIndex = ReadPODTypeAndMove<int>(pSentinel);
		int NumPosition = ReadPODTypeAndMove<int>(pSentinel);
		int NumNormal = ReadPODTypeAndMove<int>(pSentinel);
		assert(NumPosition == NumNormal);
		// Skip tangent and UV count
		pSentinel += sizeof(int) * 2;
		// Skip section offset
		// constexpr int SectionSize = sizeof(int) * 2 + sizeof(float) * 6;
		// pSentinel += SectionSize * NumSection;
		pSentinel += sizeof(int);
		int IndexOffset = ReadPODTypeAndMove<int>(pSentinel);
		int PositionOffset = ReadPODTypeAndMove<int>(pSentinel);
		int NormalOffset = ReadPODTypeAndMove<int>(pSentinel);

		struct ScopedGuard {
			ID3D11Buffer* m_pIndexBuffer = nullptr;
			~ScopedGuard() {
				SAFE_RELEASE(m_pIndexBuffer);
			}
		} Guard;

		HRESULT hr;

		D3D11_BUFFER_DESC IndexBufferDesc;
		IndexBufferDesc.ByteWidth = sizeof(unsigned) * NumIndex;
		IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		IndexBufferDesc.CPUAccessFlags = 0;
		IndexBufferDesc.MiscFlags = 0;
		IndexBufferDesc.StructureByteStride = sizeof(unsigned);
		D3D11_SUBRESOURCE_DATA IndexData;
		IndexData.pSysMem = pMem + IndexOffset;
		IndexData.SysMemPitch = IndexData.SysMemSlicePitch = 0;
		if (SUCCEEDED(hr = g_pd3dDevice->CreateBuffer(&IndexBufferDesc, &IndexData, &Guard.m_pIndexBuffer)))
		{
			VB_PositionNormal* pPosNormalVB = new VB_PositionNormal;
			if (pPosNormalVB->Initialize(pMem + PositionOffset, pMem + NormalOffset, NumPosition))
			{
				AddGeom(pPosNormalVB, Guard.m_pIndexBuffer, NumIndex, XMMatrixIdentity());
				Guard.m_pIndexBuffer = nullptr;
			}
			else
				delete pPosNormalVB;
		}

		free(pMem);
	}
}

bool LoadResources()
{
	return true;
}

void LoadGeoms()
{
	wchar_t szGeomPath[MAX_PATH];
	wcscpy_s(szGeomPath, GetExePath());
	wcscat_s(szGeomPath, L"Geoms\\");

	wchar_t szSearchPath[MAX_PATH];
	wcscpy_s(szSearchPath, szGeomPath);
	wcscat_s(szSearchPath, L"*.*");

	wchar_t szFileName[MAX_PATH];

	WIN32_FIND_DATA FindFileData;
	HANDLE hListFile;

	hListFile = FindFirstFile(szSearchPath, &FindFileData);
	if (hListFile == INVALID_HANDLE_VALUE)
		return;
	else {
		do
		{
			if (wcscmp(FindFileData.cFileName, L".") == 0 || wcscmp(FindFileData.cFileName, L"..") == 0)
				continue;

			if (FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;

			if (wcsstr(FindFileData.cFileName, L"geo") == nullptr)
				continue;

			wcscpy_s(szFileName, szGeomPath);
			wcscat_s(szFileName, FindFileData.cFileName);
			LoadGeom(szFileName);
		} while (FindNextFile(hListFile, &FindFileData));
	}

	// float PositionBuffer[] = {
	// 	-100.0f, -100.0f, 0.0f,
	// 	-100.0f, 100.0f, 0.0f,
	// 	100.0f, -100.0f, 0.0f,
	// 	100.0f, 100.0f, 0.0f
	// };
	// float NormalBuffer[] = {
	// 	0.0f, 0.0f, 1.0f,
	// 	0.0f, 0.0f, 1.0f,
	// 	0.0f, 0.0f, 1.0f,
	// 	0.0f, 0.0f, 1.0f
	// };
	// VB_PositionNormal* pPlaneVB = new VB_PositionNormal;
	// if (pPlaneVB->Initialize(PositionBuffer, NormalBuffer, 4))
	// 	g_arrOpaqueGeoms.emplace_back(pPlaneVB, nullptr, 0);
	// else
	// 	delete pPlaneVB;
}

constexpr float MAX_DEPTH = 1.0f;
constexpr float ALPHA = 0.6f;
float BLEND_FACTOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };

struct {
	XMFLOAT4 g_Dir;
	float g_Color[4];
} g_ShadingParamData;

void RenderOneFrame()
{
	XMMATRIX World(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);
	auto Dir = MakeD3DVECTOR(1.0f, 0.7f, 0.5f);
	Dir = XMVector3Normalize(Dir);
	XMStoreFloat4(&g_ShadingParamData.g_Dir, Dir);

	g_ShadingParamData.g_Color[0] = 1.0f;
	g_ShadingParamData.g_Color[1] = 0.5f;
	g_ShadingParamData.g_Color[2] = 0.6f;
	g_ShadingParamData.g_Color[3] = ALPHA; 

	float ClearColorFront[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(m_FrontBlenderRenderTarget.m_pRTV, ClearColorFront);
	float ClearColorBack[4] = { 1.0f, 1.0f, 1.0f, 0.0f };
	g_pImmediateContext->ClearRenderTargetView(m_BackBlenderRenderTarget.m_pRTV, ClearColorBack);
	float ClearColorMinZ[4] = { -MAX_DEPTH, -MAX_DEPTH, 0.0f, 0.0f };
	g_pImmediateContext->ClearRenderTargetView(m_MinMaxZRenderTargets[0].m_pRTV, ClearColorMinZ);

	// g_pImmediateContext->UpdateSubresource(g_pShadingParamsCB, 0, nullptr, &g_ShadingParamData, 0, 0);
	D3D11_MAPPED_SUBRESOURCE SubRc;
	ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
	g_pImmediateContext->Map(g_pShadingParamsCB, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_ShadingParamData, sizeof(g_ShadingParamData));
	g_pImmediateContext->Unmap(g_pShadingParamsCB, 0);

	g_pImmediateContext->RSSetState(g_pNoCullRS);
	g_pImmediateContext->OMSetDepthStencilState(g_pNoDepthNoStencilDS, 0);

	g_pImmediateContext->IASetInputLayout(g_pInputLayout);
	g_pImmediateContext->VSSetShader(g_pGeomShader, nullptr, 0);
	// g_pImmediateContext->VSSetConstantBuffers(0, 1, &g_pParamsCB);
	

	// Min-Max Z 
	g_pImmediateContext->OMSetRenderTargets(1, &m_MinMaxZRenderTargets[0].m_pRTV, nullptr);
	// 输出最大Z到G通道，R通道为-z最大的值
	g_pImmediateContext->OMSetBlendState(g_pMaxBS, BLEND_FACTOR, 0xffffffff);
	g_pImmediateContext->PSSetShader(g_pDDPFirstPassShader, nullptr, 0);
	// g_pImmediateContext->PSSetConstantBuffers(0, 1, &g_pParamsCB);
	g_pImmediateContext->PSSetConstantBuffers(3, 1, g_pShadingParamsCB);

	for (const auto& Geom : GetGeoms())
	{
		SyncGeomConstantBuffer(World);
		DrawOneGeom(Geom);
	}

	UINT currId = 0;
	for (UINT layer = 1; layer < 5; ++layer)
	{
		currId = layer % 2;
		UINT prevId = 1 - currId;

		g_pImmediateContext->ClearRenderTargetView(m_MinMaxZRenderTargets[currId].m_pRTV, ClearColorMinZ);

		ID3D11RenderTargetView* MRTs[3] = {
			m_MinMaxZRenderTargets[currId].m_pRTV,
			m_FrontBlenderRenderTarget.m_pRTV,
			m_BackBlenderRenderTarget.m_pRTV,
		};
		g_pImmediateContext->OMSetRenderTargets(3, MRTs, NULL);

		g_pImmediateContext->VSSetShader(g_pGeomShader, NULL, 0);
		g_pImmediateContext->PSSetShader(g_pDDPDepthPeelShader, NULL, 0);
		g_pImmediateContext->PSSetShaderResources(0, 1, &m_MinMaxZRenderTargets[prevId].m_pSRV);
		g_pImmediateContext->OMSetBlendState(g_pDDPBS, BLEND_FACTOR, 0xffffffff);

		for (const auto& Geom : GetGeoms())
		{
			SyncGeomConstantBuffer(World);
			DrawOneGeom(Geom);
		}
	}

	// 3. Final full-screen pass
	float ClearColorBackbuffer[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pOrigRTV, ClearColorBackbuffer);
	g_pImmediateContext->OMSetRenderTargets(1, g_pOrigRTV, NULL);
	g_pImmediateContext->OMSetBlendState(g_pNoBlendBS, BLEND_FACTOR, 0xffffffff);

	g_pImmediateContext->VSSetShader(g_pFullScreenRectShader, NULL, 0);
	g_pImmediateContext->PSSetShader(g_pDDPFinalShader, NULL, 0);

	ID3D11ShaderResourceView* pSRVs[3] =
	{
		m_MinMaxZRenderTargets[currId].m_pSRV,
		m_FrontBlenderRenderTarget.m_pSRV,
		m_BackBlenderRenderTarget.m_pSRV
	};
	g_pImmediateContext->PSSetShaderResources(0, 3, pSRVs);
	g_pImmediateContext->Draw(3, 0);
}

#endif