#include "Effect.h"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
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

void ConvertMeshToGeom(const aiMesh* pMesh, FXMMATRIX myMatrix)
{
	if (pMesh->mNumVertices == 0)
		return;
	bool bHasNormal = pMesh->mNormals != nullptr;
	bool bHasTangent = pMesh->mTangents != nullptr;
	bool bHasUV0 = pMesh->mNumUVComponents[0] > 0 && pMesh->mTextureCoords[0] != nullptr;
	VB_Base* pVB = nullptr;
	if (bHasNormal && bHasTangent && bHasUV0)
	{
		switch (pMesh->mNumUVComponents[0])
		{
			case 2:
			{
				aiVector2D* pUVBuffer = new aiVector2D[pMesh->mNumVertices];
				for (unsigned int i = 0; i < pMesh->mNumVertices; ++i)
				{
					const auto& UVW = pMesh->mTextureCoords[0][i];
					pUVBuffer[i].Set(UVW.x, UVW.y);
				}
				VB_PositionNormalTangentUV0* pVB_PNTUV0 = new VB_PositionNormalTangentUV0;
				pVB_PNTUV0->Initialize(pMesh->mVertices, pMesh->mNormals, pMesh->mTangents, pUVBuffer, pMesh->mNumVertices);
				delete[] pUVBuffer;
				pVB = pVB_PNTUV0;
				break;
			}
			default:
				return;
		}
	}
	else if (bHasNormal)
	{
		VB_PositionNormal* pVB_PN = new VB_PositionNormal;
		pVB_PN->Initialize(pMesh->mVertices, pMesh->mNormals, pMesh->mNumVertices);
		pVB = pVB_PN;
	}
	else
	{
		VB_Position* pVB_P = new VB_Position;
		pVB_P->Initialize(pMesh->mVertices, pMesh->mNumVertices);
		pVB = pVB_P;
	}

	ID3D11Buffer* pIndexBuffer = nullptr;
	UINT nNumIndices = 0;
	if (pMesh->mNumFaces > 0 && pMesh->mFaces != nullptr)
	{
		for (unsigned int i = 0; i < pMesh->mNumFaces; ++i)
			nNumIndices += pMesh->mFaces[i].mNumIndices;
		UINT* pIndexBufferSysMem = (UINT*)malloc(sizeof(UINT) * nNumIndices);
		UINT nIndexOffset = 0;
		for (unsigned int i = 0; i < pMesh->mNumFaces; ++i)
		{
			const aiFace& faceRef = pMesh->mFaces[i];
			const UINT* pFaceIndices = faceRef.mIndices;
			for (unsigned int nIndex = 0; nIndex < faceRef.mNumIndices; ++nIndex)
				pIndexBufferSysMem[nIndexOffset + nIndex] = pFaceIndices[nIndex];
			nIndexOffset += faceRef.mNumIndices;
		}

		D3D11_BUFFER_DESC IndexBufferDesc;
		IndexBufferDesc.ByteWidth = sizeof(UINT) * nNumIndices;
		IndexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
		IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
		IndexBufferDesc.CPUAccessFlags = 0;
		IndexBufferDesc.MiscFlags = 0;
		IndexBufferDesc.StructureByteStride = sizeof(UINT);

		D3D11_SUBRESOURCE_DATA IndexData;
		IndexData.pSysMem = pIndexBufferSysMem;
		IndexData.SysMemPitch = IndexData.SysMemSlicePitch = 0;
		HRESULT hr = g_D3DInterface.m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexData, &pIndexBuffer);
		free(pIndexBufferSysMem);
		if (FAILED(hr))
			return;
	}

	AddGeom(pVB, pIndexBuffer, nNumIndices, myMatrix);
}

bool LoadMesh(LPCTSTR lpszFileName)
{
	FileContent TextureContent;
	if (!LoadFile(lpszFileName, TextureContent))
		return false;

	Assimp::Importer imp;
	const aiScene* pLoadedScene = imp.ReadFileFromMemory(TextureContent.m_pContent, TextureContent.m_nLength, aiProcess_ConvertToLeftHanded | aiProcessPreset_TargetRealtime_MaxQuality);	
	if (pLoadedScene != nullptr && pLoadedScene->HasMeshes())
	{
		std::function<void(const aiNode*, const aiMatrix4x4&, aiMesh** const, const int)>
			RecursiveCreateGeom = [&RecursiveCreateGeom](const aiNode* pNode, const aiMatrix4x4& parentMatrix, aiMesh** const pMeshes, const int nNumMeshes)
			{
				aiMatrix4x4 myMatrix = pNode->mTransformation * parentMatrix;
				if (pNode->mNumMeshes > 0)
				{
					XMMATRIX myXMMatrix(
						XMVectorSet(myMatrix.a1, myMatrix.a2, myMatrix.a3, myMatrix.a4),
						XMVectorSet(myMatrix.b1, myMatrix.b2, myMatrix.b3, myMatrix.b4),
						XMVectorSet(myMatrix.c1, myMatrix.c2, myMatrix.c3, myMatrix.c4),
						XMVectorSet(myMatrix.d1, myMatrix.d2, myMatrix.d3, myMatrix.d4)
					);
					for (int i = 0; i < pNode->mNumMeshes; ++i)
					{
						int nMeshIndex = pNode->mMeshes[i];
						if (nMeshIndex >= 0 && nMeshIndex < nNumMeshes)
						{
							// 一个有效的Mesh
							const aiMesh* myMesh = pMeshes[nMeshIndex];
							ConvertMeshToGeom(myMesh, XMMatrixTranspose(myXMMatrix));
						}
					}
				}
				if (pNode->mNumChildren > 0)
				{
					for (int i = 0; i < pNode->mNumChildren; ++i)
					{
						RecursiveCreateGeom(pNode->mChildren[i], myMatrix, pMeshes, nNumMeshes);
					}
				}
			};

		if (pLoadedScene->mRootNode != nullptr)
		{
			RecursiveCreateGeom(pLoadedScene->mRootNode, 
				aiMatrix4x4(1.0f, 0.0f, 0.0f, 0.0f,
							0.0f, 1.0f, 0.0f, 0.0f,
							0.0f, 0.0f, 1.0f, 0.0f,
							0.0f, 0.0f, 0.0f, 1.0f),
				pLoadedScene->mMeshes,
				pLoadedScene->mNumMeshes);
		}
		imp.FreeScene();
		return true;	
	}
	else
	{
		const char* pszErrorStr = imp.GetErrorString();
		imp.FreeScene();
		return false;
	}
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