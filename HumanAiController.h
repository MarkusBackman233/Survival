// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "HumanAiController.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVAL_API AHumanAiController : public AAIController
{
	GENERATED_BODY()
	
public:
	void MoveToRandomLocation(const FVector Location, float Radius, float AccaptanceRadius);
	void MoveToRandomLocationDonut(const FVector Location, float MinRadius, float MaxRadius, float AccaptanceRadius);

	virtual void BeginPlay() override;


protected:
};
