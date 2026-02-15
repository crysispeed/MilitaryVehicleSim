// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MilitaryVehicleGameMode.generated.h"

/**
 * 
 */
UCLASS()
class MILITARYVEHICLESIM_API AMilitaryVehicleGameMode : public AGameModeBase
{
	GENERATED_BODY()
	
public:
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};
