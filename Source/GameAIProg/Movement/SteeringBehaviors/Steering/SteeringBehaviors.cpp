#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"
#include "DrawDebugHelpers.h"
#include "Components/CapsuleComponent.h"

// ─── Shared helper ────────────────────────────────────────────────────────────
// Given a desired 2-D direction, return the clamped angular velocity (deg/s)
// that steers the agent toward it, using the same pattern as Seek.
static float CalcAngularVelocity(const FVector2D& DesiredDir,
                                  ASteeringAgent&   Agent,
                                  float             DeltaT)
{
    if (DesiredDir.IsNearlyZero()) return 0.f;

    float DesiredAngle = FMath::RadiansToDegrees(
        FMath::Atan2(DesiredDir.Y, DesiredDir.X));

    float CurrentAngle = Agent.GetActorRotation().Yaw;
    float AngleDiff    = FMath::FindDeltaAngleDegrees(CurrentAngle, DesiredAngle);

    return FMath::Clamp(AngleDiff / DeltaT,
                        -Agent.GetMaxAngularSpeed(),
                         Agent.GetMaxAngularSpeed());
}

// ─── Debug ────────────────────────────────────────────────────────────────────
void ISteeringBehavior::DrawDebugLines(ASteeringAgent& Agent,
                                       const SteeringOutput& steering) const
{
    const float   Height   = Agent.GetActorTransform().GetTranslation().Z;
    const FVector StartPos = FVector(Agent.GetPosition(), Height);
    const double  Scale    = steering.LinearVelocity.Length() / Agent.GetMaxLinearSpeed();

    // Target
    DrawDebugSphere(Agent.GetWorld(), FVector(Target.Position, Height),
                    20, 10, FColorList::Red);
    // Desired direction
    FVector EndDir = FVector(Agent.GetPosition()
                             + steering.LinearVelocity.GetSafeNormal() * 200, Height);
    DrawDebugLine(Agent.GetWorld(), StartPos, EndDir, FColorList::Green);
    // Forward
    DrawDebugLine(Agent.GetWorld(), StartPos,
                  StartPos + Agent.GetActorTransform().GetRotation().GetForwardVector() * 50,
                  FColorList::Magenta);
    // Current velocity
    DrawDebugLine(Agent.GetWorld(), StartPos,
                  StartPos + FVector(Agent.GetLinearVelocity() * Scale, 0),
                  FColorList::Cyan);

    switch (Type)
    {
    case BehaviorType::Arrive:
        DrawDebugCylinder(Agent.GetWorld(), StartPos, StartPos, 75,  20, FColor::Orange);
        DrawDebugCylinder(Agent.GetWorld(), StartPos, StartPos, 500, 20, FColor::Blue);
        break;
    default: break;
    }
}

// ─── Seek ─────────────────────────────────────────────────────────────────────
SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Output{};
    Type = BehaviorType::Seek;

    FVector2D ToTarget = Target.Position - FVector2D(Agent.GetActorLocation());
    if (ToTarget.IsNearlyZero()) return Output;

    Output.AngularVelocity = CalcAngularVelocity(ToTarget.GetSafeNormal(), Agent, DeltaT);
    Output.LinearVelocity  = FVector2D(Agent.GetMaxLinearSpeed(), 0.f);

    DrawDebugLines(Agent, Output);
    return Output;
}

// ─── Flee ─────────────────────────────────────────────────────────────────────
SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Output{};
    Type = BehaviorType::Flee;

    // Opposite direction to Seek — away from target
    FVector2D ToTarget  = Target.Position - FVector2D(Agent.GetActorLocation());
    FVector2D AwayDir   = -ToTarget.GetSafeNormal();
    if (ToTarget.IsNearlyZero()) return Output;

    Output.AngularVelocity = CalcAngularVelocity(AwayDir, Agent, DeltaT);
    Output.LinearVelocity  = FVector2D(Agent.GetMaxLinearSpeed(), 0.f);

    DrawDebugLines(Agent, Output);
    return Output;
}

// ─── Arrive ───────────────────────────────────────────────────────────────────
SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Output{};
    Type = BehaviorType::Arrive;

    // Cache max speed once so SetMaxLinearSpeed mutations don't corrupt it
    if (!bCachedMaxSpeed)
    {
        MaxSpeed       = Agent.GetMaxLinearSpeed();
        bCachedMaxSpeed = true;
    }

    FVector2D ToTarget = Target.Position - Agent.GetPosition();
    if (ToTarget.IsNearlyZero())
    {
        Agent.SetMaxLinearSpeed(0.f);
        return Output;
    }

    Output.AngularVelocity = CalcAngularVelocity(ToTarget.GetSafeNormal(), Agent, DeltaT);

    // Scale speed by distance
    float Distance    = ToTarget.Size();
    float DesiredSpeed = 0.f;
    if (Distance > OutsideDistance)
        DesiredSpeed = MaxSpeed;
    else if (Distance > InsideDistance)
        DesiredSpeed = MaxSpeed * (Distance / OutsideDistance);
    // else DesiredSpeed stays 0

    Agent.SetMaxLinearSpeed(DesiredSpeed);
    Output.LinearVelocity = FVector2D(DesiredSpeed, 0.f);

    DrawDebugLines(Agent, Output);
    return Output;
}

// ─── Face ─────────────────────────────────────────────────────────────────────
SteeringOutput Face::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Output{};
    Type = BehaviorType::Face;

    FVector2D ToTarget = Target.Position - FVector2D(Agent.GetActorLocation());
    if (ToTarget.IsNearlyZero()) return Output;

    Output.AngularVelocity = CalcAngularVelocity(ToTarget.GetSafeNormal(), Agent, DeltaT);
    Output.LinearVelocity  = FVector2D(0.f, 0.f); // rotate only, no translation

    DrawDebugLines(Agent, Output);
    return Output;
}

// ─── Wander ───────────────────────────────────────────────────────────────────
SteeringOutput Wander::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Output{};
    Type = BehaviorType::Wander;

    // Project a circle ahead of the agent and jitter a point on its rim
    const float WanderOffset = 150.f;
    const float WanderRadius = 50.f;
    const float WanderJitter = 30.f;

    WanderAngle += FMath::FRandRange(-WanderJitter, WanderJitter) * DeltaT;

    FVector2D AgentPos    = Agent.GetPosition();
    FVector2D AgentFwd    = FVector2D(Agent.GetActorForwardVector());
    FVector2D CircleCenter = AgentPos + AgentFwd * WanderOffset;
    FVector2D WanderTarget = CircleCenter
                           + FVector2D(FMath::Cos(WanderAngle),
                                       FMath::Sin(WanderAngle)) * WanderRadius;

    FVector2D ToTarget = WanderTarget - AgentPos;

    Output.AngularVelocity = CalcAngularVelocity(ToTarget.GetSafeNormal(), Agent, DeltaT);
    Output.LinearVelocity  = FVector2D(Agent.GetMaxLinearSpeed(), 0.f);

    DrawDebugLines(Agent, Output);
    return Output;
}

// ─── Pursuit ──────────────────────────────────────────────────────────────────
SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Output{};
    Type = BehaviorType::Pursuit;

    FVector2D ToTarget  = Target.Position - Agent.GetPosition();
    float     T         = ToTarget.Size() / FMath::Max(Agent.GetMaxLinearSpeed(), 1.f);
    FVector2D Predicted = Target.Position + Target.LinearVelocity * T;

    FVector2D ToPredicted = Predicted - FVector2D(Agent.GetActorLocation());
    if (ToPredicted.IsNearlyZero()) return Output;

    Output.AngularVelocity = CalcAngularVelocity(ToPredicted.GetSafeNormal(), Agent, DeltaT);
    Output.LinearVelocity  = FVector2D(Agent.GetMaxLinearSpeed(), 0.f);

    DrawDebugLines(Agent, Output);
    return Output;
}

// ─── Evade ────────────────────────────────────────────────────────────────────
SteeringOutput Evade::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput Output{};
    Type = BehaviorType::Evade;

    FVector2D ToTarget = Target.Position - Agent.GetPosition();

    // Only evade when the threat is within radius
    if (ToTarget.SizeSquared() > m_EvadeRadius * m_EvadeRadius)
    {
        Output.IsValid = false;
        return Output;
    }

    float     T         = ToTarget.Size() / FMath::Max(Agent.GetMaxLinearSpeed(), 1.f);
    FVector2D Predicted = Target.Position + Target.LinearVelocity * T;

    // Flee from the predicted position
    FVector2D AwayDir = (Agent.GetPosition() - Predicted).GetSafeNormal();
    if (AwayDir.IsNearlyZero()) return Output;

    Output.AngularVelocity = CalcAngularVelocity(AwayDir, Agent, DeltaT);
    Output.LinearVelocity  = FVector2D(Agent.GetMaxLinearSpeed(), 0.f);

    DrawDebugLines(Agent, Output);
    return Output;
}