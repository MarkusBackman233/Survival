// Copyright Epic Games, Inc. All Rights Reserved.


#include "SurvivalWeaponComponent.h"
#include "SurvivalCharacter.h"
#include "SurvivalProjectile.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimInstance.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Components/SphereComponent.h"
// Sets default values for this component's properties
ASurvivalWeaponActor::ASurvivalWeaponActor() 
	: APickupBase()
	, LastFireTime(0.0f)
{
	bIsReloading = false;
	Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	SetRootComponent(Mesh);

	PickupSphere = CreateDefaultSubobject<USphereComponent>(TEXT("PickupSphere"));
	PickupSphere->InitSphereRadius(50.f);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PickupSphere->SetCollisionObjectType(ECC_WorldDynamic);
	PickupSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	PickupSphere->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	PickupSphere->SetupAttachment(Mesh);


}


void ASurvivalWeaponActor::Reload()
{
	if (Character == nullptr || 
		Character->GetController() == nullptr || 
		bIsReloading || 
		Character->IsDead() || 
		Character->AmmoByWeaponType.Find(WeaponType)->CurrentAmmo <= 0 ||
		BulletsLeftInMagazine >= GetModifiedMagazineSize() ||
		Character->GetCurrentHeldWeapon() != this || 
		Character->IsEquiping())
	{
		return;
	}

	float ReloadSpeed = 1.0f + static_cast<float>(Character->PowerUpCounter[static_cast<int>(EPowerUp::ReloadSpeed)]) * 0.1f;


	bIsReloading = true;
	if (CharacterReloadAnimation != nullptr)
	{
		UAnimInstance* AnimInstance = Character->GetMesh1P()->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(CharacterReloadAnimation, ReloadSpeed);
		}
	}
	if (WeaponReloadAnimation != nullptr)
	{
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(WeaponReloadAnimation, ReloadSpeed);

			// Bind callback
			FOnMontageEnded MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &ASurvivalWeaponActor::OnReloadAnimationEnded);

			AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, WeaponReloadAnimation);
		}
	}




	Character->SetHoldingMagazine(true);
}

void ASurvivalWeaponActor::PlayFireAudio()
{
	float MinDistance = 0.f;
	float MaxDistance = 1000.f;
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	FVector PlayerLocation{};
	if (PlayerPawn)
	{
		PlayerLocation = PlayerPawn->GetActorLocation();
	}
	float DistanceToPlayer = (GetActorLocation() - PlayerLocation).Length();
	float Blend = FMath::Clamp(DistanceToPlayer / MaxDistance, 0.f, 1.f);
	float CloseVolume = 1.0f - Blend; 
	float FarVolume = Blend; 
	if (FireSoundsNear.Num() > 0 && FireSoundAttenuation && FireSoundConcurrency)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSoundsNear[FMath::RandHelper(FireSoundsNear.Num())],
			GetActorLocation(),
			CloseVolume,
			FMath::FRandRange(1.0f, 1.0f),
			0.0f,
			FireSoundAttenuation
			//FireSoundConcurrency
		);
	}
	if (FireSoundsFar.Num() > 0 && FireSoundAttenuation && FireSoundConcurrency)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			FireSoundsFar[FMath::RandHelper(FireSoundsFar.Num())],
			GetActorLocation(),
			FarVolume,
			FMath::FRandRange(1.0f, 1.0f),
			0.0f,
			FireSoundAttenuation
			//FireSoundConcurrency
		);
	}
}

void ASurvivalWeaponActor::OnReloadAnimationEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == WeaponReloadAnimation)
	{
		if (Character)
		{
			Character->SetHoldingMagazine(false);
		}
	}

	if (Character)
	{
		int WantedBullets = std::min(GetModifiedMagazineSize() - BulletsLeftInMagazine, Character->AmmoByWeaponType.Find(WeaponType)->CurrentAmmo);

		BulletsLeftInMagazine += WantedBullets;
		Character->AmmoByWeaponType.Find(WeaponType)->CurrentAmmo -= WantedBullets;

		Character->UpdateUI();
	}
	
	bIsReloading = false;
}



void ASurvivalWeaponActor::EnableSimulation()
{
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	Mesh->SetSimulatePhysics(true);

	PickupSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

}

int ASurvivalWeaponActor::GetModifiedMagazineSize()
{
	return Character->PowerUpCounter[static_cast<int>(EPowerUp::AmmoCapacity)] + MagazineSize;
}

void ASurvivalWeaponActor::OnPlayerInteract(ASurvivalCharacter* Interactor)
{
	if (Interactor->IsEquiping())
	{
		return;
	}
	AttachWeapon(Interactor);
	Interactor->EquipOrReplaceWeapon(this);
}

void ASurvivalWeaponActor::DisableSimulation()
{
	Mesh->SetSimulatePhysics(false);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	PickupSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);

}

void ASurvivalWeaponActor::Fire()
{
	if (Character == nullptr || Character->GetController() == nullptr || Character->IsDead() || Character->IsRunning() || Character->GetCurrentHeldWeapon() != this || Character->IsEquiping())
	{
		return;
	}
	if (BulletsLeftInMagazine <= 0 || bIsReloading)
	{
		return;
	}
	float lifeTime = GetWorld()->GetTimeSeconds();

	if (lifeTime - LastFireTime < FireRate)
	{
		return;
	}

	BulletsLeftInMagazine--;
	Character->UpdateUI();

	LastFireTime = lifeTime;
	FTransform MuzzleTranform;


	if (Mesh->DoesSocketExist(TEXT("Muzzle")))
	{
		MuzzleTranform = Mesh->GetSocketTransform(TEXT("Muzzle"), RTS_World);
	}



	// Try and fire a projectile
	if (ProjectileClass != nullptr)
	{
		UWorld* const World = GetWorld();
		if (World != nullptr)
		{
			APlayerController* PlayerController = Cast<APlayerController>(Character->GetController());
			Character->ApplyRecoilKick(Recoil);
			FActorSpawnParameters ActorSpawnParams;
			ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			auto projectile = World->SpawnActor<ASurvivalProjectile>(ProjectileClass, MuzzleTranform.GetLocation(), MuzzleTranform.GetRotation().Rotator(), ActorSpawnParams);
			if (projectile)
			{
				projectile->IgnoreActor = Character;

				float Spread = Character->IsAiming() ?  0.01f : BulletSpreadInDegrees;


				APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
				if (PC)
				{
					FVector CameraLocation;
					FRotator CameraRotation;

					PC->GetPlayerViewPoint(CameraLocation, CameraRotation);
					FVector CameraDirection = CameraRotation.Vector();
					FVector ShotDirection = FMath::VRandCone(CameraDirection, FMath::DegreesToRadians(Spread));
					projectile->SetFireDirection(ShotDirection);

					



					projectile->DamageAmount = DamageAmount + DamageAmount * static_cast<float>(Character->PowerUpCounter[static_cast<int>(EPowerUp::Damage)]) * 0.03f;
				}

			}

		}
	}
	PlayFireAudio();

	if (MuzzleParticle != nullptr)
	{
		UGameplayStatics::SpawnEmitterAttached(
			MuzzleParticle,
			Mesh,                 // Parent component
			FName("Muzzle"),       // Socket name
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::SnapToTarget,
			true                         // Auto destroy
		);
	}

	// Try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
		if (AnimInstance != nullptr)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

bool ASurvivalWeaponActor::AttachWeapon(ASurvivalCharacter* TargetCharacter)
{
	Character = TargetCharacter;

	// Check that the character is valid, and has no weapon component yet
	if (Character == nullptr || Character->GetInstanceComponents().FindItemByClass<ASurvivalWeaponActor>() || Character->GetMesh1P() == nullptr)
	{
		return false;
	}

	// Attach the weapon to the First Person Character
	
	FAttachmentTransformRules AttachRules(EAttachmentRule::SnapToTarget, true);
	AttachToComponent(Character->GetMesh1P(), AttachRules, FName("Weapon"));

	DisableSimulation();



	// Set up action bindings
	if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
	{

		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			// Set the priority of the mapping to 1, so that it overrides the Jump action with the Fire action when using touch input
			Subsystem->AddMappingContext(FireMappingContext, 1);
		}

		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			if (Automatic)
			{
				EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Triggered, this, &ASurvivalWeaponActor::Fire);
			}
			else
			{
				EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ASurvivalWeaponActor::Fire);
			}
		}
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerController->InputComponent))
		{
			EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Triggered, this, &ASurvivalWeaponActor::Reload);
		}
	}

	return true;
}

void ASurvivalWeaponActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// ensure we have a character owner
	if (Character != nullptr)
	{
		// remove the input mapping context from the Player Controller
		if (APlayerController* PlayerController = Cast<APlayerController>(Character->GetController()))
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
			{
				Subsystem->RemoveMappingContext(FireMappingContext);
			}
		}
	}

	// maintain the EndPlay call chain
	Super::EndPlay(EndPlayReason);
}

void ASurvivalWeaponActor::BeginPlay()
{
	Super::BeginPlay();
	SetPickupItemName(WeaponName);

	BulletsLeftInMagazine = MagazineSize;
}

