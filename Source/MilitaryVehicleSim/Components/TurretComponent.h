#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "TurretComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MILITARYVEHICLESIM_API UTurretComponent : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	UTurretComponent();

	/** Rotates the turret on the yaw axis. */
	void RotateTurret(float YawInput);

	/** Returns the location of the MuzzleSocket. */
	FVector GetMuzzleLocation() const;

	/** Returns the rotation of the MuzzleSocket. */
	FRotator GetMuzzleRotation() const;

protected:
	UPROPERTY(EditDefaultsOnly, Category = "Turret")
	FName MuzzleSocketName;

	UPROPERTY(EditDefaultsOnly, Category = "Turret")
	float RotationSpeed;

	/** Offset applied to the mesh to align its forward axis. */
	UPROPERTY(EditDefaultsOnly, Category = "Turret")
	FRotator VisualRotationOffset;

	/** Offset applied to the muzzle rotation to align the firing direction. */
	UPROPERTY(EditDefaultsOnly, Category = "Turret")
	FRotator MuzzleRotationOffset;

public:
	/** Sets the muzzle rotation offset. */
	void SetMuzzleRotationOffset(const FRotator& NewOffset) { MuzzleRotationOffset = NewOffset; }
};
