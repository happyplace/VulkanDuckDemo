#pragma once

#include <chrono>

class GameTimer
{
public:
	GameTimer();
	~GameTimer();

	double TotalTime() const;
	double DeltaTime() const { return mDeltaTime; }
	uint64_t FrameCount() const { return m_frameCount; }

	void Reset();
	void Start();
	void Stop();
	void Tick();

private:
	typedef std::chrono::milliseconds::rep TimePoint;

	const double mMilliToSec;
	bool mStopped;
	double mDeltaTime;
	TimePoint mBaseTime;
	TimePoint mPausedTime;
	TimePoint mStopTime;
	TimePoint mPrevTime;
	TimePoint mCurrTime;
	uint64_t m_frameCount;
};
