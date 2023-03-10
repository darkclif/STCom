#pragma once
#include <chrono>

namespace stc::tools {
	/*
	*	Simple timer.
	*/
	struct Timer 
	{
	public:
		Timer() 
		{
			bStarted = false;
		}

		Timer(float Seconds)
		{
			Start(Seconds);
		}

		void Start(float Seconds)
		{
			bStarted = true;

			Finish = std::chrono::high_resolution_clock::now();
			Finish += std::chrono::milliseconds(static_cast<uint32_t>(Seconds * 1000.f));
		}

		bool Done() 
		{
			return bStarted && std::chrono::high_resolution_clock::now() >= Finish;
		}

	private:
		std::chrono::steady_clock::time_point Finish;
		bool bStarted;
	};

	/*
	*	Throttle execution time with system sleep.
	*	
	*	Reset	- set start time to Now().
	*	Wait	- wait remaining amount of time if not enough time passed since the Reset().
	*/
	struct Throttle
	{
	public:
		Throttle() 
		{
			Reset();
		}

		void Reset() 
		{
			bCompleted = false;
			Start = std::chrono::high_resolution_clock::now();
		}

		void Wait(float Seconds)
		{
			if (bCompleted)
			{
				return;
			}

			auto Now = std::chrono::high_resolution_clock::now();
			auto Delta = std::chrono::milliseconds(static_cast<uint32_t>(Seconds * 1000.f));

			if (Start + Delta > Now)
			{
				std::chrono::duration<float> SleepTime = (Start + Delta) - Now;
				std::this_thread::sleep_for(SleepTime);
			}

			bCompleted = true;
		}

	private:
		std::chrono::high_resolution_clock::time_point Start;
		bool bCompleted;
	};
}
