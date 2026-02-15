// Fill out your copyright notice in the Description page of Project Settings.


#include "MilitaryVehicleBase.h"

#include "MilitaryVehicleSim/Components/TurretComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Net/UnrealNetwork.h"
#include "ChaosWheeledVehicleMovementComponent.h"
#include "MilitaryVehicleSim/Components/HealthComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"

AMilitaryVehicleBase::AMilitaryVehicleBase()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	AActor::SetReplicateMovement(true);

	// Create health component
	HealthComponent = CreateDefaultSubobject<UHealthComponent>(TEXT("HealthComponent"));

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);

	TurretComponent = CreateDefaultSubobject<UTurretComponent>(TEXT("TurretComponent"));
	TurretComponent->SetupAttachment(GetMesh());
	// Turret should not affect center of mass or physics
	TurretComponent->SetMassOverrideInKg(NAME_None, 0.001f);
	TurretComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	TurretComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	// Set initial rotation to handle potential model axis issues (e.g. looking right instead of forward)
	TurretComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	// Fix projectile firing direction (currently goes left, which is +90 deg relative to forward if it was looking right)
	// Actually if it was looking right (X=Right), and we rotated it -90 (X=Forward).
	// If it fires left, it fires along Mesh Y. Mesh Y is Mesh X + 90.
	// So we need to subtract 90 degrees to bring it back to Mesh X.
	TurretComponent->SetMuzzleRotationOffset(FRotator(0.0f, 90.0f, 0.0f));
	TurretComponent->SetIsReplicated(true);

	// Create third person camera setup
	ThirdPersonSpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonSpringArm"));
	ThirdPersonSpringArm->SetupAttachment(GetMesh());
	ThirdPersonSpringArm->SetMobility(EComponentMobility::Movable);
	ThirdPersonSpringArm->TargetArmLength = 800.0f;
	ThirdPersonSpringArm->bUsePawnControlRotation = true;
	ThirdPersonSpringArm->bInheritPitch = true;
	ThirdPersonSpringArm->bInheritYaw = true;
	ThirdPersonSpringArm->bInheritRoll = false;

	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(ThirdPersonSpringArm, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;

	// Create gunner sight camera
	GunnerSightCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("GunnerSightCamera"));
	GunnerSightCamera->SetupAttachment(TurretComponent);
	// Position it at the back of the turret and slightly up
	GunnerSightCamera->SetRelativeLocation(FVector(0.0f, -250.0f, 200.0f));
	GunnerSightCamera->SetRelativeRotation(FRotator(0.0f, 90.0f, 0.0f));
	GunnerSightCamera->bUsePawnControlRotation = false;
		
	// Default state
	bIsDriverRole = true;
	bIsThirdPersonCamera = true;
	TurretYaw = -90.0f; // Initialize to match the TurretComponent's relative rotation
}

void AMilitaryVehicleBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMilitaryVehicleBase, bIsDriverRole);
	DOREPLIFETIME(AMilitaryVehicleBase, bIsThirdPersonCamera);
	DOREPLIFETIME(AMilitaryVehicleBase, TurretYaw);
}

void AMilitaryVehicleBase::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent && HasAuthority())
	{
		for (TSubclassOf<UGameplayAbility>& AbilityClass : InitialAbilities)
		{
			if (AbilityClass)
			{
				AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
			}
		}
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	if (ThirdPersonCamera && ThirdPersonSpringArm)
	{
		ThirdPersonCamera->AttachToComponent(ThirdPersonSpringArm, FAttachmentTransformRules::KeepRelativeTransform, USpringArmComponent::SocketName);
	}

	// Initialize camera state
	UpdateCameraState();
	
	// Add Input Mapping Context
	if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
		{
			if (InputMappingContext)
			{
				Subsystem->AddMappingContext(InputMappingContext, 0);
			}
		}
	}
}

void AMilitaryVehicleBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

UChaosWheeledVehicleMovementComponent* AMilitaryVehicleBase::GetChaosVehicleMovement() const
{
	return Cast<UChaosWheeledVehicleMovementComponent>(GetVehicleMovementComponent());
}

void AMilitaryVehicleBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Cast to Enhanced Input Component
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	// Bind Enhanced Input Actions
	if (ThrottleAction)
	{
		EnhancedInputComponent->BindAction(ThrottleAction, ETriggerEvent::Triggered, this, &AMilitaryVehicleBase::OnThrottle);
	}
	
	if (SteeringAction)
	{
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Triggered, this, &AMilitaryVehicleBase::OnSteer);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Started, this, &AMilitaryVehicleBase::OnSteer);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Ongoing, this, &AMilitaryVehicleBase::OnSteer);
		EnhancedInputComponent->BindAction(SteeringAction, ETriggerEvent::Completed, this, &AMilitaryVehicleBase::OnSteer);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AMilitaryVehicleBase::OnLook);
	}

	if (BrakeAction)
	{
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Completed, this, &AMilitaryVehicleBase::OnBrake);
		EnhancedInputComponent->BindAction(BrakeAction, ETriggerEvent::Triggered, this, &AMilitaryVehicleBase::OnBrake);
	}
	
	if (HandbrakeAction)
	{
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Triggered, this, &AMilitaryVehicleBase::OnHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Started, this, &AMilitaryVehicleBase::OnHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Ongoing, this, &AMilitaryVehicleBase::OnHandbrake);
		EnhancedInputComponent->BindAction(HandbrakeAction, ETriggerEvent::Completed, this, &AMilitaryVehicleBase::OnHandbrake);
	}

	if (FireAction)
	{
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AMilitaryVehicleBase::OnFire);
	}
	
	if (LookWithMouseAction)
	{
		EnhancedInputComponent->BindAction(LookWithMouseAction, ETriggerEvent::Triggered, this, &AMilitaryVehicleBase::OnLookWithMouse);
	}

	if (ToggleRoleAction)
	{
		EnhancedInputComponent->BindAction(ToggleRoleAction, ETriggerEvent::Started, this, &AMilitaryVehicleBase::OnToggleRole);
	}
}

void AMilitaryVehicleBase::UpdateCameraState()
{
	if (bIsDriverRole)
	{
		// Driver always uses third person
		ThirdPersonCamera->SetActive(true);
		GunnerSightCamera->SetActive(false);
	}
	else
	{
		// Gunner can switch between third person and gunner sight
		ThirdPersonCamera->SetActive(bIsThirdPersonCamera);
		GunnerSightCamera->SetActive(!bIsThirdPersonCamera);
	}
}

// Enhanced Input Callbacks
void AMilitaryVehicleBase::OnThrottle(const FInputActionValue& Value)
{
	// Get the 2D vector from input
	const float SteerAmount = Value.Get<float>();

	if (bIsDriverRole)
	{
		// Forward/Backward (Y axis)
		if (UChaosWheeledVehicleMovementComponent* VehicleMovement = GetChaosVehicleMovement())
		{
			VehicleMovement->SetThrottleInput(SteerAmount);
		}
	}
}
void AMilitaryVehicleBase::OnSteer(const FInputActionValue& Value)
{
	const float SteerAmount = Value.Get<float>();

	if (bIsDriverRole)
	{
		// Forward/Backward (Y axis)
		if (UChaosWheeledVehicleMovementComponent* VehicleMovement = GetChaosVehicleMovement())
		{
			VehicleMovement->SetSteeringInput(SteerAmount);
		}
	}
}

void AMilitaryVehicleBase::OnBrake(const FInputActionValue& Value)
{
	const float SteerAmount = Value.Get<float>();
	
	if (bIsDriverRole)
	{
		if (UChaosWheeledVehicleMovementComponent* VehicleMovement = GetChaosVehicleMovement())
		{
			VehicleMovement->SetBrakeInput(SteerAmount);
		}
	}
}

void AMilitaryVehicleBase::OnHandbrake(const FInputActionValue& Value)
{
	const bool bNewHandbrake = Value.Get<bool>();
	
	if (bIsDriverRole)
	{
		if (UChaosWheeledVehicleMovementComponent* VehicleMovement = GetChaosVehicleMovement())
		{
			VehicleMovement->SetHandbrakeInput(bNewHandbrake);
		}
	}
}

void AMilitaryVehicleBase::OnLook(const FInputActionValue& Value)
{
	// Get the 2D vector from mouse movement
	const FVector2D LookVector = Value.Get<FVector2D>();


}

void AMilitaryVehicleBase::OnBrakeStarted(const FInputActionValue& Value)
{
	if (bIsDriverRole)
	{
		if (UChaosWheeledVehicleMovementComponent* VehicleMovement = GetChaosVehicleMovement())
		{
			VehicleMovement->SetBrakeInput(1.0f);
		}
	}
}

void AMilitaryVehicleBase::OnBrakeCompleted(const FInputActionValue& Value)
{
	if (bIsDriverRole)
	{
		if (UChaosWheeledVehicleMovementComponent* VehicleMovement = GetChaosVehicleMovement())
		{
			VehicleMovement->SetBrakeInput(0.0f);
		}
	}
}

void AMilitaryVehicleBase::OnFire(const FInputActionValue& Value)
{
	if (bIsDriverRole) return;

	if (AbilitySystemComponent && TurretComponent)
	{
		FGameplayEventData Payload;
		Payload.Instigator = this;
		
		FHitResult Hit;
		Hit.Location = TurretComponent->GetMuzzleLocation();
		Hit.ImpactNormal = TurretComponent->GetMuzzleRotation().Vector();
		
		Payload.TargetData.Add(new FGameplayAbilityTargetData_SingleTargetHit(Hit));

		// Use a generic tag or a specific one if known. 
		// Since we don't have tags defined, we can try to activate by tag "Ability.Fire" 
		// but we also need to make sure the ability has this tag.
		// For now, let's use the existing TryActivateAbility but we need a way to pass data.
		
		// Actually, the best way without tags is to use a custom RPC if we want to be sure.
		// But let's try to find if there's any tag on the spec.
		
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			// We can't easily pass TriggerEventData to TryActivateAbility.
			// However, we can use TriggerAbilityFromGameplayEvent if we know the tag.
			// If we don't know the tag, we can try to use the ability's own tags.
			if (Spec.Ability->AbilityTags.HasTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Fire"), false)))
			{
				AbilitySystemComponent->TriggerAbilityFromGameplayEvent(Spec.Handle, AbilitySystemComponent->AbilityActorInfo.Get(), FGameplayTag::RequestGameplayTag(TEXT("Ability.Fire")), &Payload, *AbilitySystemComponent);
				return;
			}
		}

		// Fallback to original behavior if no tag matches
		for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
		{
			AbilitySystemComponent->TryActivateAbility(Spec.Handle);
		}
	}
}

void AMilitaryVehicleBase::OnToggleRole(const FInputActionValue& Value)
{
	if (HasAuthority())
	{
		Server_ToggleRole_Implementation();
	}
	else
	{
		Server_ToggleRole();
	}
}

void AMilitaryVehicleBase::Server_ToggleRole_Implementation()
{
	bIsDriverRole = !bIsDriverRole;
	
	// When switching roles, also update the camera preference
	// Driver role always implies third person
	// Gunner role will now default to gunner sight when switched to
	bIsThirdPersonCamera = bIsDriverRole;
	
	if (IsNetMode(NM_Standalone) || HasAuthority())
	{
		OnRep_IsDriverRole();
	}
}

void AMilitaryVehicleBase::OnLookWithMouse(const FInputActionValue& Value)
{
	// Get the 2D vector from mouse movement
	const FVector2D LookVector = Value.Get<FVector2D>();
	
	if (bIsDriverRole)
	{
		if (ThirdPersonSpringArm)
		{
			const FRotator LookRotation = FRotator(-LookVector.Y, LookVector.X, 0.0f);
			// Add local rotation based on mouse input
			ThirdPersonSpringArm->AddLocalRotation(LookRotation);
			
			// Get current relative rotation and clamp Pitch and Yaw
			FRotator CurrentRotation = ThirdPersonSpringArm->GetRelativeRotation();
			
			CurrentRotation.Pitch = FMath::ClampAngle(CurrentRotation.Pitch, -50.0f, -10.0f);
			CurrentRotation.Yaw = FMath::ClampAngle(CurrentRotation.Yaw, -90.0f, 90.0f);
			CurrentRotation.Roll = 0.0f;
			
			// Apply the clamped rotation
			ThirdPersonSpringArm->SetRelativeRotation(CurrentRotation);
		}
	}
	else
	{
		// Gunner role - rotate turret
		if (TurretComponent)
		{
			TurretComponent->RotateTurret(LookVector.X);
			
			// Update the replicated variable on the client so that 
			// it's immediately available for the local ability activation
			TurretYaw = TurretComponent->GetRelativeRotation().Yaw;
			
			// Send to server
			if (!HasAuthority())
			{
				Server_RotateTurret(LookVector.X, TurretYaw);
			}
		}
	}
}

void AMilitaryVehicleBase::Server_RotateTurret_Implementation(float YawInput, float CurrentYaw)
{
	if (TurretComponent)
	{
		// Use the client's current yaw as a base to maintain synchronization
		FRotator NewRotation = TurretComponent->GetRelativeRotation();
		NewRotation.Yaw = CurrentYaw;
		TurretComponent->SetRelativeRotation(NewRotation);

		// Also apply the input for the current frame to keep it smooth
		TurretComponent->RotateTurret(YawInput);
		
		// Update TurretYaw for replication to other clients
		TurretYaw = TurretComponent->GetRelativeRotation().Yaw;
	}
}

void AMilitaryVehicleBase::OnRep_TurretYaw()
{
	if (TurretComponent)
	{
		FRotator NewRotation = TurretComponent->GetRelativeRotation();
		NewRotation.Yaw = TurretYaw;
		TurretComponent->SetRelativeRotation(NewRotation);
	}
}

void AMilitaryVehicleBase::SwitchCamera()
{
	bIsThirdPersonCamera = !bIsThirdPersonCamera;
	UpdateCameraState();
}

void AMilitaryVehicleBase::PossessAsDriver(AController* VehicleController)
{
	if (VehicleController)
	{
		VehicleController->Possess(this);
		bIsDriverRole = true;
		OnRep_IsDriverRole();
	}
}

void AMilitaryVehicleBase::PossessAsGunner(AController* VehicleController)
{
	if (VehicleController)
	{
		VehicleController->Possess(this);
		bIsDriverRole = false;
		OnRep_IsDriverRole();
	}
}

void AMilitaryVehicleBase::OnRep_IsDriverRole()
{
	UpdateCameraState();
}