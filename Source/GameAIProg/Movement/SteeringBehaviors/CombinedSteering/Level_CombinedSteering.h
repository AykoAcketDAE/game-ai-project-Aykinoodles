// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>
#include <string>

#include "CoreMinimal.h"
#include "CombinedSteeringBehaviors.h"
#include "GameAIProg/Shared/Level_Base.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"
#include "Level_CombinedSteering.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_CombinedSteering : public ALevel_Base
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALevel_CombinedSteering();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

private:
	//Datamembers
	bool UseMouseTarget = false;
	bool CanDebugRender = false;
	
	
	std::unique_ptr<BlendedSteering> m_BlendedSteeringUPtr{}; 
	std::unique_ptr<Seek> m_SeekUPtr{}; 
	std::unique_ptr<Wander> m_WanderBlendUPtr{};
	ASteeringAgent* m_WanderAgentPtr{nullptr};

	std::unique_ptr<PrioritySteering> m_PrioritySteeringUPtr{}; 
	std::unique_ptr<Evade> m_EvadeUPtr{}; 
	std::unique_ptr<Wander> m_WanderPriorityUPtr{};
	ASteeringAgent* m_EvadingAgentPtr{nullptr};
	
	
};
