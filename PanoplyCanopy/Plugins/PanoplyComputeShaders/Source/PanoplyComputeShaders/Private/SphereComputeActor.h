// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "SphereComputeActor.generated.h"

UCLASS()
class ASphereComputeActor : public AActor
{
	GENERATED_BODY()
	
public:	
	ASphereComputeActor();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

private:

	void DispatchComputeShader();
	void CreateRenderTarget();

	UPROPERTY(EditAnywhere, Category = "ComputeShader")
	UTextureRenderTarget2D* RenderTarget;
};
