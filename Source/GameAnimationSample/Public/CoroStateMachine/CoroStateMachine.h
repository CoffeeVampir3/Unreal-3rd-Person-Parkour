// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include <concepts>
#include <coroutine>
#include <stack>
#include <deque>
#include <vector>

#include "CoroState.h"
#include "CoroTask.h"

struct CoroState;
struct CoroTask;

struct CoroStateMachine;
struct TaskAwaiter;

using namespace std;

struct TransitionBundle
{
	std::function<bool()> TransFunc;
	std::function<CoroState()> StateFunc;
};

struct CoroStateMachine
{
	friend struct TaskAwaiter;
	CoroStateMachine() {};
	
	void Destroy();
	
	void ChangeToState(const CoroState& NewState);
	void Reset();
	bool Run();

	TaskAwaiter WaitForTask(CoroTask&& TaskToAwait);
	
	CoroStateMachine& AddStatelessTask(const std::function<void()>& Task);
	CoroStateMachine& AddTransition(const std::function<bool()>& TransitionFunc, const std::function<CoroState()>& StateConstructor);
	CoroStateMachine& ContinueWith(const std::function<CoroState()>& StateConstructor);
	CoroStateMachine& OnExit(const std::function<void()>& Finalizer);
	
private:
	void FreeEntireCoroutineStack();
	void AwaitPush(const coroutine_handle<> NewHandle);
	std::function<CoroState()> CheckNextTransition();
	
	CoroState CurrentState {nullptr};
	std::function<CoroState()> NextState {nullptr};
	std::function<void()> OnExitFunc {nullptr};
	coroutine_handle<> CurrentTask{nullptr};
	stack<coroutine_handle<>> CoroutineStack;
	std::deque<TransitionBundle> CurrentStateTransitions{};
	std::vector<std::function<void()>> CurrentStatelessTasks{};
	bool Sleeping{false};
};

struct TaskAwaiter
{
	CoroStateMachine& SM;
	CoroTask Task;

	explicit TaskAwaiter(CoroStateMachine& InSM, CoroTask&& TaskToAwait) : SM{InSM}, Task{std::move(TaskToAwait)} {}

	bool await_ready() const noexcept { return false; }
	bool await_suspend(const std::coroutine_handle<> Handle) noexcept
	{
		SM.AwaitPush(Task.Handle);
		return true;
	}
	void await_resume() const noexcept {}
};
