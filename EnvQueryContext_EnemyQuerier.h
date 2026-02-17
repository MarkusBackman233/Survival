// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "EnvQueryContext_EnemyQuerier.generated.h"

/**
 * 
 */
UCLASS()
class SURVIVAL_API UEnvQueryContext_EnemyQuerier : public UEnvQueryContext_Querier
{
	GENERATED_BODY()
	

	virtual void ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const override;
};
