// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
#include "UObject/NoExportTypes.h"
#include "MyWeaponType.generated.h"

UENUM(BlueprintType) // Make it usable in Blueprints
enum class EMyWeaponType : uint8
{
    Nothing    UMETA(DisplayName = "Nothing"),
    M4         UMETA(DisplayName = "M4"),
    ASVAL      UMETA(DisplayName = "ASVAL"),
    Makarov      UMETA(DisplayName = "Makarov"),
};

USTRUCT(BlueprintType)
struct FAmmoData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
    int32 MaxAmmo = 0;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ammo")
    int32 CurrentAmmo = 0;
};