// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnemySpawnerSystem.generated.h"

class AHumanAi;

UCLASS()
class SURVIVAL_API AEnemySpawnerSystem : public AActor
{
	GENERATED_BODY()
	
public:	

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* NextRoundSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundBase* PreNextRoundSound;


	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TSubclassOf<AHumanAi> EnemyClass;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	TArray<AActor*> SpawnPoints;

	/** Player detection radius */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
	float PlayerToNearRadius = 1500.f;

	AEnemySpawnerSystem();

	static int CurrentRound;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	void PreNextRound();

	void Update();

	void NextRound();
	void TrySpawn();

	float SpawnFrequency = 1.0f;

	int NbSpawnedEnemiesThisRound = 0;
	int DesiredAmountOfEnemies = 3;
	TArray<TWeakObjectPtr<AHumanAi>> SpawnedEnemies;

	FTimerHandle NextRoundTimerHandle;
	FTimerHandle PreNextRoundTimerHandle;

	FTimerHandle CustomTickTimerHandle;

	float HealthMultiplier = 1.0f;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
