#include "CoroStateMachine/CoroStateMachine.h"

void CoroStateMachine::Destroy()
{
	Reset();
	Sleeping = true;
}

void CoroStateMachine::ChangeToState(const CoroState& NewState)
{
	if (CurrentState && OnExitFunc)
	{
		OnExitFunc();
	}
	Reset();
	CurrentTask = NewState.Handle;
	CurrentState = NewState;
}

void CoroStateMachine::Reset()
{
	FreeEntireCoroutineStack();
	CurrentStateTransitions.clear();
	CurrentStatelessTasks.clear();
	NextState = nullptr;
	OnExitFunc = nullptr;
	CurrentTask = nullptr;
	Sleeping = false;
}

void CoroStateMachine::FreeEntireCoroutineStack()
{
	while (!CoroutineStack.empty())
	{
		if (auto Top = CoroutineStack.top())
		{
			Top.destroy();
		}
		CoroutineStack.pop();
	}
}

void CoroStateMachine::AwaitPush(const coroutine_handle<> NewHandle)
{
	if (CurrentTask)
	{
		CoroutineStack.push(CurrentTask);
	}
	CurrentTask = NewHandle;
}

std::function<CoroState()> CoroStateMachine::CheckNextTransition()
{
	if (CurrentStateTransitions.empty())
	{
		return nullptr;
	}

	auto& [TransFunc, StateFunc] = CurrentStateTransitions.front();
	if (TransFunc())
	{
		return StateFunc;
	}

	//Rotate our deque so the transition we just evaluated is now the back.
	CurrentStateTransitions.push_back(CurrentStateTransitions.front());
	CurrentStateTransitions.pop_front();

	return nullptr;
}

bool CoroStateMachine::Run()
{
	if (Sleeping)
	{
		return true;
	}
	if (!CurrentState)
	{
		if (NextState)
		{
			//We have a valid next state, so we move to that.
			ChangeToState(std::move(NextState()));
			return true;
		}
		return false;
	}

	for (const auto& StatelessTask : CurrentStatelessTasks)
	{
		StatelessTask();
	}

	//Check trasition, if we should transition run any exit code and switch states.
	if (const auto CheckResult = CheckNextTransition())
	{
		ChangeToState(CheckResult());
	}

	//If we have no valid task or the current one is done, pop the stack until we have a valid task or return false.
	while (!CurrentTask || CurrentTask.done())
	{
		if (CoroutineStack.empty())
		{
			if (NextState)
			{
				//We have a valid next state, so we move to that.
				ChangeToState(std::move(NextState()));
				return true;
			}

			//CurrentState.ExitAndDestroy();
			return false;
		}
		if (CurrentTask)
		{
			CurrentTask.destroy();
		}
		CurrentTask = CoroutineStack.top();
		CoroutineStack.pop();
	}

	CurrentTask.resume();
	return true;
}

CoroStateMachine& CoroStateMachine::AddTransition(
	const std::function<bool()>& TransitionFunc, const std::function<CoroState()>& StateConstructor)
{
	CurrentStateTransitions.push_back(TransitionBundle{TransitionFunc, StateConstructor});
	return *this;
}

CoroStateMachine& CoroStateMachine::ContinueWith(
	const std::function<CoroState()>& StateConstructor)
{
	NextState = StateConstructor;
	return *this;
}

CoroStateMachine& CoroStateMachine::OnExit(const std::function<void()>& Finalizer)
{
	OnExitFunc = Finalizer;
	return *this;
}

TaskAwaiter CoroStateMachine::WaitForTask(CoroTask&& TaskToAwait)
{
	return TaskAwaiter{*this, std::forward<CoroTask>(TaskToAwait)};
}

CoroStateMachine& CoroStateMachine::AddStatelessTask(const std::function<void()>& Task)
{
	CurrentStatelessTasks.push_back(std::move(Task));
	return *this;
}
