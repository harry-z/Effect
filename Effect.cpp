#include "Effect.h"

TaskQueue g_TaskQueue;
bool PopTaskAndRun()
{
	if (HasMoreTasks())
	{
		g_TaskQueue.front()->DoTask();
		g_TaskQueue.pop_front();
		return true;
	}
	else
		return false;
}

bool HasMoreTasks()
{
	return g_TaskQueue.size() > 0;
}

LARGE_INTEGER Timer::s_Frequency;

void Timer::Start()
{
	m_ElapsedTime = 0;
	m_LastTime = 0;
}

void Timer::Tick()
{
	if (m_LastTime == -1)
		return;

	LARGE_INTEGER CurrentTime;
	::QueryPerformanceCounter(&CurrentTime);
	if (m_LastTime == 0)
		m_LastTime = CurrentTime.QuadPart;
	else
	{
		LONGLONG DeltaTime = CurrentTime.QuadPart - m_LastTime;
		m_ElapsedTime += DeltaTime;
		float ElapsedTimeInMicroSeconds = m_ElapsedTime * 1000000.0f / s_Frequency.QuadPart;
		if (ElapsedTimeInMicroSeconds >= m_TimeSpan * 1000000.0f)
		{
			if (m_Func)
				m_Func();
			if (m_TriggerType == ETriggerType::Once)
				m_LastTime = -1;
			else
				Start();
		}
	}
}

void Timer::GlobalInitialize()
{
	::QueryPerformanceFrequency(&s_Frequency);
}

TCHAR g_szModulePath[MAX_PATH];
void CacheModulePath()
{
	GetModuleFileName(GetModuleHandle(NULL), g_szModulePath, MAX_PATH);
	TCHAR* pSlash = _tcsrchr(g_szModulePath, _T('\\'));
	if (pSlash)
		*(pSlash + 1) = 0;
}

LPCTSTR GetExePath()
{
	return g_szModulePath;
}

Global_D3D_Interface g_D3DInterface;
Global_Data g_GlobalData;

// D3D11SwapChainWrapper g_pSwapChain;
// D3D11DeviceWrapper g_pd3dDevice;
// D3D11DeviceContextWrapper g_pImmediateContext;
// D3D11RTVWrapper g_pOrigRTV;
// D3D11Texture2DWrapper g_pOrigDSTexture;
// D3D11DSVWrapper g_pOrigDSV;

// D3D11BSWrapper g_pOverwriteBS;
// D3D11RSWrapper g_pBackCullRS;
// D3D11DSSWrapper g_pLessEqualDS;
// D3D11SSWrapper g_pLinearSamplerState;

// D3D11BufferWrapper g_pGeomBuffer;
// D3D11BufferWrapper g_pGeomInvBuffer;
// D3D11BufferWrapper g_pGeomITBuffer;

// XMMATRIX g_CameraMatrix;
// XMMATRIX g_ProjectionMatrix;
// XMMATRIX g_CameraProjectionMatrix;
// XMMATRIX g_InvCameraMatrix;
// XMMATRIX g_InvProjectionMatrix;
// XMMATRIX g_InvCameraProjectionMatrix;

// GeomBuffer g_GeomBuffer;
// GeomInvBuffer g_GeomInvBuffer;
// GeomITBuffer g_GeomITBuffer;

bool CreateDeviceAndImmediateContext(HWND hWnd)
{
	RECT rect;
	GetClientRect(hWnd, &rect);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = rect.right - rect.left;
	sd.BufferDesc.Height = rect.bottom - rect.top;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	// sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	D3D_FEATURE_LEVEL  FeatureLevelsRequested = D3D_FEATURE_LEVEL_11_0;
	UINT               numLevelsRequested = 1;
	D3D_FEATURE_LEVEL  FeatureLevelsSupported;

	UINT CreationFlags = D3D11_CREATE_DEVICE_DISABLE_GPU_TIMEOUT;
#ifdef _DEBUG
	CreationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	HRESULT hr;
	if (FAILED(hr = D3D11CreateDeviceAndSwapChain(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		CreationFlags,
		&FeatureLevelsRequested,
		numLevelsRequested,
		D3D11_SDK_VERSION,
		&sd,
		g_D3DInterface.m_pSwapChain,
		g_D3DInterface.m_pDevice,
		&FeatureLevelsSupported,
		g_D3DInterface.m_pDeviceContext)))
	{
		return false;
	}

	// Create a render target view
	ID3D11Texture2D* pBackBuffer = NULL;
	hr = g_D3DInterface.m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
	if (FAILED(hr))
		return false;

	hr = g_D3DInterface.m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, g_pOrigRTV);
	pBackBuffer->Release();
	if (FAILED(hr))
		return false;

	// Create depth stencil texture
	D3D11_TEXTURE2D_DESC descDepth;
	ZeroMemory(&descDepth, sizeof(descDepth));
	descDepth.Width = sd.BufferDesc.Width;
	descDepth.Height = sd.BufferDesc.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	hr = g_D3DInterface.m_pDevice->CreateTexture2D(&descDepth, NULL, g_D3DInterface.m_pMainDepthTexture);
	if (FAILED(hr))
		return false;

	// Create the depth stencil view
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
	ZeroMemory(&descDSV, sizeof(descDSV));
	descDSV.Format = descDepth.Format;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0;
	hr = g_D3DInterface.m_pDevice->CreateDepthStencilView(g_D3DInterface.m_pMainDepthTexture, &descDSV, g_D3DInterface.m_pMainDepthbuffer);
	if (FAILED(hr))
		return false;
	
	D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)sd.BufferDesc.Width;
    vp.Height = (FLOAT)sd.BufferDesc.Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_D3DInterface.m_pDeviceContext->RSSetViewports( 1, &vp );

	// D3D11_BUFFER_DESC BufferDesc;
	// BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	// BufferDesc.ByteWidth = sizeof(GeomBuffer);
	// BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	// BufferDesc.MiscFlags = 0;
	// BufferDesc.StructureByteStride = 0;
	// BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	// if (FAILED(hr = g_pd3dDevice->CreateBuffer(&BufferDesc, nullptr, g_pGeomBuffer)))
	// 	return false;
	// BufferDesc.ByteWidth = sizeof(GeomInvBuffer);
	// if (FAILED(hr = g_pd3dDevice->CreateBuffer(&BufferDesc, nullptr, g_pGeomInvBuffer)))
	// 	return false;
	// BufferDesc.ByteWidth = sizeof(GeomITBuffer);
	// if (FAILED(hr = g_pd3dDevice->CreateBuffer(&BufferDesc, nullptr, g_pGeomITBuffer)))
	// 	return false;

	// D3D11_BLEND_DESC blendState;
	// blendState.AlphaToCoverageEnable = FALSE;
	// blendState.IndependentBlendEnable = FALSE;
	// for (int i = 0; i < 8; ++i)
	// {
	// 	blendState.RenderTarget[i].BlendEnable = TRUE;
	// 	blendState.RenderTarget[i].SrcBlend = D3D11_BLEND_ONE;
	// 	blendState.RenderTarget[i].DestBlend = D3D11_BLEND_ZERO;
	// 	blendState.RenderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
	// 	blendState.RenderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
	// 	blendState.RenderTarget[i].DestBlendAlpha = D3D11_BLEND_ZERO;
	// 	blendState.RenderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	// 	blendState.RenderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	// }
	// if (FAILED(g_pd3dDevice->CreateBlendState(&blendState, g_pOverwriteBS)))
	// 	return false;

	// D3D11_RASTERIZER_DESC rasterizerState;
	// rasterizerState.FillMode = D3D11_FILL_SOLID;
	// rasterizerState.CullMode = D3D11_CULL_BACK;
	// rasterizerState.FrontCounterClockwise = TRUE;
	// rasterizerState.DepthBias = FALSE;
	// rasterizerState.DepthBiasClamp = 0;
	// rasterizerState.SlopeScaledDepthBias = 0;
	// rasterizerState.DepthClipEnable = FALSE;
	// rasterizerState.ScissorEnable = FALSE;
	// rasterizerState.MultisampleEnable = FALSE;
	// rasterizerState.AntialiasedLineEnable = FALSE;
	// if (FAILED(g_pd3dDevice->CreateRasterizerState(&rasterizerState, g_pBackCullRS)))
	// 	return false;

	// D3D11_DEPTH_STENCIL_DESC depthstencilState;
	// depthstencilState.DepthEnable = TRUE;
	// depthstencilState.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	// depthstencilState.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	// depthstencilState.StencilEnable = FALSE;
	// if (FAILED(g_pd3dDevice->CreateDepthStencilState(&depthstencilState, g_pLessEqualDS)))
	// 	return false;

	// D3D11_SAMPLER_DESC Desc;
	// ZeroMemory(&Desc, sizeof(D3D11_SAMPLER_DESC));
	// Desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	// Desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	// Desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	// Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	// Desc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
	// Desc.MaxAnisotropy = 0;
	// Desc.MinLOD = 0;
	// Desc.MaxLOD = D3D11_FLOAT32_MAX;
	// if (FAILED(g_pd3dDevice->CreateSamplerState(&Desc, g_pLinearSamplerState)))
	// 	return false;

	return true;
}

bool CreateShaders()
{
	
}

std::vector<Geom> g_arrGeom;

void AddGeom(VB_Base* pVB, ID3D11Buffer* pIndexBuffer, UINT NumIndex, XMMATRIX WorldMatrix)
{
	g_arrGeom.emplace_back(pVB, pIndexBuffer, NumIndex, WorldMatrix);
}

const std::vector<Geom>& GetGeoms()
{
	return g_arrGeom;
}

void DrawOneGeom(const Geom& InGeom)
{
	g_pImmediateContext->IASetVertexBuffers(0, 
		InGeom.m_pVertexBuffer->GetNumVertexBuffers(), 
		InGeom.m_pVertexBuffer->GetVertexBuffers(),
		InGeom.m_pVertexBuffer->GetStrides(), 
		InGeom.m_pVertexBuffer->GetOffsets());
	if (InGeom.m_NumIndex > 0)
	{
		g_pImmediateContext->IASetIndexBuffer(InGeom.m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g_pImmediateContext->DrawIndexed(InGeom.m_NumIndex, 0, 0);
	}
	else
	{
		g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_pImmediateContext->Draw(InGeom.m_pVertexBuffer->GetNumVerts(), 0);
	}
}

struct alignas(16) CameraData
{
	XMVECTOR m_UpVector;
	XMVECTOR m_CameraX;
	XMVECTOR m_CameraY;
	XMVECTOR m_CameraZ;
	XMVECTOR m_CameraLocation;

	CameraData() {
		// 左手坐标系，Z为Up方向
		m_UpVector = MakeD3DVECTOR(0.0f, 0.0f, 1.0f);
		m_CameraX = MakeD3DVECTOR(0.0f, 1.0f, 0.0f);
		m_CameraY = MakeD3DVECTOR(0.0f, 0.0f, 1.0f);
		m_CameraZ = MakeD3DVECTOR(1.0f, 0.0f, 0.0f);
		m_CameraLocation = MakeD3DPOINT(-50.0f, 0.0f, 0.0f);
	}
} g_CameraData;

struct InputData
{
	byte m_KeyFlag = 0;
	byte m_MouseFlag = 0;
	WORD m_LastMouseX = 0;
	WORD m_LastMouseY = 0;
} g_InputData;

void NotifyKeyDown(UINT_PTR KeyValue)
{
	switch (KeyValue)
	{
	case 0x57: // W
		g_InputData.m_KeyFlag |= 0x01;
		break;
	case 0x53: // S
		g_InputData.m_KeyFlag |= 0x02;
		break;
	case 0x41: // A
		g_InputData.m_KeyFlag |= 0x04;
		break;
	case 0x44: // D
		g_InputData.m_KeyFlag |= 0x08;
		break;
	case 0x51: // Q
		g_InputData.m_KeyFlag |= 0x10;
		break;
	case 0x45:
		g_InputData.m_KeyFlag |= 0x20;
		break;
	}
}

void NotifyKeyUp(UINT_PTR KeyValue)
{
	switch (KeyValue)
	{
	case 0x57: // W
		g_InputData.m_KeyFlag &= ~(0x01);
		break;
	case 0x53: // S
		g_InputData.m_KeyFlag &= ~(0x02);
		break;
	case 0x41: // A
		g_InputData.m_KeyFlag &= ~(0x04);
		break;
	case 0x44: // D
		g_InputData.m_KeyFlag &= ~(0x08);
		break;
	case 0x51: // Q
		g_InputData.m_KeyFlag &= ~(0x10);
		break;
	case 0x45: // E
		g_InputData.m_KeyFlag &= ~(0x20);
		break;
	}
}

void NotifyLButtonDown(WORD X, WORD Y)
{
	g_InputData.m_MouseFlag |= 0x01;
}

void NotifyLButtonUp(WORD X, WORD Y)
{
	g_InputData.m_MouseFlag &= ~(0x01);
}

void NotifyMouseMove(WORD X, WORD Y)
{
	if ((g_InputData.m_MouseFlag & 0x01) != 0)
	{
		short DeltaX = X - g_InputData.m_LastMouseX;
		short DeltaY = Y - g_InputData.m_LastMouseY;

		auto qHorizontal = XMQuaternionRotationAxis(g_CameraData.m_UpVector, DeltaX * 0.1f * PI / 180.0f);
		g_CameraData.m_CameraZ = XMVector3Rotate(g_CameraData.m_CameraZ, qHorizontal);
		g_CameraData.m_CameraX = XMVector3Cross(g_CameraData.m_UpVector, g_CameraData.m_CameraZ);
		g_CameraData.m_CameraX = XMVector3Normalize(g_CameraData.m_CameraX);

		auto qVertical = XMQuaternionRotationAxis(g_CameraData.m_CameraX, DeltaY * 0.1f * PI / 180.0f);
		g_CameraData.m_CameraZ = XMVector3Rotate(g_CameraData.m_CameraZ, qVertical);
		g_CameraData.m_CameraY = XMVector3Cross(g_CameraData.m_CameraZ, g_CameraData.m_CameraX);
		g_CameraData.m_CameraY = XMVector3Normalize(g_CameraData.m_CameraY);
	}
	g_InputData.m_LastMouseX = X;
	g_InputData.m_LastMouseY = Y;
}

void NotifyRButtonUp(WORD X, WORD Y)
{
	
}

void TickInput()
{
	XMVECTOR Scalar = XMVectorSet(0.2f, 0.2f, 0.2f, 0.0f);
	XMVECTOR MinusScalar = XMVectorSet(-0.2f, -0.2f, -0.2f, 0.0f);
	if ((g_InputData.m_KeyFlag & 0x01) != 0)
	{
		g_CameraData.m_CameraLocation = XMVectorMultiplyAdd(g_CameraData.m_CameraZ, Scalar, g_CameraData.m_CameraLocation);
	}
	else if ((g_InputData.m_KeyFlag & 0x02) != 0)
	{
		g_CameraData.m_CameraLocation = XMVectorMultiplyAdd(g_CameraData.m_CameraZ, MinusScalar, g_CameraData.m_CameraLocation);
	}
	else if ((g_InputData.m_KeyFlag & 0x04) != 0)
	{
		g_CameraData.m_CameraLocation = XMVectorMultiplyAdd(g_CameraData.m_CameraX, MinusScalar, g_CameraData.m_CameraLocation);
	}
	else if ((g_InputData.m_KeyFlag & 0x08) != 0)
	{
		g_CameraData.m_CameraLocation = XMVectorMultiplyAdd(g_CameraData.m_CameraX, Scalar, g_CameraData.m_CameraLocation);
	}
	else if ((g_InputData.m_KeyFlag & 0x10) != 0)
	{
		g_CameraData.m_CameraLocation = XMVectorMultiplyAdd(g_CameraData.m_UpVector, Scalar, g_CameraData.m_CameraLocation);
	}
	else if ((g_InputData.m_KeyFlag & 0x20) != 0)
	{
		g_CameraData.m_CameraLocation = XMVectorMultiplyAdd(g_CameraData.m_UpVector, MinusScalar, g_CameraData.m_CameraLocation);
	}
}

XMVECTOR GetCameraViewDirection()
{
	return g_CameraData.m_CameraZ;
}

XMMATRIX GetCameraMatrix()
{
	XMVECTOR Minus = XMVectorSet(-1.0f, -1.0f, -1.0f, -1.0f);
	XMVECTOR Row4 = MakeD3DPOINT(0.0f, 0.0f, 0.0f);

	auto CalculateRow = [&Minus](XMVECTOR Axis, XMVECTOR Location) -> XMVECTOR {
		XMVECTOR SwizzleAxis = XMVectorSwizzle(Axis, 3, 0, 1, 2);
		XMVECTOR DotAxis = XMVector3Dot(Axis, Location); 
		DotAxis = XMVectorMultiply(DotAxis, Minus); 
		return XMVectorShiftLeft(SwizzleAxis, DotAxis, 1); 
	};

	XMMATRIX CameraMatrixInCameraSpace(
		CalculateRow(g_CameraData.m_CameraX, g_CameraData.m_CameraLocation),
		CalculateRow(g_CameraData.m_CameraY, g_CameraData.m_CameraLocation),
		CalculateRow(g_CameraData.m_CameraZ, g_CameraData.m_CameraLocation),
		Row4
	);
	return XMMatrixTranspose(CameraMatrixInCameraSpace);
}

XMMATRIX GetCameraMatrixWithoutTranslation()
{
	XMVECTOR Row4 = MakeD3DPOINT(0.0f, 0.0f, 0.0f);
	XMMATRIX CameraMatrix(
		g_CameraData.m_CameraX,
		g_CameraData.m_CameraY,
		g_CameraData.m_CameraZ,
		Row4
	);
	return XMMatrixTranspose(CameraMatrix);
}

XMMATRIX GetInverseCameraMatrix()
{
	XMVECTOR Deter;
	return XMMatrixInverse(&Deter, GetCameraMatrix());
}

XMMATRIX GetProjectionMatrix()
{
	constexpr float Fov = 45.0f * PI / 180.0f;
	constexpr float Near = 0.1f;
	constexpr float Far = 10000.0f;
	RECT rect;
	GetClientRect(GetHWnd(), &rect);
	float AspectRatio = (float)(rect.right - rect.left) / (float)(rect.bottom - rect.top);
	float YScale = 1.0f / tanf(Fov * 0.5f);
	float XScale = YScale / AspectRatio;
	return XMMATRIX(
		XScale, 0.0f, 0.0f, 0.0f,
		0.0f, YScale, 0.0f, 0.0f,
		0.0f, 0.0f, Far / (Far - Near), 1.0f,
		0.0f, 0.0f, -Near * Far / (Far - Near), 0.0f);
}

XMMATRIX GetInverseProjectionMatrix()
{
	XMVECTOR Deter;
	return XMMatrixInverse(&Deter, GetProjectionMatrix());
}

void SyncGeomConstantBuffer(XMMATRIX World)
{
	D3D11_MAPPED_SUBRESOURCE SubRc;
	ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));

	XMStoreFloat4x4A(&g_GeomBuffer.World, World);
	XMStoreFloat4x4A(&g_GeomBuffer.WorldView, XMMatrixMultiply(World, g_CameraMatrix));
	XMStoreFloat4x4A(&g_GeomBuffer.WorldViewProj, XMMatrixMultiply(World, g_CameraProjectionMatrix));
	g_pImmediateContext->Map(g_pGeomBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomBuffer, sizeof(GeomBuffer));
	g_pImmediateContext->Unmap(g_pGeomBuffer, 0);

	XMVECTOR Deter;
	XMStoreFloat4x4A(&g_GeomITBuffer.WorldIT, XMMatrixTranspose(XMMatrixInverse(&Deter, World)));
	XMStoreFloat4x4A(&g_GeomITBuffer.WorldViewIT, XMMatrixTranspose(XMMatrixInverse(&Deter, XMMatrixMultiply(World, g_CameraMatrix))));
	g_pImmediateContext->Map(g_pGeomITBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomITBuffer, sizeof(GeomITBuffer));
	g_pImmediateContext->Unmap(g_pGeomITBuffer, 0);

	g_pImmediateContext->VSSetConstantBuffers(0, 1, g_pGeomBuffer);
	g_pImmediateContext->VSSetConstantBuffers(2, 1, g_pGeomITBuffer);
}

XMFLOAT4A SRGBToRGB(FXMVECTOR srgb)
{
	XMVECTOR rgb = XMColorSRGBToRGB(srgb);
	XMFLOAT4A ret;
	XMStoreFloat4A(&ret, rgb);
	return ret;
}

HRESULT CShaderHeaderInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
	FILE* pFile = nullptr;
	char szFilePath[MAX_PATH];
	strcpy(szFilePath, g_szcModulePath);
	strcat(szFilePath, pFileName);
	if (fopen_s(&pFile, szFilePath, "rb") == 0)
	{
		fseek(pFile, 0, SEEK_END);
		long n = ftell(pFile);
		fseek(pFile, 0, SEEK_SET);
		char* p = (char*)malloc(n + 1);
		fread(p, sizeof(char), n, pFile);
		fclose(pFile);
		p[n] = 0;
		*ppData = p;
		*pBytes = n;
		return S_OK;
	}
	else
		return E_FAIL;
}

HRESULT CShaderHeaderInclude::Close(LPCVOID pData)
{
	// free(const_cast<LPVOID>(pData));
	return S_OK;
}