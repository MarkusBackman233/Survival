// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"  // For SpawnSystemAtLocation
#include "NiagaraComponent.h"         // For UNiagaraComponent
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SurvivalProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS(config=Game)
class ASurvivalProjectile : public AActor
{
	GENERATED_BODY()



	/** Projectile movement component */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;


	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UNiagaraSystem* TracerEffect;
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* ConcreteHitParticle;
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* BloodHitParticle;

	UPROPERTY(EditAnywhere, Category = "Decal")
	UMaterialInterface* BulletHoleDecal;

	UPROPERTY(EditAnywhere, Category = "Decal")
	UMaterialInterface* BloodDecal;
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* HeadshotSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* BodyhitSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundAttenuation* HitSoundAttenuation = nullptr;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundConcurrency* HitSoundConcurrency = nullptr;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FX", meta = (ClampMin = "0.0"))
	float TrailLength = 100.0f;  // default length

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Projectile", meta = (ClampMin = "0.0"))
	float ProjectileSpeed = 3000.0f;  // default speed

public:
	ASurvivalProjectile();


	void SetFireDirection(const FVector& direction);

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;


	float DamageAmount = 20.0f;

	AActor* IgnoreActor = nullptr;

	/** Returns ProjectileMovement subobject **/
	UProjectileMovementComponent* GetProjectileMovement() const { return ProjectileMovement; }

private:
	FVector m_direction;
	FVector LastPosition;
	
	FVector ProjectileVelocity;

	UNiagaraComponent* m_tracer;
};

