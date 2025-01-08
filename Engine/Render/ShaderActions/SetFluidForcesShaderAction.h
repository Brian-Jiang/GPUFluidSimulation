#pragma once

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <d3d11.h>

// Inter-Engine includes
#include "PrimeEngine/Render/IRenderer.h"
// Sibling/Children includes
#include "ShaderAction.h"
//#include "PrimeEngine/APIAbstraction/GPUBuffers/FluidSetBufferGPU.h"

namespace PE {

    namespace Components {
        struct Effect;
    }

    struct SetFluidForcesShaderAction : ShaderAction
    {
        virtual ~SetFluidForcesShaderAction() {}
        virtual void bindToPipeline(Components::Effect* pCurEffect = NULL);
        virtual void unbindFromPipeline(Components::Effect* pCurEffect = NULL);
        virtual void releaseData();

        static ID3D11Buffer* s_pBuffer; // the buffer is first filled with data and then bound to pipeline
        // unlike DX9 where we just set registers
        static ID3D11ShaderResourceView* s_pSRV;

        struct Data {
            Vector2 forcePositions[100];
        } m_data;
    };
}

