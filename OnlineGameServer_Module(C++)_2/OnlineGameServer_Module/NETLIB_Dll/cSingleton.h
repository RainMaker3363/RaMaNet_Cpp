#pragma once

//// �� ��ũ�δ� ��ӹ޴� Ŭ���� �ȿ��� ����Ǿ�� �ϸ� ���ڷ� ��ӹ޴� Ŭ������ �̸��� �޴´�.
//#define DECLEAR_SINGLETON(className) public: static className *Instance(); virtual void releaseInstance(); private: static className *m_pInstance;
//
//// �� ��ũ�δ� ��ӹ޴� Ŭ���� �ۿ��� ����Ǿ�� �ϸ� ���ڷδ� ��ӹ��� Ŭ���� �̸���
//// ����ü Ŭ������ �����͸� ���� �Լ��� �̸��� ���ڷ� �޴´�.
//#define CREATE_FUNCTION(className, funcName) static className *##funcName(){return className::Instance();}
//
//// �� ��ũ�δ� ��� �޴� Ŭ������ ���� �κп��� ����Ǿ�� �ϸ� ���ڷδ� ��ӹ޴� Ŭ������ �̸��� �޴´�.
//// DECLEARE_SINGLETON()���� ������ �Լ����� ���� �κ��� ��ġ�ϴ� ��ũ���̴�.
//#define IMPLEMENT_SINGLETON(className) className* className::m_pInstance = NULL; className* className::Instance() {if(NULL == m_pInstance){m_pInstance = new className;} return m_pInstance;} void className::ReleaseInstance(){if(NULL != m_pInstance){delete m_pInstance; m_pInstance = NULL;}}

// �� ��ũ�δ� ��ӹ޴� Ŭ���� �ȿ��� ����Ǿ�� �ϸ� ���ڷ� ��ӹ޴� Ŭ������ �̸��� �޴´�.
#define DECLEAR_SINGLETON( className )\
public:\
	static className* Instance();\
	virtual void ReleaseInstance();\
private:\
	static className* m_pInstance;

// �� ��ũ�δ� ��ӹ޴� Ŭ���� �ۿ��� ����Ǿ�� �ϸ� ���ڷδ� ��ӹ��� Ŭ���� �̸���
// ����ü Ŭ������ �����͸� ���� �Լ��� �̸��� ���ڷ� �޴´�.
#define CREATE_FUNCTION( className , funcName ) \
	static className* ##funcName()\
		{\
	return className::Instance();\
		};

// �� ��ũ�δ� ��� �޴� Ŭ������ ���� �κп��� ����Ǿ�� �ϸ� ���ڷδ� ��ӹ޴� Ŭ������ �̸��� �޴´�.
// DECLEARE_SINGLETON()���� ������ �Լ����� ���� �κ��� ��ġ�ϴ� ��ũ���̴�.
#define IMPLEMENT_SINGLETON( className )\
	className* className::m_pInstance = NULL;\
	className* className::Instance()\
		{\
		if ( NULL == m_pInstance )\
				{\
		m_pInstance = new className;\
				}\
	return m_pInstance;\
		}\
	void className::ReleaseInstance()\
		{\
		if ( NULL != m_pInstance )\
				{\
		delete m_pInstance;\
		m_pInstance = NULL;\
				}\
		}

class NETLIB_API cSingleton
{
public:
	typedef std::list<cSingleton *> SINGLETON_LIST;

	cSingleton();
	virtual ~cSingleton();

public:

	virtual void ReleaseInstance() = 0;
	static void ReleaseAll();

private:
	static SINGLETON_LIST m_listSingleton;
};