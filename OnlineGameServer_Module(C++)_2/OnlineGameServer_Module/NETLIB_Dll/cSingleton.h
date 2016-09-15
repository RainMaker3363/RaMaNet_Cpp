#pragma once

//// 이 매크로는 상속받는 클래스 안에서 선언되어야 하며 인자로 상속받는 클래스의 이름을 받는다.
//#define DECLEAR_SINGLETON(className) public: static className *Instance(); virtual void releaseInstance(); private: static className *m_pInstance;
//
//// 이 매크로는 상속받는 클래스 밖에서 선언되어야 하며 인자로는 상속받은 클래스 이름과
//// 단일체 클래스의 포인터를 얻어올 함수의 이름을 인자로 받는다.
//#define CREATE_FUNCTION(className, funcName) static className *##funcName(){return className::Instance();}
//
//// 이 매크로는 상속 받는 클래스의 구현 부분에서 선언되어야 하며 인자로는 상속받는 클래스의 이름을 받는다.
//// DECLEARE_SINGLETON()으로 선언한 함수들의 구현 부분을 대치하는 매크로이다.
//#define IMPLEMENT_SINGLETON(className) className* className::m_pInstance = NULL; className* className::Instance() {if(NULL == m_pInstance){m_pInstance = new className;} return m_pInstance;} void className::ReleaseInstance(){if(NULL != m_pInstance){delete m_pInstance; m_pInstance = NULL;}}

// 이 매크로는 상속받는 클래스 안에서 선언되어야 하며 인자로 상속받는 클래스의 이름을 받는다.
#define DECLEAR_SINGLETON( className )\
public:\
	static className* Instance();\
	virtual void ReleaseInstance();\
private:\
	static className* m_pInstance;

// 이 매크로는 상속받는 클래스 밖에서 선언되어야 하며 인자로는 상속받은 클래스 이름과
// 단일체 클래스의 포인터를 얻어올 함수의 이름을 인자로 받는다.
#define CREATE_FUNCTION( className , funcName ) \
	static className* ##funcName()\
		{\
	return className::Instance();\
		};

// 이 매크로는 상속 받는 클래스의 구현 부분에서 선언되어야 하며 인자로는 상속받는 클래스의 이름을 받는다.
// DECLEARE_SINGLETON()으로 선언한 함수들의 구현 부분을 대치하는 매크로이다.
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