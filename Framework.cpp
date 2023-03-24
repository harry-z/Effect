#include "Framework.h"

TaskQueue g_TaskQueue;

bool PopTaskAndRun()
{
	if (HasMoreTasks())
	{
		g_TaskQueue.front()->DoTask();
		g_TaskQueue.pop_front();
		return true;
	}
	else
		return false;
}

bool HasMoreTasks()
{
	return g_TaskQueue.size() > 0;
}

LARGE_INTEGER Timer::s_Frequency;

void Timer::Start()
{
	m_ElapsedTime = 0;
	m_LastTime = 0;
}

void Timer::Tick()
{
	if (m_LastTime == -1)
		return;

	LARGE_INTEGER CurrentTime;
	::QueryPerformanceCounter(&CurrentTime);
	if (m_LastTime == 0)
		m_LastTime = CurrentTime.QuadPart;
	else
	{
		LONGLONG DeltaTime = CurrentTime.QuadPart - m_LastTime;
		m_ElapsedTime += DeltaTime;
		float ElapsedTimeInMicroSeconds = m_ElapsedTime * 1000000.0f / s_Frequency.QuadPart;
		if (ElapsedTimeInMicroSeconds >= m_TimeSpan * 1000000.0f)
		{
			if (m_Func)
				m_Func();
			if (m_TriggerType == ETriggerType::Once)
				m_LastTime = -1;
			else
				Start();
		}
	}
}

void Timer::GlobalInitialize()
{
	::QueryPerformanceFrequency(&s_Frequency);
}

TCHAR g_szModulePath[MAX_PATH];
void CacheModulePath()
{
	GetModuleFileName(GetModuleHandle(NULL), g_szModulePath, MAX_PATH);
	TCHAR* pSlash = _tcsrchr(g_szModulePath, _T('\\'));
	if (pSlash)
		*(pSlash + 1) = 0;
}

LPCTSTR GetExePath()
{
	return g_szModulePath;
}