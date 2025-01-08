#ifndef __PYENGINE_2_0_BindResourceToGpu_SHADER_ACTION_H__
#define __PYENGINE_2_0_BindResourceToGpu_SHADER_ACTION_H__

#define NOMINMAX
// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes


// Inter-Engine includes
#include "PrimeEngine/APIAbstraction/Texture/SamplerState.h"

// Sibling/Children includes
#include "ShaderAction.h"

namespace PE {

namespace Components{
struct Effect;
}

// mapping of samplers and textures to bind to gpu
enum EGpuResourceSlot {
	// per object group
	DEPTHMAP_TEXTURE_2D_SAMPLER_SLOT = 0, // register(t0)
	SHADOWMAP_TEXTURE_2D_SAMPLER_SLOT = 1, // register(t1)

	// per object/draw call
	DIFFUSE_TEXTURE_2D_SAMPLER_SLOT = 2, // register(t2)
	DIFFUSE_TEXTURE_2D_ARRAY_SLOT = 2, // register(t2)
	DIFFUSE_TEXTURE_CUBE_SLOT = 2, // register(t2)


	GLOW_TEXTURE_2D_SAMPLER_SLOT = 3,
	BUMPMAP_TEXTURE_2D_SAMPLER_SLOT = 4,
	SPECULAR_TEXTURE_2D_SAMPLER_SLOT = 5,

	ADDITIONAL_DIFFUSE_TEXTURE_2D_SAMPLER_SLOT = 6,
	DIFFUSE_BLUR_TEXTURE_2D_SAMPLER_SLOT = 6,

	WIND_TEXTURE_2D_SAMPLER_SLOT = 7,
	RANDOM_TEXTURE_1D_SAMPLER_SLOT = 7,

	NUM_GPU_SAMPLERS, // how many samplers can be bound at the same time

	// we can surpass NUM_GPU_SAMPLERS since below buffer ids dont require samplers

	GpuResourceSlot_SingleObjectPalette_ConstResource = 65,
	GpuResourceSlot_AnimSetData_ConstResource = 66, // register(t66)
	GpuResourceSlot_AnimationCSResultResource = 67,
	GpuResourceSlot_AnimationCSReduceResultResource = 67,
	GpuResourceSlot_SkeletonStructure_ConstResource = 68,
	GpuResourceSlot_SkeletonBindInverses_ConstResource = 69,

	GpuResourceSlot_ParticleEmitterResource = 70,
	GpuResourceSlot_ParticleCSResult1Resource = 71,
	GpuResourceSlot_ParticleCSResult2Resource = 72,

	GpuResourceSlot_FluidVelocityResource = 73,
	GpuResourceSlot_FluidPressureResource = 74,
	GpuResourceSlot_FluidForcePositionsResource = 75,
	GpuResourceSlot_DepthStencilResource = 76,

	GpuResourceSlot_CopySrc = 100,

	GpuResourceSlot_FirstUAV = 1024, // all values past thsi will be subreacted 1024
	GpuResourceSlot_AnimationCSUAV = GpuResourceSlot_FirstUAV, // register(u0)
	GpuResourceSlot_AnimationCSReduceUAV = GpuResourceSlot_FirstUAV, // register(u0)
	GpuResourceSlot_ParticleCS1UAV = 1025,  // register(u1)
	GpuResourceSlot_ParticleCS2UAV = 1026,  // register(u2)
	GpuResourceSlot_ParticleDrawArgsUAV = 1027,  // register(u3)
	GpuResourceSlot_ParticleDispatchArgsUAV = 1028,  // register(u4)

	GpuResourceSlot_FluidVelocityUAV = GpuResourceSlot_FirstUAV + 5,  // register(u5)
	GpuResourceSlot_FluidAdvectionResultUAV = GpuResourceSlot_FirstUAV + 6,  // register(u6)
	GpuResourceSlot_FluidDivergenceResultUAV = GpuResourceSlot_FirstUAV + 7,  // register(u7)
	GpuResourceSlot_FluidPressureUAV = GpuResourceSlot_FirstUAV + 8,  // register(u8)
	GpuResourceSlot_FluidDivergenceFreeResultUAV = GpuResourceSlot_FirstUAV + 9,  // register(u9)
	GpuResourceSlot_FluidBoundaryUAV = GpuResourceSlot_FirstUAV + 10,  // register(u10)

	GpuResourceSlot_FluidSolverConstantUAV = GpuResourceSlot_FirstUAV + 20,  // register(u20)
	GpuResourceSlot_FluidSolver1UAV = GpuResourceSlot_FirstUAV + 21,  // register(u21)
	GpuResourceSlot_FluidSolver2UAV = GpuResourceSlot_FirstUAV + 22,  // register(u22)

	GpuResourceSlot_FluidVelocityTextureUAV = GpuResourceSlot_FirstUAV + 23,  // register(u23)

	GpuResourceSlot_CopyDst = GpuResourceSlot_FirstUAV + 30,  // register(u30)
	GpuResourceSlot_ClearSrc = GpuResourceSlot_FirstUAV + 31,  // register(u31)
};


struct SA_Bind_Resource : ShaderAction
{
	virtual ~SA_Bind_Resource() {}

	virtual void bindToPipeline(Components::Effect *pCurEffect = NULL) ;
	virtual void unbindFromPipeline(Components::Effect *pCurEffect = NULL) ;
	virtual void releaseData() ;

	SA_Bind_Resource(PE::GameContext &context, PE::MemoryArena arena);
	SA_Bind_Resource(
		PE::GameContext &context, PE::MemoryArena arena,
		EGpuResourceSlot bufferId,
		ESamplerState samplerState,
		API_CHOOSE_DX11_DX9_OGL_PSVITA_NO_PAREN(ID3D11ShaderResourceView* pShaderResource, IDirect3DTexture9* pTextureResource, GLuint texture, void texture),
		const char *dbgStr = NULL
	);

	void set(
		EGpuResourceSlot bufferId,
		ESamplerState samplerState,
		API_CHOOSE_DX11_DX9_OGL_PSVITA_NO_PAREN(ID3D11ShaderResourceView* pShaderResource, IDirect3DTexture9* pTextureResource, GLuint texture, void texture),
		const char *dbgStr = NULL
	);

	//compute version
#if APIABSTRACTION_D3D11
	void set(
		EGpuResourceSlot bufferId,
		ID3D11UnorderedAccessView* pShaderResource,
		ID3D11Buffer *m_pCopyBackBuffer,
		void *pCopyBackMem,
		int memSize,
		const char *dbgStr = NULL
		);
#endif

    void setUnbindRT() { m_unbindRT = true; }
    void setRebindRT() { m_rebindRT = true; }


#if !APIABSTRACTION_D3D9 && !APIABSTRACTION_D3D11 && !PE_PLAT_IS_PSVITA
	// in non dx9/dx11 and not vita we need to track sampler ids per effect:

#if PE_PLAT_IS_PSVITA

#elif APIABSTRACTION_IOS
	typedef GLuint ApiBindingType;
#else
	typedef CGparameter ApiBindingType;
#endif

	/// Used to track which gpu register texture id actually maps to
	struct PerEffectBindIds
	{
		ApiBindingType m_samplerParameters[NUM_GPU_SAMPLERS];
		const char *m_samplerParameterNames[NUM_GPU_SAMPLERS];

		void initialize(Components::Effect *pEffect);
	};
#	endif

private:

	PE::MemoryArena m_arena; PE::GameContext *m_pContext;
	const char *m_dbgStr;
	ESamplerState m_samplerState;
	PrimitiveTypes::UInt32 m_bufferId;
#	if APIABSTRACTION_D3D9
		IDirect3DTexture9* m_pTextureResource;
#	elif APIABSTRACTION_D3D11
	union{
		ID3D11ShaderResourceView* m_pShaderResource;
		ID3D11UnorderedAccessView *m_pShaderUAV;
	};

	// debug functionality
	ID3D11Buffer *m_pCopyBackBuffer;
	void *m_pCompyBackMem;
	int m_memSize;
	bool m_unbindRT;
    bool m_rebindRT;
#	elif APIABSTRACTION_OGL
		GLuint m_texture;
#elif PE_PLAT_IS_PSVITA

#endif
		
};

}; // namespace PE
#endif
