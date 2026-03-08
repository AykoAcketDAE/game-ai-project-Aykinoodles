
#include "CombinedSteeringBehaviors.h"
#include <algorithm>
#include "../SteeringAgent.h"

BlendedSteering::BlendedSteering(const std::vector<WeightedBehavior>& WeightedBehaviors)
	:WeightedBehaviors(WeightedBehaviors)
{};

//****************
//BLENDED STEERING
SteeringOutput BlendedSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput BlendedSteering = {};
	// TODO: Calculate the weighted average steeringbehavior
	FVector2D WeightedLinearVelocity{};
	float WeightedAngularVelocity{};
	float TotalWeight = 0.f;

	for (const auto& behavior : WeightedBehaviors)
	{
		auto output = behavior.pBehavior->CalculateSteering(DeltaT, Agent);

		WeightedLinearVelocity += output.LinearVelocity * behavior.Weight;
		WeightedAngularVelocity += output.AngularVelocity * behavior.Weight;
		
		TotalWeight += behavior.Weight;
	}

	if (TotalWeight > 0.f)
	{
		WeightedLinearVelocity /= TotalWeight;
		WeightedAngularVelocity /= TotalWeight;
	}

	BlendedSteering.LinearVelocity = WeightedLinearVelocity;
	BlendedSteering.AngularVelocity = WeightedAngularVelocity;
	
	// TODO: Add debug drawing
	return BlendedSteering;
}

float* BlendedSteering::GetWeight(ISteeringBehavior* const SteeringBehavior)
{
	auto it = find_if(WeightedBehaviors.begin(),
		WeightedBehaviors.end(),
		[SteeringBehavior](const WeightedBehavior& Elem)
		{
			return Elem.pBehavior == SteeringBehavior;
		}
	);

	if(it!= WeightedBehaviors.end())
		return &it->Weight;
	
	return nullptr;
}

//*****************
//PRIORITY STEERING
SteeringOutput PrioritySteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering = {};

	for (ISteeringBehavior* const pBehavior : m_PriorityBehaviors)
	{
		Steering = pBehavior->CalculateSteering(DeltaT, Agent);

		if (Steering.IsValid)
			break;
	}

	//If non of the behavior return a valid output, last behavior is returned
	return Steering;
}