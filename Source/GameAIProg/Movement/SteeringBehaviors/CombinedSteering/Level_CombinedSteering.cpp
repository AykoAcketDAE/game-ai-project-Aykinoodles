#include "Level_CombinedSteering.h"

#include "imgui.h"


// Sets default values
ALevel_CombinedSteering::ALevel_CombinedSteering()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	
}

// Called when the game starts or when spawned
void ALevel_CombinedSteering::BeginPlay()
{
	Super::BeginPlay();
	PrimaryActorTick.bCanEverTick = true;
	m_SeekUPtr = std::make_unique<Seek>();
	m_WanderBlendUPtr = std::make_unique<Wander>();
	std::vector<BlendedSteering::WeightedBehavior> combinedBehavior{{m_SeekUPtr.get(),0.5f},{m_WanderBlendUPtr.get(),0.5f}};
	m_BlendedSteeringUPtr = std::make_unique<BlendedSteering>(combinedBehavior);
	
	m_WanderAgentPtr = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{0,0,90}, FRotator::ZeroRotator);
	if (IsValid(m_WanderAgentPtr))
	{
		m_WanderAgentPtr->SetSteeringBehavior(m_BlendedSteeringUPtr.get());
	}
	m_EvadeUPtr = std::make_unique<Evade>();
	m_WanderPriorityUPtr = std::make_unique<Wander>();
	std::vector<ISteeringBehavior*> priorityBehavior{m_EvadeUPtr.get(),m_WanderPriorityUPtr.get()};
	m_PrioritySteeringUPtr = std::make_unique<PrioritySteering>(priorityBehavior);
	
	m_EvadingAgentPtr = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{0,0,90}, FRotator::ZeroRotator);
	if (IsValid(m_EvadingAgentPtr))
	{
		m_EvadingAgentPtr->SetSteeringBehavior(m_PrioritySteeringUPtr.get());
	}
}

void ALevel_CombinedSteering::BeginDestroy()
{
	Super::BeginDestroy();

}

// Called every frame
void ALevel_CombinedSteering::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
#pragma region UI
	//UI
	{
		//Setup
		bool windowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Game AI", &windowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	
		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: place target");
		ImGui::Text("RMB: move cam.");
		ImGui::Text("Scrollwheel: zoom cam.");
		ImGui::Unindent();
	
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();
	
		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();
	
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();
	
		ImGui::Text("Flocking");
		ImGui::Spacing();
		ImGui::Spacing();
	
		if (ImGui::Checkbox("Debug Rendering", &CanDebugRender))
		{
   // TODO: Handle the debug rendering of your agents here :)
		}
		ImGui::Checkbox("Trim World", &TrimWorld->bShouldTrimWorld);
		if (TrimWorld->bShouldTrimWorld)
		{
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Trim Size",
				TrimWorld->GetTrimWorldSize(), 1000.f, 3000.f,
				[this](float InVal) { TrimWorld->SetTrimWorldSize(InVal); });
		}
		
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
	
		ImGui::Text("Behavior Weights");
		ImGui::Spacing();


		 ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",
		 	m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[0].Weight, 0.f, 1.f,
		 	[this](float InVal) { m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[0].Weight = InVal; }, "%.2f");
		
		 ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",
		 m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[1].Weight, 0.f, 1.f,
		 [this](float InVal) { m_BlendedSteeringUPtr->GetWeightedBehaviorsRef()[1].Weight = InVal; }, "%.2f");
	
		//End
		ImGui::End();
	}
#pragma endregion
	
	// Combined Steering Update
 // TODO: implement handling mouse click input for seek
	m_SeekUPtr->SetTarget(MouseTarget);
 // TODO: implement Make sure to also evade the wanderer

	FTargetData evadeTarget;
	evadeTarget.LinearVelocity = m_WanderAgentPtr->GetLinearVelocity();
	evadeTarget.AngularVelocity = m_WanderAgentPtr->GetAngularVelocity();
	evadeTarget.Position = m_WanderAgentPtr->GetPosition();

	m_EvadeUPtr->SetTarget(evadeTarget);
	m_EvadingAgentPtr->Tick(DeltaTime);
}
