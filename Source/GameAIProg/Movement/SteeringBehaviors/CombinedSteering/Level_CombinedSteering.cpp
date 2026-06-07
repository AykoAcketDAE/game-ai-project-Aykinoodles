#include "Level_CombinedSteering.h"

#include "imgui.h"


ALevel_CombinedSteering::ALevel_CombinedSteering()
{
   
}


void ALevel_CombinedSteering::BeginPlay()
{
    Super::BeginPlay();
    PrimaryActorTick.bCanEverTick = true;

    
    m_SeekUPtr        = std::make_unique<Seek>();
    m_WanderBlendUPtr = std::make_unique<Wander>();

    std::vector<BlendedSteering::WeightedBehavior> combinedBehavior{
        { m_SeekUPtr.get(), 0.5f },
        { m_WanderBlendUPtr.get(), 0.5f }
    };
    m_BlendedSteeringUPtr = std::make_unique<BlendedSteering>(combinedBehavior);

    m_WanderAgentPtr = GetWorld()->SpawnActor<ASteeringAgent>(
        SteeringAgentClass, FVector{ 0, 0, 90 }, FRotator::ZeroRotator);
    if (IsValid(m_WanderAgentPtr))
        m_WanderAgentPtr->SetSteeringBehavior(m_BlendedSteeringUPtr.get());
    
    m_EvadeUPtr          = std::make_unique<Evade>();
    m_WanderPriorityUPtr = std::make_unique<Wander>();

    std::vector<ISteeringBehavior*> priorityBehavior{
        m_EvadeUPtr.get(),
        m_WanderPriorityUPtr.get()
    };
    m_PrioritySteeringUPtr = std::make_unique<PrioritySteering>(priorityBehavior);

    m_EvadingAgentPtr = GetWorld()->SpawnActor<ASteeringAgent>(
        SteeringAgentClass, FVector{ 0, 0, 90 }, FRotator::ZeroRotator);
    if (IsValid(m_EvadingAgentPtr))
        m_EvadingAgentPtr->SetSteeringBehavior(m_PrioritySteeringUPtr.get());
    
    m_WolfpackUPtr = std::make_unique<WolfpackSteering>();

    for (int32 i = 0; i < WolfCount; ++i)
    {
        const float   Angle    = FMath::DegreesToRadians(360.f * i / WolfCount);
        const float   SpawnR   = 600.f;
        const FVector SpawnPos = FVector(
            FMath::Cos(Angle) * SpawnR,
            FMath::Sin(Angle) * SpawnR,
            90.f);

        ASteeringAgent* Wolf = GetWorld()->SpawnActor<ASteeringAgent>(
            SteeringAgentClass, SpawnPos, FRotator::ZeroRotator);

        if (IsValid(Wolf))
        {
            int32 Slot = m_WolfpackUPtr->RegisterMember(Wolf);
            Wolf->SetSteeringBehavior(m_WolfpackUPtr.get());
            m_WolfAgents.Add(Wolf);
        }
    }

    m_BoardingUPtr = std::make_unique<MetroBoardingSteering>();
    
    m_BoardingUPtr->DoorPosition  = FVector2D(800.f, 0.f);
    m_BoardingUPtr->DoorDirection = FVector2D(0.f, 1.f);
    m_BoardingUPtr->DoorWidth     = 120.f;
    
    const float SeatRowSpacing = 100.f;
    const float SeatColOffset  = 60.f;   
    const float SeatDepthStart = 180.f;  
 
    TArray<FVector2D> SeatPositions;
    for (int32 Row = 0; Row < 4; ++Row)
    {
        float Depth = SeatDepthStart + Row * SeatRowSpacing;
        SeatPositions.Add(FVector2D(800.f - SeatColOffset, Depth));
        SeatPositions.Add(FVector2D(800.f + SeatColOffset, Depth));
    }
 
    m_PassengerTickets.Reserve(PassengerCount);
    m_PassengerAgents.Reserve(PassengerCount);
    
    for (int32 i = 0; i < PassengerCount; ++i)
    {
        
        const float PlatformSpread = 300.f;
        const float PlatformDepth  = 400.f; 
 
        FVector SpawnPos = FVector(
            800.f + FMath::FRandRange(-PlatformSpread, PlatformSpread),
            -PlatformDepth + FMath::FRandRange(-80.f, 80.f),
            90.f);
 
        ASteeringAgent* Passenger = GetWorld()->SpawnActor<ASteeringAgent>(
            SteeringAgentClass, SpawnPos, FRotator::ZeroRotator);
 
        if (!IsValid(Passenger)) continue;
        FVector2D SeatPos = SeatPositions[i % SeatPositions.Num()];
        
        m_PassengerTickets.Add(m_BoardingUPtr->RegisterPassenger(Passenger, SeatPos));
        m_PassengerAgents.Add(Passenger);
    }
    m_DoorState = EDoorState::Closed;
    m_DoorTimer = 0.f;

}


void ALevel_CombinedSteering::BeginDestroy()
{
    for (ASteeringAgent* Wolf : m_WolfAgents)
        if (IsValid(Wolf))
            m_WolfpackUPtr->UnregisterMember(Wolf);

    for (ASteeringAgent* Passenger : m_PassengerAgents)
        if (IsValid(Passenger))
            m_BoardingUPtr->UnregisterPassenger(Passenger);

    Super::BeginDestroy();
}


void ALevel_CombinedSteering::TickDoorStateMachine(float DeltaTime)
{
    if (m_PendingDoorState >= 0)
    {
        m_DoorState        = static_cast<EDoorState>(m_PendingDoorState);
        m_DoorTimer        = 0.f;
        m_PendingDoorState = -1;
    }
    else
    {
        m_DoorTimer += DeltaTime;
    }
 
    switch (m_DoorState)
    {
    case EDoorState::Closed:
        m_BoardingUPtr->SetExitFlow(FVector2D::ZeroVector, false);
 
        if (m_DoorTimer >= m_ClosedDuration)
        {
            m_DoorState = EDoorState::ExitFlow;
            m_DoorTimer = 0.f;
        }
        break;
 
    case EDoorState::ExitFlow:
        m_BoardingUPtr->SetExitFlow(FVector2D(0.f, -1.f), true);
 
        if (m_DoorTimer >= m_ExitFlowDuration)
        {
            m_DoorState = EDoorState::Boarding;
            m_DoorTimer = 0.f;
        }
        break;
 
    case EDoorState::Boarding:
        m_BoardingUPtr->SetExitFlow(FVector2D::ZeroVector, false);
 
        if (m_DoorTimer >= m_BoardingDuration)
        {
            m_DoorState = EDoorState::Departed;
            m_DoorTimer = 0.f;
        }
        break;
 
    case EDoorState::Departed:
        m_BoardingUPtr->SetExitFlow(FVector2D::ZeroVector, false);
        break;
    }

}



int32 ALevel_CombinedSteering::CountPassengersInPhase(
    MetroBoardingSteering::EBoardingPhase Phase) const
{
    int32 Count = 0;
    for (const auto& T : m_PassengerTickets)
        if (T.Phase == Phase) ++Count;
    return Count;
}



void ALevel_CombinedSteering::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

#pragma region UI
    {
        bool windowActive = true;
        ImGui::SetNextWindowPos(WindowPos);
        ImGui::SetNextWindowSize(WindowSize);
        ImGui::Begin("Game AI", &windowActive,
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

        // ── Controls ────────────────────────────────────────────────────────
        ImGui::Text("CONTROLS");
        ImGui::Indent();
        ImGui::Text("LMB: place target");
        ImGui::Text("RMB: move cam.");
        ImGui::Text("Scrollwheel: zoom cam.");
        ImGui::Unindent();

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

        // ── Stats ────────────────────────────────────────────────────────────
        ImGui::Text("STATS");
        ImGui::Indent();
        ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
        ImGui::Unindent();

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

        // ── Blended weights ──────────────────────────────────────────────────
        ImGui::Text("Blended Behavior Weights");
        ImGui::Spacing();

        ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",
            m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[0].Weight, 0.f, 1.f,
            [this](float InVal) { m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[0].Weight = InVal; }, "%.2f");

        ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",
            m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[1].Weight, 0.f, 1.f,
            [this](float InVal) { m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[1].Weight = InVal; }, "%.2f");

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

        // ── Wolfpack ─────────────────────────────────────────────────────────
        ImGui::Text("Wolfpack");
        ImGui::Spacing();

        ImGui::Text("Pack size: %d", m_WolfpackUPtr->GetPackSize());
        ImGui::Spacing();

        ImGuiHelpers::ImGuiSliderFloatWithSetter("Surround Radius",
            m_WolfpackUPtr->SurroundRadius, 50.f, 600.f,
            [this](float InVal) { m_WolfpackUPtr->SurroundRadius = InVal; }, "%.0f");

        ImGuiHelpers::ImGuiSliderFloatWithSetter("Separation Weight",
            m_WolfpackUPtr->SeparationWeight, 0.f, 2.f,
            [this](float InVal) { m_WolfpackUPtr->SeparationWeight = InVal; }, "%.2f");

        ImGuiHelpers::ImGuiSliderFloatWithSetter("Separation Radius",
            m_WolfpackUPtr->SeparationRadius, 50.f, 400.f,
            [this](float InVal) { m_WolfpackUPtr->SeparationRadius = InVal; }, "%.0f");

        ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

        // ── Metro Boarding ───────────────────────────────────────────────────
                ImGui::Text("Metro Boarding");
        ImGui::Spacing();
 
        // Door state label
        const char* DoorStateNames[] = { "Closed", "Exit Flow", "Boarding", "Departed" };
        ImGui::Text("Door state:  %s  (%.1fs)", 
            DoorStateNames[static_cast<uint8>(m_DoorState)],
            m_DoorTimer);
 
        // Per-phase passenger counts
        ImGui::Text("Queueing:  %d", CountPassengersInPhase(MetroBoardingSteering::EBoardingPhase::Queueing));
        ImGui::Text("Yielding:  %d", CountPassengersInPhase(MetroBoardingSteering::EBoardingPhase::Yielding));
        ImGui::Text("Funneling: %d", CountPassengersInPhase(MetroBoardingSteering::EBoardingPhase::Funneling));
        ImGui::Text("Seated:    %d", CountPassengersInPhase(MetroBoardingSteering::EBoardingPhase::Seated));
        ImGui::Spacing();
 
        // Manual door state buttons — active state highlighted in green,
        // clicking any button jumps to that state and resets its timer.
        ImGui::Text("Manual Control:");
        ImGui::Spacing();
 
        for (int32 i = 0; i < 4; ++i)
        {
            bool bIsCurrent = (static_cast<int32>(m_DoorState) == i);
            if (bIsCurrent)
            {
                // Highlight the active state instead of disabling it so you
                // can still click it to reset the timer for that state
                ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.2f, 0.6f, 0.2f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.f));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.1f, 0.5f, 0.1f, 1.f));
            }
 
            if (ImGui::Button(DoorStateNames[i], ImVec2(90.f, 0.f)))
                m_PendingDoorState = i;
 
            if (bIsCurrent)
                ImGui::PopStyleColor(3);
            
        }
 
        ImGui::Spacing();
 
        // Timing sliders
        ImGuiHelpers::ImGuiSliderFloatWithSetter("Closed Duration",
            m_ClosedDuration, 1.f, 15.f,
            [this](float InVal) { m_ClosedDuration = InVal; }, "%.1fs");
 
        ImGuiHelpers::ImGuiSliderFloatWithSetter("Exit Flow Duration",
            m_ExitFlowDuration, 1.f, 10.f,
            [this](float InVal) { m_ExitFlowDuration = InVal; }, "%.1fs");
 
        ImGuiHelpers::ImGuiSliderFloatWithSetter("Boarding Duration",
            m_BoardingDuration, 3.f, 20.f,
            [this](float InVal) { m_BoardingDuration = InVal; }, "%.1fs");
 
        ImGui::Spacing();
 
        // Behaviour tuning
        ImGuiHelpers::ImGuiSliderFloatWithSetter("Queue Spacing",
            m_BoardingUPtr->QueueSpacing, 40.f, 200.f,
            [this](float InVal) { m_BoardingUPtr->QueueSpacing = InVal; }, "%.0f");
 
        ImGuiHelpers::ImGuiSliderFloatWithSetter("Surround Dist##boarding",
            m_BoardingUPtr->SeparationRadius, 30.f, 150.f,
            [this](float InVal) { m_BoardingUPtr->SeparationRadius = InVal; }, "%.0f");
 
        ImGuiHelpers::ImGuiSliderFloatWithSetter("Separation Strength##boarding",
            m_BoardingUPtr->SeparationStrength, 0.f, 3.f,
            [this](float InVal) { m_BoardingUPtr->SeparationStrength = InVal; }, "%.2f");
 
        ImGuiHelpers::ImGuiSliderFloatWithSetter("Yield Sidestep",
            m_BoardingUPtr->YieldSidestepDist, 20.f, 200.f,
            [this](float InVal) { m_BoardingUPtr->YieldSidestepDist = InVal; }, "%.0f");
 
        ImGui::Spacing();
        if (ImGui::Checkbox("Debug Rendering", &CanDebugRender))
        {
            // TODO: toggle per-agent debug rendering
        }
        ImGui::Checkbox("Trim World", &TrimWorld->bShouldTrimWorld);
        if (TrimWorld->bShouldTrimWorld)
        {
            ImGuiHelpers::ImGuiSliderFloatWithSetter("Trim Size",
                TrimWorld->GetTrimWorldSize(), 1000.f, 3000.f,
                [this](float InVal) { TrimWorld->SetTrimWorldSize(InVal); });
        }
 
        ImGui::End();

    }
#pragma endregion
    
    m_SeekUPtr->SetTarget(MouseTarget);

    FTargetData evadeTarget;
    evadeTarget.LinearVelocity  = m_WanderAgentPtr->GetLinearVelocity();
    evadeTarget.AngularVelocity = m_WanderAgentPtr->GetAngularVelocity();
    evadeTarget.Position        = m_WanderAgentPtr->GetPosition();
    m_EvadeUPtr->SetTarget(evadeTarget);

    m_EvadingAgentPtr->Tick(DeltaTime);

    FTargetData preyTarget;
    preyTarget.Position        = m_WanderAgentPtr->GetPosition();
    preyTarget.LinearVelocity  = m_WanderAgentPtr->GetLinearVelocity();
    preyTarget.AngularVelocity = m_WanderAgentPtr->GetAngularVelocity();
    m_WolfpackUPtr->SetTarget(preyTarget);

    for (ASteeringAgent* Wolf : m_WolfAgents)
    {
        if (IsValid(Wolf))
            Wolf->Tick(DeltaTime);
    }
    
    TickDoorStateMachine(DeltaTime);
    
    for (int32 i = 0; i < m_PassengerAgents.Num(); ++i)
    {
        ASteeringAgent* Passenger = m_PassengerAgents[i];
        if (!IsValid(Passenger)) continue;
 
        SteeringOutput Out = m_BoardingUPtr->CalculateSteering(
            DeltaTime, *Passenger, m_PassengerTickets[i]);
 
        if (!Out.IsValid) continue;
 
        Passenger->AddActorLocalRotation(
            FRotator(0.f, Out.AngularVelocity * DeltaTime, 0.f));
 
        const float MaxSpeed = Passenger->GetMaxLinearSpeed();
        if (MaxSpeed > KINDA_SMALL_NUMBER)
        {
            Passenger->AddMovementInput(
                Passenger->GetActorForwardVector(),
                Out.LinearVelocity.Length() / MaxSpeed);
        }
    }


}