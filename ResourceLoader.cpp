#include "Effect.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

bool LoadFile(LPCTSTR lpszFileName, FileContent& FileContentRef)
{
	HANDLE hFile = ::CreateFile(lpszFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	DWORD nFileSize = ::GetFileSize(hFile, nullptr);
	LPVOID pFileContent = malloc(nFileSize);
	DWORD nSizeRead = 0;
	if (::ReadFile(hFile, pFileContent, nFileSize, &nSizeRead, nullptr) == FALSE)
	{
		free(pFileContent);
		return false;
	}
	if (nFileSize != nSizeRead)
	{
		free(pFileContent);
		return false;
	}
	FileContentRef.m_pContent = pFileContent;
	FileContentRef.m_nLength = nFileSize;
	::CloseHandle(hFile);
	return true;
}

enum class EShaderType {
	EST_Vertex,
	EST_Compute,
	EST_Pixel
};

LPCSTR GetTargetString(EShaderModel ShaderModel, EShaderType ShaderType)
{
	if (ShaderModel == EShaderModel::ESM_5)
	{
		switch (ShaderType)
		{
			case EShaderType::EST_Vertex:
				return "vs_5_0";
			case EShaderType::EST_Compute:
				return "cs_5_0";
			case EShaderType::EST_Pixel:
				return "ps_5_0";
		}
	}
	return nullptr;
}

bool CreateVertexShaderAndInputLayout(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11VertexShader** ppVertexShader,
	const D3D11_INPUT_ELEMENT_DESC* InputLayoutElements, UINT NumElements, ID3D11InputLayout** ppInputLayout)
{
	assert(ppVertexShader);
	*ppVertexShader = nullptr;

	FileContent ShaderContent;
	if (!LoadFile(pszFilePath, ShaderContent))
		return false;

	LPCSTR TargetString = GetTargetString(EShaderModel::ESM_5, EShaderType::EST_Vertex);
	if (TargetString == nullptr)
		return false;
	ID3DBlob *pCode, *pError;
	CShaderHeaderInclude ShaderHeaderInclude;
#ifdef DEBUG_SHADER
	UINT CompileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT CompileFlag = 0;
#endif
	if (FAILED(D3DCompile(ShaderContent.m_pContent, ShaderContent.m_nLength, nullptr, nullptr, &ShaderHeaderInclude, pszEntryPoint, TargetString, CompileFlag, 0, &pCode, &pError)))
	{
		OutputDebugStringA((LPCSTR)pError->GetBufferPointer());
		SAFE_RELEASE(pError);
		return false;
	}
	if (FAILED(g_D3DInterface.m_pDevice->CreateVertexShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, ppVertexShader)))
	{
		SAFE_RELEASE(pCode);
		return false;
	}

	bool bRet = true;
	if (ppInputLayout)
	{
		assert(InputLayoutElements && NumElements > 0);
		bRet = SUCCEEDED(g_D3DInterface.m_pDevice->CreateInputLayout(InputLayoutElements, NumElements, pCode->GetBufferPointer(), pCode->GetBufferSize(), ppInputLayout));
	}

	SAFE_RELEASE(pCode);
	return bRet;
}

bool CreateComputeShader(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11ComputeShader** ppComputeShader)
{
	assert(ppComputeShader);
	*ppComputeShader = nullptr;

	FileContent ShaderContent;
	if (!LoadFile(pszFilePath, ShaderContent))
		return false;

	LPCSTR TargetString = GetTargetString(EShaderModel::ESM_5, EShaderType::EST_Compute);
	if (TargetString == nullptr)
		return false;
	ID3DBlob *pCode, *pError;
	CShaderHeaderInclude ShaderHeaderInclude;
	if (FAILED(D3DCompile(ShaderContent.m_pContent, ShaderContent.m_nLength, nullptr, nullptr, &ShaderHeaderInclude, pszEntryPoint, TargetString, 0, 0, &pCode, &pError)))
	{
		OutputDebugStringA((LPCSTR)pError->GetBufferPointer());
		SAFE_RELEASE(pError);
		return false;
	}
	bool bRet = SUCCEEDED(g_D3DInterface.m_pDevice->CreateComputeShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, ppComputeShader));
	SAFE_RELEASE(pCode);
	return bRet;
}

bool CreatePixelShader(LPCTSTR pszFilePath, LPCSTR pszEntryPoint, EShaderModel ShaderModel, ID3D11PixelShader** ppPixelShader)
{
	assert(ppPixelShader);
	*ppPixelShader = nullptr;

	FileContent ShaderContent;
	if (!LoadFile(pszFilePath, ShaderContent))
		return false;

	LPCSTR TargetString = GetTargetString(ShaderModel, EShaderType::EST_Pixel);
	if (TargetString == nullptr)
		return false;
	ID3DBlob *pCode, *pError;
	CShaderHeaderInclude ShaderHeaderInclude;
#ifdef DEBUG_SHADER
	UINT CompileFlag = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT CompileFlag = 0;
#endif
	if (FAILED(D3DCompile(ShaderContent.m_pContent, ShaderContent.m_nLength, nullptr, nullptr, &ShaderHeaderInclude, pszEntryPoint, TargetString, CompileFlag, 0, &pCode, &pError)))
	{
		OutputDebugStringA((LPCSTR)pError->GetBufferPointer());
		SAFE_RELEASE(pError);
		return false;
	}
	bool bRet = SUCCEEDED(g_D3DInterface.m_pDevice->CreatePixelShader(pCode->GetBufferPointer(), pCode->GetBufferSize(), nullptr, ppPixelShader));
	SAFE_RELEASE(pCode);
	return bRet;
}

bool LoadJpegTextureFromFile(LPCTSTR lpszFileName, bool bGammaCorrection, ID3D11Texture2D** ppTexture2D, ID3D11ShaderResourceView** ppSRV)
{
	assert(ppTexture2D && ppSRV);
	*ppTexture2D = nullptr;
	*ppSRV = nullptr;

	FileContent TextureContent;
	if (!LoadFile(lpszFileName, TextureContent))
		return false;

	int width, height, actual_comps;
	stbi_uc* pData = stbi_load_from_memory((stbi_uc const*)TextureContent.m_pContent, TextureContent.m_nLength, &width, &height, &actual_comps, STBI_rgb_alpha);
	if (pData)
	{
		D3D11_TEXTURE2D_DESC Desc;
		Desc.Width = width;
		Desc.Height = height;
		Desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		Desc.CPUAccessFlags = 0;
		Desc.Format = bGammaCorrection ? DXGI_FORMAT_R8G8B8A8_UNORM_SRGB : DXGI_FORMAT_R8G8B8A8_UNORM;
		Desc.ArraySize = 1;
		Desc.MipLevels = 1;
		Desc.MiscFlags = 0;
		Desc.Usage = D3D11_USAGE_DEFAULT;
		Desc.SampleDesc.Count = 1;
		Desc.SampleDesc.Quality = 0;

		D3D11_SUBRESOURCE_DATA SubData;
		SubData.pSysMem = pData;
		SubData.SysMemPitch = width * sizeof(stbi_uc) * STBI_rgb_alpha;
		SubData.SysMemSlicePitch = 0;

		if (SUCCEEDED(g_D3DInterface.m_pDevice->CreateTexture2D(&Desc, &SubData, ppTexture2D)))
		{
			stbi_image_free(pData);

			if (SUCCEEDED(g_D3DInterface.m_pDevice->CreateShaderResourceView(*ppTexture2D, nullptr, ppSRV)))
				return true;
			else
			{
				SAFE_RELEASE(*ppTexture2D);
				return false;
			}
		}

		stbi_image_free(pData);
	}

	return false;
}

HRESULT CShaderHeaderInclude::Open(D3D_INCLUDE_TYPE IncludeType, LPCSTR pFileName, LPCVOID pParentData, LPCVOID *ppData, UINT *pBytes)
{
	HANDLE hFile = ::CreateFileA(pFileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)
		return false;
	DWORD nFileSize = ::GetFileSize(hFile, nullptr);
	m_pFileContent = malloc(nFileSize);
	DWORD nSizeRead = 0;
	if (::ReadFile(hFile, m_pFileContent, nFileSize, &nSizeRead, nullptr) == FALSE)
	{
		return E_FAIL;
	}
	if (nFileSize != nSizeRead)
	{
		return E_FAIL;
	}
	*ppData = m_pFileContent;
	*pBytes = nFileSize;
	CloseHandle(hFile);
	return S_OK;
}

HRESULT CShaderHeaderInclude::Close(LPCVOID pData)
{
	if (m_pFileContent)
	{
		free(m_pFileContent);
		m_pFileContent = nullptr;
	}
	return S_OK;
}