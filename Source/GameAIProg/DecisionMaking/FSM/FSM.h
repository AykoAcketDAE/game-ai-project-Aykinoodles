#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <string>

#include "AIController.h"
#include "BrainComponent.h"
#include "BehaviorTree/BlackboardComponent.h"

namespace GameAI::FSM
{
    class Transition;

    // ----------------------------------------------------------------
    // State base class — receives Blackboard + Controller on enter/tick/exit
    // ----------------------------------------------------------------
    class State
    {
    public:
        explicit State(std::string InName) : Name(std::move(InName)) {}
        virtual ~State() = default;

        virtual void OnEnter(AAIController* Controller, UBlackboardComponent* BB) {}
        virtual void OnTick(float Dt, AAIController* Controller, UBlackboardComponent* BB) {}
        virtual void OnExit(AAIController* Controller, UBlackboardComponent* BB) {}

        const std::string& GetName() const { return Name; }

        void AddTransition(std::unique_ptr<Transition> T);
        Transition* EvaluateTransitions() const;

    private:
        std::string Name;
        std::vector<std::unique_ptr<Transition>> Transitions;
    };

    // ----------------------------------------------------------------
    // Transition — condition lambda, no Blackboard needed here since
    // lambdas can capture whatever they need at setup time
    // ----------------------------------------------------------------
    class Transition
    {
    public:
        Transition(State* InFrom, State* InTo, std::function<bool()> InEval)
            : From(InFrom), To(InTo), EvalFunc(std::move(InEval)) {}

        bool    Evaluate()  const { return EvalFunc ? EvalFunc() : false; }
        State*  GetTo()     const { return To; }
        State*  GetFrom()   const { return From; }

    private:
        State*                  From;
        State*                  To;
        std::function<bool()>   EvalFunc;
    };

    // inline definitions that depend on Transition being complete
    inline void State::AddTransition(std::unique_ptr<Transition> T)
    {
        Transitions.push_back(std::move(T));
    }

    inline Transition* State::EvaluateTransitions() const
    {
        for (const auto& T : Transitions)
            if (T->Evaluate()) return T.get();
        return nullptr;
    }

    // ----------------------------------------------------------------
    // FSM — drives the active state, forwards Blackboard + Controller
    // ----------------------------------------------------------------
    class FSM
    {
    public:
        // First state added = initial state
        void AddState(std::unique_ptr<State> NewState)
        {
            if (States.empty())
                ActiveState = NewState.get();
            States.push_back(std::move(NewState));
        }

        void AddTransition(State* From, State* To, std::function<bool()> EvalFunc)
        {
            if (From && To)
                From->AddTransition(std::make_unique<Transition>(From, To, std::move(EvalFunc)));
        }

        void Start(AAIController* Controller, UBlackboardComponent* BB)
        {
            if (ActiveState)
                ActiveState->OnEnter(Controller, BB);
        }

        void Stop(AAIController* Controller, UBlackboardComponent* BB)
        {
            if (ActiveState)
                ActiveState->OnExit(Controller, BB);
        }

        void Tick(float DeltaTime, AAIController* Controller, UBlackboardComponent* BB)
        {
            if (!ActiveState) return;

            ActiveState->OnTick(DeltaTime, Controller, BB);

            if (Transition* Fired = ActiveState->EvaluateTransitions())
            {
                ActiveState->OnExit(Controller, BB);
                ActiveState = Fired->GetTo();
                ActiveState->OnEnter(Controller, BB);
            }
        }

        State* GetActiveState() const { return ActiveState; }

    private:
        std::vector<std::unique_ptr<State>> States;
        State* ActiveState{nullptr};
    };

} // namespace GameAI::FSM