// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbility_FireWeapon.generated.h"

class AProjectileBase;
/**
 * 
 */
UCLASS()
class MILITARYVEHICLESIM_API UGameplayAbility_FireWeapon : public UGameplayAbility
{
	GENERATED_BODY()
public:
	UGameplayAbility_FireWeapon();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<AProjectileBase> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float FireCooldown;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon")
	float ProjectileDamage;

private:
	void SpawnProjectile(const FVector& SpawnLocation, const FRotator& SpawnRotation);
};
