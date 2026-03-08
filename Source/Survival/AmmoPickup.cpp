// Fill out your copyright notice in the Description page of Project Settings.


#include "Survival/AmmoPickup.h"
#include "Survival/SurvivalCharacter.h"
#include <Kismet/GameplayStatics.h>
void AAmmoPickup::OnPlayerInteract(ASurvivalCharacter* Interactor)
{
    if (!Interactor) 
        return;

    FAmmoData* PlayerAmmo = Interactor->AmmoByWeaponType.Find(AmmoType);

    if (!PlayerAmmo) 
        return;
    if (InteractionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            InteractionSound,
            GetActorLocation()
        );
    }
    int32 AmmoToGive = FMath::Min(Bullets, PlayerAmmo->MaxAmmo - PlayerAmmo->CurrentAmmo);

    PlayerAmmo->CurrentAmmo += AmmoToGive;

    Bullets -= AmmoToGive;

    if (Bullets <= 0)
    {
        SetActorEnableCollision(false);
        Destroy();
    }
    Interactor->UpdateUI();
}

void AAmmoPickup::BeginPlay()
{
	Super::BeginPlay();
	SetPickupItemName(BulletName);
}
