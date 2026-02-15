#include "ProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "MilitaryVehicleSim/Components/HealthComponent.h"
#include "Net/UnrealNetwork.h"

AProjectileBase::AProjectileBase()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	AActor::SetReplicateMovement(true);

	// Create collision component
	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.0f);
	CollisionComponent->SetCollisionProfileName(TEXT("Projectile"));
	CollisionComponent->OnComponentHit.AddDynamic(this, &AProjectileBase::OnProjectileHit);
	RootComponent = CollisionComponent;

	// Create mesh component
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create projectile movement component for ballistic simulation
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 3000.0f;
	ProjectileMovement->MaxSpeed = 3000.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 1.0f; // Realistic gravity for ballistic trajectory
	ProjectileMovement->bInitialVelocityInLocalSpace = false;

	// Default values
	InitialSpeed = 3000.0f;
	GravityScale = 1.0f;
	Damage = 30.0f;
	LifeSpan = 10.0f;
}

void AProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AProjectileBase, Damage);
}

void AProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// Set up collision ignore for owner immediately on server
	if (HasAuthority())
	{
		if (AActor* OwnerActor = GetOwner())
		{
			if (UPrimitiveComponent* ProjectileRoot = Cast<UPrimitiveComponent>(GetRootComponent()))
			{
				TArray<UPrimitiveComponent*> OwnerComponents;
				OwnerActor->GetComponents<UPrimitiveComponent*>(OwnerComponents);
				for (UPrimitiveComponent* Comp : OwnerComponents)
				{
					ProjectileRoot->IgnoreComponentWhenMoving(Comp, true);
				}
			}
		}
	}

	// Set lifespan for cleanup
	SetLifeSpan(LifeSpan);

	// Apply gravity scale
	if (ProjectileMovement)
	{
		ProjectileMovement->ProjectileGravityScale = GravityScale;
	}
}

void AProjectileBase::PostNetInit()
{
	Super::PostNetInit();

	// On client, PostNetInit is called after properties like Owner have been replicated
	if (!HasAuthority())
	{
		if (AActor* OwnerActor = GetOwner())
		{
			if (UPrimitiveComponent* ProjectileRoot = Cast<UPrimitiveComponent>(GetRootComponent()))
			{
				TArray<UPrimitiveComponent*> OwnerComponents;
				OwnerActor->GetComponents<UPrimitiveComponent*>(OwnerComponents);
				for (UPrimitiveComponent* Comp : OwnerComponents)
				{
					ProjectileRoot->IgnoreComponentWhenMoving(Comp, true);
				}
			}
		}
	}
}

void AProjectileBase::InitializeVelocity(const FVector& Direction)
{
	if (ProjectileMovement)
	{
		ProjectileMovement->Velocity = Direction * InitialSpeed;
	}
}

void AProjectileBase::OnProjectileHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	// Only process on server
	if (!HasAuthority())
	{
		return;
	}

	// Don't hit owner
	if (OtherActor == GetOwner() || OtherActor == this)
	{
		return;
	}

	// Apply damage to hit actor
	if (OtherActor)
	{
		ApplyDamageToActor(OtherActor);
	}

	// Destroy projectile
	DestroyProjectile();
}

void AProjectileBase::ApplyDamageToActor(AActor* DamagedActor)
{
	if (!DamagedActor || !HasAuthority())
	{
		return;
	}

	// Try to find health component
	UHealthComponent* HealthComp = Cast<UHealthComponent>(DamagedActor->GetComponentByClass(UHealthComponent::StaticClass()));
	if (HealthComp)
	{
		HealthComp->ApplyDamage(Damage, GetOwner());
	}
}

void AProjectileBase::DestroyProjectile()
{
	// TODO: Add explosion effect or particle system here if needed
	Destroy();
}
