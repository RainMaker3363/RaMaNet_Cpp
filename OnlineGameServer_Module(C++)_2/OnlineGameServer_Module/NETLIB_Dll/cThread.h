#pragma once

class NETLIB_API cThread
{
protected:
	HANDLE	m_hThread;
	HANDLE  m_hQuietEvent;
	bool    m_bIsRun;
	DWORD	m_dwWaitTick;
	DWORD	m_dwTickCount;

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


};