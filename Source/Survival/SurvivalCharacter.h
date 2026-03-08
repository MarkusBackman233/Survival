// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "Components/TimelineComponent.h"
#include <InputAction.h>
#include "MyWeaponType.h"
#include "SurvivalCharacter.generated.h"



UENUM(BlueprintType)
enum class EPowerUp : uint8
{
	MaxHealth,
	Damage,
	ReloadSpeed,
	MovementSpeed,
	AmmoCapacity,
	RestoreHealth,
	ExtraAmmo,
	Grenades,
	Num
};

inline FString GetPowerUpDisplayName(EPowerUp PowerUp)
{
	switch (PowerUp)
	{
	case EPowerUp::MaxHealth: return TEXT("Increase Max Health + 10");
	case EPowerUp::Damage: return TEXT("Damage + 3%");
	case EPowerUp::ReloadSpeed: return TEXT("Reload Speed + 10%");
	case EPowerUp::MovementSpeed: return TEXT("Movement Speed + 5%");
	case EPowerUp::AmmoCapacity: return TEXT("1 Extra Bullet Per Magazine");
	case EPowerUp::RestoreHealth: return TEXT("Restore 100 Health");
	case EPowerUp::ExtraAmmo: return TEXT("Get Two Extra Magazines");
	case EPowerUp::Grenades: return TEXT("Get Three Grenades");
	default: return TEXT("Unknown");
	}
}


class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class ASurvivalWeaponActor;
class UCurveVector;
class AGrenade;
class APickupBase;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

UCLASS(config=Game)
class ASurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Mesh, meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* Mesh1P;



	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCameraComponent;

	/** MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;

	/** Jump Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;

	/** Look Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* LookAction;

	/** Aim Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* AimAction;

	/** Sprint Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SprintAction;

	/** Crouch Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* CrouchAction;

	/** Grenade Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* GrenadeAction;

	/** Interaction Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* InteractionAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchToFirstWeaponAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	class UInputAction* SwitchToSecondaryWeaponAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Headbob", meta = (AllowPrivateAccess = "true"))
	class UCurveVector* HeadbobCurve;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Headbob", meta = (AllowPrivateAccess = "true"))
	float HeadbobSpeedWalking = 350.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Headbob", meta = (AllowPrivateAccess = "true"))
	float HeadbobMultiplierWalking = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Headbob", meta = (AllowPrivateAccess = "true"))
	float HeadbobSpeedRunning = 400.0f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Headbob", meta = (AllowPrivateAccess = "true"))
	float HeadbobMultiplierRunning = 10.0f;

	UPROPERTY(EditAnywhere)
	TSubclassOf<AGrenade> Grenade;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ASurvivalWeaponActor> StarterWeapon;

	/** Move Input Action */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gameplay", meta = (AllowPrivateAccess = "true"))
	bool HeadbobbingEnabled = true;
public:



	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TSubclassOf<UCameraShakeBase> CameraShakeRecoilClass;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FTransform GetWeaponLeftHandSocketTransform() const;


	UFUNCTION(BlueprintCallable, Category = "Weapon")
	FTransform GetRecoilOffset() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	ASurvivalWeaponActor* GetCurrentHeldWeapon() const;



	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetHorizontalInputVelocity() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	float GetVerticalInputVelocity() const;

	UFUNCTION(BlueprintCallable, Category = "Weapon")
	bool IsAiming() const;

	UFUNCTION(BlueprintCallable, Category = "Gameplay")
	bool IsRunning() const;

	UPROPERTY(EditAnywhere, Category = "Movement")
	float WalkingMovementSpeed = 300.f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float SprintingMovementSpeed = 600.f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float AimingMovementSpeed = 100.f;
	UPROPERTY(EditAnywhere, Category = "Movement")
	float CrouchingMovementSpeed = 200.f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float RecoilStiffness = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float RecoilDamping = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float WeaponRecoilMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float SwayStiffness = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float SwayDamping = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float SwayMultiplier = 1.f;

	UPROPERTY(EditAnywhere, Category = "Weapon")
	float CameraStiffness = 7.0f;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float CameraDamping = 0.8f;
	UPROPERTY(EditAnywhere, Category = "Weapon")
	float CameraRecoilMultiplier = 1.f;


	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ammo")
	TMap<EMyWeaponType, FAmmoData> AmmoByWeaponType;



	UPROPERTY(EditAnywhere, Category = "Gameplay")
	float fMaxHealth = 100.0f;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PowerupSelectorWidgetClass;
	UPROPERTY()
	UUserWidget* PowerupSelectorWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
	TSubclassOf<UUserWidget> PlayerHealthWidgetClass;
	UPROPERTY()
	UUserWidget* PlayerHealthWidget = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	TArray<USoundBase*> FootstepSounds;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Audio)
	USoundAttenuation* FootstepSoundAttenuation = nullptr;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Audio)
	UAudioComponent* FootstepAudioComponent;

	UFUNCTION()
	void OnPowerupButton1Clicked();
	UFUNCTION()
	void OnPowerupButton2Clicked();
	UFUNCTION()
	void OnPowerupButton3Clicked();

	UFUNCTION()
	void ApplyPowerUp(int ButtonSlot);

	UFUNCTION()
	void OpenPowerUpSelector();



	void AddMoney(int Amount);


	EPowerUp PowerUpButtons[3];


	UPROPERTY()
	int PowerUpCounter[static_cast<int32>(EPowerUp::Num)];


public:
	ASurvivalCharacter();

protected:
	/** Called for movement input */
	void Move(const FInputActionValue& Value);

	/** Called for looking input */
	void Look(const FInputActionValue& Value);
	void Tick(float DeltaTime);

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	void StartAiming(const FInputActionValue& Instance);
	void StopAiming(const FInputActionValue& Instance);

	void StartSprinting(const FInputActionValue& Instance);
	void StopSprinting(const FInputActionValue& Instance);

	void StartCrouching(const FInputActionValue& Instance);
	void StopCrouching(const FInputActionValue& Instance);

	void ThrowGrenade(const FInputActionValue& Instance);

	void Interaction(const FInputActionValue& Instance);

	void SwitchToFirstWeapon(const FInputActionValue& Instance);
	void SwitchToSecondaryWeapon(const FInputActionValue& Instance);

	void ResetGrenadeCooldown();
	float fHitReaction = 0.0f;

protected:
	// APawn interface
	virtual void NotifyControllerChanged() override;
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	virtual void Landed(const FHitResult& Hit) override;

	virtual void BeginPlay() override;
	// End of APawn interface

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser
	) override;

	void PlayRandomFootstepSound();

	UPROPERTY()
	APickupBase* FocusedItem = nullptr;

	static constexpr int MaxItems = 2;

	UPROPERTY()
	ASurvivalWeaponActor* CurrentItems[MaxItems] = { nullptr, nullptr };

	UPROPERTY()
	ASurvivalWeaponActor* CurrentHeldWeapon = nullptr;

	UPROPERTY()
	ASurvivalWeaponActor* QueuedEquip = nullptr;

	FTimeline HeadbobTimeline;
	FTimerHandle FootstepTimerHandle;
	FTimerHandle GrenadeCooldownTimerHandle;
	UFUNCTION()
	void HeadbobUpdate(FVector Alpha);
	UFUNCTION()
	void HeadbobFinished();
	UFUNCTION()
	void OnUnequipAnimationEnded(UAnimMontage* Montage, bool bInterrupted);
	UFUNCTION()
	void OnEquipAnimationEnded(UAnimMontage* Montage, bool bInterrupted);



	FTransform CameraHeadbobOffset;
	FTransform WeaponRecoilOffset;

	struct Spring
	{
		UE::Math::TRotator<float> Value;
		UE::Math::TRotator<float> Rate;

		void Update(const float Stiffness, const float Damping, const float DeltaTime);
	};


	Spring HandRecoilSpring;
	Spring HandSwaySpring;
	Spring CameraRecoilSpring;
	

	float fFallingPitch = 0.0f;

	FVector2D InputVelocity;
	FVector2D WantedInputVelocity;

	FTransform CurrentLeftHandIkTransform;
	
	float FSmoothedVelocityLength = 0.0f;


	bool bIsAiming = false;
	bool bHoldingMagazine = false;

	bool bIsEquiping = false;
	bool bIsRunning = false;
	bool bWantsToRun = false;
	bool bCanThrowGrenade = true;
	float fLocalCameraTargetHeight = 0.0f;

	float fCrouchAlpha = 0.0f;

	float fRecoiling = 0.0f;

	float fHealth = 100.0f;

	int HeldItemIndex = 0;

	int Money = 0;

public:

	int NbGrenades = 3;

	/** Returns Mesh1P subobject **/
	USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

	bool IsDead() const { return fHealth <= 0.0f; }


	void ApplyRecoilKick(float Amount);
	void SetHoldingMagazine(bool State);

	void UpdateUI();

	bool IsEquiping();
	void QueueEquip(ASurvivalWeaponActor* Weapon);

	// if player already has two weapons, replace the one in the hands
	void EquipOrReplaceWeapon(ASurvivalWeaponActor* Weapon);

	private:
	void ThrowCurrentHeldWeapon();
	void SetCurrentHeldWeapon(ASurvivalWeaponActor* weapon);
};
