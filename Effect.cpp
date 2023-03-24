#include "Effect.h"



Global_D3D_Interface g_D3DInterface;


D3D11BufferWrapper g_pGeomBuffer;
D3D11BufferWrapper g_pGeomInvBuffer;
D3D11BufferWrapper g_pGeomITBuffer;

Global_Data& GetGlobalData()
{
	static Global_Data s_GlobalData;
	return s_GlobalData;
}

void RegisterEffects()
{
	REGISTER_EFFECT(SimpleHLSL, SimpleHLSL)
	REGISTER_EFFECT(EnvMap, EnvMap)
}

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

	hr = g_D3DInterface.m_pDevice->CreateRenderTargetView(pBackBuffer, NULL, g_D3DInterface.m_pMainBackbuffer);
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

	D3D11_BUFFER_DESC BufferDesc;
	BufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	BufferDesc.ByteWidth = sizeof(GeomBuffer);
	BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufferDesc.MiscFlags = 0;
	BufferDesc.StructureByteStride = 0;
	BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	if (FAILED(hr = g_D3DInterface.m_pDevice->CreateBuffer(&BufferDesc, nullptr, g_pGeomBuffer)))
		return false;
	BufferDesc.ByteWidth = sizeof(GeomInvBuffer);
	if (FAILED(hr = g_D3DInterface.m_pDevice->CreateBuffer(&BufferDesc, nullptr, g_pGeomInvBuffer)))
		return false;
	BufferDesc.ByteWidth = sizeof(GeomITBuffer);
	if (FAILED(hr = g_D3DInterface.m_pDevice->CreateBuffer(&BufferDesc, nullptr, g_pGeomITBuffer)))
		return false;

	return true;
}

std::weak_ptr<Effect_Instance> g_pCurrentEffectInst;

bool CreateShaders()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->CreateShadersCallback ? (*(EffectInst->CreateShadersCallback))() : true;
	else
		return false;
}

bool CreateBlendStates()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->CreateBlendStatesCallback ? (*(EffectInst->CreateBlendStatesCallback))() : true;
	else
		return false;
}

bool CreateRasterizerStates()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->CreateRasterizerStatesCallback ? (*(EffectInst->CreateRasterizerStatesCallback))() : true;
	else
		return false;
}

bool CreateDepthStencilStates()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->CreateDepthStencilStatesCallback ? (*(EffectInst->CreateDepthStencilStatesCallback))() : true;
	else
		return false;
}

bool CreateSamplerStates()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->CreateSamplerStatesCallback ? (*(EffectInst->CreateSamplerStatesCallback))() : true;
	else
		return false;
}

bool CreateRenderTargets()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->CreateRenderTargetsCallback ? (*(EffectInst->CreateRenderTargetsCallback))() : true;
	else
		return false;
}

bool CreateConstantBuffers()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->CreateConstantBuffersCallback ? (*(EffectInst->CreateConstantBuffersCallback))() : true;
	else
		return false;
}

bool LoadResources()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst)
		return EffectInst->LoadResourcesCallback ? (*(EffectInst->LoadResourcesCallback))() : true;
	else
		return false;
}

void RenderOneFrame()
{
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->RenderOneFrameCallback)
		(*(EffectInst->RenderOneFrameCallback))();
}

struct InputData
{
	byte m_KeyFlag = 0;
	byte m_MouseFlag = 0;
	WORD m_LastMouseX = 0;
	WORD m_LastMouseY = 0;

	void Reset() {
		m_KeyFlag = m_MouseFlag = 0;
		m_LastMouseX = m_LastMouseY = 0;
	}
} g_InputData;

struct alignas(16) CameraData
{
	XMVECTOR m_UpVector;
	XMVECTOR m_CameraX;
	XMVECTOR m_CameraY;
	XMVECTOR m_CameraZ;
	XMVECTOR m_CameraLocation;

	CameraData() {
		Reset();
	}
	void Reset() {
		// 左手坐标系，Z为Up方向
		m_UpVector = MakeD3DVECTOR(0.0f, 0.0f, 1.0f);
		m_CameraX = MakeD3DVECTOR(0.0f, 1.0f, 0.0f);
		m_CameraY = MakeD3DVECTOR(0.0f, 0.0f, 1.0f);
		m_CameraZ = MakeD3DVECTOR(1.0f, 0.0f, 0.0f);
		m_CameraLocation = MakeD3DPOINT(-50.0f, 0.0f, 0.0f);
	}
} g_CameraData;

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
	
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->KeyDownCallback)
		(*(EffectInst->KeyDownCallback))(KeyValue); 
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

	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->KeyUpCallback)
		(*(EffectInst->KeyUpCallback))(KeyValue);
}

void NotifyLButtonDown(WORD X, WORD Y)
{
	g_InputData.m_MouseFlag |= 0x01;
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->LButtonDownCallback)
		(*(EffectInst->LButtonDownCallback))(X, Y);
}

void NotifyLButtonUp(WORD X, WORD Y)
{
	g_InputData.m_MouseFlag &= ~(0x01);
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->LButtonUpCallback)
		(*(EffectInst->LButtonUpCallback))(X, Y);
}

void NotifyMouseMove(WORD X, WORD Y)
{
	if ((g_InputData.m_MouseFlag & 0x02) != 0)
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

	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->MouseMoveCallback)
		(*(EffectInst->MouseMoveCallback))(X, Y);
}

void NotifyRButtonDown(WORD X, WORD Y)
{
	g_InputData.m_MouseFlag |= 0x02;
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->RButtonDownCallback)
		(*(EffectInst->RButtonDownCallback))(X, Y);
}

void NotifyRButtonUp(WORD X, WORD Y)
{
	g_InputData.m_MouseFlag &= ~(0x02);
	auto EffectInst = g_pCurrentEffectInst.lock();
	if (EffectInst && EffectInst->RButtonUpCallback)
		(*(EffectInst->RButtonUpCallback))(X, Y);
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

void GlobalEffectReset()
{
	g_D3DInterface.Reset();
	g_pGeomBuffer.Reset();
	g_pGeomInvBuffer.Reset();
	g_pGeomITBuffer.Reset();
	g_pCurrentEffectInst.reset();
	g_InputData.Reset();
	g_CameraData.Reset();
	ClearGeoms();
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

void ClearGeoms()
{
	g_arrGeom.clear();
}

void DrawOneGeom(const Geom& InGeom)
{
	g_D3DInterface.m_pDeviceContext->IASetVertexBuffers(0, 
		InGeom.m_pVertexBuffer->GetNumVertexBuffers(), 
		InGeom.m_pVertexBuffer->GetVertexBuffers(),
		InGeom.m_pVertexBuffer->GetStrides(), 
		InGeom.m_pVertexBuffer->GetOffsets());
	if (InGeom.m_NumIndex > 0)
	{
		g_D3DInterface.m_pDeviceContext->IASetIndexBuffer(InGeom.m_pIndexBuffer, DXGI_FORMAT_R32_UINT, 0);
		g_D3DInterface.m_pDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		g_D3DInterface.m_pDeviceContext->DrawIndexed(InGeom.m_NumIndex, 0, 0);
	}
	else
	{
		g_D3DInterface.m_pDeviceContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_D3DInterface.m_pDeviceContext->Draw(InGeom.m_pVertexBuffer->GetNumVerts(), 0);
	}
}

XMMATRIX g_CameraMatrix;
XMMATRIX g_ProjectionMatrix;
XMMATRIX g_CameraProjectionMatrix;
XMMATRIX g_InvCameraMatrix;
XMMATRIX g_InvProjectionMatrix;
XMMATRIX g_InvCameraProjectionMatrix;

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

	XMMATRIX CameraMatrixTransposed(
		CalculateRow(g_CameraData.m_CameraX, g_CameraData.m_CameraLocation),
		CalculateRow(g_CameraData.m_CameraY, g_CameraData.m_CameraLocation),
		CalculateRow(g_CameraData.m_CameraZ, g_CameraData.m_CameraLocation),
		Row4
	);
	return XMMatrixTranspose(CameraMatrixTransposed);
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

XMMATRIX GetPerspectiveProjectionMatrix()
{
	constexpr float Fov = 45.0f * PI / 180.0f;
	constexpr float Near = 0.1f;
	constexpr float Far = 10000.0f;
	RECT rect;
	GetClientRect(GetHWnd(), &rect);
	float AspectRatio = (float)(rect.right - rect.left) / (float)(rect.bottom - rect.top);
	return XMMatrixPerspectiveFovLH(Fov, AspectRatio, Near, Far);
}

XMMATRIX GetInversePerspectiveProjectionMatrix()
{
	XMVECTOR Deter;
	return XMMatrixInverse(&Deter, GetPerspectiveProjectionMatrix());
}

GeomBuffer g_GeomBuffer;
GeomInvBuffer g_GeomInvBuffer;
GeomITBuffer g_GeomITBuffer;

void SyncGeomConstantBuffer(XMMATRIX World)
{
	D3D11_MAPPED_SUBRESOURCE SubRc;
	ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));

	g_CameraMatrix = GetCameraMatrix();
	g_ProjectionMatrix = GetPerspectiveProjectionMatrix();
	g_CameraProjectionMatrix = XMMatrixMultiply(g_CameraMatrix, g_ProjectionMatrix);

	XMVECTOR Deter;
	g_InvCameraMatrix = XMMatrixInverse(&Deter, g_CameraMatrix);
	g_InvProjectionMatrix = XMMatrixInverse(&Deter, g_ProjectionMatrix);
	g_InvCameraProjectionMatrix = XMMatrixInverse(&Deter, g_CameraProjectionMatrix);

	XMStoreFloat4x4A(&g_GeomBuffer.World, World);
	XMStoreFloat4x4A(&g_GeomBuffer.WorldView, XMMatrixMultiply(World, g_CameraMatrix));
	XMStoreFloat4x4A(&g_GeomBuffer.WorldViewProj, XMMatrixMultiply(World, g_CameraProjectionMatrix));
	g_D3DInterface.m_pDeviceContext->Map(g_pGeomBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomBuffer, sizeof(GeomBuffer));
	g_D3DInterface.m_pDeviceContext->Unmap(g_pGeomBuffer, 0);

	
	XMStoreFloat4x4A(&g_GeomInvBuffer.InvProj, g_InvProjectionMatrix);
	XMStoreFloat4x4A(&g_GeomInvBuffer.InvViewProj, g_InvCameraProjectionMatrix);
	g_D3DInterface.m_pDeviceContext->Map(g_pGeomInvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomInvBuffer, sizeof(GeomInvBuffer));
	g_D3DInterface.m_pDeviceContext->Unmap(g_pGeomInvBuffer, 0);

	XMStoreFloat4x4A(&g_GeomITBuffer.WorldIT, XMMatrixTranspose(XMMatrixInverse(&Deter, World)));
	XMStoreFloat4x4A(&g_GeomITBuffer.WorldViewIT, XMMatrixTranspose(XMMatrixInverse(&Deter, XMMatrixMultiply(World, g_CameraMatrix))));
	g_D3DInterface.m_pDeviceContext->Map(g_pGeomITBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomITBuffer, sizeof(GeomITBuffer));
	g_D3DInterface.m_pDeviceContext->Unmap(g_pGeomITBuffer, 0);

	g_D3DInterface.m_pDeviceContext->VSSetConstantBuffers(0, 1, g_pGeomBuffer);
	g_D3DInterface.m_pDeviceContext->VSSetConstantBuffers(1, 1, g_pGeomInvBuffer);
	g_D3DInterface.m_pDeviceContext->VSSetConstantBuffers(2, 1, g_pGeomITBuffer);
}

XMFLOAT4A SRGBToRGB(FXMVECTOR srgb)
{
	XMVECTOR rgb = XMColorSRGBToRGB(srgb);
	XMFLOAT4A ret;
	XMStoreFloat4A(&ret, rgb);
	return ret;
}

