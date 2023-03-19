#pragma once
#include "Framework.h"

#include <d3d11.h>
#include <dxgi.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
using namespace DirectX;

#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <functional>

#define SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = NULL; }

#ifdef UNICODE
	using String = std::wstring;
#else
	using String = std::string;
#endif

template <class InterfaceType>
class CD3DInterfaceWrapper {
private:
	InterfaceType* m_pInterface = nullptr;
public:
	CD3DInterfaceWrapper() {}
	CD3DInterfaceWrapper(InterfaceType* pInterface) : m_pInterface(pInterface) {}
	~CD3DInterfaceWrapper() {
		SAFE_RELEASE(m_pInterface);
	}
	CD3DInterfaceWrapper& operator= (InterfaceType* pInterface) {
		Reset();
		m_pInterface = pInterface;
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

class ITask
{
public:
	virtual ~ITask() {}
	virtual void DoTask() = 0;
};

using TaskQueue = std::deque<std::shared_ptr<ITask>>;
extern TaskQueue g_TaskQueue;

template <class TaskType, class ... Args>
void AddTask(Args && ... args) {
	g_TaskQueue.emplace_back(std::make_shared<TaskType>(std::forward<Args>(args)...));
}
bool PopTaskAndRun();
bool HasMoreTasks();

class Timer
{
public:
	enum class ETriggerType {
		Once,
		Repeat
	};
	Timer(float TimeSpanInSeconds, ETriggerType TriggerType, std::function<void()>&& Func) 
	: m_TimeSpan(TimeSpanInSeconds) 
	, m_TriggerType(TriggerType) 
	, m_Func(Func) { m_ElapsedTime = 0; m_LastTime = -1; }
	void Start();
	void Tick();

	static void GlobalInitialize();

private:
	LONGLONG m_ElapsedTime, m_LastTime;
	float m_TimeSpan;
	ETriggerType m_TriggerType;
	std::function<void()> m_Func;

	static LARGE_INTEGER s_Frequency;
};

HWND GetHWnd();
void CacheModulePath();
LPCTSTR GetExePath();

// extern D3D11SwapChainWrapper g_pSwapChain;
// extern D3D11DeviceWrapper g_pd3dDevice;
// extern D3D11DeviceContextWrapper g_pImmediateContext;
// extern D3D11RTVWrapper g_pOrigRTV;
// extern D3D11Texture2DWrapper g_pOrigDSTexture;
// extern D3D11DSVWrapper g_pOrigDSV;

// extern D3D11BSWrapper g_pOverwriteBS;
// extern D3D11RSWrapper g_pBackCullRS;
// extern D3D11DSSWrapper g_pLessEqualDS;
// extern D3D11SSWrapper g_pLinearSamplerState;

// extern D3D11BufferWrapper g_pGeomBuffer;
// extern D3D11BufferWrapper g_pGeomInvBuffer;
// extern D3D11BufferWrapper g_pGeomITBuffer;

struct Global_D3D_Interface
{
	D3D11DeviceWrapper 					m_pDevice;
	D3D11DeviceContextWrapper			m_pDeviceContext;
	D3D11SwapChainWrapper 				m_pSwapChain;
	D3D11RTVWrapper						m_pMainBackbuffer;
	D3D11Texture2DWrapper				m_pMainDepthTexture;
	D3D11DSVWrapper						m_pMainDepthbuffer;
};
extern Global_D3D_Interface g_D3DInterface;

struct Global_Data
{
	std::vector<String> m_vecEffects;
}; 
extern Global_Data g_GlobalData;

bool CreateDeviceAndImmediateContext(HWND hWnd);
bool CreateShaders();
bool CreateBlendStates();
bool CreateRasterizerStates();
bool CreateDepthStencilStates();
bool CreateSamplerStates();
bool CreateRenderTargets();
bool CreateConstantBuffers();
bool LoadResources();
void LoadGeoms();
void RenderOneFrame();

// Inputs
void NotifyKeyDown(UINT_PTR KeyValue);
void NotifyKeyUp(UINT_PTR KeyValue);
void NotifyLButtonDown(WORD X, WORD Y);
void NotifyLButtonUp(WORD X, WORD Y);
void NotifyMouseMove(WORD X, WORD Y);
void NotifyRButtonUp(WORD X, WORD Y);
void TickInput();

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
		return SUCCEEDED(g_pd3dDevice->CreateBuffer(&PositionBufferDesc, &PositionData, ppBuffer));
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

void AddGeom(VB_Base* pVB, ID3D11Buffer* pIndexBuffer, UINT NumIndex, XMMATRIX WorldMatrix);
const std::vector<Geom>& GetGeoms();

void DrawOneGeom(const Geom& InGeom);

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

extern XMMATRIX g_CameraMatrix;
extern XMMATRIX g_ProjectionMatrix;
extern XMMATRIX g_CameraProjectionMatrix;
extern XMMATRIX g_InvCameraMatrix;
extern XMMATRIX g_InvProjectionMatrix;
extern XMMATRIX g_InvCameraProjectionMatrix;

struct alignas(16) GeomBuffer 
{
	XMFLOAT4X4A World;
	XMFLOAT4X4A WorldView;
	XMFLOAT4X4A WorldViewProj;
};
extern GeomBuffer g_GeomBuffer;

struct alignas(16) GeomInvBuffer
{
	XMFLOAT4X4A InvProj;
	XMFLOAT4X4A InvViewProj;
};
extern GeomInvBuffer g_GeomInvBuffer;

struct alignas(16) GeomITBuffer
{
	XMFLOAT4X4A WorldIT;
	XMFLOAT4X4A WorldViewIT;
};
extern GeomITBuffer g_GeomITBuffer;

XMVECTOR GetCameraViewDirection();
XMMATRIX GetCameraMatrix();
XMMATRIX GetCameraMatrixWithoutTranslation();
XMMATRIX GetInverseCameraMatrix();
XMMATRIX GetProjectionMatrix();
XMMATRIX GetInverseProjectionMatrix();

void SyncGeomConstantBuffer(XMMATRIX World);

XMFLOAT4A SRGBToRGB(FXMVECTOR srgb);

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

bool LoadFile(LPCTSTR lpszFileName, _Out_ FileContent& FileContentRef);
enum class EShaderModel {
	ESM_5
};
bool CreateVertexShaderAndInputLayout(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11VertexShader** ppVertexShader,
	const D3D11_INPUT_ELEMENT_DESC* InputLayoutElements, UINT NumElements, ID3D11InputLayout** ppInputLayout);
bool CreateComputeShader(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11ComputeShader** ppComputeShader);
bool CreatePixelShader(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11PixelShader** ppPixelShader);
bool LoadJpegTextureFromFile(LPCTSTR lpszFileName, bool bGammaCorrection, ID3D11Texture2D** ppTexture2D, ID3D11ShaderResourceView** ppSRV);

class CShaderHeaderInclude : public ID3DInclude
{
public:
    STDMETHOD(Open)(THIS_ D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes) override;
    STDMETHOD(Close)(THIS_ LPCVOID pData) override;
};