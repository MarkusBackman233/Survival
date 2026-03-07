// Fill out your copyright notice in the Description page of Project Settings.


#include "Survival/PickupBase.h"

APickupBase::APickupBase()
{
	PrimaryActorTick.bCanEverTick = true;

}



void APickupBase::SetPickupOnOverlap(bool Enable)
{
	PickupOnOverlap = Enable;
}

void APickupBase::SetPickupItemName(const FString& Name)
{
	ItemName = Name;
}

FString APickupBase::GetPickupInteractionPrompt() const
{
	return FString::Printf(TEXT("Press E to pick up %s"), *ItemName);
}