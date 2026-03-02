// Fill out your copyright notice in the Description page of Project Settings.


#include "EnemySpawnerSystem.h"
#include "HumanAi.h"
#include <Kismet/GameplayStatics.h>
#include "SurvivalCharacter.h"
// Sets default values
int AEnemySpawnerSystem::CurrentRound = 0;

AEnemySpawnerSystem::AEnemySpawnerSystem()
{
	PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.TickInterval = 2.5f;
}

void AEnemySpawnerSystem::BeginPlay()
{
	Super::BeginPlay();
    CurrentRound = 0;
    NextRound();

    GetWorldTimerManager().SetTimer(
        CustomTickTimerHandle,
        this,
        &AEnemySpawnerSystem::Update,
        2.5f,
        false
    );
}



void AEnemySpawnerSystem::TrySpawn()
{
    if (!EnemyClass || SpawnPoints.Num() == 0) return;

    int32 Index = FMath::RandRange(0, SpawnPoints.Num() - 1);
    AActor* SpawnPoint = SpawnPoints[Index];

    if (!SpawnPoint) return;

    AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);
    if (!PlayerActor) return;


    if ((PlayerActor->GetActorLocation() - SpawnPoint->GetActorLocation()).Length() < PlayerToNearRadius)
    {
        return;
    }

    FHitResult HitResult;
    FCollisionQueryParams RaycastParams;
    RaycastParams.AddIgnoredActor(this);

    bool bHit = GetWorld()->LineTraceSingleByChannel(
        HitResult,
        SpawnPoint->GetActorLocation(),
        PlayerActor->GetActorLocation(),
        ECC_Visibility,
        RaycastParams
    );

    if (bHit)
    {
        if (HitResult.GetActor() == PlayerActor)
        {
            return;
        }
    }


    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

    AHumanAi* SpawnedEnemy = GetWorld()->SpawnActorDeferred<AHumanAi>(
        EnemyClass,
        SpawnPoint->GetActorTransform()
    );

    if (SpawnedEnemy)
    {
        SpawnedEnemies.Add(SpawnedEnemy);

        FVector PlayerLocation{};
        APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
        if (PlayerPawn)
        {
            PlayerLocation = PlayerPawn->GetActorLocation();
        }

        float RandomAmount = 500.0f;

        PlayerLocation.X += FMath::RandRange(-RandomAmount, RandomAmount);
        PlayerLocation.Y += FMath::RandRange(-RandomAmount, RandomAmount);

        SpawnedEnemy->GetInformationAboutEnemy(PlayerLocation,FVector(0.0f,0.0f,0.0f));



        SpawnedEnemy->MaxHealth *= HealthMultiplier;



        SpawnedEnemy->FinishSpawning(SpawnPoint->GetActorTransform());

        NbSpawnedEnemiesThisRound++;
    }
}

void AEnemySpawnerSystem::PreNextRound()
{
    if (PreNextRoundSound)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), PreNextRoundSound);
    }

    if (ASurvivalCharacter* MyCharacter = Cast<ASurvivalCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0)))
    {
        MyCharacter->OpenPowerUpSelector();
    }
}

void AEnemySpawnerSystem::Update()
{

    SpawnedEnemies.RemoveAll(
        [](const TWeakObjectPtr<AHumanAi>& Enemy)
    {
        return !Enemy.IsValid() || Cast<AHumanAi>(Enemy)->IsDead();
    }
    );


    bool HasSpawnedAllEnemiesForThisRound = NbSpawnedEnemiesThisRound == DesiredAmountOfEnemies;

    if (HasSpawnedAllEnemiesForThisRound && SpawnedEnemies.Num() == 0)
    {
        if (!GetWorldTimerManager().IsTimerActive(NextRoundTimerHandle))
        {
            GetWorldTimerManager().SetTimer(
                NextRoundTimerHandle,
                this,
                &AEnemySpawnerSystem::NextRound,
                15.0f,
                false
            );
            GetWorldTimerManager().SetTimer(
                PreNextRoundTimerHandle,
                this,
                &AEnemySpawnerSystem::PreNextRound,
                3.0f,
                false
            );
        }
    }


    if (NbSpawnedEnemiesThisRound < DesiredAmountOfEnemies)
    {
        TrySpawn();
    }

    GetWorldTimerManager().SetTimer(
        CustomTickTimerHandle,
        this,
        &AEnemySpawnerSystem::Update,
        SpawnFrequency,
        false
    );
}
void AEnemySpawnerSystem::NextRound()
{
    if (NextRoundSound && CurrentRound != 0)
    {
        UGameplayStatics::PlaySound2D(GetWorld(), NextRoundSound);
    }


    NbSpawnedEnemiesThisRound = 0;
    CurrentRound++;


    switch (CurrentRound)
    {
    case 1:
        DesiredAmountOfEnemies = 6;
        HealthMultiplier = 1.0f;
        SpawnFrequency = 4.0f;
        break;
    case 2:
        DesiredAmountOfEnemies = 8;
        HealthMultiplier = 1.0f;
        SpawnFrequency = 3.5f;
        break;
    case 3:
        DesiredAmountOfEnemies = 10;
        HealthMultiplier = 1.2f;
        SpawnFrequency = 3.0f;
        break;
    case 4:
        DesiredAmountOfEnemies = 12;
        HealthMultiplier = 1.4f;
        SpawnFrequency = 2.5f;
        break;
    case 5:
        DesiredAmountOfEnemies = 14;
        HealthMultiplier = 1.6f;
        SpawnFrequency = 2.0f;
        break;
    case 6:
        DesiredAmountOfEnemies = 16;
        HealthMultiplier = 1.8f;
        SpawnFrequency = 2.0f;
        break;
    case 7:
        DesiredAmountOfEnemies = 18;
        HealthMultiplier = 2.0f;
        break;
    case 8:
        DesiredAmountOfEnemies = 20;
        HealthMultiplier = 2.2f;
        break;
    case 9:
        DesiredAmountOfEnemies = 22;
        HealthMultiplier = 2.4f;
        break;
    case 10:
        DesiredAmountOfEnemies = 24;
        HealthMultiplier = 2.6f;
        break;
    default:
        DesiredAmountOfEnemies = 26;
        HealthMultiplier = 2.8f;
        break;
    }
}

void AEnemySpawnerSystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

