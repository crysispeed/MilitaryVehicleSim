// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "HealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHealthChanged, float, CurrentHealth, float, MaxHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);

/**
 * Component responsible for managing actor health
 * Follows Single Responsibility: Only handles health and damage
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MILITARYVEHICLESIM_API UHealthComponent : public UActorComponent
{
	GENERATED_BODY()
	
public:
	UHealthComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintCallable, Category = "Health")
	float GetHealthPercentage() const;

	UFUNCTION(BlueprintCallable, Category = "Health")
	bool IsAlive() const { return CurrentHealth > 0.0f; }

	// Apply damage to this component (should be called on server)
	UFUNCTION(BlueprintCallable, Category = "Health")
	void ApplyDamage(float DamageAmount, AActor* DamageCauser);

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnHealthChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Health")
	FOnDeath OnDeath;

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Health", meta = (ClampMin = "0.0"))
	float MaxHealth;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, BlueprintReadOnly, Category = "Health")
	float CurrentHealth;

	UFUNCTION()
	void OnRep_CurrentHealth();

private:
	void BroadcastHealthChanged();
};
