// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Level_Base.h"
#include "GuardStates.h"
#include "Level_FSM.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_FSM : public ALevel_Base
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALevel_FSM();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

private:
	UPROPERTY()
	ASteeringAgent* Guard{nullptr}; // ref
	ASteeringAgent* Thief{nullptr}; // ref
	std::unique_ptr<ISteeringBehavior> ThiefBehavior{std::make_unique<Arrive>()};
	std::unique_ptr<ISteeringBehavior> GuardBehavior{std::make_unique<Seek>()};
	TArray<FVector> PatrolWaypoints{{700,-700,0},{700,950,0},{-550,950,0},{-550,-700,0}};
};
