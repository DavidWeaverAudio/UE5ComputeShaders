#include "SphereComputeShader.h"
#include "RenderGraphBuilder.h"
#include "RendererInterface.h"
#include "RHIStaticStates.h"
#include "ShaderParameterUtils.h"
#include "PipelineStateCache.h"
#include "GlobalShader.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"

IMPLEMENT_GLOBAL_SHADER(FSphereComputeShader, "/Plugins/PanoplyComputeShaders/Shaders/SphereComputeShader.usf", "MainComputeShader", SF_Compute);

void DispatchSphereComputeShader(FRHICommandListImmediate& RHICmdList, FTextureRHIRef OutputTexture)
{
    if (!OutputTexture.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("Invalid Output Texture"));
        return;
    }

    // Check if the texture supports UAV
    if (!(OutputTexture->GetFlags() & TexCreate_UAV))
    {
        UE_LOG(LogTemp, Error, TEXT("Texture does not support UAV"));
        return;
    }

    FRDGBuilder GraphBuilder(RHICmdList);

    // Create RDG texture from RHI texture
    FRDGTextureRef RDGTexture = GraphBuilder.RegisterExternalTexture(CreateRenderTarget(OutputTexture, TEXT("OutputTexture")));

    if (!RDGTexture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to register external texture"));
        return;
    }

    FSphereComputeShader::FParameters* PassParameters = GraphBuilder.AllocParameters<FSphereComputeShader::FParameters>();
    PassParameters->OutputTexture = GraphBuilder.CreateUAV(FRDGTextureUAVDesc(RDGTexture));

    if (!PassParameters->OutputTexture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create UAV"));
        return;
    }

    TShaderMapRef<FSphereComputeShader> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
    FComputeShaderUtils::AddPass(GraphBuilder, RDG_EVENT_NAME("SphereComputeShader"), ComputeShader, PassParameters, FIntVector(256 / 8, 256 / 8, 1));

    GraphBuilder.Execute();
}