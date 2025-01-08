#pragma once

// API Abstraction
#include "PrimeEngine/APIAbstraction/APIAbstractionDefines.h"

// Outer-Engine includes
#include <d3d11.h>

// Inter-Engine includes
#include "PrimeEngine/Render/IRenderer.h"
// Sibling/Children includes
#include "ShaderAction.h"

namespace PE {

    namespace Components {
        struct Effect;
    }

    struct SetFluidConstantsShaderAction : ShaderAction {
        virtual ~SetFluidConstantsShaderAction() {}
        virtual void bindToPipeline(Components::Effect* pCurEffect = NULL);
        virtual void unbindFromPipeline(Components::Effect* pCurEffect = NULL);
        virtual void releaseData();

        static ID3D11Buffer* s_pBuffer; // the buffer is first filled with data and then bound to pipeline
        // unlike DX9 where we just set registers

        struct Data {
            UINT dimension;
            float boundaryScale;
            Vector2 forcePosition;
            float forceScale;
            float timeScale;
            //Vector2 forceDirection;
            float solverCenterFactor;
            float solverDiagonalFactor;
        } m_data;
    };

}