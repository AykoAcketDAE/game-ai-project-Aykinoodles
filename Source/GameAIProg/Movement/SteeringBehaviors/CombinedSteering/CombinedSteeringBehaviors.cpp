
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

	
	return Steering;
}


int32 WolfpackSteering::RegisterMember(ASteeringAgent* Agent)
{
    if (!Agent) return INDEX_NONE;
	
    int32 Existing = Members.IndexOfByKey(Agent);
    if (Existing != INDEX_NONE) return Existing;
 
    Members.Add(Agent);
    return Members.Num() - 1;
}
 
void WolfpackSteering::UnregisterMember(ASteeringAgent* Agent)
{
    Members.Remove(Agent);
    CachedMaxSpeeds.Remove(Agent);
}

FVector2D WolfpackSteering::CalcSlotPosition(int32 SlotIndex) const
{
    const int32 PackSize = Members.Num();
    if (PackSize == 0) return Target.Position;
	
    const FVector2D LeadOffset = Target.LinearVelocity * 0.25f;
    const float     AngleRad   = FMath::DegreesToRadians(360.f * static_cast<float>(SlotIndex) / static_cast<float>(PackSize));
 
    return Target.Position + LeadOffset + FVector2D(FMath::Cos(AngleRad), FMath::Sin(AngleRad)) * SurroundRadius;
}
 
FVector2D WolfpackSteering::CalcSeparationDir(const ASteeringAgent& Agent) const
{
    FVector2D SepForce = FVector2D::ZeroVector;
 
    for (const ASteeringAgent* Other : Members)
    {
        if (!Other || Other == &Agent) continue;
 
        FVector2D ToOther = FVector2D(Other->GetActorLocation()) - FVector2D(Agent.GetActorLocation());
        float DistSq = ToOther.SizeSquared();
 
        if (DistSq > 0.f && DistSq < SeparationRadius * SeparationRadius)
        {
            float Dist = FMath::Sqrt(DistSq);
            
            float Strength = 1.f - (Dist / SeparationRadius);
            SepForce -= (ToOther / Dist) * Strength;
        }
    }
 
    return SepForce; 
}
 
 
SteeringOutput WolfpackSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    int32 SlotIndex = Members.IndexOfByKey(&Agent);
    if (SlotIndex == INDEX_NONE)
    {
        SlotIndex = RegisterMember(&Agent);
    }
    return CalculateSteering(DeltaT, Agent, SlotIndex);
}
 

SteeringOutput WolfpackSteering::CalculateSteering(float DeltaT,ASteeringAgent& Agent,int32 SlotIndex)
{
    SteeringOutput Output{};
    Type = BehaviorType::Seek; 
    if (!CachedMaxSpeeds.Contains(&Agent))
        CachedMaxSpeeds.Add(&Agent, Agent.GetMaxLinearSpeed());
 
    const float MaxSpeed = CachedMaxSpeeds[&Agent];
	
    const FVector2D SlotPos   = CalcSlotPosition(SlotIndex);
    const FVector2D AgentPos  = Agent.GetPosition();
    const FVector2D ToSlot    = SlotPos - AgentPos;
    const float     Distance  = ToSlot.Size();
 
    FVector2D SepDir    = CalcSeparationDir(Agent);
    float     SepLen    = SepDir.Size();
	
    float SlotWeight = FMath::Clamp(Distance / OutsideDistance, 0.f, 1.f);
    float SepWeightDynamic = SeparationWeight * (1.f - SlotWeight * 0.5f);
 
    FVector2D DesiredDir = ToSlot.GetSafeNormal() * SlotWeight + (SepLen > 0.f ? SepDir.GetSafeNormal() : FVector2D::ZeroVector) * SepWeightDynamic;
 
    if (DesiredDir.IsNearlyZero())
    {
        Agent.SetMaxLinearSpeed(0.f);
        return Output;
    }
 
    DesiredDir.Normalize();
	
    float DesiredSpeed = 0.f;
    if (Distance > OutsideDistance)
        DesiredSpeed = MaxSpeed;
    else if (Distance > InsideDistance)
        DesiredSpeed = MaxSpeed * (Distance / OutsideDistance);
 
    Agent.SetMaxLinearSpeed(DesiredSpeed);
	
    Output.LinearVelocity  = FVector2D(DesiredSpeed, 0.f);
    Output.AngularVelocity = [&]() -> float
    {
        if (DesiredDir.IsNearlyZero()) return 0.f;
        float DesiredAngle  = FMath::RadiansToDegrees(FMath::Atan2(DesiredDir.Y, DesiredDir.X));
        float CurrentAngle  = Agent.GetActorRotation().Yaw;
        float AngleDiff     = FMath::FindDeltaAngleDegrees(CurrentAngle, DesiredAngle);
        return FMath::Clamp(AngleDiff / DeltaT,-Agent.GetMaxAngularSpeed(),Agent.GetMaxAngularSpeed());
    }();
 
    Output.IsValid = true;
 

#if ENABLE_DRAW_DEBUG
    const float   Height    = Agent.GetActorTransform().GetTranslation().Z;
    const FVector AgentVec  = FVector(AgentPos, Height);
 
    
    DrawDebugSphere(Agent.GetWorld(),FVector(SlotPos, Height),16, 8, FColor::Orange);
 
    
    DrawDebugLine(Agent.GetWorld(),
                  AgentVec,
                  FVector(SlotPos, Height),
                  FColor::Orange, false, -1.f, 0, 1.f);
 
   
    if (SepLen > 0.f)
        DrawDebugLine(Agent.GetWorld(),
                      AgentVec,
                      AgentVec + FVector(SepDir.GetSafeNormal() * 80.f, 0.f),
                      FColor::Red, false, -1.f, 0, 1.5f);
 
  
    DrawDebugLine(Agent.GetWorld(),
                  AgentVec,
                  AgentVec + FVector(DesiredDir * 120.f, 0.f),
                  FColor::Green, false, -1.f, 0, 2.f);
    
    if (SlotIndex == 0)
    {
        DrawDebugCircle(Agent.GetWorld(),
                        FVector(Target.Position, Height),
                        SurroundRadius,
                        32, FColor::Yellow,
                        false, -1.f, 0, 1.f,
                        FVector(1, 0, 0), FVector(0, 1, 0));
    }
#endif
 
    DrawDebugLines(Agent, Output);
    return Output;
}

MetroBoardingSteering::Ticket MetroBoardingSteering::RegisterPassenger(
    ASteeringAgent*  Agent,
    const FVector2D& SeatPosition)
{
    Ticket T;
    T.QueueSlot = Passengers.Num();   
    T.SeatPos   = SeatPosition;
    T.Phase     = EBoardingPhase::Queueing;
 
    Passengers.Add(Agent);
    return T;
    
}
 
void MetroBoardingSteering::UnregisterPassenger(ASteeringAgent* Agent)
{
    Passengers.Remove(Agent);
}

 
void MetroBoardingSteering::SetExitFlow(const FVector2D& FlowDir, bool bActive)
{
    ExitFlowDir     = FlowDir.IsNearlyZero() ? FlowDir : FlowDir.GetSafeNormal();
    bExitFlowActive = bActive;
}

 
FVector2D MetroBoardingSteering::CalcQueueSlotPos(int32 SlotIndex) const
{
   
    FVector2D Perp(-DoorDirection.Y, DoorDirection.X);
 
   
    int32 Side          = (SlotIndex % 2 == 0) ? 1 : -1;
    int32 Row           = SlotIndex / 2;
    float LateralOffset = Side * Row * (DoorWidth * 0.5f + 10.f);
    float DepthOffset   = QueueOffset + Row * QueueSpacing * 0.5f;
 
    
    return DoorPosition
         - DoorDirection * DepthOffset
         + Perp          * LateralOffset;
}
 
FVector2D MetroBoardingSteering::CalcFunnelEntryPos() const
{
    return DoorPosition - DoorDirection * (DoorWidth * 0.5f);
}
 
bool MetroBoardingSteering::IsInsideTrain(const FVector2D& AgentPos) const
{
 
    FVector2D ToAgent = AgentPos - DoorPosition;
    return FVector2D::DotProduct(ToAgent, DoorDirection) > 0.f;
}
 

float MetroBoardingSteering::ArriveSpeed(float Distance, float MaxSpeed) const
{
    if (Distance > ArriveFullSpeed) return MaxSpeed;
    if (Distance > ArriveTolerance) return MaxSpeed * (Distance / ArriveFullSpeed);
    return 0.f;
}
 
FVector2D MetroBoardingSteering::CalcSeparationDir(const ASteeringAgent& Agent) const
{
    FVector2D SepForce = FVector2D::ZeroVector;
 
    for (const ASteeringAgent* Other : Passengers)
    {
        if (!Other || Other == &Agent) continue;
 
        FVector2D ToOther = FVector2D(Other->GetActorLocation())
                          - FVector2D(Agent.GetActorLocation());
        float DistSq = ToOther.SizeSquared();
 
        if (DistSq > 0.f && DistSq < SeparationRadius * SeparationRadius)
        {
            float Dist     = FMath::Sqrt(DistSq);
            float Strength = SeparationStrength * (1.f - Dist / SeparationRadius);
            SepForce      -= (ToOther / Dist) * Strength;
        }
    }
 
    return SepForce;
}
 
float MetroBoardingSteering::TurnToward(const FVector2D& DesiredDir,
                                         ASteeringAgent&  Agent,
                                         float            DeltaT)
{
    if (DesiredDir.IsNearlyZero())       return 0.f;
    if (DeltaT < KINDA_SMALL_NUMBER)     return 0.f;
 
    float DesiredAngle = FMath::RadiansToDegrees(
        FMath::Atan2(DesiredDir.Y, DesiredDir.X));
    float CurrentAngle = Agent.GetActorRotation().Yaw;
    float AngleDiff    = FMath::FindDeltaAngleDegrees(CurrentAngle, DesiredAngle);
 
    return FMath::Clamp(AngleDiff / DeltaT,-Agent.GetMaxAngularSpeed(),Agent.GetMaxAngularSpeed());
}
 

SteeringOutput MetroBoardingSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    return SteeringOutput{};
}

SteeringOutput MetroBoardingSteering::CalculateSteering(float           DeltaT,
                                                         ASteeringAgent& Agent,
                                                         Ticket&         T)
{
    SteeringOutput Output{};
    Output.IsValid = true;
 
    const FVector2D AgentPos = Agent.GetPosition();
    const float     MaxSpeed = Agent.GetMaxLinearSpeed();
 
  
    FVector2D SepDir    = CalcSeparationDir(Agent);
    float     SepWeight = 0.35f;
 
    switch (T.Phase)
    {
 
    case EBoardingPhase::Queueing:
    {
        FVector2D SlotPos  = CalcQueueSlotPos(T.QueueSlot);
        FVector2D ToSlot   = SlotPos - AgentPos;
        float     Distance = ToSlot.Size();
 
        FVector2D DesiredDir = ToSlot.GetSafeNormal()
                             + SepDir.GetSafeNormal() * SepWeight;
        if (!DesiredDir.IsNearlyZero()) DesiredDir.Normalize();
 
        float Speed = ArriveSpeed(Distance, MaxSpeed);
        Output.LinearVelocity  = FVector2D(Speed, 0.f);
        Output.AngularVelocity = TurnToward(DesiredDir, Agent, DeltaT);
            
        if (Distance < ArriveTolerance && bExitFlowActive)
            T.Phase = EBoardingPhase::Yielding;
 
        
        if (Distance < ArriveTolerance && !bExitFlowActive)
            T.Phase = EBoardingPhase::Funneling;
 
        break;
    }
 
  
    case EBoardingPhase::Yielding:
    {
        
        if (!T.YieldSet)
        {
            FVector2D Perp(-ExitFlowDir.Y, ExitFlowDir.X);
 
           
            FVector2D ToAgent = AgentPos - DoorPosition;
            float     Side    = FVector2D::DotProduct(ToAgent, Perp) >= 0.f ? 1.f : -1.f;
 
            T.YieldTarget = AgentPos + Perp * (Side * YieldSidestepDist);
            T.YieldSet    = true;
        }
 
        FVector2D ToYield  = T.YieldTarget - AgentPos;
        float     Distance = ToYield.Size();
        float     Speed    = ArriveSpeed(Distance, MaxSpeed * 0.6f);
 
        FVector2D DesiredDir = ToYield.GetSafeNormal()
                             + SepDir.GetSafeNormal() * SepWeight;
        if (!DesiredDir.IsNearlyZero()) DesiredDir.Normalize();
 
        Output.LinearVelocity  = FVector2D(Speed, 0.f);
        Output.AngularVelocity = TurnToward(DesiredDir, Agent, DeltaT);
 
       
        if (!bExitFlowActive)
        {
            T.Phase    = EBoardingPhase::Funneling;
            T.YieldSet = false; 
        }
 
        break;
    }
    case EBoardingPhase::Funneling:
    {
        FVector2D ToDoor   = DoorPosition - AgentPos;
        float     Distance = ToDoor.Size();
 
        FVector2D DesiredDir = ToDoor.GetSafeNormal()
                             + SepDir.GetSafeNormal() * (SepWeight * 2.f);
        if (!DesiredDir.IsNearlyZero()) DesiredDir.Normalize();
 
        float Speed = ArriveSpeed(Distance, MaxSpeed);
        Output.LinearVelocity  = FVector2D(Speed, 0.f);
        Output.AngularVelocity = TurnToward(DesiredDir, Agent, DeltaT);
            
        if (IsInsideTrain(AgentPos))
            T.Phase = EBoardingPhase::Seated;
 
        break;
    }
    
    case EBoardingPhase::Seated:
    {
        FVector2D ToSeat   = T.SeatPos - AgentPos;
        float     Distance = ToSeat.Size();
 
        if (Distance < ArriveTolerance)
        {
            
            Output.LinearVelocity  = FVector2D::ZeroVector;
            Output.AngularVelocity = 0.f;
            Output.IsValid         = false;
            break;
        }
 
        FVector2D DesiredDir = ToSeat.GetSafeNormal()
                             + SepDir.GetSafeNormal() * (SepWeight * 0.5f);
        if (!DesiredDir.IsNearlyZero()) DesiredDir.Normalize();
 
        float Speed = ArriveSpeed(Distance, MaxSpeed);
        Output.LinearVelocity  = FVector2D(Speed, 0.f);
        Output.AngularVelocity = TurnToward(DesiredDir, Agent, DeltaT);
 
        break;
    }
 
    }
#if ENABLE_DRAW_DEBUG
    const float   Z        = Agent.GetActorTransform().GetTranslation().Z;
    const FVector AgentVec = FVector(AgentPos, Z);
    
    static const FColor PhaseColors[] = {
        FColor::Blue,    
        FColor::Orange,  
        FColor::Yellow,  
        FColor::Green    
    };
    const FColor PColor = PhaseColors[static_cast<uint8>(T.Phase)];
 
    if (!Output.LinearVelocity.IsNearlyZero())
    {
        FVector2D FwdDir = FVector2D(Agent.GetActorForwardVector());
        DrawDebugLine(Agent.GetWorld(),
                      AgentVec,
                      AgentVec + FVector(FwdDir * 80.f, 0.f),
                      PColor, false, -1.f, 0, 2.f);
    }
    if (!SepDir.IsNearlyZero())
        DrawDebugLine(Agent.GetWorld(),
                      AgentVec,
                      AgentVec + FVector(SepDir.GetSafeNormal() * 60.f, 0.f),
                      FColor::Red, false, -1.f, 0, 1.5f);

    FVector2D DoorPerp(-DoorDirection.Y, DoorDirection.X);
    DrawDebugLine(Agent.GetWorld(),
                  FVector(DoorPosition - DoorPerp * DoorWidth * 0.5f, Z),
                  FVector(DoorPosition + DoorPerp * DoorWidth * 0.5f, Z),
                  FColor::White, false, -1.f, 0, 3.f);
 
    DrawDebugSphere(Agent.GetWorld(),
                    FVector(CalcQueueSlotPos(T.QueueSlot), Z),
                    12, 6, FColor::Cyan);
 
    DrawDebugSphere(Agent.GetWorld(),
                    FVector(T.SeatPos, Z),
                    12, 6, FColor::Green);
#endif
    return Output;
}

