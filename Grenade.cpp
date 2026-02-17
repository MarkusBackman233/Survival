// Fill out your copyright notice in the Description page of Project Settings.


#include "Grenade.h"
#include <GameFramework/ProjectileMovementComponent.h>
#include "Components/MeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "SurvivalCharacter.h"
#include <Kismet/GameplayStatics.h>
// Sets default values
AGrenade::AGrenade()
{
    PrimaryActorTick.bCanEverTick = false;

    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->InitSphereRadius(10.f);
    CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    CollisionSphere->SetCollisionProfileName(TEXT("Projectile"));
    CollisionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
    RootComponent = CollisionSphere;

    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(RootComponent);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    //Movement->UpdatedComponent = CollisionSphere;
    Movement->InitialSpeed = 1200.f;
    Movement->MaxSpeed = 1200.f;
    Movement->bRotationFollowsVelocity = true;
    Movement->bShouldBounce = true;
    Movement->ProjectileGravityScale = 1.0f;
}

// Called when the game starts or when spawned
void AGrenade::BeginPlay()
{
	Super::BeginPlay();
    GetWorldTimerManager().SetTimer(
        ExplosionTimerHandle,
        this,
        &AGrenade::Explode,
        FuseTime,
        false
    );
}

void AGrenade::Explode()
{
    if (ExplosionSound)
    {
        UGameplayStatics::PlaySoundAtLocation(
            this,
            ExplosionSound,
            GetActorLocation(),
            1.0f,
            FMath::FRandRange(0.95f, 1.05f),
            0.0f,
            ExplosionAttenuation
        );
    }
    if (ExplosionParticle)
    {
        UGameplayStatics::SpawnEmitterAtLocation(
            GetWorld(),
            ExplosionParticle,
            GetActorLocation(),
            FRotator{},
            FVector(1.0f),
            true
        );
    }
    float DamageRadius = 400.0f;
    float DamageAmount = 200.0f;

    TArray<AActor*> IgnoredActors;
    IgnoredActors.Add(this);

    UGameplayStatics::ApplyRadialDamage(
        GetWorld(),
        DamageAmount,
        GetActorLocation(),
        DamageRadius,
        UDamageType::StaticClass(),
        IgnoredActors,
        this,
        GetInstigatorController(),
        true
    );
    Destroy();
}

// Called every frame
void AGrenade::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

