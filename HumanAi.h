// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include <Perception/AIPerceptionTypes.h>
#include "HumanAi.generated.h"

class ASurvivalWeaponActor;
class AHumanAiController;
class UEnvQuery;
struct FEnvQueryRequest;
struct FEnvQueryResult;

USTRUCT(BlueprintType)
struct FWeaponSpawnEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<class ASurvivalWeaponActor> WeaponClass;

	// Relative chance (weight, not percentage)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<float> SpawnChancePerRound;
};

UCLASS()
class SURVIVAL_API AHumanAi : public ACharacter
{
	GENERATED_BODY()
public:



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FWeaponSpawnEntry> WeaponSpawnTable;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	UAIPerceptionComponent* AIPerceptionComponent;






	UFUNCTION(BlueprintCallable, Category = "Ai")
	APawn* GetSeenTarget() const;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ai")
	TArray<UAnimMontage*> HitReactions;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ai")
	UAnimMontage* ReloadMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UEnvQuery* FindCoverQuery;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	UEnvQuery* FindAmbushQuery;


	UFUNCTION(BlueprintCallable, Category = "Ai")
	float GetAimPitchAngleInDegrees() const;
	UFUNCTION(BlueprintCallable, Category = "Ai")
	float GetAimYawAngleInDegrees() const;

	UPROPERTY(EditAnywhere, Category = "Collision")
	TEnumAsByte<ECollisionChannel> RagdollIgnoreChannel;


	UPROPERTY(EditAnywhere, Category = "Weapon")
	float fWeaponSpreadInDegrees = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	TArray<USoundBase*> FootstepSounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundAttenuation* FootstepSoundAttenuation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundConcurrency* FootstepSoundConcurrency = nullptr;



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float MaxHealth = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float BodyTurnSpeed = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	float AimTurnSpeed = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	bool DebugEnabled = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	TArray<USoundBase*> InjuredSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	TArray<USoundBase*> NoticedEnemySounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	TArray<USoundBase*> DeathSounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundAttenuation* VoiceSoundAttenuation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundConcurrency* VoiceSoundConcurrency = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Audio)
	UAudioComponent* VoiceSoundComponent;

public:


public:
	// Sets default values for this character's properties
	AHumanAi();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	void ForgetTarget();


	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	bool IsDead() const;


	void GetInformationAboutEnemy(const FVector& EnemyLocation, const FVector& EnemyVelocity);


	FVector LastSeenTargetLocation;
	FVector LastSeenTargetVelocity;

protected:

	void PlayVoiceLine(USoundBase* Sound, bool InteruptCurrent = false);

	void ShareInformation();

	void PlayRandomFootstepSound();

	void Fire();
	void SetupFireBurst(int NbShoots);
	int BulletsToFire = 0;
	FTimerHandle FireTimerHandle;

	FEnvQueryRequest* FindCoverRequest;

	enum AiStateTree
	{
		Idle,
		AdjustPositionToLookAtTarget,
		InvestigateLastKnownPosition,
		RandomWalking,
		StalkPlayer,
		Cover,
		Ambush,
		Dead
	};
	AiStateTree CurrentState;


	void EvaluateNextState();
	void UpdateState();
	void TransitionToState(AiStateTree State);
	void LeaveState();

	void Reload();
	void OnReloadComplete(UAnimMontage* Montage, bool bInterrupted);

	void OnConverQueryComplete(TSharedPtr<FEnvQueryResult> Result);
	void OnAmbushQueryComplete(TSharedPtr<FEnvQueryResult> Result);

	// Runtime spawned weapon
	UPROPERTY()
	ASurvivalWeaponActor* HeldWeapon;

	APawn* SeenTarget = nullptr;

	FVector ENVQueryLocation;

	bool bIsInCover = false;
	bool bIsReloading = false;

	FVector AimingLocation;
	bool bHasRecentlySeenPlayer = false;
	bool bHasClearLineOfSight = false;


	float fSearchRadius = 100.0f;

	float fAimPitchAngle = 0.0f;
	float fAimYawAngle = 0.0f;

	float fSeenPlayerTime = 0.0f;

	float fHealth;

	FTimerHandle ForgetTargetTimerHandle;
	FTimerHandle EvaluateStateHandle;
	FTimerHandle FootstepTimerHandle;

};
