#pragma once

class NETLIB_API cRingBuffer : public cMonitor
{

private:
	char*	m_pRingBuffer;	// 실제 데이터를 저장하는 버퍼 포인터
	char*	m_pBeginMark;	// 버퍼의 처음부분을 가리키고 있는 포인터
	char*	m_pEndMark;		// 버퍼의 마지막 부분을 가리키고 있는 포인터
	char*	m_pCurrentMark;	// 버퍼가 현재까지 사용된 부분을 가리키고 있는 포인터
	char*	m_pGettedBufferMark;	// 현재까지 데이터를 읽은 버퍼 포인터
	char*	m_pLastMoveMark;	// recycle되기 전에 마지막 포인터

	int		m_nBufferSize;	// 내부 버퍼의 총 크기
	int		m_nUserBufferSize;	// 현재 사용된 내부 버퍼의 크기
	unsigned int	m_uiAllUserBufSize;	// 총 처리된 데이터 양

	cMonitor m_csRingBuffer;		// 동기화 객체

public:
	cRingBuffer();
	~cRingBuffer();

	// 링 버퍼 메모리 할당
	bool	Create(int nBufferSize = MAX_RINGBUFSIZE);

	// 초기화
	bool	Initalize();

	// 할당된 버퍼 크기를 반환한다.
	inline int	GetBufferSize() { return m_nBufferSize; }

	// 해당하는 내부 버퍼의 포인터를 반환
	inline char*	GetBeginMark() { return m_pBeginMark; }
	inline char*	GetCurrentMark() { return m_pCurrentMark; }
	inline char*	GetEndMark()	{ return m_pEndMark; }

	// 내부 버퍼의 현재 포인터를 전진
	char*	ForwardMark(int nFowardLength);
	char*	ForwardMark(int nForwardLength, int nNextLength, DWORD dwRemainLength);
	
	// 사용된 내부 버퍼 해제
	void	ReleaseBuffer(int nReleaseSize);

	// 사용된 내부 버퍼 크기 반환
	int		GetUserBufferSize() { return m_nUserBufferSize; }

	//사용된 버퍼양 설정(이것을 하는 이유는 SendPost()함수가 멀티 쓰레드에서 돌아가기때문에
	//PrepareSendPacket()에서(ForwardMark()에서) 사용된 양을 늘려버리면 PrepareSendPacket한다음에 데이터를
	//채워 넣기전에 바로 다른 쓰레드에서 SendPost()가 불린다면 엉뚱한 쓰레기 데이터가 갈 수 있다.
	//그걸 방지하기 위해 데이터를 다 채운 상태에서만 사용된 버퍼 사이즈를 설정할 수 있어야한다.
	//이함수는 sendpost함수에서 불리게 된다.
	void			SetUsedBufferSize(int nUsedBufferSize);

	// 누벅 버퍼 사용량 반환
	int		GetAllUserBufferSize() { return m_uiAllUserBufSize; }

	// 내부 버퍼 데이터를 읽어서 반환
	char*	GetBuffer(int nReadSize, int* pReadSize);



private:
	cRingBuffer(const cRingBuffer &rhs);
	cRingBuffer &operator=(const cRingBuffer &rhs);
};