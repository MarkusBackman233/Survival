// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "EnemyAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVAL_API UEnemyAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	// This variable will store speed
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Animation")
	float Speed;
};
