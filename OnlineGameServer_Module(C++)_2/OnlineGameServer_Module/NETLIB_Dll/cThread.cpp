#include "precompile.h"
#include "cThread.h"
#include "cLog.h"

cThread::cThread()
{
	m_hThread = NULL;
	m_bIsRun = false;
	m_dwWaitTick = 0;
	m_dwTickCount = 0;
	m_hQuietEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
}

cThread::~cThread()
{
	CloseHandle(m_hQuietEvent);

	if (m_hThread)
		CloseHandle(m_hThread);
}

unsigned int WINAPI CallTickThread(LPVOID p)
{
	cThread* pTickThread = (cThread*)p;
	pTickThread->TickThread();

	return 1;
}

bool cThread::CreateThread(DWORD dwWaitTick)
{
	unsigned int uiThreadID = 0;

	m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CallTickThread, this, CREATE_SUSPENDED, &uiThreadID);

	if (m_hThread == NULL)
	{
		return false;
	}

	m_dwWaitTick = dwWaitTick;
	return true;
}

void cThread::Run()
{
	if (m_bIsRun == FALSE)
	{
		m_bIsRun = true;
		ResumeThread(m_hThread);
	}
}

void cThread::Stop()
{
	if (m_bIsRun == TRUE)
	{
		m_bIsRun = false;
		SuspendThread(m_hThread);
	}
}

void cThread::TickThread()
{
	while (true)
	{
		DWORD dwRet = WaitForSingleObject(m_hQuietEvent, m_dwWaitTick);

		if (WAIT_OBJECT_0 == dwRet)
		{
			m_dwTickCount++;
			OnProcess();
		}
	}
}

void cThread::DestroyThread()
{
	Run();
	SetEvent(m_hQuietEvent);

	WaitForSingleObject(m_hThread, INFINITE);
}