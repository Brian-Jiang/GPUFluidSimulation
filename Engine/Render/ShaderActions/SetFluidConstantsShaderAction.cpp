#include "SetFluidConstantsShaderAction.h"

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"

namespace PE {
	using namespace Components;

	void SetFluidConstantsShaderAction::bindToPipeline(Effect* pCurEffect /* = NULL*/) {

		D3D11Renderer* pD3D11Renderer = static_cast<D3D11Renderer*>(pCurEffect->m_pContext->getGPUScreen());
		ID3D11Device* pDevice = pD3D11Renderer->m_pD3DDevice;
		ID3D11DeviceContext* pDeviceContext = pD3D11Renderer->m_pD3DContext;

		Effect::setConstantBuffer(pDevice, pDeviceContext, s_pBuffer, 10, &m_data, sizeof(Data));
	}

	void SetFluidConstantsShaderAction::unbindFromPipeline(Components::Effect* pCurEffect/* = NULL*/) {}

	void SetFluidConstantsShaderAction::releaseData() {}
}