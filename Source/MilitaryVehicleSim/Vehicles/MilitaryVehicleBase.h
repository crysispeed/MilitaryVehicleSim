// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "WheeledVehiclePawn.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "MilitaryVehicleBase.generated.h"

class UCameraComponent;
class USpringArmComponent;
class UTurretComponent;
class UChaosWheeledVehicleMovementComponent;
class UHealthComponent;
class UAbilitySystemComponent;

class UInputMappingContext;
class UInputAction;

/**
 * 
 */
UCLASS()
class MILITARYVEHICLESIM_API AMilitaryVehicleBase : public AWheeledVehiclePawn, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AMilitaryVehicleBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override { return AbilitySystemComponent; }

	// Role management
	void PossessAsDriver(AController* Controller);
	void PossessAsGunner(AController* Controller);

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	bool IsDriverRole() const { return bIsDriverRole; }

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	bool IsGunnerRole() const { return !bIsDriverRole; }

	UFUNCTION(Server, Reliable)
	void Server_ToggleRole();

	UFUNCTION(Server, Reliable)
	void Server_RotateTurret(float YawInput, float CurrentYaw);

	UPROPERTY(ReplicatedUsing = OnRep_TurretYaw)
	float TurretYaw;

	UFUNCTION()
	void OnRep_TurretYaw();

	// Camera management
	void SwitchCamera();

	UFUNCTION(BlueprintCallable, Category = "Vehicle")
	UHealthComponent* GetHealthComponent() const { return HealthComponent; }

	// Get typed vehicle movement component
	UChaosWheeledVehicleMovementComponent* GetChaosVehicleMovement() const;

protected:
	virtual void BeginPlay() override;
	
	// Input handlers
	void OnThrottle(const struct FInputActionValue& Value);
	void OnSteer(const struct FInputActionValue& Value);
	void OnBrake(const struct FInputActionValue& Value);
	void OnHandbrake(const struct FInputActionValue& Value);
	void OnLook(const struct FInputActionValue& Value);
	void OnBrakeStarted(const struct FInputActionValue& Value);
	void OnBrakeCompleted(const struct FInputActionValue& Value);
	void OnFire(const struct FInputActionValue& Value);
	void OnToggleRole(const struct FInputActionValue& Value);
	void OnLookWithMouse(const struct FInputActionValue& Value);
	
	UFUNCTION()
	void OnRep_IsDriverRole();
	
	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UHealthComponent> HealthComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UTurretComponent> TurretComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> ThirdPersonSpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> GunnerSightCamera;

	// Enhanced Input Actions and Mapping Context
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SteeringAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ThrottleAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> BrakeAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> HandbrakeAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> FireAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ToggleRoleAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookWithMouseAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Abilities")
	TArray<TSubclassOf<UGameplayAbility>> InitialAbilities;
	
	// Role state
	UPROPERTY(ReplicatedUsing = OnRep_IsDriverRole)
	bool bIsDriverRole;

	UPROPERTY(Replicated)
	bool bIsThirdPersonCamera;

private:
	void UpdateCameraState();
};
