// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvQueryContext_EnemyQuerier.h"
#include "EnvironmentQuery/Contexts/EnvQueryContext_Querier.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Controller.h"
#include "AISystem.h"
#include "VisualLogger/VisualLogger.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"
#include "HumanAi.h"
#include <EnvironmentQuery/Items/EnvQueryItemType_Point.h>


void UEnvQueryContext_EnemyQuerier::ProvideContext(FEnvQueryInstance& QueryInstance, FEnvQueryContextData& ContextData) const
{
	AActor* QueryOwner = Cast<AActor>(QueryInstance.Owner.Get());

	if (auto Human = Cast<AHumanAi>(QueryOwner))
	{
		UEnvQueryItemType_Point::SetContextHelper(ContextData, Human->LastSeenTargetLocation);
		return;
	}
	UEnvQueryItemType_Point::SetContextHelper(ContextData, QueryOwner->GetActorLocation());
}

