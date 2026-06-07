// Fill out your copyright notice in the Description page of Project Settings.


#include "Level_FSM.h"
#include "GuardAIController.h"
#include "FSMComponent.h"
#include "DecisionMaking/GameAIController.h"


// Sets default values
ALevel_FSM::ALevel_FSM()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_FSM::BeginPlay()
{
	Super::BeginPlay();

	// Spawn Thief first so we can pass it to the guard's blackboard
	Thief = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass,
		FVector{0, 0, 0}, FRotator::ZeroRotator);
	Thief->SetDebugRenderingEnabled(false);
	Thief->SetSteeringBehavior(ThiefBehavior.get());

	// Spawn Guard
	Guard = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass,
		FVector{PatrolWaypoints[0]}, FRotator::ZeroRotator);
	Guard->SetDebugRenderingEnabled(false);
	Guard->SetSteeringBehavior(GuardBehavior.get());
	if (AGuardAIController* AIController = Cast<AGuardAIController>(Guard->GetController()))
	{
		if (UFSMComponent* FSM = Cast<UFSMComponent>(AIController->GetBrainComponent()))
		{
			// Add all three states — first added = initial state (Patrol)
			auto* Patrol = FSM->AddState(std::make_unique<PatrolState>(PatrolWaypoints));
			auto* Chase  = FSM->AddState(std::make_unique<ChaseState>());
			auto* Search = FSM->AddState(std::make_unique<SearchState>(10.f));

			// Wire transitions — lambdas capture the controller
			FSM->AddTransition(Patrol, Chase, [AIController]()
			{
				return AIController->IsTargetVisible();
			});

			FSM->AddTransition(Chase, Search, [AIController]()
			{
				return !AIController->IsTargetVisible();
			});

			FSM->AddTransition(Search, Chase, [AIController]()
			{
				return AIController->IsTargetVisible();
			});

			FSM->AddTransition(Search, Patrol, [AIController]()
			{
				return AIController->IsSearchingTooLong();
			});

			// Give the guard a reference to the thief via the blackboard
			AIController->SetThiefActor(Thief);

			AIController->RunFiniteStateMachine();
		}
	}
}

// Called every frame
void ALevel_FSM::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ThiefBehavior->SetTarget(MouseTarget);
	
	FTargetData Target;
	Target.Position = Thief->GetPosition();
	Target.Orientation = Thief->GetRotation();
	Target.LinearVelocity = Thief->GetLinearVelocity();
	Target.AngularVelocity = Thief->GetAngularVelocity();
	GuardBehavior->SetTarget(Target);
}

