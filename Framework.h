#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>
// C RunTime Header Files
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <assert.h>
#include <iostream>
#include <string>
#include <vector>
#include <deque>
#include <memory>
#include <functional>

#ifdef EFFECT_EXPORT_SYMBOL
	#define EFFECT_API __declspec(dllexport)
#else
	#define EFFECT_API __declspec(dllimport)
#endif

constexpr float PI = 3.1415926535897932f;

#if defined(UNICODE) || defined(_UNICODE)
	using String = std::wstring;
#else
	using String = std::string;
#endif

class ITask
{
public:
	virtual ~ITask() {}
	virtual void DoTask() = 0;
};

using TaskQueue = std::deque<std::shared_ptr<ITask>>;
extern TaskQueue g_TaskQueue;

template <class TaskType, class ... Args>
void AddTask(Args && ... args) {
	g_TaskQueue.emplace_back(std::make_shared<TaskType>(std::forward<Args>(args)...));
}
bool PopTaskAndRun();
bool HasMoreTasks();

class Timer
{
public:
	enum class ETriggerType {
		Once,
		Repeat
	};
	Timer(float TimeSpanInSeconds, ETriggerType TriggerType, std::function<void()>&& Func) 
	: m_TimeSpan(TimeSpanInSeconds) 
	, m_TriggerType(TriggerType) 
	, m_Func(Func) { m_ElapsedTime = 0; m_LastTime = -1; }
	void Start();
	void Tick();

	static void GlobalInitialize();

private:
	LONGLONG m_ElapsedTime, m_LastTime;
	float m_TimeSpan;
	ETriggerType m_TriggerType;
	std::function<void()> m_Func;

	static LARGE_INTEGER s_Frequency;
};

EFFECT_API HWND GetHWnd();
EFFECT_API void CacheModulePath();
EFFECT_API LPCTSTR GetExePath();

EFFECT_API int EffectMain(int argc, TCHAR** argv);