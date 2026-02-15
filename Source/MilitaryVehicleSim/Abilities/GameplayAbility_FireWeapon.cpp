// Fill out your copyright notice in the Description page of Project Settings.


#include "GameplayAbility_FireWeapon.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "MilitaryVehicleSim/Components/TurretComponent.h"
#include "MilitaryVehicleSim/Projectiles/ProjectileBase.h"
#include "AbilitySystemBlueprintLibrary.h"

UGameplayAbility_FireWeapon::UGameplayAbility_FireWeapon()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;

	FireCooldown = 1.0f;
	ProjectileDamage = 30.0f;

	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Fire")));
}

void UGameplayAbility_FireWeapon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// Get turret component from owner
	UTurretComponent* TurretComponent = Cast<UTurretComponent>(ActorInfo->OwnerActor->GetComponentByClass(UTurretComponent::StaticClass()));
	if (!TurretComponent)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FVector MuzzleLocation = TurretComponent->GetMuzzleLocation();
	FRotator MuzzleRotation = TurretComponent->GetMuzzleRotation();

	// If we have event data, use it for spawn location/rotation (it's better for network)
	if (TriggerEventData && TriggerEventData->TargetData.Num() > 0)
	{
		FGameplayAbilityTargetDataHandle TargetData = TriggerEventData->TargetData;
		FHitResult HitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetData, 0);
		
		MuzzleLocation = HitResult.Location;
		MuzzleRotation = HitResult.ImpactNormal.Rotation();
	}

	// Spawn projectile on server
	if (ActorInfo->OwnerActor->HasAuthority())
	{
		SpawnProjectile(MuzzleLocation, MuzzleRotation);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UGameplayAbility_FireWeapon::SpawnProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (!ProjectileClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwningActorFromActorInfo();
	SpawnParams.Instigator = Cast<APawn>(GetOwningActorFromActorInfo());
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AProjectileBase* Projectile = World->SpawnActor<AProjectileBase>(ProjectileClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (Projectile)
	{
		Projectile->SetDamage(ProjectileDamage);
		Projectile->InitializeVelocity(SpawnRotation.Vector());
	}
}
