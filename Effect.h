#pragma once
#include "Framework.h"

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
using namespace DirectX;


#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = NULL; }



template <class InterfaceType>
class CD3DInterfaceWrapper {
private:
	InterfaceType* m_pInterface = nullptr;
public:
	CD3DInterfaceWrapper() {}
	CD3DInterfaceWrapper(InterfaceType* pInterface) : m_pInterface(pInterface) {}
	template <class OtherInterfaceType>
	CD3DInterfaceWrapper(const CD3DInterfaceWrapper<OtherInterfaceType>& Other) = delete;
	template <class OtherInterfaceType>
	CD3DInterfaceWrapper(CD3DInterfaceWrapper<OtherInterfaceType>&& rOther) {
		m_pInterface = rOther.m_pInterface;
		rOther.m_pInterface = nullptr;
	}
	~CD3DInterfaceWrapper() {
		SAFE_RELEASE(m_pInterface);
	}
	CD3DInterfaceWrapper& operator= (InterfaceType* pInterface) {
		Reset();
		m_pInterface = pInterface;
		return *this;
	}
	template <class OtherInterfaceType>
	CD3DInterfaceWrapper& operator= (const CD3DInterfaceWrapper<OtherInterfaceType>& Other) = delete;
	template <class OtherInterfaceType>
	CD3DInterfaceWrapper& operator= (CD3DInterfaceWrapper<OtherInterfaceType>&& rOther) {
		Reset();
		m_pInterface = rOther.m_pInterface;
		rOther.m_pInterface = nullptr;
		return *this;
	}
	operator InterfaceType* () {
		return m_pInterface;
	}
	operator const InterfaceType* () const {
		return m_pInterface;
	}
	operator InterfaceType** () {
		return &m_pInterface;
	}
	operator InterfaceType* const* () const {
		return &m_pInterface;
	}
	bool IsValid() const {
		return m_pInterface != nullptr;
	}
	InterfaceType* operator-> () {
		return m_pInterface;
	}
	InterfaceType* Get() { return m_pInterface; }
	void Reset() { SAFE_RELEASE(m_pInterface); }
};

using D3D11DeviceWrapper = CD3DInterfaceWrapper<ID3D11Device>;
using D3D11DeviceContextWrapper = CD3DInterfaceWrapper<ID3D11DeviceContext>;
using D3D11SwapChainWrapper = CD3DInterfaceWrapper<IDXGISwapChain>;
using D3D11VertexShaderWrapper = CD3DInterfaceWrapper<ID3D11VertexShader>;
using D3D11InputLayoutWrapper = CD3DInterfaceWrapper<ID3D11InputLayout>;
using D3D11ComputeShaderWrapper = CD3DInterfaceWrapper<ID3D11ComputeShader>;
using D3D11PixelShaderWrapper = CD3DInterfaceWrapper<ID3D11PixelShader>;
using D3D11Texture2DWrapper = CD3DInterfaceWrapper<ID3D11Texture2D>;
using D3D11SRVWrapper = CD3DInterfaceWrapper<ID3D11ShaderResourceView>;
using D3D11RTVWrapper = CD3DInterfaceWrapper<ID3D11RenderTargetView>;
using D3D11UAVWrapper = CD3DInterfaceWrapper<ID3D11UnorderedAccessView>;
using D3D11DSVWrapper = CD3DInterfaceWrapper<ID3D11DepthStencilView>;
using D3D11BufferWrapper = CD3DInterfaceWrapper<ID3D11Buffer>;
using D3D11BSWrapper = CD3DInterfaceWrapper<ID3D11BlendState>;
using D3D11RSWrapper = CD3DInterfaceWrapper<ID3D11RasterizerState>;
using D3D11DSSWrapper = CD3DInterfaceWrapper<ID3D11DepthStencilState>;
using D3D11SSWrapper = CD3DInterfaceWrapper<ID3D11SamplerState>;
using D3DBlobWrapper = CD3DInterfaceWrapper<ID3DBlob>;

template <
	UINT8 RT0ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT0ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT0ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT0ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT0AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT0AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT0AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT1ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT1ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT1ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT1ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT1AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT1AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT1AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT2ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT2ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT2ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT2ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT2AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT2AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT2AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT3ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT3ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT3ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT3ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT3AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT3AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT3AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT4ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT4ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT4ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT4ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT4AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT4AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT4AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT5ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT5ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT5ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT5ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT5AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT5AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT5AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT6ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT6ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT6ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT6ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT6AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT6AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT6AlphaDestBlend = D3D11_BLEND_ZERO,
	UINT8 RT7ColorWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL,
	D3D11_BLEND_OP RT7ColorBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT7ColorSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT7ColorDestBlend = D3D11_BLEND_ZERO,
	D3D11_BLEND_OP RT7AlphaBlendOp = D3D11_BLEND_OP_ADD,
	D3D11_BLEND    RT7AlphaSrcBlend = D3D11_BLEND_ONE,
	D3D11_BLEND    RT7AlphaDestBlend = D3D11_BLEND_ZERO,
	bool			bUseAlphaToCoverage = false
>
class TStaticBlendState
{
public:
	TStaticBlendState()
	{
		m_Desc.IndependentBlendEnable = TRUE;
		m_Desc.AlphaToCoverageEnable = bUseAlphaToCoverage;
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[0], RT0ColorBlendOp, RT0ColorSrcBlend, RT0ColorDestBlend, RT0AlphaBlendOp, RT0AlphaSrcBlend, RT0AlphaDestBlend, RT0ColorWriteMask);
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[1], RT1ColorBlendOp, RT1ColorSrcBlend, RT1ColorDestBlend, RT1AlphaBlendOp, RT1AlphaSrcBlend, RT1AlphaDestBlend, RT1ColorWriteMask);
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[2], RT2ColorBlendOp, RT2ColorSrcBlend, RT2ColorDestBlend, RT2AlphaBlendOp, RT2AlphaSrcBlend, RT2AlphaDestBlend, RT2ColorWriteMask);
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[3], RT3ColorBlendOp, RT3ColorSrcBlend, RT3ColorDestBlend, RT3AlphaBlendOp, RT3AlphaSrcBlend, RT3AlphaDestBlend, RT3ColorWriteMask);
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[4], RT4ColorBlendOp, RT4ColorSrcBlend, RT4ColorDestBlend, RT4AlphaBlendOp, RT4AlphaSrcBlend, RT4AlphaDestBlend, RT4ColorWriteMask);
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[5], RT5ColorBlendOp, RT5ColorSrcBlend, RT5ColorDestBlend, RT5AlphaBlendOp, RT5AlphaSrcBlend, RT5AlphaDestBlend, RT5ColorWriteMask);
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[6], RT6ColorBlendOp, RT6ColorSrcBlend, RT6ColorDestBlend, RT6AlphaBlendOp, RT6AlphaSrcBlend, RT6AlphaDestBlend, RT6ColorWriteMask);
		ConvertToRenderTargetDesc(m_Desc.RenderTarget[7], RT7ColorBlendOp, RT7ColorSrcBlend, RT7ColorDestBlend, RT7AlphaBlendOp, RT7AlphaSrcBlend, RT7AlphaDestBlend, RT7ColorWriteMask);
	}

	void ConvertToRenderTargetDesc(_Out_ D3D11_RENDER_TARGET_BLEND_DESC& RenderTargetDesc, 
		D3D11_BLEND_OP ColorBlendOp, D3D11_BLEND ColorBlendSrc, D3D11_BLEND ColorBlendDest, 
		D3D11_BLEND_OP AlphaBlendOp, D3D11_BLEND AlphaBlendSrc, D3D11_BLEND AlphaBlendDesc,
		UINT8 ColorWriteMask) {
		RenderTargetDesc.BlendEnable = TRUE;
		RenderTargetDesc.BlendOp = ColorBlendOp;
		RenderTargetDesc.SrcBlend = ColorBlendSrc;
		RenderTargetDesc.DestBlend = ColorBlendDest;
		RenderTargetDesc.BlendOpAlpha = AlphaBlendOp;
		RenderTargetDesc.SrcBlendAlpha = AlphaBlendSrc;
		RenderTargetDesc.DestBlendAlpha = AlphaBlendDesc;
		RenderTargetDesc.RenderTargetWriteMask = ColorWriteMask;
	}

	D3D11BSWrapper GetBlendState() const {
		ID3D11BlendState* pBlendState;
		g_D3DInterface.m_pDevice->CreateBlendState(&m_Desc, &pBlendState);
		return pBlendState;
	}

private:
	D3D11_BLEND_DESC m_Desc;
};

template <
	D3D11_FILL_MODE FillMode = D3D11_FILL_SOLID,
	D3D11_CULL_MODE CullMode = D3D11_CULL_BACK,
	BOOL FrontCCW = TRUE,
	int DepthBias = 0
>
class TStaticRasterizerState
{
public:
	TStaticRasterizerState() {
		m_Desc.FillMode = FillMode;
		m_Desc.CullMode = CullMode;
		m_Desc.FrontCounterClockwise = FrontCCW;
		m_Desc.DepthBias = DepthBias;
		m_Desc.AntialiasedLineEnable = FALSE;
		m_Desc.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
		m_Desc.DepthClipEnable = TRUE;
		m_Desc.MultisampleEnable = FALSE;
		m_Desc.ScissorEnable = FALSE;
		m_Desc.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	}

	D3D11RSWrapper GetRasterizerState() const {
		ID3D11RasterizerState* pRasterizerState;
		g_D3DInterface.m_pDevice->CreateRasterizerState(&m_Desc, &pRasterizerState);
		return pRasterizerState;
	}

private:
	D3D11_RASTERIZER_DESC m_Desc;
};

template <
	BOOL bEnableDepthWrite = TRUE,
	D3D11_COMPARISON_FUNC DepthTest = D3D11_COMPARISON_LESS,
	BOOL bEnableFrontFaceStencil = FALSE,
	D3D11_COMPARISON_FUNC FrontFaceStencilTest = D3D11_COMPARISON_ALWAYS,
	D3D11_STENCIL_OP FrontFaceStencilFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP FrontFaceDepthFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP FrontFacePassStencilOp = D3D11_STENCIL_OP_KEEP,
	BOOL bEnableBackFaceStencil = FALSE,
	D3D11_COMPARISON_FUNC BackFaceStencilTest = D3D11_COMPARISON_ALWAYS,
	D3D11_STENCIL_OP BackFaceStencilFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP BackFaceDepthFailStencilOp = D3D11_STENCIL_OP_KEEP,
	D3D11_STENCIL_OP BackFacePassStencilOp = D3D11_STENCIL_OP_KEEP,
	UINT8 StencilReadMask = 0xFF,
	UINT8 StencilWriteMask = 0xFF
>
class TStaticDepthStencilState
{
public:
	TStaticDepthStencilState() {
		m_Desc.DepthEnable = DepthTest != D3D11_COMPARISON_ALWAYS || bEnableDepthWrite;
		m_Desc.DepthFunc = DepthTest;
		m_Desc.DepthWriteMask = bEnableDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		m_Desc.StencilEnable = bEnableFrontFaceStencil || bEnableBackFaceStencil;
		m_Desc.StencilWriteMask = StencilWriteMask;
		m_Desc.StencilReadMask = StencilReadMask;
		m_Desc.FrontFace.StencilFunc = FrontFaceStencilTest;
		m_Desc.FrontFace.StencilPassOp = FrontFacePassStencilOp;
		m_Desc.FrontFace.StencilDepthFailOp = FrontFaceDepthFailStencilOp;
		m_Desc.FrontFace.StencilFailOp = FrontFaceStencilFailStencilOp;
		m_Desc.BackFace.StencilFunc = BackFaceStencilTest;
		m_Desc.BackFace.StencilPassOp = BackFacePassStencilOp;
		m_Desc.BackFace.StencilDepthFailOp = BackFaceDepthFailStencilOp;
		m_Desc.BackFace.StencilFailOp = BackFaceStencilFailStencilOp;
	}

	D3D11DSSWrapper GetDepthStencilState() const {
		ID3D11DepthStencilState* pDepthStencilState;
		g_D3DInterface.m_pDevice->CreateDepthStencilState(&m_Desc, &pDepthStencilState);
		return pDepthStencilState;
	}

private:
	D3D11_DEPTH_STENCIL_DESC m_Desc;
};

struct Global_D3D_Interface
{
	D3D11DeviceWrapper 					m_pDevice;
	D3D11DeviceContextWrapper			m_pDeviceContext;
	D3D11SwapChainWrapper 				m_pSwapChain;
	D3D11RTVWrapper						m_pMainBackbuffer;
	D3D11Texture2DWrapper				m_pMainDepthTexture;
	D3D11DSVWrapper						m_pMainDepthbuffer;

	void Reset() {
		m_pDevice.Reset();
		m_pDeviceContext.Reset();
		m_pSwapChain.Reset();
		m_pMainBackbuffer.Reset();
		m_pMainDepthTexture.Reset();
		m_pMainDepthbuffer.Reset();
	}
};
extern EFFECT_API Global_D3D_Interface g_D3DInterface;

struct Global_Data
{
	std::vector<String> m_vecEffects;
}; 
EFFECT_API Global_Data& GetGlobalData();
EFFECT_API void RegisterEffects();

#define REGISTER_EFFECT(ClassName, DisplayName) \
	struct Effect##ClassName##Register { \
		Effect##ClassName##Register() { \
			GetGlobalData().m_vecEffects.emplace_back(_T(#DisplayName)); \
		} \
	} _Effect##ClassName##Register; \

bool CreateDeviceAndImmediateContext(HWND hWnd);
bool CreateShaders();
bool CreateBlendStates();
bool CreateRasterizerStates();
bool CreateDepthStencilStates();
bool CreateSamplerStates();
bool CreateRenderTargets();
bool CreateConstantBuffers();
bool LoadResources();
void RenderOneFrame();

// Inputs
void NotifyKeyDown(UINT_PTR KeyValue);
void NotifyKeyUp(UINT_PTR KeyValue);
void NotifyLButtonDown(WORD X, WORD Y);
void NotifyLButtonUp(WORD X, WORD Y);
void NotifyMouseMove(WORD X, WORD Y);
void NotifyRButtonDown(WORD X, WORD Y);
void NotifyRButtonUp(WORD X, WORD Y);
void TickInput();

void GlobalEffectReset();

using RetBoolFunc = bool(*)();
using Func = void(*)();
using OneParamUINTPTRFunc = void(*)(UINT_PTR);
using TwoParamsWORDFunc = void(*)(WORD, WORD);

struct Effect_Instance {
	RetBoolFunc 				CreateShadersCallback = nullptr;
	RetBoolFunc 				CreateBlendStatesCallback = nullptr;
	RetBoolFunc 				CreateRasterizerStatesCallback = nullptr;
	RetBoolFunc 				CreateDepthStencilStatesCallback = nullptr;
	RetBoolFunc 				CreateSamplerStatesCallback = nullptr;
	RetBoolFunc 				CreateRenderTargetsCallback = nullptr;
	RetBoolFunc 				CreateConstantBuffersCallback = nullptr;
	RetBoolFunc 				LoadResourcesCallback = nullptr;
	Func 						RenderOneFrameCallback = nullptr;

	OneParamUINTPTRFunc			KeyDownCallback = nullptr;
	OneParamUINTPTRFunc			KeyUpCallback = nullptr;
	TwoParamsWORDFunc			LButtonDownCallback = nullptr;
	TwoParamsWORDFunc			LButtonUpCallback = nullptr;
	TwoParamsWORDFunc			RButtonDownCallback = nullptr;
	TwoParamsWORDFunc			RButtonUpCallback = nullptr;
	TwoParamsWORDFunc			MouseMoveCallback = nullptr;				
};
extern std::weak_ptr<Effect_Instance> g_pCurrentEffectInst;

class VB_Base {
public:
	virtual ~VB_Base() {
		if (m_ppVertexBuffer)
		{
			for (UINT n = 0; n < m_nNumVB; ++n)
			{
				SAFE_RELEASE(m_ppVertexBuffer[n]);
			}

			delete[] m_ppVertexBuffer;
			m_ppVertexBuffer = nullptr;
		}

		if (m_pStrides)
		{
			delete[] m_pStrides;
			m_pStrides = nullptr;
		}

		if (m_pOffsets)
		{
			delete[] m_pOffsets;
			m_pOffsets = nullptr;
		}

		m_nNumVB = 0;
	}

	ID3D11Buffer** GetVertexBuffers() { return m_ppVertexBuffer; }
	UINT* GetStrides() { return m_pStrides; }
	UINT* GetOffsets() { return m_pOffsets; }
	UINT GetNumVertexBuffers() const { return m_nNumVB; }
	UINT GetNumVerts() const { return m_nNumVerts; }

protected:
	bool InitializeBuffer(LPVOID pData, UINT Stride, UINT Num, ID3D11Buffer** ppBuffer) {
		D3D11_BUFFER_DESC PositionBufferDesc;
		PositionBufferDesc.ByteWidth = Stride * Num;
		PositionBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		PositionBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		PositionBufferDesc.CPUAccessFlags = 0;
		PositionBufferDesc.MiscFlags = 0;
		PositionBufferDesc.StructureByteStride = Stride;
		D3D11_SUBRESOURCE_DATA PositionData;
		PositionData.pSysMem = pData;
		PositionData.SysMemPitch = PositionData.SysMemSlicePitch = 0;
		return SUCCEEDED(g_D3DInterface.m_pDevice->CreateBuffer(&PositionBufferDesc, &PositionData, ppBuffer));
	}

protected:
	ID3D11Buffer** m_ppVertexBuffer = nullptr;
	UINT* m_pStrides = nullptr;
	UINT* m_pOffsets = nullptr;
	UINT m_nNumVB = 0;
	UINT m_nNumVerts = 0;
};

class VB_Position final : public VB_Base {
public:
	VB_Position() {
		m_nNumVB = 1;
		m_ppVertexBuffer = new ID3D11Buffer*[m_nNumVB];
		m_pStrides = new UINT[m_nNumVB];
		m_pOffsets = new UINT[m_nNumVB];
	}
	bool Initialize(LPVOID pMem, UINT NumVertices) {
		if (InitializeBuffer(pMem, sizeof(float) * 3, NumVertices, &m_ppVertexBuffer[0]))
		{
			m_pStrides[0] = sizeof(float) * 3;
			m_pOffsets[0] = 0;
			m_nNumVerts = NumVertices;
			return true;
		}
		else
			return false;
	}
};

class VB_PositionNormal final : public VB_Base {
public:
	VB_PositionNormal() {
		m_nNumVB = 2;
		m_ppVertexBuffer = new ID3D11Buffer*[m_nNumVB];
		m_pStrides = new UINT[m_nNumVB];
		m_pOffsets = new UINT[m_nNumVB];
	}
	bool Initialize(LPVOID pPosMem, LPVOID pNormalMem, UINT NumVertices) {
		if (!InitializeBuffer(pPosMem, sizeof(float) * 3, NumVertices, &m_ppVertexBuffer[0]))
			return false;
		if (!InitializeBuffer(pNormalMem, sizeof(float) * 3, NumVertices, &m_ppVertexBuffer[1]))
			return false;
		m_pStrides[0] = m_pStrides[1] = sizeof(float) * 3;
		m_pOffsets[0] = m_pOffsets[1] = 0;
		m_nNumVerts = NumVertices;
		return true;
	}
};

class VB_PositionNormalTangent final : public VB_Base {
public:
	VB_PositionNormalTangent() {
		m_nNumVB = 3;
		m_ppVertexBuffer = new ID3D11Buffer*[m_nNumVB];
		m_pStrides = new UINT[m_nNumVB];
		m_pOffsets = new UINT[m_nNumVB];
	}
	bool Initialize(LPVOID pPosMem, LPVOID pNormalMem, LPVOID pTangentMem, UINT NumVertices) {
		if (!InitializeBuffer(pPosMem, sizeof(float) * 3, NumVertices, &m_ppVertexBuffer[0]))
			return false;
		if (!InitializeBuffer(pNormalMem, sizeof(float) * 3, NumVertices, &m_ppVertexBuffer[1]))
			return false;
		if (!InitializeBuffer(pTangentMem, sizeof(float) * 3, NumVertices, &m_ppVertexBuffer[2]))
			return false;
		m_pStrides[0] = m_pStrides[1] = m_pStrides[2] = sizeof(float) * 3;
		m_pOffsets[0] = m_pOffsets[1] = m_pOffsets[2] = 0;
		m_nNumVerts = NumVertices;
		return true;
	}
};

struct Geom {
	VB_Base* m_pVertexBuffer = nullptr;
	ID3D11Buffer* m_pIndexBuffer = nullptr;
	UINT m_NumIndex = 0;
	XMMATRIX m_WorldMatrix;
	Geom(VB_Base* pVertexBuffer, ID3D11Buffer* pIndexBuffer, UINT NumIndex, XMMATRIX WorldMatrix)
		: m_pVertexBuffer(pVertexBuffer)
		, m_pIndexBuffer(pIndexBuffer)
		, m_NumIndex(NumIndex)
		, m_WorldMatrix(WorldMatrix) {
	}
	Geom(Geom&& Other) {
		m_pVertexBuffer = Other.m_pVertexBuffer; Other.m_pVertexBuffer = nullptr;
		m_pIndexBuffer = Other.m_pIndexBuffer; Other.m_pIndexBuffer = nullptr;
		m_NumIndex = Other.m_NumIndex; Other.m_NumIndex = 0;
		m_WorldMatrix = Other.m_WorldMatrix;
	}
	~Geom() {
		if (m_pVertexBuffer)
		{
			delete m_pVertexBuffer;
			m_pVertexBuffer = nullptr;
		}
		SAFE_RELEASE(m_pIndexBuffer);
		m_NumIndex = 0;
	}
};

EFFECT_API void AddGeom(VB_Base* pVB, ID3D11Buffer* pIndexBuffer, UINT NumIndex, XMMATRIX WorldMatrix);
EFFECT_API const std::vector<Geom>& GetGeoms();
EFFECT_API void ClearGeoms();
EFFECT_API void DrawOneGeom(const Geom& InGeom);

class SimpleRT
{
public:
	ID3D11Texture2D* m_pTexture = nullptr;
	ID3D11RenderTargetView* m_pRTV = nullptr;
	ID3D11ShaderResourceView* m_pSRV = nullptr;

	bool Initialize(ID3D11Device* pd3dDevice, D3D11_TEXTURE2D_DESC* pTexDesc, DXGI_FORMAT Format)
	{
		pTexDesc->Format = Format;

		HRESULT hr;
		if (FAILED(hr = pd3dDevice->CreateTexture2D(pTexDesc, NULL, &m_pTexture)))
			return false;
		if (FAILED(hr = pd3dDevice->CreateShaderResourceView(m_pTexture, NULL, &m_pSRV)))
			return false;
		if (FAILED(hr = pd3dDevice->CreateRenderTargetView(m_pTexture, NULL, &m_pRTV)))
			return false;
		return true;
	}

	~SimpleRT()
	{
		SAFE_RELEASE(m_pTexture);
		SAFE_RELEASE(m_pRTV);
		SAFE_RELEASE(m_pSRV);
	}
};

inline XMVECTOR MakeD3DPOINT(float x, float y, float z)
{
	return XMVectorSet(x, y, z, 1.0f);
}
inline XMVECTOR MakeD3DVECTOR(float x, float y, float z)
{
	return XMVectorSet(x, y, z, 0.0f);
}

extern EFFECT_API XMMATRIX g_CameraMatrix;
extern EFFECT_API XMMATRIX g_ProjectionMatrix;
extern EFFECT_API XMMATRIX g_CameraProjectionMatrix;
extern EFFECT_API XMMATRIX g_InvCameraMatrix;
extern EFFECT_API XMMATRIX g_InvProjectionMatrix;
extern EFFECT_API XMMATRIX g_InvCameraProjectionMatrix;

struct alignas(16) GeomBuffer 
{
	XMFLOAT4X4A World;
	XMFLOAT4X4A WorldView;
	XMFLOAT4X4A WorldViewProj;
};
// extern GeomBuffer g_GeomBuffer;

struct alignas(16) GeomInvBuffer
{
	XMFLOAT4X4A InvProj;
	XMFLOAT4X4A InvViewProj;
};
// extern GeomInvBuffer g_GeomInvBuffer;

struct alignas(16) GeomITBuffer
{
	XMFLOAT4X4A WorldIT;
	XMFLOAT4X4A WorldViewIT;
};
// extern GeomITBuffer g_GeomITBuffer;

EFFECT_API XMVECTOR GetCameraViewDirection();
EFFECT_API XMMATRIX GetCameraMatrix();
EFFECT_API XMMATRIX GetCameraMatrixWithoutTranslation();
EFFECT_API XMMATRIX GetInverseCameraMatrix();
EFFECT_API XMMATRIX GetPerspectiveProjectionMatrix();
EFFECT_API XMMATRIX GetInversePerspectiveProjectionMatrix();

EFFECT_API void SyncGeomConstantBuffer(XMMATRIX World);

EFFECT_API XMFLOAT4A SRGBToRGB(FXMVECTOR srgb);

struct FileContent
{
	LPVOID m_pContent = nullptr;
	DWORD m_nLength = 0;
	~FileContent()
	{
		if (m_pContent)
		{
			free(m_pContent);
			m_pContent = nullptr;
			m_nLength = 0;
		}
	}
};

EFFECT_API bool LoadFile(LPCTSTR lpszFileName, _Out_ FileContent& FileContentRef);
enum class EShaderModel {
	ESM_5
};
EFFECT_API bool CreateVertexShaderAndInputLayout(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11VertexShader** ppVertexShader,
	const D3D11_INPUT_ELEMENT_DESC* InputLayoutElements, UINT NumElements, ID3D11InputLayout** ppInputLayout);
EFFECT_API bool CreateComputeShader(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11ComputeShader** ppComputeShader);
EFFECT_API bool CreatePixelShader(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11PixelShader** ppPixelShader);
EFFECT_API bool LoadJpegTextureFromFile(LPCTSTR lpszFileName, bool bGammaCorrection, ID3D11Texture2D** ppTexture2D, ID3D11ShaderResourceView** ppSRV);

class EFFECT_API CShaderHeaderInclude : public ID3DInclude
{
public:
    STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override;
    STDMETHOD(Close)(THIS_ LPCVOID pData) override;

private:
	LPVOID m_pFileContent = nullptr;
};