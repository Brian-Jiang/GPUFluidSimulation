#pragma once

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Inter-Engine includes
#include "PrimeEngine/Render/IRenderer.h"
#include "PrimeEngine/Geometry/PositionBufferCPU/PositionBufferCPU.h"
#include "PrimeEngine/Geometry/TangentBufferCPU/TangentBufferCPU.h"
#include "PrimeEngine/Geometry/TexCoordBufferCPU/TexCoordBufferCPU.h"
#include "PrimeEngine/Geometry/ParticleSystemCPU/ParticleBufferCPU.h"
#include "PrimeEngine/Scene/MeshInstance.h"
#include "PrimeEngine/Scene/MeshManager.h"
#include "PrimeEngine/Scene/DrawList.h"
#include "PrimeEngine/APIAbstraction/Effect/Effect.h"
#include "PrimeEngine/Render/ShaderActions/SetFluidConstantsShaderAction.h"
#include "PrimeEngine/Render/ShaderActions/SetFluidForcesShaderAction.h"

#include "PrimeEngine/Geometry/NormalBufferCPU/NormalBufferCPU.h"
#include "PrimeEngine/Geometry/ColorBufferCPU.h"
#if APIABSTRACTION_D3D9
#include "PrimeEngine/APIAbstraction/DirectX9/D3D9_GPUBuffers/D3D9_VertexBufferGPU.h"
#elif APIABSTRACTION_D3D11
#include "PrimeEngine/APIAbstraction/DirectX11/D3D11_GPUBuffers/D3D11_VertexBufferGPU.h"
#elif APIABSTRACTION_OGL
#include "PrimeEngine/APIAbstraction/OGL/OGL_GPUBuffers/OGL_VertexBufferGPU.h"
#endif

#include "IndexBufferGPU.h"
#include "BufferInfo.h"

#define MAX_FLUID_FORCES 100


namespace PE {
    namespace Components {
        struct Effect;
        struct DrawList;
    }

    //struct FluidConstantData {
    //    UINT dimension;
    //    float boundaryScale;
    //    Vector2 forcePosition;
    //    Vector2 forceDirection;
    //    float solverCenterFactor;
    //    float solverDiagonalFactor;
    //};

    class FluidSetBufferGPU : public PE::PEAllocatableAndDefragmentable
    {
    public:
        static void createBuffers(PE::GameContext& ctx, UINT width, UINT height);
        
        static void updateForce(Vector2 forcePosition, Vector2 forceDirection, float forceScale);
        static void setWillUpdate();
        static void handleFluidSimulatorDrawCalls(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent);
        static void handleFluidSurfaceResources(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent);

        static ID3D11ShaderResourceView* getVelocityTextureSRV() { return s_velocityTextureSRV; }

        static UINT s_fieldWidth;
        static UINT s_fieldHeight;

        static const UINT c_maxForcePositions;

    private:
        static void handleProject(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent);
        static void handleBoundaryVeloctiy(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent);
        static void handleBoundaryPressure(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent, ID3D11UnorderedAccessView* uav);
        static void handleExternalForces(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent);
        static void handleAdvect(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent);
        static void handleDiffusion(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent);
        static void handleClearBuffer(PE::GameContext& ctx, Components::DrawList* pDrawList, Events::Event_GATHER_DRAWCALLS* pDrawEvent, ID3D11UnorderedAccessView* uav);
        static void updateSolverFactors(PE::GameContext& ctx, Components::DrawList* pDrawList, float centerFactor, float diagonalFactor);
        static void updateBoundaryScale(PE::GameContext& ctx, Components::DrawList* pDrawList, float boundaryScale);
        static void updateForceFrame(PE::GameContext& ctx, Components::DrawList* pDrawList);
        static void bindConstantBuffer(Components::DrawList* pDrawList);
        static void bindUAV(PE::GameContext& context, PE::MemoryArena arena, Components::DrawList* pDrawList, EGpuResourceSlot slot, ID3D11UnorderedAccessView* uav);
        static void bindSRV(PE::GameContext& context, PE::MemoryArena arena, Components::DrawList* pDrawList, EGpuResourceSlot slot, ID3D11ShaderResourceView* srv);

        //static Effect* getEffect(PE::GameContext& ctx, const char* effectFilename);

        //static ID3D11Buffer* s_fluidConstantBuffer;
        static ID3D11Buffer* s_velocityBuffer;
        static ID3D11Buffer* s_divergenceBuffer;
        static ID3D11Buffer* s_pressureBuffer;
        static ID3D11Buffer* s_fluidPingpongBuffer;
        //static ID3D11Buffer* s_fluidForcePositionsBuffer;
        static ID3D11Texture2D* s_velocityTexture;

        static ID3D11UnorderedAccessView* s_velocityUAV;
        static ID3D11UnorderedAccessView* s_velocityUAV2;
        static ID3D11UnorderedAccessView* s_divergenceUAV;
        static ID3D11UnorderedAccessView* s_pressureUAV;
        static ID3D11UnorderedAccessView* s_fluidPingpongUAV;
        static ID3D11UnorderedAccessView* s_fluidPingpongUAV2;
        static ID3D11UnorderedAccessView* s_fluidForcePositionsUAV;

        static ID3D11UnorderedAccessView* s_velocityTextureUAV;

        static ID3D11ShaderResourceView* s_velocitySRV;
        static ID3D11ShaderResourceView* s_pressureSRV;
        static ID3D11ShaderResourceView* s_divergenceSRV;
        static ID3D11ShaderResourceView* s_velocityTextureSRV;
        static ID3D11ShaderResourceView* s_fluidPingpongSRV;
        //static ID3D11ShaderResourceView* s_fluidForcePositionsSRV;

        static UINT s_bufferSize;
        static UINT s_numGroups;
        static UINT s_numGroupsSingle;
        static UINT s_numGroupsBoundary;

        static SetFluidConstantsShaderAction::Data s_constantData;

        static int forcePositionsCount[MAX_FLUID_FORCES];
        static Vector2 forcePositions[MAX_FLUID_FORCES];

        static bool s_willUpdate;
        static bool s_initialized;
        static UINT s_willCancelForce;

        const static UINT c_numThreadsPerGroup = 32;
        const static UINT c_numThreadsPerBoundary = 256;
        const static UINT c_solverIteration = 30;
        const static float c_simulationSpeed;
    };

}