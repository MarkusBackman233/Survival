// Fill out your copyright notice in the Description page of Project Settings.


#include "HumanAiController.h"
#include <NavigationSystem.h>
#include "HumanAi.h"




void AHumanAiController::MoveToRandomLocation(const FVector Location, float Radius, float AccaptanceRadius)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return;

    FNavLocation RandomPoint;
    if (!NavSys->GetRandomPointInNavigableRadius(Location, Radius, RandomPoint)) return;

    MoveToLocation(RandomPoint, AccaptanceRadius);
}

void AHumanAiController::MoveToRandomLocationDonut(const FVector Location, float MinRadius, float MaxRadius, float AccaptanceRadius)
{
    APawn* ControlledPawn = GetPawn();
    if (!ControlledPawn) return;

    UNavigationSystemV1* NavSys =
        FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    if (!NavSys) return;

    constexpr int32 MaxAttempts = 10;
    FNavLocation RandomPoint;

    for (int32 i = 0; i < MaxAttempts; ++i)
    {
        if (!NavSys->GetRandomPointInNavigableRadius(Location, MaxRadius, RandomPoint))
            continue;

        float DistSq = FVector::DistSquared(Location, RandomPoint.Location);

        if (DistSq >= FMath::Square(MinRadius))
        {
            MoveToLocation(RandomPoint.Location, AccaptanceRadius);
            return;
        }
    }

    // Optional fallback: just go max radius if all attempts fail
    NavSys->GetRandomPointInNavigableRadius(Location, MaxRadius, RandomPoint);
    MoveToLocation(RandomPoint.Location, AccaptanceRadius);
}




void AHumanAiController::BeginPlay()
{
    Super::BeginPlay();

}