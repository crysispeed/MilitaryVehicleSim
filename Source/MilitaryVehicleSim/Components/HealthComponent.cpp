// Fill out your copyright notice in the Description page of Project Settings.


#include "HealthComponent.h"
#include "Net/UnrealNetwork.h"

UHealthComponent::UHealthComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);

	MaxHealth = 300.0f;
	CurrentHealth = MaxHealth;
}

void UHealthComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UHealthComponent, CurrentHealth);
}

void UHealthComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHealth = MaxHealth;
	BroadcastHealthChanged();
}

float UHealthComponent::GetHealthPercentage() const
{
	return MaxHealth > 0.0f ? (CurrentHealth / MaxHealth) : 0.0f;
}

void UHealthComponent::ApplyDamage(float DamageAmount, AActor* DamageCauser)
{
	// Only apply damage on server
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	if (!IsAlive() || DamageAmount <= 0.0f)
	{
		return;
	}

	CurrentHealth = FMath::Max(0.0f, CurrentHealth - DamageAmount);
	BroadcastHealthChanged();

	if (CurrentHealth <= 0.0f)
	{
		OnDeath.Broadcast();
	}
}

void UHealthComponent::OnRep_CurrentHealth()
{
	BroadcastHealthChanged();

	if (CurrentHealth <= 0.0f)
	{
		OnDeath.Broadcast();
	}
}

void UHealthComponent::BroadcastHealthChanged()
{
	OnHealthChanged.Broadcast(CurrentHealth, MaxHealth);
}

