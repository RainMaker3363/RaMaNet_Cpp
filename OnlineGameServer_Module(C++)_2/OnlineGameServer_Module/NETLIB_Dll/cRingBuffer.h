#pragma once

class NETLIB_API cRingBuffer : public cMonitor
{

private:
	char*	m_pRingBuffer;	// ���� �����͸� �����ϴ� ���� ������
	char*	m_pBeginMark;	// ������ ó���κ��� ����Ű�� �ִ� ������
	char*	m_pEndMark;		// ������ ������ �κ��� ����Ű�� �ִ� ������
	char*	m_pCurrentMark;	// ���۰� ������� ���� �κ��� ����Ű�� �ִ� ������
	char*	m_pGettedBufferMark;	// ������� �����͸� ���� ���� ������
	char*	m_pLastMoveMark;	// recycle�Ǳ� ���� ������ ������

	int		m_nBufferSize;	// ���� ������ �� ũ��
	int		m_nUserBufferSize;	// ���� ���� ���� ������ ũ��
	unsigned int	m_uiAllUserBufSize;	// �� ó���� ������ ��

	cMonitor m_csRingBuffer;		// ����ȭ ��ü

public:
	cRingBuffer();
	~cRingBuffer();

	// �� ���� �޸� �Ҵ�
	bool	Create(int nBufferSize = MAX_RINGBUFSIZE);

	// �ʱ�ȭ
	bool	Initalize();

	// �Ҵ�� ���� ũ�⸦ ��ȯ�Ѵ�.
	inline int	GetBufferSize() { return m_nBufferSize; }

	// �ش��ϴ� ���� ������ �����͸� ��ȯ
	inline char*	GetBeginMark() { return m_pBeginMark; }
	inline char*	GetCurrentMark() { return m_pCurrentMark; }
	inline char*	GetEndMark()	{ return m_pEndMark; }

	// ���� ������ ���� �����͸� ����
	char*	ForwardMark(int nFowardLength);
	char*	ForwardMark(int nForwardLength, int nNextLength, DWORD dwRemainLength);
	
	// ���� ���� ���� ����
	void	ReleaseBuffer(int nReleaseSize);

	// ���� ���� ���� ũ�� ��ȯ
	int		GetUserBufferSize() { return m_nUserBufferSize; }

	//���� ���۾� ����(�̰��� �ϴ� ������ SendPost()�Լ��� ��Ƽ �����忡�� ���ư��⶧����
	//PrepareSendPacket()����(ForwardMark()����) ���� ���� �÷������� PrepareSendPacket�Ѵ����� �����͸�
	//ä�� �ֱ����� �ٷ� �ٸ� �����忡�� SendPost()�� �Ҹ��ٸ� ������ ������ �����Ͱ� �� �� �ִ�.
	//�װ� �����ϱ� ���� �����͸� �� ä�� ���¿����� ���� ���� ����� ������ �� �־���Ѵ�.
	//���Լ��� sendpost�Լ����� �Ҹ��� �ȴ�.
	void			SetUsedBufferSize(int nUsedBufferSize);

	// ���� ���� ��뷮 ��ȯ
	int		GetAllUserBufferSize() { return m_uiAllUserBufSize; }

	// ���� ���� �����͸� �о ��ȯ
	char*	GetBuffer(int nReadSize, int* pReadSize);



private:
	cRingBuffer(const cRingBuffer &rhs);
	cRingBuffer &operator=(const cRingBuffer &rhs);
};