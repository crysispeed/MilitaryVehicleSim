#include "TurretComponent.h"

UTurretComponent::UTurretComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	MuzzleSocketName = TEXT("MuzzleSocket");
	RotationSpeed = 50.0f;
	VisualRotationOffset = FRotator::ZeroRotator;
	MuzzleRotationOffset = FRotator::ZeroRotator;

	// Ensure the turret doesn't affect the center of mass or physics movement
	UPrimitiveComponent::SetMassOverrideInKg(NAME_None, 0.001f);
	UStaticMeshComponent::SetCollisionProfileName(TEXT("NoCollision"));
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);

	SetGenerateOverlapEvents(false);
	bApplyImpulseOnDamage = false;
	SetIsReplicatedByDefault(true);
	SetCanEverAffectNavigation(false);
}

void UTurretComponent::RotateTurret(float YawInput)
{
	if (YawInput == 0.0f) return;

	float DeltaYaw = YawInput * RotationSpeed * GetWorld()->GetDeltaSeconds();
	AddLocalRotation(FRotator(0.0f, DeltaYaw, 0.0f));
}

FVector UTurretComponent::GetMuzzleLocation() const
{
	if (DoesSocketExist(MuzzleSocketName))
	{
		return GetSocketLocation(MuzzleSocketName);
	}
	return GetComponentLocation();
}

FRotator UTurretComponent::GetMuzzleRotation() const
{
	FRotator BaseRotation = GetComponentRotation();
	if (DoesSocketExist(MuzzleSocketName))
	{
		BaseRotation = GetSocketRotation(MuzzleSocketName);
	}
	
	return (BaseRotation.Quaternion() * MuzzleRotationOffset.Quaternion()).Rotator();
}
