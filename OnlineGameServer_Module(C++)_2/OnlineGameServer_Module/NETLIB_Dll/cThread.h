#pragma once

#ifdef NETLIB_DLL_EXPORTS
#define NETLIB_API _declspec(dllexport)
#else
#define NETLIB_API _declspec(dllimport)
#endif

#include <Windows.h>
#include <process.h>

class NETLIB_API cThread
{
public:
	cThread();
	~cThread();

	bool CreateThread(DWORD dwWaitTick);
	void DestroyThread();
	void Run();
	void Stop();
	void TickThread();
	virtual void OnProcess() = 0;

	inline DWORD GetTickCount() { return m_dwTickCount; }
	bool IsRun() { return m_bIsRun; }

protected:
	HANDLE	m_hThread;
	HANDLE  m_hQuietEvent;
	bool    m_bIsRun;
	DWORD	m_dwWaitTick;
	DWORD	m_dwTickCount;
};