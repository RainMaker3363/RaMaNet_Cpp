#include "CMonitor.h"

cMonitor::cMonitor()
{
	InitializeCriticalSection(&m_csSyncObject);
}

cMonitor::~cMonitor()
{
	DeleteCriticalSection(&m_csSyncObject);
}

#if(_WIN32_WININT >= 0x0400) 
// CRITICAL_SECTION 객체의 소유권을 획득하는데
// 이 함수는 CRITICAL_SECTION 객체의 소유권을 다른 쓰레드가 이미 소유하고 있다면
// 바로 대기상태로 바뀌지 않고 사용자가 설정해준 횟수만큼 소유권을 가져오려고 시도한다.
// 이런 것을 스핀락(SpinLock)이라 부르는데 CPU가 2개 이상일때만 유용하다.
BOOL cMonitor::TryEnter()
{
	return TryEnterCriticalSection(&m_csSyncObject);
}
#endif

void cMonitor::Enter()
{
	::EnterCriticalSection(&m_csSyncObject);
}

void cMonitor::Leave()
{
	::LeaveCriticalSection(&m_csSyncObject);
}

cMonitor::Owner::Owner(cMonitor &crit) : m_csSyncObject(crit)
{
	m_csSyncObject.Enter();
}

cMonitor::Owner::~Owner()
{
	m_csSyncObject.Leave();
}