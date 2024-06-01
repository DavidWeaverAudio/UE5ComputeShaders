#pragma once

#include "GlobalShader.h"
#include "ShaderParameterStruct.h"
#include "ShaderParameterUtils.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"

class FSphereComputeShader : public FGlobalShader
{
    DECLARE_GLOBAL_SHADER(FSphereComputeShader);
    SHADER_USE_PARAMETER_STRUCT(FSphereComputeShader, FGlobalShader);

    BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
        SHADER_PARAMETER_RDG_TEXTURE_UAV(RWTexture2D<float4>, OutputTexture)
        END_SHADER_PARAMETER_STRUCT()
};

void DispatchSphereComputeShader(FRHICommandListImmediate& RHICmdList, FTextureRHIRef OutputTexture);
