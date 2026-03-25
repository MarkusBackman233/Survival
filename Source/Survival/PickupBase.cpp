// Fill out your copyright notice in the Description page of Project Settings.


#include "Survival/PickupBase.h"
#include "Components/SphereComponent.h"
#include "Survival/SurvivalCharacter.h"

APickupBase::APickupBase()
{
	PrimaryActorTick.bCanEverTick = true;

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	SetRootComponent(PickupSphere);

	PickupSphere->InitSphereRadius(50.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PickupSphere->OnComponentBeginOverlap.AddDynamic(this, &APickupBase::OnPickupSphereOverlap);

}

bool APickupBase::ShouldPickupOnOverlap() const {
	return PickupOnOverlap;
}

void APickupBase::SetPickupOnOverlap(bool Enable)
{
	PickupOnOverlap = Enable;
}

void APickupBase::OnPickupSphereOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	ASurvivalCharacter* PickupCharacter = Cast<ASurvivalCharacter>(OtherActor);
	if (!PickupCharacter)
	{
		return;
	}

	if (!ShouldPickupOnOverlap())
	{
		return;
	}

	OnPlayerInteract(PickupCharacter);
}

void APickupBase::SetPickupItemName(const FString& Name)
{
	ItemName = Name;
}

FString APickupBase::GetPickupInteractionPrompt() const
{
	return FString::Printf(TEXT("Press E to pick up %s"), *ItemName);
}