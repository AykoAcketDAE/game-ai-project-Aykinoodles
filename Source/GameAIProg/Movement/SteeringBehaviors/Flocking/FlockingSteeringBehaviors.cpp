#include "FlockingSteeringBehaviors.h"
#include "Flock.h"
#include "../SteeringAgent.h"
#include "../SteeringHelpers.h"


//*******************
//COHESION (FLOCKING)
SteeringOutput Cohesion::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput steering{};

	// If no neighbors -> no cohesion
	if (pFlock->GetNrOfNeighbors() == 0)
		return steering;

	// Target = average neighbor position
	FVector2D avgPos = pFlock->GetAverageNeighborPos();

	FVector AgentPos = pAgent.GetActorLocation();
	FVector TargetPos(avgPos.X, avgPos.Y, AgentPos.Z);

	FVector ToTarget = TargetPos - AgentPos;
	ToTarget.Z = 0.f;

	// Direction to center of neighbors
	FVector DesiredDir = ToTarget.GetSafeNormal();

	FVector Forward = pAgent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Signed angle between forward and desired direction
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);

	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);

	float MaxAngularSpeed = pAgent.GetMaxAngularSpeed();
	float SlowDownAngle = 30.f;

	float AngularVelocity =
		FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f)
		* MaxAngularSpeed;

	steering.AngularVelocity = AngularVelocity;

	// Move toward center of flock
	steering.LinearVelocity = FVector2D(avgPos.X, avgPos.Y) - pAgent.GetPosition();

	DrawDebugLines(pAgent, steering);
	return steering;
}
//*********************
//SEPARATION (FLOCKING)
SteeringOutput Separation::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput steering{};

	if (pFlock->GetNrOfNeighbors() == 0)
		return steering;

	FVector AgentPos = pAgent.GetActorLocation();

	FVector SeparationForce = FVector::ZeroVector;

	for (ASteeringAgent* Neighbor : pFlock->GetNeighbors())
	{
		FVector NeighborPos = Neighbor->GetActorLocation();

		FVector Away = AgentPos - NeighborPos;
		Away.Z = 0.f;

		float Distance = Away.Size();

		if (Distance > 0.f)
		{
			Away.Normalize();

			// Stronger repulsion when closer
			SeparationForce += Away / Distance;
		}
	}

	if (SeparationForce.IsNearlyZero())
		return steering;

	SeparationForce.Normalize();

	FVector Forward = pAgent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Signed angle toward separation direction
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, SeparationForce).Z,
		FVector::DotProduct(Forward, SeparationForce)
	);

	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);

	float MaxAngularSpeed = pAgent.GetMaxAngularSpeed();
	float SlowDownAngle = 30.f;

	float AngularVelocity =
		FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f)
		* MaxAngularSpeed;

	steering.AngularVelocity = AngularVelocity;

	// Move away from neighbors
	steering.LinearVelocity = FVector2D{SeparationForce} * pAgent.GetMaxLinearSpeed();

	DrawDebugLines(pAgent, steering);

	return steering;
}
//*************************
//VELOCITY MATCH (FLOCKING)
SteeringOutput VelocityMatch::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput steering{};

	// If no neighbors -> no alignment
	if (pFlock->GetNrOfNeighbors() == 0)
		return steering;

	FVector2D AvgVelocity2D = pFlock->GetAverageNeighborVelocity();

	if (AvgVelocity2D.IsNearlyZero())
		return steering;

	// Convert to FVector
	FVector DesiredDir(AvgVelocity2D.X, AvgVelocity2D.Y, 0.f);
	DesiredDir.Normalize();

	FVector Forward = pAgent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Signed angle toward average velocity direction
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);

	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);

	float MaxAngularSpeed = pAgent.GetMaxAngularSpeed();
	float SlowDownAngle = 30.f;

	float AngularVelocity =
		FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f)
		* MaxAngularSpeed;

	steering.AngularVelocity = AngularVelocity;

	// Move in the same direction as the flock
	steering.LinearVelocity = FVector2D{DesiredDir} * pAgent.GetMaxLinearSpeed();

	DrawDebugLines(pAgent, steering);

	return steering;
}




