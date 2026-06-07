#include "Flock.h"
#include "FlockingSteeringBehaviors.h"
#include "Shared/ImGuiHelpers.h"


Flock::Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize,
	float WorldSize,
	ASteeringAgent* const pAgentToEvade,
	bool bTrimWorld)
	: pWorld{pWorld}
	, FlockSize{ FlockSize }
	, pAgentToEvading{pAgentToEvade}
{
	Agents.Reserve(FlockSize);
	Neighbors.Reserve(FlockSize);
	
 // TODO: initialize the flock and the memory pool
	// Spawn agents
	pSeparationBehavior = std::make_unique<Separation>(this);
	pCohesionBehavior = std::make_unique<Cohesion>(this);
	pVelMatchBehavior = std::make_unique<VelocityMatch>(this);
	pWanderBehavior = std::make_unique<Wander>();
	pSeekBehavior = std::make_unique<Seek>();
	pEvadeBehavior = std::make_unique<Evade>();

	std::vector<BlendedSteering::WeightedBehavior> blended {
		{pSeparationBehavior.get(), 0.5f},
		{pCohesionBehavior.get(), 0.3f},
		{pVelMatchBehavior.get(), 0.2f},
		{pWanderBehavior.get(), 0.3f}};
	pBlendedSteering = std::make_unique<BlendedSteering>(blended);
	
	std::vector<ISteeringBehavior*> priority {pEvadeBehavior.get(),pBlendedSteering.get()};
	pPrioritySteering = std::make_unique<PrioritySteering>(priority);
	
	for (int i = 0; i < FlockSize; ++i)
	{
		FVector SpawnLocation(
			FMath::FRandRange(-500.f, 500),
			FMath::FRandRange(-500.f, 500),
			0.f);

		FActorSpawnParameters Params;
		ASteeringAgent* Agent = pWorld->SpawnActor<ASteeringAgent>(
			AgentClass,
			SpawnLocation,
			FRotator::ZeroRotator,
			Params);

		if (Agent)
		{
			Agent->SetSteeringBehavior(pPrioritySteering.get());
			Agents.Add(Agent);
		}
	}
	FActorSpawnParameters Params;

	pAgentToEvading = pWorld->SpawnActor<ASteeringAgent>(
			AgentClass,
				FVector (0,0,0.f),
			FRotator::ZeroRotator,
			Params);
	
	
	pAgentToEvading->SetSteeringBehavior(pWanderBehavior.get());
	
	
	
}

Flock::~Flock()
{
 // TODO: Cleanup any additional data
	for (ASteeringAgent* Agent : Agents)
	{
		if (Agent)
			Agent->Destroy();
	}
	pAgentToEvading->Destroy();
	Agents.Empty();
}

void Flock::Tick(float DeltaTime)
{
 // TODO: update the flock
 // TODO: for every agent:
  // TODO: register the neighbors for this agent (-> fill the memory pool with the neighbors for the currently evaluated agent)
  // TODO: update the agent (-> the steeringbehaviors use the neighbors in the memory pool)
  // TODO: trim the agent to the world
	FTargetData data {pAgentToEvading->GetPosition()};
	pEvadeBehavior->SetTarget(data);
	
	for (ASteeringAgent* Agent : Agents)
	{
		RegisterNeighbors(Agent);
		
	}
}

void Flock::RenderDebug()
{
 // TODO: Render all the agents in the flock
}

void Flock::ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize)
{
#ifdef PLATFORM_WINDOWS
#pragma region UI
	//UI
	{
		//Setup
		bool bWindowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &bWindowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

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

		ImGui::Text("Flocking");
		ImGui::Spacing();

  // TODO: implement ImGUI checkboxes for debug rendering here

		ImGui::Text("Behavior Weights");
		ImGui::Spacing();

  // TODO: implement ImGUI sliders for steering behavior weights here
		//End
		ImGui::End();
	}
#pragma endregion
#endif
}

void Flock::RenderNeighborhood()
{
 // TODO: Debugrender the neighbors for the first agent in the flock
}

#ifndef GAMEAI_USE_SPACE_PARTITIONING
void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
 // TODO: Implement
	Neighbors.Reset();
	NrOfNeighbors = 0;

	for (ASteeringAgent* Other : Agents)
	{
		if (Other == pAgent)
			continue;

		float DistanceSq =
			FVector2D::DistSquared(
				pAgent->GetPosition(),
				Other->GetPosition());

		if (DistanceSq < NeighborhoodRadius * NeighborhoodRadius)
		{
			Neighbors.Add(Other);
			NrOfNeighbors++;
		}
	}
}
#endif

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;

	if (NrOfNeighbors == 0)
		return FVector2D::ZeroVector;

	for (ASteeringAgent* Neighbor : Neighbors)
	{
		avgPosition += Neighbor->GetPosition();
	}

	avgPosition /= NrOfNeighbors;
	
	return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;

	if (NrOfNeighbors == 0)
		return FVector2D::ZeroVector;

	for (ASteeringAgent* Neighbor : Neighbors)
	{
		avgVelocity += Neighbor->GetLinearVelocity();
	}

	avgVelocity /= NrOfNeighbors;
	
	return avgVelocity;

}

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{
 // TODO: Implement
	pSeekBehavior->SetTarget(Target);
	
}

