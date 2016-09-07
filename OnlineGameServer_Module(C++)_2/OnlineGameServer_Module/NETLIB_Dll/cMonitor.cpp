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
// CRITICAL_SECTION ��ü�� �������� ȹ���ϴµ�
// �� �Լ��� CRITICAL_SECTION ��ü�� �������� �ٸ� �����尡 �̹� �����ϰ� �ִٸ�
// �ٷ� �����·� �ٲ��� �ʰ� ����ڰ� �������� Ƚ����ŭ �������� ���������� �õ��Ѵ�.
// �̷� ���� ���ɶ�(SpinLock)�̶� �θ��µ� CPU�� 2�� �̻��϶��� �����ϴ�.
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