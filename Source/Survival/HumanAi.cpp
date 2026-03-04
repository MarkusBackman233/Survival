// Fill out your copyright notice in the Description page of Project Settings.


#include "HumanAi.h"
#include "HumanAiController.h"
#include "SurvivalWeaponComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "Perception/AISenseConfig_Sight.h"
#include "SurvivalCharacter.h"
#include <Components/StateTreeComponent.h>
#include "Navigation/PathFollowingComponent.h"
#include "SurvivalProjectile.h"
#include <Kismet/GameplayStatics.h>
#include <Kismet/KismetMathLibrary.h>
#include <Components/CapsuleComponent.h>
#include "GameFramework/CharacterMovementComponent.h"
#include "SurvivalPickUpComponent.h"
#include "Engine/DamageEvents.h"
#include "Components/AudioComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnemySpawnerSystem.h"
APawn* AHumanAi::GetSeenTarget() const
{
    return SeenTarget;
}

float AHumanAi::GetAimPitchAngleInDegrees() const
{
    return fAimPitchAngle;
}

float AHumanAi::GetAimYawAngleInDegrees() const
{
    return fAimYawAngle;
}

AHumanAi::AHumanAi()
{
	PrimaryActorTick.bCanEverTick = true;
    GetMesh()->SetupAttachment(RootComponent);
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    AIControllerClass = AHumanAiController::StaticClass();


    AIPerceptionComponent = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerceptionComponent"));

    UAISenseConfig_Sight* SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 2000.f;
    SightConfig->LoseSightRadius = 2200.f;
    SightConfig->PeripheralVisionAngleDegrees = 70.f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->AutoSuccessRangeFromLastSeenLocation = 600.f;
    AIPerceptionComponent->ConfigureSense(*SightConfig);
    AIPerceptionComponent->SetDominantSense(SightConfig->GetSenseImplementation());
    AIPerceptionComponent->OnTargetPerceptionUpdated.AddDynamic(this, &AHumanAi::OnTargetPerceptionUpdated);

    VoiceSoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("VoiceComp"));
    VoiceSoundComponent->SetupAttachment(GetMesh(), TEXT("head"));
    VoiceSoundComponent->bAutoActivate = false;
    //GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
    fHealth = MaxHealth;
}

void AHumanAi::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    if (CurrentState == AHumanAi::Dead)
        return;

    ASurvivalCharacter* PlayerPawn = Cast<ASurvivalCharacter>(Actor);
    if (!PlayerPawn)
        return;

    if (Stimulus.WasSuccessfullySensed() && !PlayerPawn->IsDead())
    {
        bHasRecentlySeenPlayer = true;
        LastSeenTargetLocation = Stimulus.StimulusLocation;
        LastSeenTargetVelocity = FVector(0.0f,0.0f,0.0f);
        SeenTarget = PlayerPawn; 

        //GetWorld()->GetTimerManager().ClearTimer(ForgetTargetTimerHandle);
        if (FMath::RandRange(0.0f, 1.0f) < 0.8f)
        {
            PlayVoiceLine(NoticedEnemySounds[FMath::RandHelper(NoticedEnemySounds.Num())], false);
        }
    }
    else
    {
        SeenTarget = nullptr;

        //if (!GetWorld()->GetTimerManager().IsTimerActive(ForgetTargetTimerHandle))
        //{
        //    GetWorld()->GetTimerManager().SetTimer(
        //        ForgetTargetTimerHandle,
        //        this,
        //        &AHumanAi::ForgetTarget,
        //        30.0f,
        //        false
        //    );
        //}
    }
}

void AHumanAi::ForgetTarget()
{
    bHasRecentlySeenPlayer = false;
}

float AHumanAi::TakeDamage(
    float DamageAmount,
    FDamageEvent const& DamageEvent,
    AController* EventInstigator,
    AActor* DamageCauser
)
{



    

    fHealth -= DamageAmount;
    USkeletalMeshComponent* MeshComp = GetMesh();
    if (fHealth <= 0.0f && MeshComp)
    {
        MeshComp->SetCollisionProfileName(TEXT("Ragdoll"));
        MeshComp->SetCollisionResponseToChannel(RagdollIgnoreChannel, ECR_Ignore);

        MeshComp->SetSimulatePhysics(true);
        MeshComp->WakeAllRigidBodies();
        MeshComp->bBlendPhysics = true;

        if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
        {
            const FPointDamageEvent* PointDamage =
                static_cast<const FPointDamageEvent*>(&DamageEvent);

            const FHitResult& Hit = PointDamage->HitInfo;

            if (Hit.BoneName != NAME_None)
            {
                FVector ImpulseDirection = PointDamage->ShotDirection.GetSafeNormal();
                float ImpulseStrength = 5000.f; // tweak this

                FVector Impulse = ImpulseDirection * ImpulseStrength;

                // Apply force to the hit bone
                MeshComp->AddImpulseAtLocation(
                    Impulse,
                    Hit.ImpactPoint,
                    Hit.BoneName
                );
            }
        }
    }


    if (CurrentState == AHumanAi::Dead)
    {
        return 0.0f;
    }



    if (EventInstigator)
    {
        GetInformationAboutEnemy(EventInstigator->GetPawn()->GetActorLocation(), FVector(0.0f, 0.0f, 0.0f));
    }
    else
    {
        return 0.0f;
    }

    if (HitReactions.Num() > 0)
    {
        UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
        if (AnimInstance != nullptr)
        {
            AnimInstance->Montage_Play(HitReactions[FMath::RandHelper(HitReactions.Num())], 1.0f);
        }
    }



    if (fHealth <= 0.0f)
    {
        fHealth = 0.0f;
        PlayVoiceLine(DeathSounds[FMath::RandHelper(DeathSounds.Num())], true);

        // Cache controller BEFORE detaching
        AAIController* AICon = Cast<AAIController>(GetController());

        // Stop AI logic & movement
        if (AICon)
        {
            AICon->StopMovement();

            if (UBrainComponent* Brain = AICon->GetBrainComponent())
            {
                Brain->StopLogic(TEXT("Killed"));
            }
        }

        // Stop character movement
        GetCharacterMovement()->DisableMovement();
        GetCharacterMovement()->StopMovementImmediately();

        // Disable capsule collision
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (AIPerceptionComponent)
        {
            AIPerceptionComponent->Deactivate();
            AIPerceptionComponent->DestroyComponent();
        }
        //// Detach controller
        //DetachFromControllerPendingDestroy();

        // Enable ragdoll


        TransitionToState(AHumanAi::Dead);
        if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
        {
            Cast<ASurvivalCharacter>(Player)->AddMoney(10);
        }
        if (HeldWeapon)
        {
            HeldWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            HeldWeapon->EnableSimulation();
        }

        SetLifeSpan(40.0f);
    }
    else
    {
        //if (fHealth < (MaxHealth / 2.0f) && FindCoverQuery)
        //{
        //    FEnvQueryRequest Request(FindCoverQuery, this);
        //    Request.Execute(EEnvQueryRunMode::Type::SingleResult, this, &AHumanAi::OnConverQueryComplete);
        //}
        if (FMath::RandRange(0.0f, 1.0f) < 0.35f)
        {
            PlayVoiceLine(InjuredSounds[FMath::RandHelper(InjuredSounds.Num())], false);
        }
    }

    return DamageAmount;
}
void AHumanAi::BeginPlay()
{
    fHealth = MaxHealth;
	Super::BeginPlay();
    EvaluateNextState();
    UpdateState();



    //GetWorldTimerManager().SetTimer(
    //    EvaluateStateHandle,
    //    this,
    //    &AHumanAi::EvaluateNextState,
    //    0.5f,
    //    true
    //);
    if (AIPerceptionComponent)
    {
        AIPerceptionComponent->RegisterComponent();
    }

    float TotalChance = 0.f;

    for (const FWeaponSpawnEntry& Entry : WeaponSpawnTable)
    {
        if (Entry.SpawnChancePerRound.Num() == 0)
        {
            continue;
        }

        if (Entry.SpawnChancePerRound.Num() > AEnemySpawnerSystem::CurrentRound)
        {
            TotalChance += Entry.SpawnChancePerRound[AEnemySpawnerSystem::CurrentRound];
        }
        else
        {
            TotalChance += Entry.SpawnChancePerRound.Last();
        }
    }


    float RandomValue = FMath::FRandRange(0.f, TotalChance);

    float RunningTotal = 0.f;
    for (const FWeaponSpawnEntry& Entry : WeaponSpawnTable)
    {
        if (Entry.SpawnChancePerRound.Num() == 0)
        {
            continue;
        }

        if (Entry.SpawnChancePerRound.Num() > AEnemySpawnerSystem::CurrentRound)
        {
            RunningTotal += Entry.SpawnChancePerRound[AEnemySpawnerSystem::CurrentRound];
        }
        else
        {
            RunningTotal += Entry.SpawnChancePerRound.Last();
        }

        if (RandomValue <= RunningTotal)
        {

            FActorSpawnParameters Params;
            Params.Owner = this;
            Params.Instigator = this;

            HeldWeapon = GetWorld()->SpawnActor<ASurvivalWeaponActor>(Entry.WeaponClass, Params);
            if (HeldWeapon)
            {
                HeldWeapon->DisableSimulation();
                HeldWeapon->AttachToComponent(
                    GetMesh(),
                    FAttachmentTransformRules::SnapToTargetNotIncludingScale,
                    TEXT("WeaponAI")
                );
            }
            break;
        }
    }
}

void AHumanAi::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

    if (IsDead())
    {
        return;
    }

    const char* StateNames[]{ 
        "Idle",
        "AdjustPositionToLookAtTarget",
        "InvestigateLastKnownPosition",
        "RandomWalking",
        "Cover",
        "Ambush",
        "Dead"
    };
    if (DebugEnabled)
    {
        DrawDebugString(
            GetWorld(),
            GetActorLocation() + FVector(0, 0, 120),
            StateNames[CurrentState],
            nullptr,
            FColor::White,
            0.f,
            true
        );
    }

    if (fHealth < MaxHealth)
    {
        fHealth += DeltaTime * 3.0f;
        fHealth = std::min(fHealth, MaxHealth);
    }

    if (!bHasRecentlySeenPlayer)
    {
        SeenTarget = nullptr;
    }

    FVector AimTarget{};
    if (SeenTarget)
    {
        AimTarget = AimingLocation;
    }
    else if (bHasRecentlySeenPlayer)
    {
        AimTarget = LastSeenTargetLocation;
    }


    if (SeenTarget || bHasRecentlySeenPlayer)
    {
        FVector MyHorizontalLocation = GetActorLocation();
        MyHorizontalLocation.Z = 0.0f;

        FVector TargetHorizontalLocation = AimTarget;
        TargetHorizontalLocation.Z = 0.0f;


        FVector Direction = MyHorizontalLocation - TargetHorizontalLocation;


        float angle = FMath::RadiansToDegrees(FMath::Acos(Direction.GetSafeNormal().Dot(GetActorRightVector()))) - 90.0f;

        fAimYawAngle = FMath::FInterpTo(fAimYawAngle, FMath::Clamp(angle, -90.0f, 90.0f), DeltaTime, AimTurnSpeed);
    }
    if (SeenTarget || bHasRecentlySeenPlayer)
    {
        float TargetHeight = AimTarget.Z;
        float MyHeight = GetActorLocation().Z;


        FVector MyHorizontalLocation = GetActorLocation();
        MyHorizontalLocation.Z = 0.0f;

        FVector TargetHorizontalLocation = AimTarget;
        TargetHorizontalLocation.Z = 0.0f;


        float DistanceToTarget = (MyHorizontalLocation - TargetHorizontalLocation).Length();
        float HeightDifferanceToTarget = TargetHeight - MyHeight;
        fAimPitchAngle= FMath::FInterpTo(fAimPitchAngle, FMath::RadiansToDegrees(FMath::Atan2(HeightDifferanceToTarget, DistanceToTarget)), DeltaTime, AimTurnSpeed);

    }

    bHasClearLineOfSight = false;



    if (SeenTarget && HeldWeapon)
    {
        if (auto TargetCharacter = Cast<ACharacter>(SeenTarget))
        {
            TArray<FVector> TargetTests{ 
                TargetCharacter->GetActorLocation() + FVector(0.0f,0.0f,TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.5f),
                TargetCharacter->GetActorLocation(), 
                TargetCharacter->GetActorLocation() + FVector(0.0f,0.0f,TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
                TargetCharacter->GetActorLocation() + FVector(0.0f,0.0f,TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+ 5.f),
                TargetCharacter->GetActorLocation() + FVector(0.0f,0.0f,TargetCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+ 10.f),
            };
            
            for (const FVector& TargetTest : TargetTests)
            {
 
                FHitResult HitResult;
                FCollisionQueryParams Params;
                Params.AddIgnoredActor(this);
                Params.AddIgnoredActor(HeldWeapon);

                bool bHit = GetWorld()->LineTraceSingleByChannel(
                    HitResult,
                    HeldWeapon->GetActorLocation(),
                    TargetTest,
                    ECC_Pawn,
                    Params
                );

                if (bHit)
                {
                    if (HitResult.GetActor() == SeenTarget)
                    {
                        bHasClearLineOfSight = true;
                        AimingLocation = TargetTest;
                        //DrawDebugLine(GetWorld(), weapon->GetComponentLocation() + weapon->GetForwardVector() * 2.0f, HitResult.Location,  FColor::Green);
                        break;
                    }
                    else
                    {
                        //DrawDebugLine(GetWorld(), weapon->GetComponentLocation() + weapon->GetForwardVector() * 2.0f, HitResult.Location,  FColor::Red);
                    }
                }
            }
        }
    }

    if (auto PlayerCharacter = Cast<ASurvivalCharacter>(SeenTarget))
    {
        if (PlayerCharacter->IsDead())
        {
            SeenTarget = nullptr;
        }
    }

    if (SeenTarget)
    {
        LastSeenTargetLocation = SeenTarget->GetActorLocation();
        LastSeenTargetVelocity = SeenTarget->GetVelocity();
        fSeenPlayerTime += DeltaTime;
        ShareInformation();
    }
    else
    {
        fSeenPlayerTime = 0.0f;
    }

    if (HeldWeapon)
    {
        if (HeldWeapon->BulletsLeftInMagazine == 0 && !bIsReloading)
        {
            Reload();
        }
    }


    if (SeenTarget && bHasClearLineOfSight && fSeenPlayerTime > 0.5f)
    {
        if (!bIsReloading && BulletsToFire <= 0)
        {
            if (HeldWeapon)
            {

                float dot = (AimingLocation - HeldWeapon->GetActorLocation()).GetSafeNormal().Dot(HeldWeapon->GetActorRightVector());

                if (dot > 0.9f)
                {
                    SetupFireBurst(FMath::RandRange(1, 5));
                }
            }
        }
    }
    if (GetVelocity().Length() > 0.0f)
    {
        if (!GetWorldTimerManager().IsTimerActive(FootstepTimerHandle))
        {
            PlayRandomFootstepSound();
        }
    }
    else
    {
        if (GetWorldTimerManager().IsTimerActive(FootstepTimerHandle))
        {
            GetWorldTimerManager().ClearTimer(FootstepTimerHandle);
        }
    }


    AHumanAiController* AiController = Cast<AHumanAiController>(Controller);
    if (!AiController) return;

    UPathFollowingComponent* PathComp = AiController->GetPathFollowingComponent();
    if (!PathComp) return;

    //if (SeenTarget)
    //{
    //    DrawDebugSphere(GetWorld(), GetActorLocation(), 50.0f, 12, FColor::Yellow);
    //}


    float MovementDot = GetVelocity().GetSafeNormal().Dot(GetActorForwardVector()) * 0.5f + 0.5f;
    GetCharacterMovement()->MaxWalkSpeed = FMath::Lerp(200.0f, 400.0f, MovementDot);


    if (SeenTarget || bHasRecentlySeenPlayer)
    {
        GetCharacterMovement()->bOrientRotationToMovement = false;
        FRotator LookAtRotation =
            UKismetMathLibrary::FindLookAtRotation(GetActorLocation(), SeenTarget ? AimingLocation : LastSeenTargetLocation);

        LookAtRotation.Pitch = 0.0f;
        LookAtRotation.Roll = 0.0f;

        // How far off-center before we rotate (degrees)
        constexpr float RotateDeadZoneDegrees = 30.0f;

        // Shortest signed yaw difference
        float YawDelta = FMath::FindDeltaAngleDegrees(
            GetActorRotation().Yaw,
            LookAtRotation.Yaw
        );

        if (FMath::Abs(YawDelta) > RotateDeadZoneDegrees)
        {
            FRotator TargetRotation = GetActorRotation();
            TargetRotation.Yaw = LookAtRotation.Yaw;

            SetActorRotation(
                FMath::RInterpTo(GetActorRotation(), TargetRotation, DeltaTime, BodyTurnSpeed)
            );
        }
    }
    else
    {
        GetCharacterMovement()->bOrientRotationToMovement = true;
    }

    
}

#define Transition(State) TransitionToState(State); \
    return

void AHumanAi::EvaluateNextState()
{
    if (IsDead())
    {
        return;
    }

    if (SeenTarget)
    {
        if (bHasClearLineOfSight)
        {
            if (FMath::RandRange(0.0f, 1.0f) < 0.15f)
            {
                Transition(AHumanAi::RandomWalking);
            }
            else
            {
                Transition(AHumanAi::Idle);
            }
        }
        else
        {
            Transition(AHumanAi::AdjustPositionToLookAtTarget);
        }
    }

    if (bHasRecentlySeenPlayer)
    {
        Transition(AHumanAi::InvestigateLastKnownPosition);
    }


    Transition(AHumanAi::StalkPlayer);
}

void AHumanAi::UpdateState()
{
    if (IsDead())
    {
        return;
    }
    GetWorldTimerManager().SetTimer(
        EvaluateStateHandle,
        this,
        &AHumanAi::UpdateState,
        1.0f,
        true
    );

    AHumanAiController* AiController = Cast<AHumanAiController>(Controller);
    if (!AiController) return;

    UPathFollowingComponent* PathComp = AiController->GetPathFollowingComponent();
    if (!PathComp) return;

    bool TaskCompleted = false;

    switch (CurrentState)
    {
    case AHumanAi::Idle:
        TaskCompleted = true;
        break;
    case AHumanAi::InvestigateLastKnownPosition:
        if (PathComp->GetStatus() == EPathFollowingStatus::Idle)
        {
            fSearchRadius += 100.0f;
            AiController->MoveToRandomLocation(LastSeenTargetLocation + LastSeenTargetVelocity.GetSafeNormal() * fSearchRadius * 0.3f, fSearchRadius, 10.0f);
        }
        if (fSearchRadius > 1500.0f || SeenTarget)
        {
            TaskCompleted = true;
        }
        break;
    case AHumanAi::AdjustPositionToLookAtTarget:
        if (bHasClearLineOfSight || PathComp->GetStatus() == EPathFollowingStatus::Idle)
        {
            TaskCompleted = true;
        }
        break;
    case AHumanAi::Cover:
        if (PathComp->GetStatus() == EPathFollowingStatus::Idle)
        {
            bIsInCover = true;
        }
        if (bIsInCover && bHasClearLineOfSight)
        {
            TaskCompleted = true;
        }
        break;
    case AHumanAi::Ambush:
        if (bHasClearLineOfSight)
        {
            TArray<AActor*> FoundActors;

            UGameplayStatics::GetAllActorsOfClass(
                GetWorld(),
                AHumanAi::StaticClass(),
                FoundActors
            );

            const float Radius = 2000.f;

            for (AActor* Actor : FoundActors)
            {
                if (Actor == this || !Cast<AHumanAi>(Actor))
                    continue;

                float DistSq = FVector::DistSquared(
                    Actor->GetActorLocation(),
                    GetActorLocation()
                );

                if (DistSq <= FMath::Square(Radius))
                {
                    AHumanAi* OtherHuman = Cast<AHumanAi>(Actor);
                    OtherHuman->GetInformationAboutEnemy(LastSeenTargetLocation, LastSeenTargetVelocity);
                    OtherHuman->TransitionToState(AHumanAi::InvestigateLastKnownPosition);
                }
            }
            TaskCompleted = true;
        }
        break;
    case AHumanAi::RandomWalking:
        if (PathComp->GetStatus() == EPathFollowingStatus::Idle)
        {
            TaskCompleted = true;
        }
        break;
    case AHumanAi::StalkPlayer:
        if (PathComp->GetStatus() == EPathFollowingStatus::Idle || SeenTarget)
        {
            TaskCompleted = true;
        }
        break;
    default:
        break;
    }


    if (TaskCompleted)
    {
        EvaluateNextState();
    }
}

void AHumanAi::TransitionToState(AiStateTree State)
{
    AHumanAiController* AiController = Cast<AHumanAiController>(Controller);
    if (!AiController) return;


    LeaveState();
    CurrentState = State;

    switch (State)
    {
    case AHumanAi::Dead:
        GetWorldTimerManager().ClearTimer(FireTimerHandle);
        GetWorldTimerManager().ClearTimer(EvaluateStateHandle);
        GetWorldTimerManager().ClearTimer(FootstepTimerHandle);
        break;
    case AHumanAi::Idle:
        AiController->StopMovement();
        break;
    case AHumanAi::RandomWalking:
        AiController->MoveToRandomLocation(GetActorLocation(), 250.0f, 10.0f);
        break;
    case AHumanAi::StalkPlayer:
        if (ACharacter* Player = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
        {
            AiController->MoveToRandomLocation(Player->GetActorLocation(), 250.0f, 10.0f);
        }
        break;
    case AHumanAi::AdjustPositionToLookAtTarget:
        AiController->MoveToActor(SeenTarget);
        break;
    case AHumanAi::InvestigateLastKnownPosition:
        AiController->StopMovement();
        fSearchRadius = 200.0f;
        break;
    case AHumanAi::Cover:
        AiController->MoveToLocation(ENVQueryLocation);
        break;
    case AHumanAi::Ambush:
        AiController->MoveToRandomLocation(ENVQueryLocation, 100.0f, 10.0f);
        break;
    default:
        break;
    }
}

void AHumanAi::LeaveState()
{
    switch (CurrentState)
    {
    case AHumanAi::Cover:
        bIsInCover = false;
        break;
    }
}

void AHumanAi::Reload()
{
    bIsReloading = true;
    BulletsToFire = 0;
    GetWorld()->GetTimerManager().ClearTimer(FireTimerHandle);
    if (ReloadMontage)
    {
        PlayAnimMontage(ReloadMontage);
        FOnMontageEnded MontageEndedDelegate;
        MontageEndedDelegate.BindUObject(this, &AHumanAi::OnReloadComplete);
        GetMesh()->GetAnimInstance()->Montage_SetEndDelegate(MontageEndedDelegate, ReloadMontage);
    }
}

void AHumanAi::OnReloadComplete(UAnimMontage* Montage, bool bInterrupted)
{
    bIsReloading = false;
    if (!HeldWeapon)
    {
        return;
    }

    HeldWeapon->BulletsLeftInMagazine = HeldWeapon->MagazineSize;
}

void AHumanAi::OnConverQueryComplete(TSharedPtr<FEnvQueryResult> Result)
{
    ENVQueryLocation = Result->GetItemAsLocation(0);
    TransitionToState(AHumanAi::Cover);
}

void AHumanAi::OnAmbushQueryComplete(TSharedPtr<FEnvQueryResult> Result)
{
    ENVQueryLocation = Result->GetItemAsLocation(0);
    TransitionToState(AHumanAi::Ambush);
}


void AHumanAi::PlayVoiceLine(USoundBase* Sound, bool InteruptCurrent)
{
    if (!VoiceSoundComponent || !Sound)
        return;
    if (VoiceSoundComponent->IsPlaying() && !InteruptCurrent)
        return;

    VoiceSoundComponent->Stop();
    VoiceSoundComponent->SetSound(Sound);
    VoiceSoundComponent->Play();
}

void AHumanAi::ShareInformation()
{
    TArray<AActor*> FoundActors;

    UGameplayStatics::GetAllActorsOfClass(
        GetWorld(),
        AHumanAi::StaticClass(),
        FoundActors
    );

    const float Radius = 2000.f;

    for (AActor* Actor : FoundActors)
    {
        if (Actor == this || !Cast<AHumanAi>(Actor))
            continue;

        float DistSq = FVector::DistSquared(
            Actor->GetActorLocation(),
            GetActorLocation()
        );

        if (DistSq <= FMath::Square(Radius))
        {
            AHumanAi* OtherHuman = Cast<AHumanAi>(Actor);

            if (!OtherHuman->SeenTarget)
            {
                OtherHuman->GetInformationAboutEnemy(LastSeenTargetLocation, LastSeenTargetVelocity);
            }
        }
    }
}

void AHumanAi::PlayRandomFootstepSound()
{
    if (CurrentState == AHumanAi::Dead)
    {
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(
        this,
        FootstepSounds[FMath::RandHelper(FootstepSounds.Num())],
        GetActorLocation(),
        0.5f,
        FMath::FRandRange(0.95f, 1.05f),
        0.0f,
        FootstepSoundAttenuation
    );

    float Speed = GetVelocity().Length();

    float SpeedAlpha = FMath::Clamp(Speed / 600.0f, 0.f, 1.f);

    float Interval = FMath::Lerp(
        0.55f,
        0.3f,
        SpeedAlpha
    );
    Interval += FMath::FRandRange(-0.04f, 0.04f);
    GetWorldTimerManager().SetTimer(
        FootstepTimerHandle,
        this,
        &AHumanAi::PlayRandomFootstepSound,
        Interval,
        true
    );
}

void AHumanAi::Fire()
{
    if (!HeldWeapon || !GetWorld() || CurrentState == AHumanAi::Dead)
    {
        return;
    }

    float dot = (AimingLocation - HeldWeapon->GetActorLocation()).GetSafeNormal().Dot(HeldWeapon->GetActorRightVector());

    if (dot < 0.9f)
    {
        BulletsToFire = 0;
        return;
    }



    FActorSpawnParameters ActorSpawnParams;
    ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    auto projectile = GetWorld()->SpawnActor<ASurvivalProjectile>(HeldWeapon->ProjectileClass, HeldWeapon->GetActorLocation(), HeldWeapon->GetActorRotation(), ActorSpawnParams);
    if (projectile)
    {
        projectile->IgnoreActor = this;

        FVector DirectionToTarget = (AimingLocation - HeldWeapon->GetActorLocation()).GetSafeNormal();
        float SpreadRadians = FMath::DegreesToRadians(fWeaponSpreadInDegrees);

        FVector ShotDirection = FMath::VRandCone(DirectionToTarget,SpreadRadians);

        projectile->SetFireDirection(ShotDirection);
    }
    HeldWeapon->BulletsLeftInMagazine--;
    HeldWeapon->PlayFireAudio();
    if (HeldWeapon->MuzzleParticle != nullptr)
    {
        UGameplayStatics::SpawnEmitterAttached(
            HeldWeapon->MuzzleParticle,
            HeldWeapon->Mesh,                 // Parent component
            FName("Muzzle"),       // Socket name
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            EAttachLocation::SnapToTarget,
            true                         // Auto destroy
        );
    }

    BulletsToFire--;
    if (BulletsToFire > 0)
    {
        float Randomness = HeldWeapon->Automatic ? 0.0f : FMath::RandRange(0.0f, 0.25f);

        GetWorldTimerManager().SetTimer(
            FireTimerHandle,
            this,
            &AHumanAi::Fire,
            HeldWeapon->AIFireRate + Randomness,
            false
        );
    }
}

bool AHumanAi::IsDead() const
{
    return fHealth <= 0.0f || CurrentState == AHumanAi::Dead;
}

void AHumanAi::GetInformationAboutEnemy(const FVector& EnemyLocation, const FVector& EnemyVelocity)
{
    if (SeenTarget || IsDead())
    {
        return;
    }

    //if (!bHasRecentlySeenPlayer)
    //{c
    //    FEnvQueryRequest Request(FindAmbushQuery, this);
    //    Request.Execute(EEnvQueryRunMode::Type::SingleResult, this, &AHumanAi::OnAmbushQueryComplete);
    //}

    bHasRecentlySeenPlayer = true;
    LastSeenTargetLocation = EnemyLocation;
    LastSeenTargetVelocity = EnemyVelocity;
}

void AHumanAi::SetupFireBurst(int NbShoots)
{
    BulletsToFire = NbShoots;

    GetWorldTimerManager().SetTimer(
        FireTimerHandle,
        this,
        &AHumanAi::Fire,
        FMath::RandRange(0.0f, 1.0f),
        false
    );
}

void AHumanAi::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

