// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Survival/PickupBase.h"
#include "MyWeaponType.h"
#include "AmmoPickup.generated.h"



/**
 * 
 */
UCLASS()
class SURVIVAL_API AAmmoPickup : public APickupBase
{
	GENERATED_BODY()



public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	EMyWeaponType AmmoType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	int Bullets = 30;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ammo")
	FString BulletName = TEXT("Null");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound")
	USoundBase* InteractionSound;


	virtual void OnPlayerInteract(class ASurvivalCharacter* Interactor) override;

protected:
	UFUNCTION()
	virtual void BeginPlay() override;

};
