#include "FluidSetBufferGPU.h"

#include "PrimeEngine/APIAbstraction/Effect/EffectManager.h"

using namespace PE;
using namespace PE::Components;

UINT FluidSetBufferGPU::s_fieldWidth = 0;
UINT FluidSetBufferGPU::s_fieldHeight = 0;

UINT FluidSetBufferGPU::s_bufferSize = 0;
UINT FluidSetBufferGPU::s_numGroups = 0;
UINT FluidSetBufferGPU::s_numGroupsSingle = 0;
UINT FluidSetBufferGPU::s_numGroupsBoundary = 0;

//ID3D11Buffer* FluidSetBufferGPU::s_fluidConstantBuffer = NULL;
ID3D11Buffer* FluidSetBufferGPU::s_velocityBuffer = NULL;
ID3D11Buffer* FluidSetBufferGPU::s_divergenceBuffer = NULL;
ID3D11Buffer* FluidSetBufferGPU::s_pressureBuffer = NULL;
ID3D11Buffer* FluidSetBufferGPU::s_fluidPingpongBuffer = NULL;
//ID3D11Buffer* FluidSetBufferGPU::s_fluidForcePositionsBuffer = NULL;
ID3D11Texture2D* FluidSetBufferGPU::s_velocityTexture = NULL;

ID3D11UnorderedAccessView* FluidSetBufferGPU::s_velocityUAV = NULL;
ID3D11UnorderedAccessView* FluidSetBufferGPU::s_velocityUAV2 = NULL;
ID3D11UnorderedAccessView* FluidSetBufferGPU::s_divergenceUAV = NULL;
ID3D11UnorderedAccessView* FluidSetBufferGPU::s_pressureUAV = NULL;
ID3D11UnorderedAccessView* FluidSetBufferGPU::s_fluidPingpongUAV = NULL;
ID3D11UnorderedAccessView* FluidSetBufferGPU::s_fluidPingpongUAV2 = NULL;
ID3D11UnorderedAccessView* FluidSetBufferGPU::s_fluidForcePositionsUAV = NULL;

ID3D11UnorderedAccessView* FluidSetBufferGPU::s_velocityTextureUAV = NULL;

ID3D11ShaderResourceView* FluidSetBufferGPU::s_velocitySRV = NULL;
ID3D11ShaderResourceView* FluidSetBufferGPU::s_pressureSRV = NULL;
ID3D11ShaderResourceView* FluidSetBufferGPU::s_divergenceSRV = NULL;
ID3D11ShaderResourceView* FluidSetBufferGPU::s_velocityTextureSRV = NULL;
ID3D11ShaderResourceView* FluidSetBufferGPU::s_fluidPingpongSRV = NULL;
//ID3D11ShaderResourceView* FluidSetBufferGPU::s_fluidForcePositionsSRV = NULL;

SetFluidConstantsShaderAction::Data FluidSetBufferGPU::s_constantData;

int FluidSetBufferGPU::forcePositionsCount[MAX_FLUID_FORCES];
Vector2 FluidSetBufferGPU::forcePositions[MAX_FLUID_FORCES];

const UINT FluidSetBufferGPU::c_maxForcePositions = MAX_FLUID_FORCES;
const float FluidSetBufferGPU::c_simulationSpeed = 0.5f;

bool FluidSetBufferGPU::s_willUpdate = false;
bool FluidSetBufferGPU::s_initialized = false;
UINT FluidSetBufferGPU::s_willCancelForce = 0;

void PE::FluidSetBufferGPU::createBuffers(PE::GameContext& ctx, UINT width, UINT height)
{
	D3D11Renderer* pD3D11Renderer = static_cast<D3D11Renderer*>(ctx.getGPUScreen());
	ID3D11Device* pDevice = pD3D11Renderer->m_pD3DDevice;
	ID3D11DeviceContext* pDeviceContext = pD3D11Renderer->m_pD3DContext;

    s_fieldWidth = width;
    s_fieldHeight = height;
    s_bufferSize = width * height;
	PEASSERT(s_fieldWidth % c_numThreadsPerGroup == 0, "Fluid simulation group has to be evenly divided");
	PEASSERT(s_fieldWidth % c_numThreadsPerBoundary == 0, "Fluid simulation boundary group has to be evenly divided");
	s_numGroups = s_fieldWidth / c_numThreadsPerGroup;
    s_numGroupsSingle = s_numGroups * s_numGroups;
    s_numGroupsBoundary = s_fieldWidth / c_numThreadsPerBoundary;

	// Constant buffer for fluid
	{
		//D3D11_BUFFER_DESC cbDesc;
		//cbDesc.Usage = D3D11_USAGE_DYNAMIC; // can only write to it, can't read
		//cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		//cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // make sure we can access it with cpu for writing only
		//cbDesc.MiscFlags = 0;
		//cbDesc.ByteWidth = sizeof(FluidConstantData);

		//HRESULT hr = pDevice->CreateBuffer(&cbDesc, NULL, &s_fluidConstantBuffer);
		//assert(SUCCEEDED(hr), "Error creating fluid constant buffer");

		SetFluidConstantsShaderAction::Data data{};
        data.dimension = s_fieldWidth;
		data.boundaryScale = 1.0f;
		data.timeScale = 0.01f;
		s_constantData = data;

		//Components::Effect::setConstantBuffer(pDevice, pDeviceContext, s_fluidConstantBuffer, 10, &data, sizeof(FluidConstantData));
	}

	// velocity field buffer
    {
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; // specify D3D11_USAGE_DEFAULT if not needed to access with cpu map()
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS; //D3D11_BIND_UNORDERED_ACCESS allows writing to resource and reading from resource at the same draw call
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.ByteWidth = sizeof(float) * 4 * s_bufferSize;
		bufferDesc.StructureByteStride = sizeof(float) * 4;

        float* initData = new float[4 * s_bufferSize];
		for (int i = 0; i < s_bufferSize; ++i) {
            initData[4 * i + 0] = 0.0f;
            initData[4 * i + 1] = 0.0f;
            initData[4 * i + 2] = 0.0f;
            initData[4 * i + 3] = 0.0f;
		}

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = initData;
		vinitData.SysMemPitch = 0;
		vinitData.SysMemSlicePitch = 0;

		HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &vinitData, &s_velocityBuffer);
		PEASSERT(SUCCEEDED(hr), "Error creating velocity field buffer");

		delete[] initData;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;  // since using StructuredBuffer, has to be UNKNOWN
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0; // could specify D3D11_BUFFER_UAV_FLAG_APPEND
		uavDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateUnorderedAccessView(s_velocityBuffer, &uavDesc, &s_velocityUAV);
		PEASSERT(SUCCEEDED(hr), "Error creating velocity field UAV");

		hr = pDevice->CreateUnorderedAccessView(s_velocityBuffer, &uavDesc, &s_velocityUAV2);
		PEASSERT(SUCCEEDED(hr), "Error creating velocity field UAV 2");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementOffset = 0;
		srvDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateShaderResourceView(s_velocityBuffer, &srvDesc, &s_velocitySRV);
		PEASSERT(SUCCEEDED(hr), "Error creating velocity srv");
    }

	// divergence buffer
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; // specify D3D11_USAGE_DEFAULT if not needed to access with cpu map()
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS; //D3D11_BIND_UNORDERED_ACCESS allows writing to resource and reading from resource at the same draw call
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.ByteWidth = sizeof(float) * 4 * s_bufferSize;
		bufferDesc.StructureByteStride = sizeof(float) * 4;

		float* initData = new float[4 * s_bufferSize];
		for (int i = 0; i < s_bufferSize; ++i) {
			initData[4 * i + 0] = 0.0f;
			initData[4 * i + 1] = 0.0f;
			initData[4 * i + 2] = 0.0f;
			initData[4 * i + 3] = 0.0f;
		}

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = initData;
		vinitData.SysMemPitch = 0;
		vinitData.SysMemSlicePitch = 0;

		HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &vinitData, &s_divergenceBuffer);
		PEASSERT(SUCCEEDED(hr), "Error creating divergence field buffer");

		delete[] initData;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;  // since using StructuredBuffer, has to be UNKNOWN
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0; // could specify D3D11_BUFFER_UAV_FLAG_APPEND
		uavDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateUnorderedAccessView(s_divergenceBuffer, &uavDesc, &s_divergenceUAV);
		PEASSERT(SUCCEEDED(hr), "Error creating divergence field UAV");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementOffset = 0;
		srvDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateShaderResourceView(s_divergenceBuffer, &srvDesc, &s_divergenceSRV);
		PEASSERT(SUCCEEDED(hr), "Error creating divergence srv");
	}

	// pressure buffer
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; // specify D3D11_USAGE_DEFAULT if not needed to access with cpu map()
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS; //D3D11_BIND_UNORDERED_ACCESS allows writing to resource and reading from resource at the same draw call
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.ByteWidth = sizeof(float) * 4 * s_bufferSize;
		bufferDesc.StructureByteStride = sizeof(float) * 4;

		float* initData = new float[4 * s_bufferSize];
		for (int i = 0; i < s_bufferSize; ++i) {
			initData[4 * i + 0] = 0.0f;
			initData[4 * i + 1] = 0.0f;
			initData[4 * i + 2] = 0.0f;
			initData[4 * i + 3] = 0.0f;
		}

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = initData;
		vinitData.SysMemPitch = 0;
		vinitData.SysMemSlicePitch = 0;

		HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &vinitData, &s_pressureBuffer);
		PEASSERT(SUCCEEDED(hr), "Error creating pressure field buffer");

		delete[] initData;

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;  // since using StructuredBuffer, has to be UNKNOWN
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0; // could specify D3D11_BUFFER_UAV_FLAG_APPEND
		uavDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateUnorderedAccessView(s_pressureBuffer, &uavDesc, &s_pressureUAV);
		PEASSERT(SUCCEEDED(hr), "Error creating pressure field UAV");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementOffset = 0;
		srvDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateShaderResourceView(s_pressureBuffer, &srvDesc, &s_pressureSRV);
		PEASSERT(SUCCEEDED(hr), "Error creating pressure srv");
	}

	// pingpong buffer
    // only need one for pingpong, other 2 in solver already set else where(divergence and pressure)
	{
		D3D11_BUFFER_DESC bufferDesc;
		bufferDesc.Usage = D3D11_USAGE_DEFAULT; // specify D3D11_USAGE_DEFAULT if not needed to access with cpu map()
		bufferDesc.CPUAccessFlags = 0;
		bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS; //D3D11_BIND_UNORDERED_ACCESS allows writing to resource and reading from resource at the same draw call
		bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bufferDesc.ByteWidth = sizeof(float) * 4 * s_bufferSize;
		bufferDesc.StructureByteStride = sizeof(float) * 4;

		float* initData = new float[4 * s_bufferSize];
		for (int i = 0; i < s_bufferSize; ++i) {
			initData[4 * i + 0] = 0.0f;
			initData[4 * i + 1] = 0.0f;
			initData[4 * i + 2] = 0.0f;
			initData[4 * i + 3] = 0.0f;
		}

		D3D11_SUBRESOURCE_DATA vinitData;
		vinitData.pSysMem = initData;
		vinitData.SysMemPitch = 0;
		vinitData.SysMemSlicePitch = 0;

		HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &vinitData, &s_fluidPingpongBuffer);
		PEASSERT(SUCCEEDED(hr), "Error creating pingpong buffer");

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;  // since using StructuredBuffer, has to be UNKNOWN
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0; // could specify D3D11_BUFFER_UAV_FLAG_APPEND
		uavDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateUnorderedAccessView(s_fluidPingpongBuffer, &uavDesc, &s_fluidPingpongUAV);
		PEASSERT(SUCCEEDED(hr), "Error creating pingpong UAV");

		hr = pDevice->CreateUnorderedAccessView(s_fluidPingpongBuffer, &uavDesc, &s_fluidPingpongUAV2);
		PEASSERT(SUCCEEDED(hr), "Error creating pingpong UAV 2");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
		srvDesc.Buffer.ElementOffset = 0;
		srvDesc.Buffer.NumElements = s_bufferSize;

		hr = pDevice->CreateShaderResourceView(s_fluidPingpongBuffer, &srvDesc, &s_fluidPingpongSRV);
		PEASSERT(SUCCEEDED(hr), "Error creating pingpong srv");
	}

	// Force positions
	//{
	//	D3D11_BUFFER_DESC bufferDesc;
	//	bufferDesc.Usage = D3D11_USAGE_DYNAMIC; // specify D3D11_USAGE_DEFAULT if not needed to access with cpu map()
	//	bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	//	bufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS; //D3D11_BIND_UNORDERED_ACCESS allows writing to resource and reading from resource at the same draw call
	//	bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	//	bufferDesc.ByteWidth = sizeof(float) * 2 * c_maxForcePositions;
	//	bufferDesc.StructureByteStride = sizeof(float) * 2;

	//	float* initData = new float[2 * c_maxForcePositions];
	//	for (int i = 0; i < c_maxForcePositions; ++i) {
	//		initData[2 * i + 0] = -100.0f;
	//		initData[2 * i + 1] = -100.0f;
	//	}

	//	D3D11_SUBRESOURCE_DATA vinitData;
	//	vinitData.pSysMem = initData;
	//	vinitData.SysMemPitch = 0;
	//	vinitData.SysMemSlicePitch = 0;

	//	HRESULT hr = pDevice->CreateBuffer(&bufferDesc, &vinitData, &s_fluidForcePositionsBuffer);
	//	PEASSERT(SUCCEEDED(hr), "Error creating force positions buffer");

	//	D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
	//	uavDesc.Format = DXGI_FORMAT_UNKNOWN;  // since using StructuredBuffer, has to be UNKNOWN
	//	uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
	//	uavDesc.Buffer.FirstElement = 0;
	//	uavDesc.Buffer.Flags = 0; // could specify D3D11_BUFFER_UAV_FLAG_APPEND
	//	uavDesc.Buffer.NumElements = c_maxForcePositions;

	//	hr = pDevice->CreateUnorderedAccessView(s_fluidForcePositionsBuffer, &uavDesc, &s_fluidForcePositionsUAV);
	//	PEASSERT(SUCCEEDED(hr), "Error creating force positions UAV");

	//	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	//	srvDesc.Format = DXGI_FORMAT_UNKNOWN;
	//	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
	//	srvDesc.Buffer.ElementOffset = 0;
	//	srvDesc.Buffer.NumElements = c_maxForcePositions;

	//	hr = pDevice->CreateShaderResourceView(s_fluidForcePositionsBuffer, &srvDesc, &s_fluidForcePositionsSRV);
	//	PEASSERT(SUCCEEDED(hr), "Error creating force positions srv");
	//}

	// texture buffer
	{
		D3D11_TEXTURE2D_DESC textureDesc = {};
		textureDesc.Width = width;  // Texture width
		textureDesc.Height = height; // Texture height
		textureDesc.MipLevels = 1; // Single mip level
		textureDesc.ArraySize = 1; // Single texture
		textureDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT; // Float format for UAV
		textureDesc.SampleDesc.Count = 1; // No multisampling
		textureDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = 0;
		textureDesc.MiscFlags = 0;

        HRESULT hr = pDevice->CreateTexture2D(&textureDesc, nullptr, &s_velocityTexture);
		PEASSERT(SUCCEEDED(hr), "Error creating velocity texture");

		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = textureDesc.Format;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;

		hr = pDevice->CreateUnorderedAccessView(s_velocityTexture, &uavDesc, &s_velocityTextureUAV);
		PEASSERT(SUCCEEDED(hr), "Error creating velocity texture UAV");

		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = textureDesc.Format;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = 1;
		srvDesc.Texture2D.MostDetailedMip = 0;

		hr = pDevice->CreateShaderResourceView(s_velocityTexture, &srvDesc, &s_velocityTextureSRV);
		PEASSERT(SUCCEEDED(hr), "Error creating velocity texture srv");
	}

	s_initialized = true;
}

void PE::FluidSetBufferGPU::updateBoundaryScale(PE::GameContext& ctx, Components::DrawList* pDrawList, float boundaryScale)
{
    //D3D11Renderer* pD3D11Renderer = static_cast<D3D11Renderer*>(ctx.getGPUScreen());
    //ID3D11Device* pDevice = pD3D11Renderer->m_pD3DDevice;
    //ID3D11DeviceContext* pDeviceContext = pD3D11Renderer->m_pD3DContext;

	auto& data = s_constantData;
    data.boundaryScale = boundaryScale;

	Handle& hsvFluid = pDrawList->nextShaderValue();
	hsvFluid = Handle("RAW_DATA", sizeof(SetFluidConstantsShaderAction));
	SetFluidConstantsShaderAction* psvFluid = new(hsvFluid) SetFluidConstantsShaderAction();
	psvFluid->m_data = data;

    //Components::Effect::setConstantBuffer(pDevice, pDeviceContext, s_fluidConstantBuffer, 10, &data, sizeof(FluidConstantData));
}

void PE::FluidSetBufferGPU::updateForceFrame(PE::GameContext& ctx, Components::DrawList* pDrawList)
{
	for (size_t i = 0; i < MAX_FLUID_FORCES; i++) {
		int v = forcePositionsCount[i];
		if (v > 0) {
			--forcePositionsCount[i];
		}

		if (v == 0) {
			forcePositions[i] = Vector2(-100.f, -100.f);
		}
	}
	Handle& hsvFluid = pDrawList->nextShaderValue();
	hsvFluid = Handle("RAW_DATA", sizeof(SetFluidForcesShaderAction));
	SetFluidForcesShaderAction* psvFluid = new(hsvFluid) SetFluidForcesShaderAction();
	SetFluidForcesShaderAction::Data data;
	memcpy(data.forcePositions, forcePositions, sizeof(forcePositions));
	psvFluid->m_data = data;
	//psvFluid->m_data = data;
}

void PE::FluidSetBufferGPU::bindConstantBuffer(Components::DrawList* pDrawList)
{
	auto& data = s_constantData;

	Handle& hsvFluid = pDrawList->nextShaderValue();
	hsvFluid = Handle("RAW_DATA", sizeof(SetFluidConstantsShaderAction));
	SetFluidConstantsShaderAction* psvFluid = new(hsvFluid) SetFluidConstantsShaderAction();
	psvFluid->m_data = data;
}

void PE::FluidSetBufferGPU::updateForce(Vector2 forcePosition, Vector2 forceDirection, float forceScale)
{
	//if (s_willCancelForce > 0) {
	//	return;
	//}

	// force position 0 to 1

	auto& data = s_constantData;
    //data.forcePosition = forcePosition;
    //data.forceDirection = forceDirection;
	data.forceScale = forceScale;

	int i = 0;
	for (; i < MAX_FLUID_FORCES; i++) {
		if (forcePositionsCount[i] <= 0) {
			break;
		}
	}

	if (i < MAX_FLUID_FORCES) {
		forcePositionsCount[i] = 5;
		forcePositions[i] = forcePosition;
	}

	//s_willUpdate = true;

	//Handle& hsvFluid = pDrawList->nextShaderValue();
	//hsvFluid = Handle("RAW_DATA", sizeof(SetFluidConstantsShaderAction));
	//SetFluidConstantsShaderAction* psvFluid = new(hsvFluid) SetFluidConstantsShaderAction();
	//psvFluid->m_data = data;

 //   Components::Effect::setConstantBuffer(pDevice, pDeviceContext, s_fluidConstantBuffer, 10, &data, sizeof(FluidConstantData));

	//s_willCancelForce = 10;
}

void PE::FluidSetBufferGPU::setWillUpdate()
{
    //s_willUpdate = true;
}

void PE::FluidSetBufferGPU::handleFluidSimulatorDrawCalls(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent)
{
	if (!s_initialized) {
		return;
	}

	//if (!s_willUpdate) {
	//	s_willUpdate = true;
	//	return;
	//}

	//s_willUpdate = false;

	pDrawList->beginDrawCallRecord("Fluid - UnbindRT");
	pDrawList->setUnbindRT();

	handleExternalForces(ctx, pDrawList, pDrawEvent);
	handleBoundaryVeloctiy(ctx, pDrawList, pDrawEvent);
	handleProject(ctx, pDrawList, pDrawEvent);

    handleDiffusion(ctx, pDrawList, pDrawEvent);
	handleBoundaryVeloctiy(ctx, pDrawList, pDrawEvent);
	handleProject(ctx, pDrawList, pDrawEvent);

	handleAdvect(ctx, pDrawList, pDrawEvent);
    handleBoundaryVeloctiy(ctx, pDrawList, pDrawEvent);
    handleProject(ctx, pDrawList, pDrawEvent);

	updateForceFrame(ctx, pDrawList);

	pDrawList->beginDrawCallRecord("Fluid - RebindRT");
	pDrawList->setRebindRT();

	pDrawList->beginDrawCallRecord("Fluid - Texture - Copy");
	pDrawList->setVertexBuffer(Handle());
	Handle hEffect = EffectManager::Instance()->getEffectHandle("CopyTexture_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_CopySrc, s_pressureSRV);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidVelocityTextureUAV, s_velocityTextureUAV);
	pDrawList->setDispatchParams(Vector3(s_numGroups, s_numGroups, 1));

	
	//if (s_willCancelForce <= 0) {
	//	updateForce(Vector2(0.0f, 0.0f), Vector2(0.0f, 0.0f), 0.0f);
	//}
	//else {
	//	--s_willCancelForce;
	//}
    
}

void PE::FluidSetBufferGPU::handleFluidSurfaceResources(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent) {
	bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidPressureResource, s_pressureSRV);
	bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidVelocityResource, s_velocitySRV);
	//bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, );
	{
		TextureGPU textureGPU = EffectManager::Instance()->m_firstPassTextureGPU;
		Handle& hSV = pDrawList->nextShaderValue(sizeof(SA_Bind_Resource));
		SA_Bind_Resource* pSetTextureAction = new(hSV) SA_Bind_Resource(ctx, ctx.getDefaultMemoryArena(), DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, textureGPU.m_samplerState, textureGPU.m_pShaderResourceView);

		//pSetTextureAction->set(
		//	slot,
		//	SamplerState_NotNeeded,
		//	srv,
		//	"srv"
		//);
	}
}

void PE::FluidSetBufferGPU::handleProject(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent) {
	pDrawList->beginDrawCallRecord("Fluid - Project - Divergence");
	pDrawList->setVertexBuffer(Handle());
	Handle hEffect = EffectManager::Instance()->getEffectHandle("Fluid_Divergence_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidVelocityUAV, s_velocityUAV);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidDivergenceResultUAV, s_divergenceUAV);
	pDrawList->setDispatchParams(Vector3(s_numGroups, s_numGroups, 1));

    handleClearBuffer(ctx, pDrawList, pDrawEvent, s_pressureUAV);

	updateSolverFactors(ctx, pDrawList, -1.0f, 0.25f);
	for (int i = 0; i < c_solverIteration; i++) {
		pDrawList->beginDrawCallRecord("Fluid - Project - Solver");
		pDrawList->setVertexBuffer(Handle());
		hEffect = EffectManager::Instance()->getEffectHandle("Fluid_Solver_CS_Tech");
		pDrawList->setEffect(hEffect);
		pDrawList->setDispatchParams(Vector3(s_numGroups, s_numGroups, 1));
		bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolverConstantUAV, s_divergenceUAV);
		if (i % 2 == 0) {
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver1UAV, s_pressureUAV);
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver2UAV, s_fluidPingpongUAV);
			handleBoundaryPressure(ctx, pDrawList, pDrawEvent, s_fluidPingpongUAV);
		}
		else {
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver1UAV, s_fluidPingpongUAV);
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver2UAV, s_pressureUAV);
			handleBoundaryPressure(ctx, pDrawList, pDrawEvent, s_pressureUAV);
		}
	}

	handleClearBuffer(ctx, pDrawList, pDrawEvent, s_fluidPingpongUAV);

	pDrawList->beginDrawCallRecord("Fluid - Project - DivergenceFree");
	pDrawList->setVertexBuffer(Handle());
	hEffect = EffectManager::Instance()->getEffectHandle("Fluid_DivergenceFree_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidVelocityUAV, s_velocityUAV);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidPressureUAV, s_pressureUAV);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidDivergenceFreeResultUAV, s_fluidPingpongUAV);
	pDrawList->setDispatchParams(Vector3(s_numGroups, s_numGroups, 1));

	pDrawList->beginDrawCallRecord("Fluid - Project - Copy");
	pDrawList->setVertexBuffer(Handle());
	hEffect = EffectManager::Instance()->getEffectHandle("CopyBuffer_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_CopySrc, s_fluidPingpongSRV);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_CopyDst, s_velocityUAV);
	pDrawList->setDispatchParams(Vector3(s_numGroupsSingle, 1, 1));
}

void PE::FluidSetBufferGPU::handleBoundaryVeloctiy(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent) {
	pDrawList->beginDrawCallRecord("Fluid - Boundary - Velocity");
	pDrawList->setVertexBuffer(Handle());
	Handle hEffect = EffectManager::Instance()->getEffectHandle("Fluid_Boundary_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidBoundaryUAV, s_velocityUAV);
	updateBoundaryScale(ctx, pDrawList, -1.0f);
	pDrawList->setDispatchParams(Vector3(1, s_numGroupsBoundary, 1));
}

void PE::FluidSetBufferGPU::handleBoundaryPressure(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent, ID3D11UnorderedAccessView* uav) {
	pDrawList->beginDrawCallRecord("Fluid - Boundary - Pressure");
	pDrawList->setVertexBuffer(Handle());
	Handle hEffect = EffectManager::Instance()->getEffectHandle("Fluid_Boundary_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidBoundaryUAV, uav);
	updateBoundaryScale(ctx, pDrawList, 1.0f);
	pDrawList->setDispatchParams(Vector3(1, s_numGroupsBoundary, 1));
}

void PE::FluidSetBufferGPU::handleExternalForces(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent) {
	//if (s_willUpdate) {
	//	auto& data = s_constantData;

	//	Handle& hsvFluid = pDrawList->nextShaderValue();
	//	hsvFluid = Handle("RAW_DATA", sizeof(SetFluidConstantsShaderAction));
	//	SetFluidConstantsShaderAction* psvFluid = new(hsvFluid) SetFluidConstantsShaderAction();
	//	psvFluid->m_data = data;

	//	s_willUpdate = false;
	//}
	
	//Handle& hSV = pDrawList->nextShaderValue(sizeof(SA_Bind_Resource));
	//SA_Bind_Resource* pSetTextureAction = new(hSV) SA_Bind_Resource(ctx, ctx.getDefaultMemoryArena(), DIFFUSE_TEXTURE_2D_SAMPLER_SLOT, textureGPU.m_samplerState, textureGPU.m_pShaderResourceView);

	
	pDrawList->beginDrawCallRecord("Fluid - External");
	pDrawList->setVertexBuffer(Handle());
	bindConstantBuffer(pDrawList);
	Handle hEffect = EffectManager::Instance()->getEffectHandle("Fluid_ExternalForce_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidVelocityUAV, s_velocityUAV);
	D3D11Renderer* renderer = (D3D11Renderer*)ctx.getGPUScreen();
	ID3D11ShaderResourceView* srv = renderer->m_pDepthStencilSRV;
    bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_DepthStencilResource, srv);
	//bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidForcePositionsResource, s_fluidForcePositionsSRV);
    //updateForce(ctx, Vector2(0.8f, 0.2f), Vector2(9.5f, 4.5f));

	pDrawList->setDispatchParams(Vector3(s_numGroups, s_numGroups, 1));
}

void PE::FluidSetBufferGPU::handleAdvect(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent) {
	pDrawList->beginDrawCallRecord("Fluid - Advect - Calculate");
	pDrawList->setVertexBuffer(Handle());
	Handle hEffect = EffectManager::Instance()->getEffectHandle("Fluid_Advection_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidVelocityResource, s_velocitySRV);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidAdvectionResultUAV, s_fluidPingpongUAV);
	pDrawList->setDispatchParams(Vector3(s_numGroups, s_numGroups, 1));

	pDrawList->beginDrawCallRecord("Fluid - Advect - Copy");
	pDrawList->setVertexBuffer(Handle());
	hEffect = EffectManager::Instance()->getEffectHandle("CopyBuffer_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindSRV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_CopySrc, s_fluidPingpongSRV);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_CopyDst, s_velocityUAV);
	pDrawList->setDispatchParams(Vector3(s_numGroupsSingle, 1, 1));

	handleClearBuffer(ctx, pDrawList, pDrawEvent, s_fluidPingpongUAV);
}

void PE::FluidSetBufferGPU::handleDiffusion(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent)
{
	// todo uav re-bind issue
	auto& data = s_constantData;
	float viscocity = 0.0001f;
	float timestep = data.timeScale;
	updateSolverFactors(ctx, pDrawList, 1.0f / (viscocity * timestep), (viscocity * timestep) / (1.0f + 4.0f * (viscocity * timestep)));
	for (int i = 0; i < c_solverIteration; i++) {
		pDrawList->beginDrawCallRecord("Fluid - Diffusion - Solver");
		pDrawList->setVertexBuffer(Handle());
		Handle hEffect = EffectManager::Instance()->getEffectHandle("Fluid_Solver_Velocity_CS_Tech");
		pDrawList->setEffect(hEffect);
		pDrawList->setDispatchParams(Vector3(s_numGroups, s_numGroups, 1));
		if (i % 2 == 0) {
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver1UAV, s_velocityUAV);
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver2UAV, s_fluidPingpongUAV);
		}
		else {
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver1UAV, s_fluidPingpongUAV);
			bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_FluidSolver2UAV, s_velocityUAV);
		}
	}

	handleClearBuffer(ctx, pDrawList, pDrawEvent, s_fluidPingpongUAV);
}

void PE::FluidSetBufferGPU::handleClearBuffer(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent, ID3D11UnorderedAccessView* uav) {
	pDrawList->beginDrawCallRecord("Fluid - ClearBuffer");
	pDrawList->setVertexBuffer(Handle());
	Handle hEffect = EffectManager::Instance()->getEffectHandle("ClearBuffer_CS_Tech");
	pDrawList->setEffect(hEffect);
	bindUAV(ctx, ctx.getDefaultMemoryArena(), pDrawList, GpuResourceSlot_ClearSrc, uav);
	pDrawList->setDispatchParams(Vector3(s_numGroupsSingle, 1, 1));
}

void PE::FluidSetBufferGPU::updateSolverFactors(PE::GameContext& ctx, Components::DrawList* pDrawList, float centerFactor, float diagonalFactor)
{
	//D3D11Renderer* pD3D11Renderer = static_cast<D3D11Renderer*>(ctx.getGPUScreen());
	//ID3D11Device* pDevice = pD3D11Renderer->m_pD3DDevice;
	//ID3D11DeviceContext* pDeviceContext = pD3D11Renderer->m_pD3DContext;

	auto& data = s_constantData;
	data.solverCenterFactor = centerFactor;
    data.solverDiagonalFactor = diagonalFactor;

	Handle& hsvFluid = pDrawList->nextShaderValue();
	hsvFluid = Handle("RAW_DATA", sizeof(SetFluidConstantsShaderAction));
	SetFluidConstantsShaderAction* psvFluid = new(hsvFluid) SetFluidConstantsShaderAction();
	psvFluid->m_data = data;

	//Components::Effect::setConstantBuffer(pDevice, pDeviceContext, s_fluidConstantBuffer, 10, &data, sizeof(FluidConstantData));
}

void PE::FluidSetBufferGPU::bindUAV(PE::GameContext& context, PE::MemoryArena arena, Components::DrawList* pDrawList, EGpuResourceSlot slot, ID3D11UnorderedAccessView* uav) {
	Handle& hSV = pDrawList->nextShaderValue(sizeof(SA_Bind_Resource));
	SA_Bind_Resource* pSetTextureAction = new(hSV) SA_Bind_Resource(context, arena);

	PEASSERT(uav != NULL, "uav not set");
	pSetTextureAction->set(
		slot,
		uav,
		nullptr,
		nullptr,
		0,
		"uav"
	);
}

void PE::FluidSetBufferGPU::bindSRV(PE::GameContext& context, PE::MemoryArena arena, Components::DrawList* pDrawList, EGpuResourceSlot slot, ID3D11ShaderResourceView* srv) {
	Handle& hSV = pDrawList->nextShaderValue(sizeof(SA_Bind_Resource));
	SA_Bind_Resource* pSetTextureAction = new(hSV) SA_Bind_Resource(context, arena);

	PEASSERT(srv != NULL, "srv not set");
	pSetTextureAction->set(
		slot,
		SamplerState_NotNeeded,
		srv,
		"srv"
	);
}

//Effect* PE::FluidSetBufferGPU::getEffect(PE::GameContext& ctx, const char* effectFilename) {
//	Handle hEffect = EffectManager::Instance()->getEffectHandle(effectFilename);
//	Effect* pEffect = hEffect.getObject<Effect>();
//	return pEffect;
//}
