#pragma once
#include <vector>

#include "../Steering/SteeringBehaviors.h"

//****************
//BLENDED STEERING
class BlendedSteering final: public ISteeringBehavior
{
public:
	struct WeightedBehavior
	{
		ISteeringBehavior* pBehavior = nullptr;
		float Weight = 0.f;

		WeightedBehavior(ISteeringBehavior* const pBehavior, float Weight) :
			pBehavior(pBehavior),
			Weight(Weight)
		{};
	};

	BlendedSteering(const std::vector<WeightedBehavior>& WeightedBehaviors);

	void AddBehaviour(const WeightedBehavior& WeightedBehavior) { WeightedBehaviors.push_back(WeightedBehavior); }
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

	float* GetWeight(ISteeringBehavior* const SteeringBehavior);
	
	// returns a reference to the weighted behaviors, can be used to adjust weighting. Is not intended to alter the behaviors themselves.
	std::vector<WeightedBehavior>& GetWeightedBehaviorsRef() { return WeightedBehaviors; }

private:
	std::vector<WeightedBehavior> WeightedBehaviors = {};

	using ISteeringBehavior::SetTarget; // made private because targets need to be set on the individual behaviors, not the combined behavior
};

//*****************
//PRIORITY STEERING
class PrioritySteering final: public ISteeringBehavior
{
public:
	PrioritySteering(const std::vector<ISteeringBehavior*>& priorityBehaviors)
		:m_PriorityBehaviors(priorityBehaviors) 
	{}

	void AddBehaviour(ISteeringBehavior* const pBehavior) { m_PriorityBehaviors.push_back(pBehavior); }
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

private:
	std::vector<ISteeringBehavior*> m_PriorityBehaviors = {};

	using ISteeringBehavior::SetTarget; // made private because targets need to be set on the individual behaviors, not the combined behavior
};

class WolfpackSteering : public ISteeringBehavior
{
public:
   
    float SurroundRadius   = 200.f;
    float OutsideDistance  = 400.f;
    float InsideDistance   = 40.f;
    float SeparationWeight = 0.6f;
    float SeparationRadius = 120.f;
	
    int32 RegisterMember(ASteeringAgent* Agent);
    void  UnregisterMember(ASteeringAgent* Agent);
    int32 GetPackSize() const { return static_cast<int32>(Members.Num()); }
	
    SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
    SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent, int32 SlotIndex);
 
private:
    
    TArray<ASteeringAgent*> Members;
    TMap<ASteeringAgent*, float> CachedMaxSpeeds;
   
    FVector2D CalcSlotPosition(int32 SlotIndex) const;
    FVector2D CalcSeparationDir(const ASteeringAgent& Agent) const;
};

class MetroBoardingSteering : public ISteeringBehavior
{
public:
    FVector2D DoorPosition  = FVector2D::ZeroVector;
    FVector2D DoorDirection = FVector2D(0.f, 1.f);
    float     DoorWidth     = 120.f;
	
    float QueueOffset       = 80.f;
    float QueueSpacing      = 90.f;
    float ArriveTolerance   = 30.f;
    float ArriveFullSpeed   = 350.f;
    float YieldSidestepDist = 80.f;
    float SeparationRadius  = 70.f;
    float SeparationStrength = 1.2f;
 
    enum class EBoardingPhase : uint8
    {
        Queueing,   
        Yielding,   
        Funneling,  
        Seated,     
    };
 
    struct Ticket
    {
        int32          QueueSlot   = 0;
        FVector2D      SeatPos     = FVector2D::ZeroVector;
        EBoardingPhase Phase       = EBoardingPhase::Queueing;
        FVector2D      YieldTarget = FVector2D::ZeroVector;
        bool           YieldSet    = false;
    };
	
    Ticket RegisterPassenger(ASteeringAgent* Agent, const FVector2D& SeatPosition);
    void   UnregisterPassenger(ASteeringAgent* Agent);
    int32  GetPassengerCount() const { return Passengers.Num(); }
	void SetExitFlow(const FVector2D& FlowDir, bool bActive);
	
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent, Ticket& AgentTicket);
 
private:
	
	TArray<ASteeringAgent*> Passengers;

	FVector2D ExitFlowDir    = FVector2D::ZeroVector;
	bool      bExitFlowActive = false;
 
	FVector2D CalcQueueSlotPos(int32 SlotIndex) const;
	FVector2D CalcFunnelEntryPos() const;
	float ArriveSpeed(float Distance, float MaxSpeed) const;
	FVector2D CalcSeparationDir(const ASteeringAgent& Agent) const;
	static float TurnToward(const FVector2D& DesiredDir,ASteeringAgent&  Agent,float            DeltaT);
	bool IsInsideTrain(const FVector2D& AgentPos) const;

};
