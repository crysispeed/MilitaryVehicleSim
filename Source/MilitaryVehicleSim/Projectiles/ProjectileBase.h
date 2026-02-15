// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ProjectileBase.generated.h"

class UProjectileMovementComponent;
class USphereComponent;

UCLASS()
class MILITARYVEHICLESIM_API AProjectileBase : public AActor
{
	GENERATED_BODY()
	
public:
	AProjectileBase();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void InitializeVelocity(const FVector& Direction);

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	void SetDamage(float NewDamage) { Damage = NewDamage; }

	UFUNCTION(BlueprintCallable, Category = "Projectile")
	float GetDamage() const { return Damage; }

	virtual void PostNetInit() override;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		FVector NormalImpulse, const FHitResult& Hit);

	// Components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;

	// Projectile properties
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float InitialSpeed;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float GravityScale;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Projectile")
	float Damage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	float LifeSpan;

private:
	void ApplyDamageToActor(AActor* DamagedActor);
	void DestroyProjectile();
};
