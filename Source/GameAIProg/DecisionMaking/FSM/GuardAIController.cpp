// GuardAIController.cpp
#include "GuardAIController.h"
#include "GuardStates.h"
#include "Perception/AIPerceptionComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/FSM/FSMComponent.h"

AGuardAIController::AGuardAIController()
{
    
}

void AGuardAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // Boot the blackboard
    
}

void AGuardAIController::SetPatrolWaypoints(TArray<FVector> Waypoints)
{
    // ---- Create states ----
    UFSMComponent* FSMComp = Cast<UFSMComponent>(BrainComponent);
    if (!FSMComp)return;
    auto* Patrol = FSMComp->AddState(std::make_unique<PatrolState>(Waypoints));
    auto* Chase  = FSMComp->AddState(std::make_unique<ChaseState>());
    auto* Search = FSMComp->AddState(std::make_unique<SearchState>(SearchTimeout));

    // ---- Wire transitions ----
    // Patrol → Chase : IsTargetVisible
    FSMComp->AddTransition(Patrol, Chase,
        [this]() { return IsTargetVisible(); });

    // Chase → Search : !IsTargetVisible
    FSMComp->AddTransition(Chase, Search,
        [this]() { return !IsTargetVisible(); });

    // Search → Chase : IsTargetVisible
    FSMComp->AddTransition(Search, Chase,
        [this]() { return IsTargetVisible(); });

    // Search → Patrol : IsSearchingTooLong
    FSMComp->AddTransition(Search, Patrol,
        [this]() { return IsSearchingTooLong(); });

    // ---- Start the brain ----
    FSMComp->StartLogic();
}

void AGuardAIController::SetThiefActor(AActor* Thief)
{
    if (Blackboard)
        Blackboard->SetValueAsObject(BBKeys::Target, Thief);
}

bool AGuardAIController::IsTargetVisible() const
{
    if (!Blackboard || !GetPawn()) return false;

    AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(BBKeys::Target));
    if (!Target) return false;

    // 1. Distance check
    float DistSq = FVector::DistSquared(
        GetPawn()->GetActorLocation(),
        Target->GetActorLocation());

    if (DistSq > FMath::Square(DetectionRadius))
        return false;

    // 2. Line of sight check
    return LineOfSightTo(Target);
}

bool AGuardAIController::IsSearchingTooLong() const
{
    if (!Blackboard || !GetWorld()) return false;

    float StartTime = Blackboard->GetValueAsFloat(BBKeys::SearchStartTime);
    return (GetWorld()->GetTimeSeconds() - StartTime) >= SearchTimeout;
}