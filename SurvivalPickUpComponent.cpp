// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurvivalPickUpComponent.h"

USurvivalPickUpComponent::USurvivalPickUpComponent()
{
	// Setup the Sphere Collision
	SphereRadius = 32.f;
}

void USurvivalPickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register our Overlap Event
	OnComponentBeginOverlap.AddDynamic(this, &USurvivalPickUpComponent::OnSphereBeginOverlap);
}

void USurvivalPickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Checking if it is a First Person Character overlapping
	ASurvivalCharacter* Character = Cast<ASurvivalCharacter>(OtherActor);
	if(Character != nullptr)
	{
		// Notify that the actor is being picked up
		OnPickUp.Broadcast(Character);

		// Unregister from the Overlap Event so it is no longer triggered
		OnComponentBeginOverlap.RemoveAll(this);
	}
}
