#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"
#include "DynamicMesh/DynamicMesh3.h"
#include "Spatial/GeometrySet3.h"

//SEEK
//*******
// TODO: Do the Week01 assignment :^)

void ISteeringBehavior::DrawDebugLines(ASteeringAgent & Agent, const SteeringOutput& steering) const
{
	// Add debug rendering for grades.
	const auto height {Agent.GetActorTransform().GetTranslation().Z}; 
	const FVector startPos {Agent.GetPosition(),height};
	const double scale {steering.LinearVelocity.Length()/Agent.GetMaxLinearSpeed()};
	//Target position
	DrawDebugSphere(Agent.GetWorld(),FVector{Target.Position,height},20,10,FColorList::Red);
	
	//Direction Vector
	const FVector endPosDirection {Agent.GetPosition() + steering.LinearVelocity.GetSafeNormal() * 200,height};
	DrawDebugLine(Agent.GetWorld(),startPos,endPosDirection,FColorList::Green);

	//Forward Direction
	DrawDebugLine(Agent.GetWorld(),startPos,startPos + Agent.GetActorTransform().GetRotation().GetForwardVector() * 50 ,FColorList::Magenta);
	
	//steering direction
	DrawDebugLine(Agent.GetWorld(),startPos,startPos + FVector(Agent.GetLinearVelocity() * scale,0) ,FColorList::Cyan);
	
	switch(Type)
	{
	case BehaviorType::Arrive:
		DrawDebugCylinder(Agent.GetWorld(),startPos,startPos,75,20,FColor::Orange);
		DrawDebugCylinder(Agent.GetWorld(),startPos,startPos,500,20,FColor::Blue);
		break;
	case BehaviorType::Wander:
		
		break;
	default:
		break;
	}
}

SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	Type = BehaviorType::Seek;
	FVector ToTarget = FVector{Target.Position,0.f} - Agent.GetActorLocation();
	ToTarget.Z = 0.f;

	FVector DesiredDir = ToTarget.GetSafeNormal();
	FVector Forward = Agent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Signed angle in radians
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);
	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);
	float MaxAngularSpeed = Agent.GetMaxAngularSpeed();
	// Slow down when close to target angle
	float SlowDownAngle = 30.f;
	
	float AngularVelocity = FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f) * MaxAngularSpeed;
	

	steering.AngularVelocity = AngularVelocity;
	steering.LinearVelocity = Target.Position - Agent.GetPosition();
	DrawDebugLines(Agent,steering);
	return steering;
}

SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	Type = BehaviorType::Flee;

	FVector ToTarget = Agent.GetActorLocation() - FVector{Target.Position,0.f};
	ToTarget.Z = 0.f;

	FVector DesiredDir = ToTarget.GetSafeNormal();
	FVector Forward = Agent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Signed angle in radians
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);
	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);
	float MaxAngularSpeed = Agent.GetMaxAngularSpeed();
	
	// Slow down when close to target angle
	float SlowDownAngle = 30.f;
	float AngularVelocity = FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f) * MaxAngularSpeed;
	

	steering.AngularVelocity = AngularVelocity;
	
	steering.LinearVelocity = Agent.GetPosition() - Target.Position;
	steering.LinearVelocity = steering.LinearVelocity.GetSafeNormal() * Agent.GetMaxLinearSpeed();
	DrawDebugLines(Agent,steering);
	return steering;
}

SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	Type = BehaviorType::Arrive;

	FVector ToTarget = FVector{Target.Position,0.f} - Agent.GetActorLocation();
	ToTarget.Z = 0.f;

	FVector DesiredDir = ToTarget.GetSafeNormal();
	FVector Forward = Agent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Signed angle in radians
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);
	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);
	float MaxAngularSpeed = Agent.GetMaxAngularSpeed();
	// Slow down when close to target angle
	float SlowDownAngle = 30.f;
	
	float AngularVelocity = FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f) * MaxAngularSpeed;
	// Reapply sign
	//AngularVelocity *= FMath::Sign(SignedAngleDegrees);

	steering.AngularVelocity = AngularVelocity;
	steering.LinearVelocity = Target.Position - Agent.GetPosition();

	auto distance = FVector2d::Distance(Agent.GetPosition() , Target.Position);
	if (FirstFrame == false)
	{
		MaxSpeed = Agent.GetMaxLinearSpeed();
		FirstFrame = true;
	}
	if (distance <= InsideDistance)
	{
		Agent.SetMaxLinearSpeed(0);
	}
	else if (distance <= OutsideDistance)
	{
		Agent.SetMaxLinearSpeed(MaxSpeed * (distance / OutsideDistance));
	}
	else
	{
		Agent.SetMaxLinearSpeed(MaxSpeed);
	}
	
	steering.LinearVelocity = steering.LinearVelocity.GetSafeNormal() * Agent.GetMaxLinearSpeed();
	DrawDebugLines(Agent,steering);
	return steering;
}

SteeringOutput Face::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	Type = BehaviorType::Face;
	Agent.SetMaxLinearSpeed(0);
	FVector ToTarget = FVector{Target.Position,0.f} - Agent.GetActorLocation();
	ToTarget.Z = 0.f;

	FVector DesiredDir = ToTarget.GetSafeNormal();
	FVector Forward = Agent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Signed angle in radians
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);
	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);
	float MaxAngularSpeed = Agent.GetMaxAngularSpeed();
	// Slow down when close to target angle
	float SlowDownAngle = 30.f;
	
	float AngularVelocity = FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f) * MaxAngularSpeed;
	// Reapply sign
	//AngularVelocity *= FMath::Sign(SignedAngleDegrees);
	
	steering.AngularVelocity = AngularVelocity;
	steering.LinearVelocity = (Target.Position - Agent.GetPosition()).GetSafeNormal() * Agent.GetMaxLinearSpeed();
	DrawDebugLines(Agent,steering);
	return steering;
}

SteeringOutput Wander::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	Type = BehaviorType::Wander;

	FVector Forward = Agent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	// Wander parameters
	float WanderOffset = 150.f;   // distance in front of agent
	float WanderRadius = 50.f;   // circle radius
	float WanderJitter = 30.f;
	// Random point on circle
	WanderAngle += FMath::FRandRange(-WanderJitter, WanderJitter) * DeltaT;

	FVector CircleCenter = Agent.GetActorLocation() + Forward * WanderOffset;

	FVector2D Displacement =
		FVector2D(FMath::Cos(WanderAngle), FMath::Sin(WanderAngle)) * WanderRadius;

	Target.Position =  Displacement + FVector2D(CircleCenter);
	

	FVector ToTarget = FVector{Target.Position,0.f} - Agent.GetActorLocation();
	ToTarget.Z = 0.f;

	FVector DesiredDir = ToTarget.GetSafeNormal();

	// Signed angle in radians
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);
	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);
	float MaxAngularSpeed = Agent.GetMaxAngularSpeed();
	// Slow down when close to target angle
	float SlowDownAngle = 30.f;
	
	float AngularVelocity = FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f) * MaxAngularSpeed;
	// Reapply sign
	AngularVelocity *= FMath::Sign(SignedAngleDegrees);

	steering.AngularVelocity = AngularVelocity;

	// Linear velocity toward wander target
	steering.LinearVelocity = (Target.Position - Agent.GetPosition()).GetSafeNormal() * Agent.GetMaxLinearSpeed();
	
	//Debug
	/*DrawDebugCircle(
				Agent.GetWorld(),
				CircleCenter,
				WanderRadius,
				16,
				FColor::Green,
				false,
				0.f,
				0,
				2.f,
				FVector(1,0,0),
				FVector(0,1,0),
				false);*/
	//DrawDebugLines(Agent,steering);
	
	return steering;
}

SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	

	FVector Forward = Agent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();
	
	FVector2D distance{ Target.Position - Agent.GetPosition() };
	double T{ distance.Size() / Agent.GetMaxLinearSpeed()};
	
	FVector ToTarget = FVector{Target.Position + Target.LinearVelocity * T,0.f} - Agent.GetActorLocation();
	Target.Position = Target.Position + Target.LinearVelocity * T;
	ToTarget.Z = 0.f;

	FVector DesiredDir = ToTarget.GetSafeNormal();

	// Signed angle in radians
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);
	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);
	float MaxAngularSpeed = Agent.GetMaxAngularSpeed();
	// Slow down when close to target angle
	float SlowDownAngle = 30.f;
	
	float AngularVelocity = FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f) * MaxAngularSpeed;
	// Reapply sign
	AngularVelocity *= FMath::Sign(SignedAngleDegrees);

	steering.AngularVelocity = AngularVelocity;
	
	// Linear velocity toward wander target
	steering.LinearVelocity = (Target.Position - Agent.GetPosition()).GetSafeNormal() * Agent.GetMaxLinearSpeed();
	DrawDebugLines(Agent,steering);
	return steering;
}

SteeringOutput Evade::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput steering{};
	
	FVector Forward = Agent.GetActorForwardVector();
	Forward.Z = 0.f;
	Forward.Normalize();

	FVector2D distance{Target.Position - Agent.GetPosition()};
	if (distance.SizeSquared() > m_EvadeRadius * m_EvadeRadius)
	{
		steering.IsValid = false;
		return steering;
	}
	
	double T{ distance.Size() / Agent.GetMaxLinearSpeed()};
	
	FVector ToTarget =  Agent.GetActorLocation() - FVector{Target.Position + Target.LinearVelocity * T,0.f};
	Target.Position = Target.Position + Target.LinearVelocity * T;
	
	ToTarget.Z = 0.f;

	FVector DesiredDir = ToTarget.GetSafeNormal();

	// Signed angle in radians
	float SignedAngle = FMath::Atan2(
		FVector::CrossProduct(Forward, DesiredDir).Z,
		FVector::DotProduct(Forward, DesiredDir)
	);
	float SignedAngleDegrees = FMath::RadiansToDegrees(SignedAngle);
	float MaxAngularSpeed = Agent.GetMaxAngularSpeed();
	// Slow down when close to target angle
	float SlowDownAngle = 30.f;
	
	float AngularVelocity = FMath::Clamp(SignedAngleDegrees / SlowDownAngle, -1.f, 1.f) * MaxAngularSpeed;
	// Reapply sign
	AngularVelocity *= FMath::Sign(SignedAngleDegrees);

	steering.AngularVelocity = AngularVelocity;
	
	// Linear velocity toward wander target
	steering.LinearVelocity = (Agent.GetPosition() - Target.Position).GetSafeNormal() * Agent.GetMaxLinearSpeed();
	DrawDebugLines(Agent,steering);
	return steering;
}
