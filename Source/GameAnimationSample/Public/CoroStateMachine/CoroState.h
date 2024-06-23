#pragma once
#include <coroutine>
#include <utility>

struct CoroState;

struct CoroState
{
	struct promise_type;
	using TaskHandle = std::coroutine_handle<promise_type>;

	struct promise_type {
		void unhandled_exception() noexcept {}
		CoroState get_return_object() { return TaskHandle::from_promise(*this); }
		std::suspend_always initial_suspend() noexcept { return {}; }
		std::suspend_always final_suspend() noexcept { return {}; }
		void return_void() const noexcept {}
	};
	
	TaskHandle Handle;
	CoroState(const TaskHandle H) : Handle{H} {}

	//~NecroAiState
	//This memory is owned and managed by the State Machine, it frees everything and manages these coroutine lifetimes.
	
	explicit operator TaskHandle() const { return Handle; }
	explicit operator bool() const { return Handle && !Handle.done(); }
};