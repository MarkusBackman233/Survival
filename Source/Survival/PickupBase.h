#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PickupBase.generated.h"

class ASurvivalCharacter;

UCLASS()
class SURVIVAL_API APickupBase : public AActor
{
	GENERATED_BODY()

public:
	APickupBase();
	FString GetPickupInteractionPrompt() const;


	virtual void OnPlayerInteract(ASurvivalCharacter* Interactor) {};
protected:
	void SetPickupOnOverlap(bool Enable);
	void SetPickupItemName(const FString& Name);


private:



	// Set this if the user does not have to press "E" on the object to pick it up
	// Walking over it picks it up instead
	bool PickupOnOverlap = false;

	// The item name shown in the interaction prompt
	// This will create: "Press E to pick up Null"
	FString ItemName = TEXT("Null");
};