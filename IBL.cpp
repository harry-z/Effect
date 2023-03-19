#include "Effect.h"

#ifdef IBL_EFFECT


UINT g_MipCount = 0;

D3D11InputLayoutWrapper g_pInputLayout_Position;
D3D11InputLayoutWrapper g_pInputLayout_PN;

D3D11VertexShaderWrapper g_pFullScreenQuadShader;
D3D11VertexShaderWrapper g_pGeometryShader;
D3D11VertexShaderWrapper g_pEnvVertexShader;
D3D11PixelShaderWrapper g_pEnvPixelShader;

// 生成DiffuseIBL的环境贴图的Shader
D3D11PixelShaderWrapper g_pIrradianceMapShader;

// 生成SpecularIBL的PreFilter环境贴图的Shader
D3D11PixelShaderWrapper g_pCopyResourceShader;
D3D11PixelShaderWrapper g_pPreFilterEnvMapShader;

// 生成LUT
D3D11PixelShaderWrapper g_pGenLUTShader;

D3D11PixelShaderWrapper g_pIBLShader;

D3D11BufferWrapper g_pFullScreenQuadBuffer;

D3D11DSSWrapper g_pNoDepthDS;
D3D11SSWrapper g_pLinearMipSamplerState;

struct PreprocessData
{
	float MipLevel[4];
	PreprocessData() {
		ZeroMemory(&MipLevel[0], GetMemorySize());
	}
	constexpr size_t GetMemorySize() { return sizeof(float) * 4; }
} g_PreprocessData;
D3D11BufferWrapper g_pPreprocessData;
D3D11BufferWrapper g_pShadingData;

bool CreateShaders()
{
	const D3D11_INPUT_ELEMENT_DESC PositionInputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements_Position = sizeof(PositionInputLayoutDesc) / sizeof(PositionInputLayoutDesc[0]);

	const D3D11_INPUT_ELEMENT_DESC PNInputLayoutDesc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};
	UINT NumElements_PN = sizeof(PNInputLayoutDesc) / sizeof(PNInputLayoutDesc[0]);

	wchar_t szShaderFileName[MAX_PATH];
	wcscpy_s(szShaderFileName, GetExePath());
	wcscat_s(szShaderFileName, L"IBL.hlsl");

	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "FullScreenQuad", EShaderModel::ESM_5, g_pFullScreenQuadShader, 
		PositionInputLayoutDesc, NumElements_Position, g_pInputLayout_Position))
		return false;
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "GeometryVS", EShaderModel::ESM_5, g_pGeometryShader, 
		PNInputLayoutDesc, NumElements_PN, g_pInputLayout_PN))
		return false;
	if (!CreateVertexShaderAndInputLayout(szShaderFileName, "EnvVS", EShaderModel::ESM_5, g_pEnvVertexShader, 
		nullptr, 0, nullptr))
		return false;
	if (!CreatePixelShader(szShaderFileName, "EnvMapPS", EShaderModel::ESM_5, g_pEnvPixelShader))
		return false;
	if (!CreatePixelShader(szShaderFileName, "GenIrradianceMap", EShaderModel::ESM_5, g_pIrradianceMapShader))
		return false;
	if (!CreatePixelShader(szShaderFileName, "CopyResource", EShaderModel::ESM_5, g_pCopyResourceShader))
		return false;
	if (!CreatePixelShader(szShaderFileName, "GenPreFilterEnvMap", EShaderModel::ESM_5, g_pPreFilterEnvMapShader))
		return false;
	if (!CreatePixelShader(szShaderFileName, "GenerateLUT", EShaderModel::ESM_5, g_pGenLUTShader))
		return false;
	if (!CreatePixelShader(szShaderFileName, "IBLShading", EShaderModel::ESM_5, g_pIBLShader))
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
	D3D11_DEPTH_STENCIL_DESC depthstencilState;
	depthstencilState.DepthEnable = FALSE;
	depthstencilState.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthstencilState.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depthstencilState.StencilEnable = FALSE;
	if (FAILED(g_pd3dDevice->CreateDepthStencilState(&depthstencilState, g_pNoDepthDS)))
		return false;
	return true;
}

bool CreateSamplerStates()
{
	D3D11_SAMPLER_DESC Desc;
	ZeroMemory(&Desc, sizeof(D3D11_SAMPLER_DESC));
	Desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	Desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	Desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	Desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	Desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	Desc.MaxAnisotropy = 0;
	Desc.MinLOD = 0;
	Desc.MaxLOD = D3D11_FLOAT32_MAX;
	if (FAILED(g_pd3dDevice->CreateSamplerState(&Desc, g_pLinearMipSamplerState)))
		return false;
	return true;
}

bool CreateRenderTargets()
{
	return true;
}

bool CreateConstantBuffers()
{
	D3D11_BUFFER_DESC Desc;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.ByteWidth = sizeof(float) * 8;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	Desc.MiscFlags = 0;
	Desc.StructureByteStride = 0;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	if (FAILED(g_pd3dDevice->CreateBuffer(&Desc, nullptr, g_pPreprocessData)))
		return false;
	Desc.ByteWidth = sizeof(float) * 12;
	if (FAILED(g_pd3dDevice->CreateBuffer(&Desc, nullptr, g_pShadingData)))
		return false;
	return true;
}

UINT g_Width, g_Height;

D3D11Texture2DWrapper g_pEnvMap;
D3D11SRVWrapper g_pEnvSRV;

D3D11Texture2DWrapper g_pIrradianceMap;
D3D11SRVWrapper g_pIrradianceSRV;
D3D11RTVWrapper g_pIrradianceRTV;
// 生成DiffuseIBL需要使用的SampleData
D3D11BufferWrapper g_pIrradianceSampleData;
D3D11SRVWrapper g_pIrradianceSampleDataSRV;

D3D11Texture2DWrapper g_pPreFilterMap;
D3D11SRVWrapper g_pPreFilterSRV;
std::vector<D3D11RTVWrapper> g_PreFilterRTVs;

D3D11Texture2DWrapper g_pLUTMap;
D3D11SRVWrapper g_pLUTSRV;
D3D11RTVWrapper g_pLUTRTV;


bool IsPowerOfTwo(UINT n)
{
	return (n > 0) && ((n & (n - 1)) == 0);
}

class GenIrradianceMapTask final : public ITask
{
private:
	ID3D11RenderTargetView* m_pRTV = nullptr;

public:
	GenIrradianceMapTask(ID3D11RenderTargetView* pRTV)
	: m_pRTV(pRTV)
	{}

	virtual void DoTask() override
	{
		D3D11_VIEWPORT Viewport;
		Viewport.TopLeftX = 0;
		Viewport.TopLeftY = 0;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.Width = g_Width;
		Viewport.Height = g_Height;
		g_pImmediateContext->RSSetViewports(1, &Viewport);
		FLOAT ClearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		g_pImmediateContext->ClearRenderTargetView(m_pRTV, ClearColor);
		g_pImmediateContext->OMSetRenderTargets(1, &m_pRTV, nullptr);
		
		g_pImmediateContext->VSSetShader(g_pFullScreenQuadShader, nullptr, 0);
		g_pImmediateContext->IASetInputLayout(g_pInputLayout_Position);
		g_pImmediateContext->PSSetShader(g_pIrradianceMapShader, nullptr, 0);
		g_pImmediateContext->PSSetSamplers(0, 1, g_pLinearSamplerState);
		g_pImmediateContext->PSSetShaderResources(0, 1, g_pEnvSRV);
		g_pImmediateContext->PSSetShaderResources(1, 1, g_pIrradianceSampleDataSRV);
		UINT Stride = sizeof(float) * 2, Offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, g_pFullScreenQuadBuffer, &Stride, &Offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_pImmediateContext->Draw(4, 0);
	}
};

class GenLUTTask final : public ITask
{
private:
	ID3D11RenderTargetView* m_pRTV;

public:
	GenLUTTask(ID3D11RenderTargetView* pRTV) : m_pRTV(pRTV) {}

	virtual void DoTask() override
	{
		D3D11_VIEWPORT Viewport;
		Viewport.TopLeftX = 0;
		Viewport.TopLeftY = 0;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.Width = 256;
		Viewport.Height = 256;
		g_pImmediateContext->RSSetViewports(1, &Viewport);
		FLOAT ClearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		g_pImmediateContext->ClearRenderTargetView(m_pRTV, ClearColor);
		g_pImmediateContext->OMSetRenderTargets(1, &m_pRTV, nullptr);

		g_pImmediateContext->VSSetShader(g_pFullScreenQuadShader, nullptr, 0);
		g_pImmediateContext->IASetInputLayout(g_pInputLayout_Position);
		g_pImmediateContext->PSSetShader(g_pGenLUTShader, nullptr, 0);
		UINT Stride = sizeof(float) * 2, Offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, g_pFullScreenQuadBuffer, &Stride, &Offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_pImmediateContext->Draw(4, 0);
	}
};

class PreFilterTask final : public ITask
{
private:
	float m_fRoughness;
	XMFLOAT2 m_Dimension;
	ID3D11PixelShader* m_pShader = nullptr;
	ID3D11RenderTargetView* m_pRTV = nullptr;

public:
	PreFilterTask(float fRoughness, const XMFLOAT2& Dimension, ID3D11PixelShader* pShader, ID3D11RenderTargetView* pRTV)
	: m_fRoughness(fRoughness)
	, m_Dimension(Dimension)
	, m_pShader(pShader)
	, m_pRTV(pRTV)
	{}

	virtual void DoTask() override
	{
		D3D11_VIEWPORT Viewport;
		Viewport.TopLeftX = 0;
		Viewport.TopLeftY = 0;
		Viewport.MinDepth = 0.0f;
		Viewport.MaxDepth = 1.0f;
		Viewport.Width = m_Dimension.x;
		Viewport.Height = m_Dimension.y;
		g_pImmediateContext->RSSetViewports(1, &Viewport);

		g_PreprocessData.MipLevel[0] = m_fRoughness;
		D3D11_MAPPED_SUBRESOURCE SubRc;
		SubRc.RowPitch = SubRc.DepthPitch = 0;
		g_pImmediateContext->Map(g_pPreprocessData, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
		memcpy(SubRc.pData, &g_PreprocessData.MipLevel[0], g_PreprocessData.GetMemorySize());
		g_pImmediateContext->Unmap(g_pPreprocessData, 0);

		FLOAT ClearColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
		g_pImmediateContext->ClearRenderTargetView(m_pRTV, ClearColor);
		g_pImmediateContext->OMSetRenderTargets(1, &m_pRTV, nullptr);

		g_pImmediateContext->VSSetShader(g_pFullScreenQuadShader, nullptr, 0);
		g_pImmediateContext->IASetInputLayout(g_pInputLayout_Position);
		g_pImmediateContext->PSSetShader(m_pShader, nullptr, 0);
		g_pImmediateContext->PSSetSamplers(0, 1, g_pLinearSamplerState);
		g_pImmediateContext->PSSetShaderResources(0, 1, g_pEnvSRV);
		g_pImmediateContext->PSSetConstantBuffers(3, 1, g_pPreprocessData);
		UINT Stride = sizeof(float) * 2, Offset = 0;
		g_pImmediateContext->IASetVertexBuffers(0, 1, g_pFullScreenQuadBuffer, &Stride, &Offset);
		g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		g_pImmediateContext->Draw(4, 0);
	}
};

bool g_bAllTasksProcessed = false;
Timer g_PreprocessTimer(6, Timer::ETriggerType::Repeat, []() {
	g_bAllTasksProcessed = !PopTaskAndRun();
});

bool LoadResources()
{
	wchar_t szTextureFileName[MAX_PATH];
	wcscpy_s(szTextureFileName, GetExePath());
	wcscat_s(szTextureFileName, L"Chelsea_Stairs_8k.jpg");
	if (LoadJpegTextureFromFile(szTextureFileName, true, g_pEnvMap, g_pEnvSRV))
	{
		// 创建PreFiltered EnvMap
		D3D11_TEXTURE2D_DESC Desc;
		g_pEnvMap->GetDesc(&Desc);
		if (!IsPowerOfTwo(Desc.Width) || !IsPowerOfTwo(Desc.Height))
			return false;
		// 计算mip层级
		UINT MinSize = Desc.Width > Desc.Height ? Desc.Height : Desc.Width;
		while (MinSize > 32)
		{
			++g_MipCount;
			MinSize = MinSize >> 1;
		}
		if (g_MipCount == 0)
			return false;
		
		g_Width = Desc.Width;
		g_Height = Desc.Height;

		D3D11_TEXTURE2D_DESC Tex2DDesc;
		Tex2DDesc.ArraySize = 1;
		Tex2DDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		Tex2DDesc.CPUAccessFlags = 0;
		Tex2DDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		Tex2DDesc.Height = Desc.Height;
		Tex2DDesc.MipLevels = 1;
		Tex2DDesc.MiscFlags = 0;
		Tex2DDesc.SampleDesc.Count = 1;
		Tex2DDesc.SampleDesc.Quality = 0;
		Tex2DDesc.Usage = D3D11_USAGE_DEFAULT;
		Tex2DDesc.Width = Desc.Width;
		if (FAILED(g_pd3dDevice->CreateTexture2D(&Tex2DDesc, nullptr, g_pIrradianceMap)))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
		SRVDesc.Format = Tex2DDesc.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = 1;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		if (FAILED(g_pd3dDevice->CreateShaderResourceView(g_pIrradianceMap, &SRVDesc, g_pIrradianceSRV)))
			return false;

		D3D11_RENDER_TARGET_VIEW_DESC RTVDesc;
		RTVDesc.Format = Tex2DDesc.Format;
		RTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = 0;
		if (FAILED(g_pd3dDevice->CreateRenderTargetView(g_pIrradianceMap, &RTVDesc, g_pIrradianceRTV)))
			return false;

		AddTask<GenIrradianceMapTask>(g_pIrradianceRTV);

		float DeltaAngle = 0.05f * PI;
		UINT PhiCount = 2 * PI / DeltaAngle;
		UINT ThetaCount = 0.5 * PI / DeltaAngle;
		float* SampleDataBuffer = new float[2 * (PhiCount + 1) * (ThetaCount + 1)];
		for (UINT i = 0; i <= PhiCount; ++i)
		{
			for (UINT j = 0; j <= ThetaCount; ++j)
			{
				SampleDataBuffer[(i * (ThetaCount + 1) + j) * 2] = i * DeltaAngle;
				SampleDataBuffer[(i * (ThetaCount + 1) + j) * 2 + 1] = j * DeltaAngle;
			}
		}
		D3D11_BUFFER_DESC BufferDesc;
		BufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		BufferDesc.ByteWidth = sizeof(float) * 2 * (PhiCount + 1) * (ThetaCount + 1);
		BufferDesc.CPUAccessFlags = 0;
		BufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_ALLOW_RAW_VIEWS;
		BufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		BufferDesc.StructureByteStride = 0;
		D3D11_SUBRESOURCE_DATA SubData;
		SubData.pSysMem = SampleDataBuffer;
		SubData.SysMemPitch = SubData.SysMemSlicePitch = 0;
		if (FAILED(g_pd3dDevice->CreateBuffer(&BufferDesc, &SubData, g_pIrradianceSampleData)))
			return false;
		delete[] SampleDataBuffer;

		SRVDesc.Format = DXGI_FORMAT_R32_TYPELESS;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		SRVDesc.BufferEx.FirstElement = 0;
		SRVDesc.BufferEx.NumElements = 2 * (PhiCount + 1) * (ThetaCount + 1);
		SRVDesc.BufferEx.Flags = D3D11_BUFFEREX_SRV_FLAG_RAW;
		if (FAILED(g_pd3dDevice->CreateShaderResourceView(g_pIrradianceSampleData, &SRVDesc, g_pIrradianceSampleDataSRV)))
			return false;

		// Specular PreFilter Map
		Tex2DDesc.MipLevels = g_MipCount;
		if (FAILED(g_pd3dDevice->CreateTexture2D(&Tex2DDesc, nullptr, g_pPreFilterMap)))
			return false;

		SRVDesc.Format = Tex2DDesc.Format;
		SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		SRVDesc.Texture2D.MipLevels = g_MipCount;
		SRVDesc.Texture2D.MostDetailedMip = 0;
		if (FAILED(g_pd3dDevice->CreateShaderResourceView(g_pPreFilterMap, &SRVDesc, g_pPreFilterSRV)))
			return false;

		g_PreFilterRTVs.reserve(g_MipCount);
		
		float RoughnessStep = 1.0f / (g_MipCount - 1);
		UINT TempWidth = g_Width;
		UINT TempHeight = g_Height;
		for (UINT i = 0; i < g_MipCount - 1; ++i)
		{
			RTVDesc.Texture2D.MipSlice = i;
			ID3D11RenderTargetView* pRTV;
			if (FAILED(g_pd3dDevice->CreateRenderTargetView(g_pPreFilterMap, &RTVDesc, &pRTV)))
				return false;
			AddTask<PreFilterTask>(RoughnessStep * i, XMFLOAT2(TempWidth, TempHeight), i == 0 ? g_pCopyResourceShader : g_pPreFilterEnvMapShader, pRTV);
			g_PreFilterRTVs.emplace_back(pRTV);

			TempWidth = TempWidth >> 1;
			TempHeight = TempHeight >> 1;
		}

		RTVDesc.Texture2D.MipSlice = g_MipCount - 1;
		ID3D11RenderTargetView* pRTV;
		if (FAILED(g_pd3dDevice->CreateRenderTargetView(g_pPreFilterMap, &RTVDesc, &pRTV)))
			return false;
		AddTask<PreFilterTask>(1.0f, XMFLOAT2(TempWidth, TempHeight), g_pPreFilterEnvMapShader, pRTV);
		g_PreFilterRTVs.emplace_back(pRTV);

		D3D11_TEXTURE2D_DESC LUTDesc;
		LUTDesc.ArraySize = 1;
		LUTDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
		LUTDesc.CPUAccessFlags = 0;
		LUTDesc.Format = DXGI_FORMAT_R16G16_FLOAT;
		LUTDesc.Height = 256;
		LUTDesc.MipLevels = 1;
		LUTDesc.MiscFlags = 0;
		LUTDesc.SampleDesc.Count = 1;
		LUTDesc.SampleDesc.Quality = 0;
		LUTDesc.Usage = D3D11_USAGE_DEFAULT;
		LUTDesc.Width = 256;
		if (FAILED(g_pd3dDevice->CreateTexture2D(&LUTDesc, nullptr, g_pLUTMap)))
			return false;

		D3D11_SHADER_RESOURCE_VIEW_DESC LUTSRVDesc;
		LUTSRVDesc.Format = LUTDesc.Format;
		LUTSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		LUTSRVDesc.Texture2D.MipLevels = 1;
		LUTSRVDesc.Texture2D.MostDetailedMip = 0;
		if (FAILED(g_pd3dDevice->CreateShaderResourceView(g_pLUTMap, &LUTSRVDesc, g_pLUTSRV)))
			return false;
		
		D3D11_RENDER_TARGET_VIEW_DESC LUTRTVDesc;
		LUTRTVDesc.Format = LUTDesc.Format;
		LUTRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
		LUTRTVDesc.Texture2D.MipSlice = 0;
		if (FAILED(g_pd3dDevice->CreateRenderTargetView(g_pLUTMap, &LUTRTVDesc, g_pLUTRTV)))
			return false;

		AddTask<GenLUTTask>(g_pLUTRTV);

		g_PreprocessTimer.Start();

		return true;
	}
	return false;
}

void LoadGeoms()
{
	float FullScreenQuad[] = {
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
	D3D11_SUBRESOURCE_DATA SubRcData;
	SubRcData.pSysMem = FullScreenQuad;
	SubRcData.SysMemPitch = SubRcData.SysMemSlicePitch = 0;
	g_pd3dDevice->CreateBuffer(&Desc, &SubRcData, g_pFullScreenQuadBuffer);


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

struct alignas(16) ShadingData
{
	XMFLOAT4A ViewDir;
	XMFLOAT4A Albedo;
	XMFLOAT4A SmoothnessAndMetalness;
} g_ShadingParams;

void RenderOneFrame()
{
	float ClearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
	UINT Stride = sizeof(float) * 2;
	UINT Offset = 0;

	float BLEND_FACTOR[] = { 1.0f, 1.0f, 1.0f, 1.0f };
	g_pImmediateContext->OMSetBlendState(g_pOverwriteBS, BLEND_FACTOR, 0xffffffff);
	g_pImmediateContext->RSSetState(g_pBackCullRS);

	XMFLOAT4A Albedo(1.0f, 1.0f, 1.0f, 0.5f);
	float Smoothness[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	float Metalness[] = { 0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f };
	XMStoreFloat4A(&g_ShadingParams.ViewDir, XMVectorNegate(GetCameraViewDirection()));

	g_ShadingParams.SmoothnessAndMetalness.z = g_MipCount;

	

	// PreFilter
	if (!g_bAllTasksProcessed)
	{
		g_PreprocessTimer.Tick();
		return;
	}

	RECT rect;
	GetClientRect(GetHWnd(), &rect);

	D3D11_VIEWPORT Viewport;
	Viewport.TopLeftX = 0;
	Viewport.TopLeftY = 0;
	Viewport.MinDepth = 0.0f;
	Viewport.MaxDepth = 1.0f;
	
	D3D11_MAPPED_SUBRESOURCE SubRc;
	SubRc.DepthPitch = SubRc.RowPitch = 0;
	Viewport.Width = rect.right - rect.left;
	Viewport.Height = rect.bottom - rect.top;
	g_pImmediateContext->RSSetViewports(1, &Viewport);
	g_pImmediateContext->OMSetDepthStencilState(g_pLessEqualDS, 0);

	g_pImmediateContext->ClearRenderTargetView(g_pOrigRTV, ClearColor);
	g_pImmediateContext->ClearDepthStencilView(g_pOrigDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	g_pImmediateContext->OMSetRenderTargets(1, g_pOrigRTV, g_pOrigDSV);

	// 绘制环境贴图
	g_pImmediateContext->VSSetShader(g_pEnvVertexShader, nullptr, 0);
	g_pImmediateContext->IASetInputLayout(g_pInputLayout_Position);
	g_pImmediateContext->PSSetShader(g_pEnvPixelShader, nullptr, 0);
	g_pImmediateContext->PSSetShaderResources(0, 1, g_pEnvSRV);
	g_pImmediateContext->PSSetSamplers(0, 1, g_pLinearSamplerState);

	XMMATRIX View = GetCameraMatrixWithoutTranslation();
	XMVECTOR Deter;
	XMMATRIX InvView = XMMatrixInverse(&Deter, View);
	XMMATRIX InvViewProj = XMMatrixMultiply(g_InvProjectionMatrix, InvView);
	XMStoreFloat4x4A(&g_GeomInvBuffer.InvViewProj, InvViewProj);
	ZeroMemory(&SubRc, sizeof(D3D11_MAPPED_SUBRESOURCE));
	g_pImmediateContext->Map(g_pGeomInvBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
	memcpy(SubRc.pData, &g_GeomInvBuffer, sizeof(GeomInvBuffer));
	g_pImmediateContext->Unmap(g_pGeomInvBuffer, 0);
	g_pImmediateContext->IASetVertexBuffers(0, 1, g_pFullScreenQuadBuffer, &Stride, &Offset);
	g_pImmediateContext->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	g_pImmediateContext->Draw(4, 0);
	


	// IBL
	g_pImmediateContext->IASetInputLayout(g_pInputLayout_PN);
	g_pImmediateContext->VSSetShader(g_pGeometryShader, nullptr, 0);
	g_pImmediateContext->PSSetShader(g_pIBLShader, nullptr, 0);
	g_pImmediateContext->PSSetShaderResources(1, 1, g_pIrradianceSRV);
	g_pImmediateContext->PSSetShaderResources(2, 1, g_pPreFilterSRV);
	g_pImmediateContext->PSSetShaderResources(3, 1, g_pLUTSRV);

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
			g_pImmediateContext->Map(g_pShadingData, 0, D3D11_MAP_WRITE_DISCARD, 0, &SubRc);
			memcpy(SubRc.pData, &g_ShadingParams.ViewDir, sizeof(ShadingData));
			g_pImmediateContext->Unmap(g_pShadingData, 0);
			g_pImmediateContext->PSSetConstantBuffers(4, 1, g_pShadingData);

			DrawOneGeom(GetGeoms()[i * 11 + j]);
		}
	}
}



#endif