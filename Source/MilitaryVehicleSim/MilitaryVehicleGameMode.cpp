// Fill out your copyright notice in the Description page of Project Settings.


#include "MilitaryVehicleGameMode.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AActor* AMilitaryVehicleGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);

	TArray<AActor*> UnoccupiedStarts;
	TArray<AActor*> OccupiedStarts;

	for (AActor* Start : PlayerStarts)
	{
		APlayerStart* PlayerStart = Cast<APlayerStart>(Start);
		if (PlayerStart)
		{
			FVector StartLocation = PlayerStart->GetActorLocation();
			FRotator StartRotation = PlayerStart->GetActorRotation();

			// Check if any pawn or vehicle is overlapping this start location
			FCollisionObjectQueryParams ObjectParams;
			ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
			ObjectParams.AddObjectTypesToQuery(ECC_Vehicle);

			// Increased radius to 500.0f as vehicles are larger than standard pawns
			if (!GetWorld()->OverlapAnyTestByObjectType(StartLocation, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(500.0f)))
			{
				UnoccupiedStarts.Add(PlayerStart);
			}
			else
			{
				OccupiedStarts.Add(PlayerStart);
			}
		}
	}

	if (UnoccupiedStarts.Num() > 0)
	{
		// Return a random unoccupied start
		return UnoccupiedStarts[FMath::RandRange(0, UnoccupiedStarts.Num() - 1)];
	}
	else if (OccupiedStarts.Num() > 0)
	{
		// If all are occupied, just return a random one (better than nothing)
		return OccupiedStarts[FMath::RandRange(0, OccupiedStarts.Num() - 1)];
	}

	// Fallback to super implementation if no PlayerStarts found
	return Super::ChoosePlayerStart_Implementation(Player);
}
