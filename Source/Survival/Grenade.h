// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Grenade.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class SURVIVAL_API AGrenade : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGrenade();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	UProjectileMovementComponent* Movement;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USkeletalMeshComponent* Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USphereComponent* CollisionSphere;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* ExplosionSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundAttenuation* ExplosionAttenuation;
	UPROPERTY(EditDefaultsOnly, Category = "FX")
	UParticleSystem* ExplosionParticle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FuseTime = 2.0f;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void Explode();

	FTimerHandle ExplosionTimerHandle;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
