#ifdef NETLIB_DLL_EXPORTS
#define NETLIB_API _declspec(dllexport)
#else
#define NETLIB_API _declspec(dllimport)
#endif

#include <Windows.h>
#include <process.h>

class NETLIB_API cMonitor
{
public:
	class NETLIB_API Owner
	{
	public:
		Owner(cMonitor &crit);
		~Owner();

	private:
		cMonitor &m_csSyncObject;

		// 포인터를 사용하지 않고 대입을 하여 복사되는것을 막기 위함
		Owner(const Owner &rhs);
		Owner &operator=(const Owner &rhs);
	};

	cMonitor();
	~cMonitor();

#if(_WIN32_WININT >= 0x0400) 
	// CRITICAL_SECTION 객체의 소유권을 획득하는데
	// 이 함수는 CRITICAL_SECTION 객체의 소유권을 다른 쓰레드가 이미 소유하고 있다면
	// 바로 대기상태로 바뀌지 않고 사용자가 설정해준 횟수만큼 소유권을 가져오려고 시도한다.
	// 이런 것을 스핀락(SpinLock)이라 부르는데 CPU가 2개 이상일때만 유용하다.
	BOOL TryEnter();
#endif

	// CRITICAL_SECTION을 락 / 언락 해주는 역할이다.
	void Enter();
	void Leave();

private:

	CRITICAL_SECTION m_csSyncObject;

	// 포인터를 사용하지 않고 대입을 하여 복사되는것을 막기 위함
	cMonitor(const cMonitor &rhs);
	cMonitor &operator=(const cMonitor &rhs);
};
