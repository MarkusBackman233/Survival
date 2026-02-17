// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurvivalProjectile.h"
#include "SurvivalCharacter.h"
#include <Kismet/GameplayStatics.h>
#include <Components/DecalComponent.h>
#include "HumanAi.h"
ASurvivalProjectile::ASurvivalProjectile()
{
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent = Root;

	// Die after 3 seconds by default
	InitialLifeSpan = 3.0f;

	PrimaryActorTick.bCanEverTick = true;
	ProjectileVelocity = FVector::ZeroVector;
	
}

void ASurvivalProjectile::SetFireDirection(const FVector& direction)
{
	ProjectileVelocity = direction * ProjectileSpeed;
}

void ASurvivalProjectile::Tick(float DeltaTime)
{
	FVector StartLocation = LastPosition;
	FVector EndLocation = GetActorLocation();
	//DrawDebugLine(
	//	GetWorld(),
	//	StartLocation,
	//	EndLocation,
	//	FColor::Green,
	//	false,
	//	30.0f,
	//	0,
	//	10.0f
	//);
	LastPosition = GetActorLocation();
	ProjectileVelocity.Z -= DeltaTime * 981.0f;
	SetActorLocation(GetActorLocation() + ProjectileVelocity * DeltaTime);

	float Speed = ProjectileVelocity.Size();
	FVector Drag = -ProjectileVelocity.GetSafeNormal() * Speed * Speed * 0.000001f;
	ProjectileVelocity += Drag * DeltaTime;

	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);

	if (IgnoreActor)
	{
		Params.AddIgnoredActor(IgnoreActor);
	}

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		LastPosition,
		GetActorLocation(),
		ECC_GameTraceChannel2,
		Params
	);

	if (bHit)
	{
		FVector Scale = FVector(1.f);
		FRotator Rot = HitResult.Normal.Rotation();
		Rot.Pitch -= 90.f;

		ACharacter* HitCharacter = Cast<ACharacter>(HitResult.GetActor());
		if (HitCharacter)
		{
			UGameplayStatics::SpawnEmitterAttached(
				BloodHitParticle,
				HitResult.GetComponent(),
				NAME_None,
				HitResult.Location,
				Rot,
				Scale,
				EAttachLocation::KeepWorldPosition,
				true
			);
			AActor* PlayerActor = UGameplayStatics::GetPlayerPawn(this, 0);
			float DamageMultiplier = 1.0f;
			FString BoneNameStr = HitResult.BoneName.ToString(); // Convert FName to FString

			if (BoneNameStr.Contains(TEXT("head")))
			{
				DamageMultiplier = 5.5f; // Headshot

				bool AlreadyDead = Cast<AHumanAi>(HitResult.GetActor()) != nullptr && Cast<AHumanAi>(HitResult.GetActor())->IsDead();

				if (HeadshotSound && !AlreadyDead && HitSoundAttenuation && HitSoundConcurrency && HitResult.GetActor() != PlayerActor)
				{
					UGameplayStatics::PlaySoundAtLocation(
						this,
						HeadshotSound,
						HitResult.Location,
						1.0f,
						FMath::FRandRange(1.0f, 1.0f),
						0.0f,
						HitSoundAttenuation,
						HitSoundConcurrency
					);
				}
			}
			else if (BoneNameStr.Contains(TEXT("spine")))
			{
				DamageMultiplier = 1.2f; // Body shot
			}
			if (BodyhitSound && HitSoundAttenuation && HitSoundConcurrency && HitResult.GetActor() != PlayerActor)
			{
				UGameplayStatics::PlaySoundAtLocation(
					this,
					BodyhitSound,
					HitResult.Location,
					0.5f,
					FMath::FRandRange(1.0f, 1.0f),
					0.0f,
					HitSoundAttenuation,
					HitSoundConcurrency
				);
			}
			if (auto InstigatorCharacter =  Cast<ACharacter>(IgnoreActor))
			{
				UGameplayStatics::ApplyPointDamage(
					HitCharacter,
					DamageAmount * DamageMultiplier,
					ProjectileVelocity.GetSafeNormal(),
					HitResult,
					InstigatorCharacter->GetController(),     // Instigator
					this,                // Damage causer
					UDamageType::StaticClass()
				);
			}
			if (BloodDecal && HitResult.GetActor() != PlayerActor)
			{
				// Decal size
				FVector DecalSize = FVector(10.f, 10.f, 10.f);

				// Spawn decal at hit location with rotation matching surface normal
				UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAttached(
					BloodDecal,
					DecalSize,           
					HitResult.GetComponent(),    
					HitResult.BoneName,         
					HitResult.Location,       
					HitResult.Normal.Rotation(), 
					EAttachLocation::KeepWorldPosition,
					120.f                 
				);

				if (DecalComp)
				{
					DecalComp->SetDecalColor(FLinearColor::Red);
					DecalComp->SetFadeScreenSize(0.f); // <- Disable distance-based fading
					DecalComp->SetFadeOut(0.f, 0.f);   // <- Disable fade out
				}
			}
		}
		else
		{
			UGameplayStatics::SpawnEmitterAtLocation(
				GetWorld(),
				ConcreteHitParticle,
				HitResult.Location,
				Rot,
				Scale,
				true // auto destroy
			);

			if (BulletHoleDecal)
			{
				// Decal size
				FVector DecalSize = FVector(10.f, 10.f, 10.f);

				// Spawn decal at hit location with rotation matching surface normal
				UDecalComponent* DecalComp = UGameplayStatics::SpawnDecalAtLocation(
					GetWorld(),
					BulletHoleDecal,          // Material
					DecalSize,                // Size
					HitResult.Location,          // Location
					HitResult.Normal.Rotation(), // Rotation
					120.f                      // LifeSpan (seconds)
				);

				if (DecalComp)
				{
					DecalComp->SetFadeScreenSize(0.f); // <- Disable distance-based fading
					DecalComp->SetFadeOut(0.f, 0.f);   // <- Disable fade out
				}
			}
		}



		//system->SetVectorParameter(FName("Cone Axis"), HitResult.Normal);
		
		UPrimitiveComponent* HitComp = HitResult.GetComponent();

		if (HitComp->IsSimulatingPhysics())
		{
			FVector ImpulseDir = (EndLocation - StartLocation).GetSafeNormal();
			float ImpulseStrength = 1000.f;

			HitComp->AddImpulse(ImpulseDir * ImpulseStrength, NAME_None, true);
		}
		Destroy();

		return; // Stop further movement this tick
	}



	//if (TracerEffect)
	//{
	//	if (m_tracer)
	//	{
	//		m_tracer->SetVectorParameter(FName("Beam Start"), NewLocation - m_direction * TrailLength);
	//		m_tracer->SetVectorParameter(FName("Beam End"), NewLocation + m_direction * 0.5);
	//	}
	//}
}

void ASurvivalProjectile::BeginPlay()
{
	Super::BeginPlay();
	//if (RootComponent)
	//{
	//	m_tracer = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
	//		GetWorld(),
	//		TracerEffect,
	//		RootComponent->GetComponentLocation()
	//	);
	//}
	LastPosition = GetActorLocation();
}

