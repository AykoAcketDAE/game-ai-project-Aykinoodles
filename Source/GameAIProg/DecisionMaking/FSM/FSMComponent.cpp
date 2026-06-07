#include "FSMComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"

UFSMComponent::UFSMComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    FSMInstance = std::make_unique<GameAI::FSM::FSM>();
}

GameAI::FSM::State* UFSMComponent::AddState(std::unique_ptr<GameAI::FSM::State> NewState)
{
    GameAI::FSM::State* RawPtr = NewState.get();
    if (FSMInstance)
        FSMInstance->AddState(std::move(NewState));
    return RawPtr; // caller holds this to wire transitions
}

void UFSMComponent::AddTransition(GameAI::FSM::State* From,
                                   GameAI::FSM::State* To,
                                   std::function<bool()> EvalFunc)
{
    if (FSMInstance)
        FSMInstance->AddTransition(From, To, std::move(EvalFunc));
}

void UFSMComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                   FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsRunning && FSMInstance)
    {
        AAIController* Controller = Cast<AAIController>(GetOwner());
        UBlackboardComponent* BB  = Controller ? Controller->GetBlackboardComponent() : nullptr;
        FSMInstance->Tick(DeltaTime, Controller, BB);
    }
}

void UFSMComponent::StartLogic()
{
    Super::StartLogic();

    if (FSMInstance)
    {
        AAIController* Controller = Cast<AAIController>(GetOwner());
        UBlackboardComponent* BB  = Controller ? Controller->GetBlackboardComponent() : nullptr;
        FSMInstance->Start(Controller, BB);
        bIsRunning = true;
    }
}

void UFSMComponent::StopLogic(const FString& Reason)
{
    if (FSMInstance)
    {
        AAIController* Controller = Cast<AAIController>(GetOwner());
        UBlackboardComponent* BB  = Controller ? Controller->GetBlackboardComponent() : nullptr;
        FSMInstance->Stop(Controller, BB);
    }
    bIsRunning = false;
}

bool UFSMComponent::IsRunning() const
{
    return bIsRunning;
}