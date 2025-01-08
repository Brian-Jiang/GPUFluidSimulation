#include "SetFluidForcesShaderAction.h"

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes

// Inter-Engine includes
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"

namespace PE {
	using namespace Components;
	void SetFluidForcesShaderAction::bindToPipeline(Effect* pCurEffect /* = NULL*/) {

		D3D11Renderer* pD3D11Renderer = static_cast<D3D11Renderer*>(pCurEffect->m_pContext->getGPUScreen());
		ID3D11Device* pDevice = pD3D11Renderer->m_pD3DDevice;
		ID3D11DeviceContext* pDeviceContext = pD3D11Renderer->m_pD3DContext;

		//Effect::setConstantBuffer(pDevice, pDeviceContext, s_pBuffer, 10, &m_data, sizeof(Data));
		D3D11_MAPPED_SUBRESOURCE mappedResource;
		HRESULT hr = pDeviceContext->Map(
			s_pBuffer, // resource
			0, // subresource
			D3D11_MAP_WRITE_DISCARD,
			0,
			&mappedResource);
		assert(SUCCEEDED(hr));

		int size = sizeof(m_data);
		memcpy(mappedResource.pData, &m_data, size);
		pDeviceContext->Unmap(s_pBuffer, 0);

		const UINT slotId = GpuResourceSlot_FluidForcePositionsResource;
		pDeviceContext->VSSetShaderResources(slotId, 1, &s_pSRV);
		pDeviceContext->GSSetShaderResources(slotId, 1, &s_pSRV);
		pDeviceContext->PSSetShaderResources(slotId, 1, &s_pSRV);
		pDeviceContext->CSSetShaderResources(slotId, 1, &s_pSRV);
	}

	void SetFluidForcesShaderAction::unbindFromPipeline(Components::Effect* pCurEffect/* = NULL*/) {}

	void SetFluidForcesShaderAction::releaseData() {}
}