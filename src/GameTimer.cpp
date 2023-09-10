#include "GameTimer.h"

using namespace std::chrono;

namespace GameTimerUtil
{
    std::chrono::milliseconds::rep GetCurrentTime()
    {
        return duration_cast<milliseconds>(high_resolution_clock::now().time_since_epoch()).count();
    }
}

GameTimer::GameTimer()
    : mMilliToSec(0.001)
    , mStopped(false)
    , mDeltaTime(0.0)
    , mBaseTime(0)
    , mPausedTime(0)
    , mStopTime(0)
    , mPrevTime(0)
    , mCurrTime(0)
    , m_frameCount(0)
{
}

GameTimer::~GameTimer()
{
}

double GameTimer::TotalTime() const
{ 
    if (mStopped)
    {
        return static_cast<double>(((mStopTime - mPausedTime) - mBaseTime) * mMilliToSec);
    }
    else
    {
        return static_cast<double>(((mCurrTime - mPausedTime) - mBaseTime) * mMilliToSec);
    }
}

void GameTimer::Reset()
{
    TimePoint currentTime = GameTimerUtil::GetCurrentTime();

    mBaseTime = currentTime;
    mPrevTime = currentTime;
    mStopTime = 0;
    mStopped = false;
}

void GameTimer::Start()
{
    TimePoint startTime = GameTimerUtil::GetCurrentTime();

    if (mStopped)
    {
        mPausedTime += (startTime - mStopTime);

        mPrevTime = startTime;
        mStopTime = 0;
        mStopped = false;
    }
}

void GameTimer::Stop()
{
    if (!mStopped)
    {
        TimePoint currentTime = GameTimerUtil::GetCurrentTime();

        mStopTime = currentTime;
        mStopped = true;
    }
}

void GameTimer::Tick()
{
    if (mStopped)
    {
        mDeltaTime = 0.0;
        return;
    }

    TimePoint currentTime = GameTimerUtil::GetCurrentTime();
    mCurrTime = currentTime;

    mDeltaTime = (mCurrTime - mPrevTime) * mMilliToSec;

    mPrevTime = mCurrTime;

    if (mDeltaTime < 0.0)
    {
        mDeltaTime = 0.0;
    }

    m_frameCount++;
}
