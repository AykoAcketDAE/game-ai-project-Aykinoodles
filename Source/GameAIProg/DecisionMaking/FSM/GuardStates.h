#pragma once
#include "FSM.h"
#include "AIController.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

// Blackboard key names — define these once, use everywhere
namespace BBKeys
{
    static const FName Target          = TEXT("Target");
    static const FName LastKnownPos    = TEXT("LastKnownPosition");
    static const FName SearchStartTime = TEXT("SearchStartTime");
    static const FName PatrolIndex     = TEXT("PatrolIndex");
}

// ----------------------------------------------------------------
// Patrol — walks a looping waypoint list stored on the controller
// ----------------------------------------------------------------
class PatrolState : public GameAI::FSM::State
{
public:
    // Patrol points are set up by the AIController/level, passed in here
    explicit PatrolState(TArray<FVector> InWaypoints)
        : GameAI::FSM::State("Patrol"), Waypoints(std::move(InWaypoints)) {}

    void OnEnter(AAIController* Controller, UBlackboardComponent* BB) override
    {
        MoveToCurrentWaypoint(Controller, BB);
    }

    void OnTick(float Dt, AAIController* Controller, UBlackboardComponent* BB) override
    {
        // Advance waypoint when we've arrived
        if (Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
        {
            int32 CurrentIndex = BB->GetValueAsInt(BBKeys::PatrolIndex);
            CurrentIndex = (CurrentIndex + 1) % Waypoints.Num();
            BB->SetValueAsInt(BBKeys::PatrolIndex, CurrentIndex);
            MoveToCurrentWaypoint(Controller, BB);
        }
    }

    void OnExit(AAIController* Controller, UBlackboardComponent* BB) override
    {
        Controller->StopMovement();
    }

private:
    TArray<FVector> Waypoints;

    void MoveToCurrentWaypoint(AAIController* Controller, UBlackboardComponent* BB)
    {
        if (Waypoints.IsEmpty()) return;
        int32 Index = BB->GetValueAsInt(BBKeys::PatrolIndex);
        Controller->MoveToLocation(Waypoints[Index]);
    }
};

// ----------------------------------------------------------------
// Chase — moves toward the target actor each tick,
//         keeps LastKnownPosition updated while visible
// ----------------------------------------------------------------
class ChaseState : public GameAI::FSM::State
{
public:
    ChaseState() : GameAI::FSM::State("Chase") {}

    void OnEnter(AAIController* Controller, UBlackboardComponent* BB) override
    {
        // Nothing special — OnTick handles the movement
    }

    void OnTick(float Dt, AAIController* Controller, UBlackboardComponent* BB) override
    {
        AActor* Target = Cast<AActor>(BB->GetValueAsObject(BBKeys::Target));
        if (!Target) return;

        // Keep last known position fresh while we have sight
        BB->SetValueAsVector(BBKeys::LastKnownPos, Target->GetActorLocation());

        // Re-issue move every tick so we always follow (could throttle this)
        Controller->MoveToActor(Target, 50.f /* acceptance radius */);
    }

    void OnExit(AAIController* Controller, UBlackboardComponent* BB) override
    {
        Controller->StopMovement();
    }
};

// ----------------------------------------------------------------
// Search — goes to LastKnownPosition, then wanders for a time limit
// ----------------------------------------------------------------
class SearchState : public GameAI::FSM::State
{
public:
    explicit SearchState(float InSearchDuration = 10.f)
        : GameAI::FSM::State("Search"), SearchDuration(InSearchDuration) {}

    void OnEnter(AAIController* Controller, UBlackboardComponent* BB) override
    {
        // Record when we started searching (used by IsSearchingTooLong)
        BB->SetValueAsFloat(BBKeys::SearchStartTime,
                            Controller->GetWorld()->GetTimeSeconds());

        // Head to the last known position first
        FVector LastPos = BB->GetValueAsVector(BBKeys::LastKnownPos);
        Controller->MoveToLocation(LastPos);
        bHeadingToLastKnown = true;
    }

    void OnTick(float Dt, AAIController* Controller, UBlackboardComponent* BB) override
    {
        if (bHeadingToLastKnown &&
            Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
        {
            // Arrived — start wandering
            bHeadingToLastKnown = false;
            IssueWanderMove(Controller);
        }
        else if (!bHeadingToLastKnown &&
                 Controller->GetMoveStatus() == EPathFollowingStatus::Idle)
        {
            // Keep wandering
            IssueWanderMove(Controller);
        }
    }

    void OnExit(AAIController* Controller, UBlackboardComponent* BB) override
    {
        Controller->StopMovement();
        bHeadingToLastKnown = false;
    }

private:
    float SearchDuration;
    bool  bHeadingToLastKnown{false};

    void IssueWanderMove(AAIController* Controller)
    {
        APawn* Pawn = Controller->GetPawn();
        if (!Pawn) return;

        FVector Origin  = Pawn->GetActorLocation();
        float   Radius  = 600.f;
        FVector WanderTarget = Origin + FVector(
            FMath::RandRange(-Radius, Radius),
            FMath::RandRange(-Radius, Radius),
            0.f);

        Controller->MoveToLocation(WanderTarget);
    }
};
