// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "MyWeaponType.h"
#include "SurvivalWeaponComponent.generated.h"
class ASurvivalCharacter;
class USphereComponent;

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SURVIVAL_API ASurvivalWeaponActor : public AActor
{
	GENERATED_BODY()

public:

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Collision")
	USphereComponent* PickupSphere;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class ASurvivalProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	TArray<USoundBase*> FireSoundsNear;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	TArray<USoundBase*> FireSoundsFar;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundAttenuation* FireSoundAttenuation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundConcurrency* FireSoundConcurrency = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh;


	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* FireAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* WeaponReloadAnimation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* CharacterReloadAnimation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* EquipAnimation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	UAnimMontage* UnequipAnimation = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float FireRate = 0.07f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float AIFireRate = 0.07f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float DamageAmount = 20.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float Recoil  = 4.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float BulletSpreadInDegrees = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool Automatic = true;

	/** Number of bullets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	int MagazineSize;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "State")
	EMyWeaponType WeaponType;


	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* MuzzleParticle;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputMappingContext* FireMappingContext = nullptr;

	/** Fire Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* FireAction = nullptr;
	/** Reload Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	class UInputAction* ReloadAction = nullptr;

	/** Sets default values for this component's properties */
	ASurvivalWeaponActor();

	/** Attaches the actor to a FirstPersonCharacter */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	bool AttachWeapon(ASurvivalCharacter* TargetCharacter);


	/** Make the weapon Fire a Projectile */
	UFUNCTION(BlueprintCallable, Category="Weapon")
	void Fire();

	void Reload();

	void PlayFireAudio();
	int BulletsLeftInMagazine;

	int ExtraAmmo = 0;

	void DisableSimulation();
	void EnableSimulation();

	int GetModifiedMagazineSize();

protected:
	UFUNCTION()
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	UFUNCTION()
	virtual void BeginPlay() override;

	virtual void OnReloadAnimationEnded(UAnimMontage* Montage, bool bInterrupted);

private:



	/** The Character holding this weapon*/
	ASurvivalCharacter* Character;

	bool bIsReloading;
	float LastFireTime;

};
