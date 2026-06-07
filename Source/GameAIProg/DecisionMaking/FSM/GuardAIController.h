// GuardAIController.h
#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "FSMComponent.h"
#include "DecisionMaking/GameAIController.h"
#include "GuardAIController.generated.h"

UCLASS()
class GAMEAIPROG_API AGuardAIController : public AGameAIController
{
	GENERATED_BODY()

public:
	AGuardAIController();

	virtual void OnPossess(APawn* InPawn) override;

	// Call these from the guard Actor or level setup
	void SetPatrolWaypoints(TArray<FVector> Waypoints);
	void SetThiefActor(AActor* Thief);

	// Transition condition helpers — lambdas capture [this]
	bool IsTargetVisible() const;
	bool IsSearchingTooLong() const;

	UPROPERTY(EditDefaultsOnly)
	float DetectionRadius = 1200.f;

	UPROPERTY(EditDefaultsOnly)
	float SearchTimeout   = 10.f;

private:
};