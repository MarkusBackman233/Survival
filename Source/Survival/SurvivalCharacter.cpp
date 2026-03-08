// Copyright Epic Games, Inc. All Rights Reserved.

#include "SurvivalCharacter.h"
#include "SurvivalProjectile.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "SurvivalWeaponComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Perception/AISense_Hearing.h>
#include "SurvivalGameMode.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include <Kismet/GameplayStatics.h>
#include <Components/AudioComponent.h>
#include "Grenade.h"
#include <GameFramework/ProjectileMovementComponent.h>
#include <Components/TextBlock.h>
#include "EnemySpawnerSystem.h"
#include <Components/Button.h>
#include <Components/VerticalBox.h>
#include <Components/VerticalBoxSlot.h>
#include "PickupBase.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

void ASurvivalCharacter::Spring::Update(const float Stiffness, const float Damping, const float DeltaTime)
{
	FMath::SpringDamper(Value.Pitch, Rate.Pitch, 0.0f, 0.0f, DeltaTime, Stiffness, Damping);
	FMath::SpringDamper(Value.Yaw, Rate.Yaw, 0.0f, 0.0f, DeltaTime, Stiffness, Damping);
	FMath::SpringDamper(Value.Roll, Rate.Roll, 0.0f, 0.0f, DeltaTime, Stiffness, Damping);
}


ASurvivalCharacter::ASurvivalCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	// Camera
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-10.f, 0.f, 60.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;

	FirstPersonCameraComponent->SetFieldOfView(90.0f);

	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	//Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeLocation(FVector(-30.f, 0.f, -150.f));

	FootstepAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("FootstepComp"));
	FootstepAudioComponent->SetupAttachment(FirstPersonCameraComponent);
	FootstepAudioComponent->bAutoActivate = false;
	bIsAiming = false;
}

float ASurvivalCharacter::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser
)
{
	fHealth -= DamageAmount;


	fHitReaction += 0.3f;

	//if (GEngine)
	//	GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Yellow, FString::Printf(TEXT("Example text that prints a float: %f"), fHealth));


	//CameraRecoilSpring.Rate.Pitch = FMath::RandRange(-DamageAmount, DamageAmount);
	//CameraRecoilSpring.Rate.Yaw = FMath::RandRange(-DamageAmount, DamageAmount);

	if (fHealth <= 0.0f)
	{
		fHealth = 0.0f;

		// Stop movement
		GetCharacterMovement()->DisableMovement();

		// Disable collisions if needed
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		// Optional: ragdoll
		if (Mesh1P)
		{
			Mesh1P->SetCollisionProfileName(TEXT("Ragdoll"));
			Mesh1P->SetSimulatePhysics(true);
			Mesh1P->WakeAllRigidBodies();
		}
		if (FirstPersonCameraComponent && Mesh1P)
		{
			Mesh1P->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
			// Detach from capsule first
			FirstPersonCameraComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);

			// Attach to the skeletal mesh (ragdoll)
			FirstPersonCameraComponent->AttachToComponent(
				Mesh1P,
				FAttachmentTransformRules::KeepWorldTransform,
				FName("head") // Attach to head bone if you want, or leave blank for root
			);

			// Optionally adjust relative location if needed
			//FirstPersonCameraComponent->SetRelativeLocation(FVector(0.f, 0.f, 10.f));
			//FirstPersonCameraComponent->SetRelativeRotation(FRotator::ZeroRotator);

			// Stop pawn control rotation (so ragdoll moves freely)
			FirstPersonCameraComponent->bUsePawnControlRotation = false;
		}


		// Trigger Game Over
		APlayerController* PC = Cast<APlayerController>(GetController());
		if (PC)
		{
			PC->SetCinematicMode(true, false, false, true, true); // freeze player input
			PC->bShowMouseCursor = true;

			// Call your GameMode to handle Game Over logic
			//if (ASurvivalGameMode* GM = Cast<ASurvivalGameMode>(GetWorld()->GetAuthGameMode()))
			//{
			//	GM->GameOver();
			//}
		}
	}

	UpdateUI();

	return DamageAmount;
}

void ASurvivalCharacter::NotifyControllerChanged()
{
	Super::NotifyControllerChanged();

	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ASurvivalCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!GetCharacterMovement() || IsDead())
	{
		return;
	}

	if (fHealth < fMaxHealth)
	{
		fHealth += DeltaTime * 3.0f;
		fHealth = std::min(fHealth, fMaxHealth);
		UpdateUI();
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
	UTextBlock* RoundText = Cast<UTextBlock>(PlayerHealthWidget->GetWidgetFromName("Round"));
	if (RoundText)
	{
		RoundText->SetText(FText::AsNumber(AEnemySpawnerSystem::CurrentRound));
	}

	UTextBlock* InteractionText = Cast<UTextBlock>(PlayerHealthWidget->GetWidgetFromName("InteractionPrompt"));
	if (InteractionText)
	{
		InteractionText->SetText(FText::FromString(""));
	}
	FHitResult HitResult;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	if (CurrentItems[0])
	{
		Params.AddIgnoredActor(CurrentItems[0]);
	}
	if (CurrentItems[1])
	{
		Params.AddIgnoredActor(CurrentItems[1]);
	}

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		FirstPersonCameraComponent->GetComponentLocation(),
		FirstPersonCameraComponent->GetComponentLocation() + FirstPersonCameraComponent->GetForwardVector() * 200.0f,
		ECC_Visibility,
		Params
	);

	if (bHit && IsValid(HitResult.GetActor()))
	{
		if (auto Pickup = Cast<APickupBase>(HitResult.GetActor()))
		{
			if (InteractionText)
			{
				InteractionText->SetText(FText::FromString(Pickup->GetPickupInteractionPrompt()));
			}
			FocusedItem = Pickup;
		}
	}
	else
	{
		FocusedItem = nullptr;
	}

	if (!GetCharacterMovement()->IsFalling())
	{
		if (HeadbobbingEnabled)
		{

			float HeadbobSpeed = FMath::GetMappedRangeValueClamped(
				FVector2D(WalkingMovementSpeed, SprintingMovementSpeed),
				FVector2D(HeadbobSpeedWalking, HeadbobSpeedRunning),
				FSmoothedVelocityLength
			);


			HeadbobSpeed *= FMath::Clamp(FSmoothedVelocityLength, 0.0f, 400.0f) / 400.0f;

			HeadbobTimeline.TickTimeline(DeltaTime);
			HeadbobTimeline.SetPlayRate(HeadbobSpeed);
		}
	}

	bIsRunning = bWantsToRun && !GetCharacterMovement()->IsCrouching() && !bHoldingMagazine && ! bIsAiming;


	float ExtraMoveSpeed = 1.0f + static_cast<float>(PowerUpCounter[static_cast<int>(EPowerUp::MovementSpeed)]) * 0.05f;

	if (bIsRunning)
	{
		GetCharacterMovement()->MaxWalkSpeed = SprintingMovementSpeed * ExtraMoveSpeed;
	}
	else if (bIsAiming)
	{
		GetCharacterMovement()->MaxWalkSpeed = AimingMovementSpeed * ExtraMoveSpeed;
	}
	else if (GetCharacterMovement()->IsCrouching())
	{
		GetCharacterMovement()->MaxWalkSpeed = CrouchingMovementSpeed * ExtraMoveSpeed;
	}
	else
	{
		GetCharacterMovement()->MaxWalkSpeed = WalkingMovementSpeed * ExtraMoveSpeed;
	}


	if (bIsAiming && !bHoldingMagazine)
	{
		FirstPersonCameraComponent->FieldOfView = FMath::FInterpTo(FirstPersonCameraComponent->FieldOfView,70.0f,DeltaTime,15.0f);
	}
	else
	{
		FirstPersonCameraComponent->FieldOfView = FMath::FInterpTo(FirstPersonCameraComponent->FieldOfView,90.0f,DeltaTime,15.0f);
	}


	FSmoothedVelocityLength = FMath::FInterpTo(FSmoothedVelocityLength, GetVelocity().Length(), DeltaTime, 6.0f);

	HandRecoilSpring.Update(RecoilStiffness,RecoilDamping,DeltaTime);
	HandSwaySpring.Update(SwayStiffness, SwayDamping, DeltaTime);

	CameraRecoilSpring.Update(CameraStiffness * (fRecoiling > 0.0f ? 0.0f : 1.0f), CameraDamping, DeltaTime);
	
	fRecoiling -= DeltaTime;

	InputVelocity.X = FMath::FInterpTo(InputVelocity.X, WantedInputVelocity.X, DeltaTime, 8.0f);
	InputVelocity.Y = FMath::FInterpTo(InputVelocity.Y, WantedInputVelocity.Y, DeltaTime, 8.0f);

	fHitReaction = FMath::FInterpTo(fHitReaction, 0.0f, DeltaTime, 1.2f);

	// less is more
	float SwayAmount = bIsAiming ? 0.5f : 0.2f;

	bool IsFalling = GetCharacterMovement()->IsFalling();
	float FallingPitchTargetDegrees = -20.0f;
	float FallingStiffness = IsFalling ? 1.0f : 8.0f;
	fFallingPitch = FMath::FInterpTo(fFallingPitch, IsFalling ? FallingPitchTargetDegrees : 0.0f, DeltaTime, FallingStiffness);



	float DegreesOfRollRotation = bIsAiming ? 2.0f : 8.0f;
	float MovementRoll = InputVelocity.X * FSmoothedVelocityLength / 400.0f * DegreesOfRollRotation;

	WeaponRecoilOffset.SetRotation(FQuat(FRotator(MovementRoll+ HandRecoilSpring.Value.Roll, HandRecoilSpring.Value.Yaw + HandSwaySpring.Value.Yaw , HandRecoilSpring.Value.Pitch + HandSwaySpring.Value.Pitch + fFallingPitch)));

	float OffsetMultiplier = bIsAiming ? 0.1f : 1.0f;
	WeaponRecoilOffset.SetLocation(
		FVector(
			-(HandRecoilSpring.Value.Yaw * 2.0f + HandSwaySpring.Value.Yaw + CameraHeadbobOffset.GetRotation().Rotator().Roll )* 0.1f,
			(HandRecoilSpring.Value.Pitch + HandSwaySpring.Value.Yaw ) * 0.2f,
			-(HandRecoilSpring.Value.Pitch  + HandSwaySpring.Value.Pitch + fFallingPitch + CameraHeadbobOffset.GetRotation().Rotator().Pitch)*0.1f
		) * OffsetMultiplier
	);
	auto GetWeaponSocketTransform = [&](const wchar_t* SocketName)
	{
		if (CurrentHeldWeapon && CurrentHeldWeapon->Mesh->DoesSocketExist(SocketName))
		{
			FTransform WorldTransform = CurrentHeldWeapon->Mesh->GetSocketTransform(SocketName, RTS_World);
			FTransform BoneTransform = Mesh1P->GetSocketTransform(TEXT("hand_r"), RTS_World);
			return WorldTransform.GetRelativeTransform(BoneTransform);
		}
		return FTransform::Identity;
	};
	{
		FTransform GripTransfrom = bHoldingMagazine ? GetWeaponSocketTransform(TEXT("Magazine")) : GetWeaponSocketTransform(TEXT("LHand"));
		FVector NewLocation = FMath::VInterpTo(
			CurrentLeftHandIkTransform.GetLocation(),
			GripTransfrom.GetLocation(),
			DeltaTime,
			8.0f
		);

		FRotator NewRotation = FMath::RInterpTo(
			CurrentLeftHandIkTransform.GetRotation().Rotator(),
			GripTransfrom.GetRotation().Rotator(),
			DeltaTime,
			8.0f
		);

		CurrentLeftHandIkTransform.SetLocation(NewLocation);
		CurrentLeftHandIkTransform.SetRotation(FQuat(NewRotation));
	}
	if (FirstPersonCameraComponent)
	{
		const float CrouchSpeed = 4.0f;

		if (GetCharacterMovement() && GetCharacterMovement()->IsCrouching())
		{
			fCrouchAlpha += DeltaTime * CrouchSpeed;
		}
		else
		{
			fCrouchAlpha -= DeltaTime * CrouchSpeed;
		}

		fCrouchAlpha = std::clamp(fCrouchAlpha, 0.0f, 1.0f);



		static FVector StartRelativeLocation = FirstPersonCameraComponent->GetRelativeLocation();

		float SmoothedAlpha = FMath::InterpEaseInOut(0.0f, 1.0f, fCrouchAlpha, 2.0f); // 2.0 is exponent, adjust for sharper or smoother
		float NewHeight = FMath::Lerp(static_cast<float>(StartRelativeLocation.Z), StartRelativeLocation.Z + fLocalCameraTargetHeight, SmoothedAlpha);

		FVector NewLocation = StartRelativeLocation;
		NewLocation.Z = NewHeight;
		if (GetCharacterMovement() && GetCharacterMovement()->IsCrouching())
			NewLocation.Z -= fLocalCameraTargetHeight;
		FirstPersonCameraComponent->SetRelativeLocation(NewLocation);


		auto Rotator = GetControlRotation();
		Rotator.Roll = 0.0f;
		Rotator.Yaw = 0.0f;
		Rotator.Pitch += CameraRecoilSpring.Value.Pitch;
		Rotator.Roll += CameraRecoilSpring.Value.Roll;
		Rotator.Yaw += CameraRecoilSpring.Value.Yaw;


		float HeadbobMultiplier = FMath::GetMappedRangeValueClamped(
			FVector2D(WalkingMovementSpeed, SprintingMovementSpeed),
			FVector2D(HeadbobMultiplierWalking, HeadbobMultiplierRunning),
			FSmoothedVelocityLength
		);
		Rotator += CameraHeadbobOffset.GetRotation().Rotator() * HeadbobMultiplier;

		//float CameraRoll = FMath::FInterpTo(fCameraRollError, GetVelocity().GetSafeNormal().Dot(GetActorRightVector()), DeltaTime, 5.0f);
		if (HeadbobbingEnabled)
		{
			Rotator.Roll += std::clamp(static_cast<float>(InputVelocity.X) * FSmoothedVelocityLength / 600.0f,0.0f,1.0f)*6.0f;
		}

		FirstPersonCameraComponent->SetRelativeRotation(Rotator);
	}

	auto BloodWidget1 = PlayerHealthWidget->GetWidgetFromName("Blood1");
	auto BloodWidget2 = PlayerHealthWidget->GetWidgetFromName("Blood2");

	float NormalizedHealthInv = (std::max(1.0f - (fHealth / fMaxHealth), 0.0f) - 0.5f) * 2.0f;
	float Opacity = FMath::Clamp(FMath::Pow(NormalizedHealthInv, 3.0f), 0.0f, 1.0f) * 0.8f;

	if (BloodWidget1)
	{
		BloodWidget1->SetRenderOpacity(Opacity);
	}
	if (BloodWidget2)
	{
		BloodWidget2->SetRenderOpacity(Opacity);
	}
	FirstPersonCameraComponent->PostProcessSettings.bOverride_VignetteIntensity = true;
	FirstPersonCameraComponent->PostProcessSettings.VignetteIntensity = pow(NormalizedHealthInv, 3.0f) + fHitReaction;
}

void ASurvivalCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(0.0f, 0.0f);
	fLocalCameraTargetHeight = -HalfHeightAdjust;
	bIsRunning = false;
}

void ASurvivalCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(0.0f, 0.0f);
}

void ASurvivalCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{	
	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Jumping
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
		EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);

		// Moving
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::Move);

		// Looking
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::Look);

		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ASurvivalCharacter::StartAiming);
		EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed , this, &ASurvivalCharacter::StopAiming);

		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &ASurvivalCharacter::StartSprinting);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed , this, &ASurvivalCharacter::StopSprinting);

		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ASurvivalCharacter::StartCrouching);
		EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Completed, this, &ASurvivalCharacter::StopCrouching);

		EnhancedInputComponent->BindAction(GrenadeAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::ThrowGrenade);
		EnhancedInputComponent->BindAction(InteractionAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::Interaction);

		EnhancedInputComponent->BindAction(SwitchToFirstWeaponAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::SwitchToFirstWeapon);
		EnhancedInputComponent->BindAction(SwitchToSecondaryWeaponAction, ETriggerEvent::Triggered, this, &ASurvivalCharacter::SwitchToSecondaryWeapon);
	}
	else
	{
		UE_LOG(LogTemplateCharacter, Error, TEXT("'%s' Failed to find an Enhanced Input Component! This template is built to use the Enhanced Input system. If you intend to use the legacy system, then you will need to update this C++ file."), *GetNameSafe(this));
	}
}

void ASurvivalCharacter::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
}

void ASurvivalCharacter::BeginPlay()
{
	Super::BeginPlay();


	if (StarterWeapon)
	{
		ASurvivalWeaponActor* SpawnedWeapon = GetWorld()->SpawnActor<ASurvivalWeaponActor>(
			StarterWeapon,
			GetMesh1P()->GetSocketTransform(FName("Weapon"))
		);
		if (SpawnedWeapon)
		{
			SpawnedWeapon->AttachWeapon(this);
			SpawnedWeapon->Mesh->SetVisibility(false);
			QueueEquip(SpawnedWeapon);
			AmmoByWeaponType.Find(SpawnedWeapon->WeaponType)->CurrentAmmo += SpawnedWeapon->MagazineSize * 3;
			CurrentItems[0] = SpawnedWeapon;
			//SpawnedWeapon->BulletsLeftInMagazine = SpawnedWeapon->MagazineSize;
		}
	}




	if (PlayerHealthWidgetClass)
	{
		PlayerHealthWidget = CreateWidget<UUserWidget>(GetWorld(), PlayerHealthWidgetClass);
		if (PlayerHealthWidget)
		{
			PlayerHealthWidget->AddToViewport();
		}
	}




	if (HeadbobCurve)
	{
		FOnTimelineVector ProgressUpdate;
		ProgressUpdate.BindUFunction(this, FName("HeadbobUpdate"));
	
		HeadbobTimeline.AddInterpVector(HeadbobCurve, ProgressUpdate);
		HeadbobTimeline.SetLooping(true);
		HeadbobTimeline.Play();
	}

	fHealth = fMaxHealth;
	UpdateUI();
}



void ASurvivalCharacter::OnPowerupButton1Clicked()
{
	ApplyPowerUp(0);
}

void ASurvivalCharacter::OnPowerupButton2Clicked()
{
	ApplyPowerUp(1);
}

void ASurvivalCharacter::OnPowerupButton3Clicked()
{
	ApplyPowerUp(2);
}

void ASurvivalCharacter::ApplyPowerUp(int ButtonSlot)
{
	switch (PowerUpButtons[ButtonSlot])
	{
	case EPowerUp::MaxHealth:
		fMaxHealth += 10.0f;
		break;
	case EPowerUp::Damage:

		break;
	case EPowerUp::ReloadSpeed:

		break;
	case EPowerUp::MovementSpeed:

		break;
	case EPowerUp::AmmoCapacity:

		break;
	case EPowerUp::RestoreHealth:
		fHealth += 100.0f;
		fHealth = std::min(fHealth, fMaxHealth);
		UpdateUI();
		break;
	case EPowerUp::ExtraAmmo:
		for (int i = 0; i < 2; i++)
		{
			if (CurrentItems[i])
			{
				auto& ammo = AmmoByWeaponType.Find(CurrentItems[i]->WeaponType)->CurrentAmmo;
				ammo += CurrentItems[i]->MagazineSize * 2;
				ammo = std::min(ammo, AmmoByWeaponType.Find(CurrentItems[i]->WeaponType)->MaxAmmo);
				UpdateUI();
			}
		}
		break;
	case EPowerUp::Grenades:
		NbGrenades += 3;
		break;

	default:
		break;
	}
	PowerUpCounter[static_cast<int>(PowerUpButtons[ButtonSlot])]++;
	UGameplayStatics::SetGamePaused(GetWorld(), false);
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (PC)
	{
		PC->bShowMouseCursor = false;

		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		PC->SetInputMode(InputMode);
	}
	if (PowerupSelectorWidget)
	{
		PowerupSelectorWidget->RemoveFromParent();
	}

}

void ASurvivalCharacter::OpenPowerUpSelector()
{

	UGameplayStatics::SetGamePaused(GetWorld(), true);
	APlayerController* PC = Cast<APlayerController>(GetController());

	if (PowerupSelectorWidgetClass)
	{
		PowerupSelectorWidget = CreateWidget<UUserWidget>(GetWorld(), PowerupSelectorWidgetClass);
		if (PowerupSelectorWidget)
		{
			PowerupSelectorWidget->AddToViewport();
		}

		for (int32 i = 0; i < 3; i++)
		{
			PowerUpButtons[i] = static_cast<EPowerUp>(rand() % static_cast<int>(EPowerUp::Num));

			FString ButtonName = FString::Printf(TEXT("Button_%d"), i + 1);
			UButton* Button = Cast<UButton>(PowerupSelectorWidget->GetWidgetFromName(*ButtonName));
			if (Button)
			{
				TScriptDelegate<> Delegate;
				FString FunctionName = FString::Printf(TEXT("OnPowerupButton%dClicked"), i + 1);
				FName FunctionFName(*FunctionName);
				Delegate.BindUFunction(this, FunctionFName);
				Button->OnClicked.Add(Delegate);
			}



			FString TextName = FString::Printf(TEXT("PowerUpText_%d"), i + 1);
			UTextBlock* Text = Cast<UTextBlock>(PowerupSelectorWidget->GetWidgetFromName(*TextName));
			if (Text)
			{
				Text->SetText(FText::FromString(GetPowerUpDisplayName(PowerUpButtons[i])));
			}
		}

		UVerticalBox* PowerupBox = Cast<UVerticalBox>(
			PowerupSelectorWidget->GetWidgetFromName(TEXT("PowerUpList"))
		);
		if (!PowerupBox) return;

		for (int32 i = 0; i < static_cast<int32>(EPowerUp::Num); i++)
		{
			EPowerUp PowerupValue = static_cast<EPowerUp>(i);


			UTextBlock* NewText = NewObject<UTextBlock>();
			if (!NewText) continue;
			PowerupBox->AddChild(NewText);
			FText DisplayName = FText::FromString(GetPowerUpDisplayName(PowerupValue));

			FString FullString = FString::Printf(TEXT("%s (%d)"), *DisplayName.ToString(), PowerUpCounter[i]);
			FText FullText = FText::FromString(FullString);


			NewText->SetText(FullText);

			NewText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			NewText->Font.Size = 24;

			UVerticalBoxSlot* Slot = PowerupBox->AddChildToVerticalBox(NewText);
			if (Slot)
			{
				Slot->SetPadding(FMargin(5.f));
				Slot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		if (PC)
		{
			PC->bShowMouseCursor = true;
			FInputModeUIOnly InputMode;
			InputMode.SetWidgetToFocus(PowerupSelectorWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		}
	}
}

void ASurvivalCharacter::AddMoney(int Amount)
{
	Money += Amount;

	UpdateUI();
}

void ASurvivalCharacter::UpdateUI()
{
	UProgressBar* ProgressBar = Cast<UProgressBar>(PlayerHealthWidget->GetWidgetFromName(FName("HealthBar")));
	if (ProgressBar)
	{
		ProgressBar->SetPercent(fHealth / fMaxHealth);
	}

	UTextBlock* MagazineText = Cast<UTextBlock>(PlayerHealthWidget->GetWidgetFromName("MagazineCount"));
	if (MagazineText && CurrentHeldWeapon)
	{
		MagazineText->SetText(FText::AsNumber(CurrentHeldWeapon->BulletsLeftInMagazine));
	}

	UTextBlock* AmmoText = Cast<UTextBlock>(PlayerHealthWidget->GetWidgetFromName("AmmoCount"));
	if (AmmoText && CurrentHeldWeapon)
	{
		AmmoText->SetText(FText::AsNumber(AmmoByWeaponType.Find(CurrentHeldWeapon->WeaponType)->CurrentAmmo));
	}

	UTextBlock* GrenadeText = Cast<UTextBlock>(PlayerHealthWidget->GetWidgetFromName("GrenadeCount"));
	if (GrenadeText)
	{
		GrenadeText->SetText(FText::AsNumber(NbGrenades));
	}
	UTextBlock* MoneyText = Cast<UTextBlock>(PlayerHealthWidget->GetWidgetFromName("Money"));
	if (MoneyText)
	{
		MoneyText->SetText(FText::FromString(FString::Printf(TEXT("%d $"), Money)));
	}
}

bool ASurvivalCharacter::IsEquiping()
{
	return QueuedEquip != nullptr;
}

void ASurvivalCharacter::HeadbobUpdate(FVector Alpha)
{
	CameraHeadbobOffset.SetRotation(FQuat(FRotator(Alpha.X, Alpha.Y, Alpha.Z)));
}

void ASurvivalCharacter::OnEquipAnimationEnded(UAnimMontage* Montage, bool bInterrupted)
{
	bIsEquiping = false;
}

void ASurvivalCharacter::HeadbobFinished()
{
}

void ASurvivalCharacter::OnUnequipAnimationEnded(UAnimMontage* Montage, bool bInterrupted)
{

	if (bInterrupted)
	{
		QueuedEquip = nullptr;
		return;
	}

	SetCurrentHeldWeapon(QueuedEquip);
	QueuedEquip = nullptr;
}

void ASurvivalCharacter::ThrowCurrentHeldWeapon()
{
	CurrentHeldWeapon->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	CurrentHeldWeapon->EnableSimulation();
	FVector ThrowDirection = FirstPersonCameraComponent->GetForwardVector();
	CurrentHeldWeapon->Mesh->AddImpulse(ThrowDirection * 300.f, NAME_None, true);
	CurrentHeldWeapon = nullptr;
	CurrentItems[HeldItemIndex] = nullptr;
}

void ASurvivalCharacter::SetCurrentHeldWeapon(ASurvivalWeaponActor* weapon)
{


	CurrentHeldWeapon = weapon;



	UpdateUI();
	bIsEquiping = true;
	//if (CurrentHeldWeapon->EquipAnimation != nullptr)
	//{
	//	UAnimInstance* AnimInstance = GetMesh1P()->GetAnimInstance();
	//	if (AnimInstance != nullptr)
	//	{
	//
	//		AnimInstance->Montage_Play(CurrentHeldWeapon->EquipAnimation, 1.0f);
	//
	//		// Bind callback
	//		FOnMontageEnded MontageEndedDelegate;
	//		MontageEndedDelegate.BindUObject(this, &ASurvivalCharacter::OnEquipAnimationEnded);
	//
	//		AnimInstance->Montage_SetEndDelegate(MontageEndedDelegate, CurrentHeldWeapon->EquipAnimation);
	//	}
	//}
	for (int i = 0; i < MaxItems; i++)
	{
		if (CurrentItems[i] != nullptr)
		{
			CurrentItems[i]->Mesh->SetVisibility(false);
		}
	}
	CurrentHeldWeapon->Mesh->SetVisibility(true);
	if (CurrentHeldWeapon && CurrentHeldWeapon->Mesh->DoesSocketExist(TEXT("LHand")))
	{
		FTransform WorldTransform = CurrentHeldWeapon->Mesh->GetSocketTransform(TEXT("LHand"), RTS_World);
		FTransform BoneTransform = Mesh1P->GetSocketTransform(TEXT("hand_r"), RTS_World);
		CurrentLeftHandIkTransform = WorldTransform.GetRelativeTransform(BoneTransform);
	}

}

void ASurvivalCharacter::ApplyRecoilKick(float Amount)
{
	
	//fHitReaction += Amount * 0.4f;




	float AimingMult = bIsAiming ? 0.7f : 1.0f;

	fRecoiling = 0.1f;

	HandRecoilSpring.Rate.Pitch += -Amount * WeaponRecoilMultiplier * AimingMult * 7.0f;
	HandRecoilSpring.Rate.Yaw += FMath::RandRange(-Amount, Amount) * WeaponRecoilMultiplier * 0.25f * AimingMult;
	HandRecoilSpring.Rate.Roll += FMath::RandRange(-Amount, Amount) * WeaponRecoilMultiplier * 0.8f * AimingMult;

	CameraRecoilSpring.Rate.Pitch = Amount * 10.0f * CameraRecoilMultiplier;
	//CameraRecoilSpring.Rate.Yaw += FMath::RandRange(-Amount * 0.6f , Amount * 2.0f) * CameraRecoilMultiplier * SprayingMultiplier;



	//if (CameraShakeRecoilClass)
	//{
	//	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	//	{
	//		PC->ClientStartCameraShake(CameraShakeRecoilClass);
	//	}
	//}
}

void ASurvivalCharacter::SetHoldingMagazine(bool State)
{
	bHoldingMagazine = State;
}

FTransform ASurvivalCharacter::GetWeaponLeftHandSocketTransform() const
{
	return CurrentLeftHandIkTransform;
}

float ASurvivalCharacter::GetHorizontalInputVelocity() const
{
	return /*InputVelocity.X **/ FSmoothedVelocityLength;
}

float ASurvivalCharacter::GetVerticalInputVelocity() const
{
	return InputVelocity.Y * FSmoothedVelocityLength;
}

bool ASurvivalCharacter::IsAiming() const
{
	return bIsAiming;
}

bool ASurvivalCharacter::IsRunning() const
{
	return bIsRunning;
}

FTransform ASurvivalCharacter::GetRecoilOffset() const
{
	return WeaponRecoilOffset;
}

ASurvivalWeaponActor* ASurvivalCharacter::GetCurrentHeldWeapon() const
{
	return CurrentHeldWeapon;
}

void ASurvivalCharacter::Move(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D MovementVector = Value.Get<FVector2D>();

	if (Controller != nullptr)
	{

		WantedInputVelocity = MovementVector;
		if (!WantedInputVelocity.IsNearlyZero())
		{
			WantedInputVelocity.Normalize();
		}




		// add movement 
		AddMovementInput(GetActorForwardVector(), MovementVector.Y < 0.0f ? MovementVector.Y * 0.8f : MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X *0.8f);
	}
}

void ASurvivalCharacter::StartAiming(const FInputActionValue& Instance)
{
	bIsAiming = true;
}

void ASurvivalCharacter::StopAiming(const FInputActionValue& Instance)
{
	bIsAiming = false;
}

void ASurvivalCharacter::StartSprinting(const FInputActionValue& Instance)
{
	bWantsToRun = true;
}

void ASurvivalCharacter::StopSprinting(const FInputActionValue& Instance)
{
	bWantsToRun = false;
}

void ASurvivalCharacter::StartCrouching(const FInputActionValue& Instance)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bWantsToCrouch = true;
	}
}

void ASurvivalCharacter::StopCrouching(const FInputActionValue& Instance)
{
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->bWantsToCrouch = false;
	}
}

void ASurvivalCharacter::ThrowGrenade(const FInputActionValue& Instance)
{
	if (!bCanThrowGrenade || NbGrenades <= 0)
	{
		return;
	}
	NbGrenades--;
	UpdateUI();
	bCanThrowGrenade = false;

	GetWorldTimerManager().SetTimer(
		GrenadeCooldownTimerHandle,
		this,
		&ASurvivalCharacter::ResetGrenadeCooldown,
		1.5f,
		false
	);


	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	Params.Instigator = this;

	FTransform GrenadeSpawn{};

	GrenadeSpawn.SetLocation(FirstPersonCameraComponent->GetComponentLocation() + FirstPersonCameraComponent->GetForwardVector());
	GrenadeSpawn.SetRotation(FQuat(FirstPersonCameraComponent->GetComponentRotation()));
	AGrenade* SpawnedGrenade = GetWorld()->SpawnActor<AGrenade>(
		Grenade,
		GrenadeSpawn,
		Params
	);
}

void ASurvivalCharacter::Interaction(const FInputActionValue& Instance)
{
	if (IsValid(FocusedItem) && !FocusedItem->IsActorBeingDestroyed())
	{
		FocusedItem->OnPlayerInteract(this);
	}
}

void ASurvivalCharacter::SwitchToFirstWeapon(const FInputActionValue& Instance)
{
	if (CurrentHeldWeapon != nullptr && !IsEquiping() && HeldItemIndex != 0)
	{
		HeldItemIndex = 0;
		if (CurrentItems[HeldItemIndex])
		{
			QueueEquip(CurrentItems[HeldItemIndex]);
		}
	}
}

void ASurvivalCharacter::SwitchToSecondaryWeapon(const FInputActionValue& Instance)
{
	if (CurrentHeldWeapon != nullptr && !IsEquiping() && HeldItemIndex != 1)
	{
		HeldItemIndex = 1;
		if (CurrentItems[HeldItemIndex])
		{
			QueueEquip(CurrentItems[HeldItemIndex]);
		}
	}
}

void ASurvivalCharacter::QueueEquip(ASurvivalWeaponActor* Weapon)
{
	Weapon->Mesh->SetVisibility(false);

	UAnimInstance* AnimInstance = GetMesh1P()->GetAnimInstance();
	if (AnimInstance != nullptr && Weapon)
	{
		QueuedEquip = Weapon;
		if (CurrentHeldWeapon && CurrentHeldWeapon->UnequipAnimation)
		{
			AnimInstance->Montage_Play(CurrentHeldWeapon->UnequipAnimation, 1.2f);
			FOnMontageBlendingOutStarted MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &ASurvivalCharacter::OnUnequipAnimationEnded);

			AnimInstance->Montage_SetBlendingOutDelegate(MontageEndedDelegate, CurrentHeldWeapon->UnequipAnimation);
		}
		else if (Weapon->UnequipAnimation)
		{
			AnimInstance->Montage_Play(Weapon->UnequipAnimation, 1.2f);
			FOnMontageBlendingOutStarted MontageEndedDelegate;
			MontageEndedDelegate.BindUObject(this, &ASurvivalCharacter::OnUnequipAnimationEnded);

			AnimInstance->Montage_SetBlendingOutDelegate(MontageEndedDelegate, Weapon->UnequipAnimation);
		}
		else
		{
			return;
		}
	}
}

void ASurvivalCharacter::EquipOrReplaceWeapon(ASurvivalWeaponActor* Weapon)
{
	constexpr int WeaponSlotInvalid = -1;

	int FreeSlot = WeaponSlotInvalid;

	for (int i = 0; i < MaxItems; i++)
	{
		if (CurrentItems[i] == nullptr)
		{
			FreeSlot = i;
			break;
		}
	}

	if (FreeSlot == WeaponSlotInvalid)
	{
		ThrowCurrentHeldWeapon();
		FreeSlot = HeldItemIndex;
	}

	CurrentItems[FreeSlot] = Weapon;
	HeldItemIndex = FreeSlot;
	QueueEquip(Weapon);
}

void ASurvivalCharacter::ResetGrenadeCooldown()
{
	bCanThrowGrenade = true;
}

void ASurvivalCharacter::Look(const FInputActionValue& Value)
{
	// input is a Vector2D
	FVector2D LookAxisVector = Value.Get<FVector2D>();
	LookAxisVector *= 0.5f;
	if (Controller != nullptr)
	{
		float AimingMultiplier = bIsAiming ? 0.6f : 1.0f;
		LookAxisVector *= AimingMultiplier;
		HandSwaySpring.Value.Yaw -= LookAxisVector.X * SwayMultiplier;
		HandSwaySpring.Value.Pitch -= LookAxisVector.Y * SwayMultiplier;

		// add yaw and pitch input to controller
		AddControllerYawInput(LookAxisVector.X);
		AddControllerPitchInput(LookAxisVector.Y);
	}
}


void ASurvivalCharacter::PlayRandomFootstepSound()
{
	UAISense_Hearing::ReportNoiseEvent(
		GetWorld(),
		GetActorLocation(),
		0.7f,              // Loudness
		this,              // Instigator
		GetVelocity().Length() / 400.0f * 1500.f,
		TEXT("Footstep")
	);
	if (!GetCharacterMovement()->IsFalling())
	{
		FootstepAudioComponent->SetSound(FootstepSounds[FMath::RandHelper(FootstepSounds.Num())]);
		FootstepAudioComponent->Play();
	}

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
		&ASurvivalCharacter::PlayRandomFootstepSound,
		Interval,
		true
	);
}