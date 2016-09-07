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

		// �����͸� ������� �ʰ� ������ �Ͽ� ����Ǵ°��� ���� ����
		Owner(const Owner &rhs);
		Owner &operator=(const Owner &rhs);
	};

	cMonitor();
	~cMonitor();

#if(_WIN32_WININT >= 0x0400) 
	// CRITICAL_SECTION ��ü�� �������� ȹ���ϴµ�
	// �� �Լ��� CRITICAL_SECTION ��ü�� �������� �ٸ� �����尡 �̹� �����ϰ� �ִٸ�
	// �ٷ� �����·� �ٲ��� �ʰ� ����ڰ� �������� Ƚ����ŭ �������� ���������� �õ��Ѵ�.
	// �̷� ���� ���ɶ�(SpinLock)�̶� �θ��µ� CPU�� 2�� �̻��϶��� �����ϴ�.
	BOOL TryEnter();
#endif

	// CRITICAL_SECTION�� �� / ��� ���ִ� �����̴�.
	void Enter();
	void Leave();

private:

	CRITICAL_SECTION m_csSyncObject;

	// �����͸� ������� �ʰ� ������ �Ͽ� ����Ǵ°��� ���� ����
	cMonitor(const cMonitor &rhs);
	cMonitor &operator=(const cMonitor &rhs);
};
