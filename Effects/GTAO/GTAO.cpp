#include "../../Effect.h"

D3D11DSSWrapper g_pDepthPassDSS;
D3D11DSSWrapper g_pRenderPassDSS;
D3D11BSWrapper g_pBlendState;
D3D11RSWrapper g_pRasterizerState;

bool CreateBlendStates()
{
	g_pBlendState = TStaticBlendState<>().GetBlendState();
	return true;
}

bool CreateRasterizerStates()
{
	g_pRasterizerState = TStaticRasterizerState<D3D11_FILL_SOLID, D3D11_CULL_BACK, FALSE>().GetRasterizerState();
	return true;
}

bool CreateDepthStencilStates()
{
	g_pDepthPassDSS = TStaticDepthStencilState<>().GetDepthStencilState();
    g_pRenderPassDSS = TStaticDepthStencilState<FALSE, D3D11_COMPARISON_LESS_EQUAL>().GetDepthStencilState();
	return true;
}

bool LoadResources()
{
	if (!LoadMesh(_T("Resources\\vbt5gh.fbx")))
		return false;
	return true;
}

#ifdef GTAO_EXPORT_SYMBOL
	#define GTAO_API __declspec(dllexport)
#else
	#define GTAO_API
#endif

extern "C" 
{
	GTAO_API void InitializeEffectInstance(Effect_Instance* pEffect)
    {

    }

    GTAO_API void UninitializeEffectInstance()
    {

    }
}