#pragma once

class NETLIB_API cIocpServer : public cMonitor
{
protected:

	SOCKET	m_ListenSock;

	HANDLE	m_hWorkerIOCP;
	HANDLE	m_hProcessIOCP;

	HANDLE	m_hWorkerThread[MAX_WORKER_THREAD];
	HANDLE	m_hProcessThread[MAX_PROCESS_THREAD];

	unsigned short m_usPort;
	char			m_szIP[MAX_IP_LENGTH];

	DWORD			m_dwTimeTick;
	DWORD			m_dwWorkerThreadCount;
	DWORD			m_dwProcessThreadCount;

	bool			m_bWorkThreadFlag;
	bool			m_bProcessThreadFlag;

	LPPROCESSPACKET m_lpProcessPacket;
	DWORD			m_dwProcessPacketCnt;

public:
	cIocpServer();
	~cIocpServer();

	/////////////////////////////////////////////////////////////////////////////////////
	// �ʱ�ȭ �Լ�
	/////////////////////////////////////////////////////////////////////////////////////
	bool	InitializeSocket();

	/////////////////////////////////////////////////////////////////////////////////////
	// ���� �Լ�
	// Network I/O�� ó���ϴ� Thread
	void	WorkerThread();
	// ������ �ִ� ��Ŷ�� ó���ϴ� Thread
	void	ProcessThread();
	// Client�� ���� ���� ó��
	bool	CloseConnection(cConnection* lpConnection);

	bool	ProcessPacket(cConnection* lpConnection, char* pCurrent, DWORD dwCurrentSize);

	virtual bool	ServerStart(INITCONFIG &initConfig);
	virtual bool	ServerOff();
	
	SOCKET			GetListenSocket()
	{
		return m_ListenSock;
	}

	unsigned short GetServerProt()
	{
		return m_usPort;
	}

	char* GetServerIP()
	{
		return m_szIP;
	}

	inline HANDLE GetWorkerIOCP()
	{
		return m_hWorkerIOCP;
	}

	void DoAccept(LPOVERLAPPED_EX lpOverlappedEx);
	void DoRecv(LPOVERLAPPED_EX lpOverlappedEx, DWORD dwIoSize);
	void DoSend(LPOVERLAPPED_EX lpOverlappedEx, DWORD dwIoSize);

	LPPROCESSPACKET GetProcessPacket(eOperationType operationType, LPARAM lParam, WPARAM wParam);
	void ClearProcessPacket(LPPROCESSPACKET lpProcessPacket);
	/////////////////////////////////////////////////////////////////////////////////////


	/////////////////////////////////////////////////////////////////////////////////////
	// ���� ���� �Լ�
	// Client�� ���ӵɶ� ȣ��Ǵ� �Լ�
	virtual bool OnAccept(cConnection* lpConnection) = 0;

	// Clien���� Packet�� �����Ͽ� ���� �� �ִ� ��Ŷ�� ó���� �� ȣ��Ǵ� �Լ�
	virtual bool OnRecv(cConnection* lpConnection, DWORD dwSize, char* pRecvedMsg) = 0;

	// Client���� packet�� �����Ͽ� ���� �� ���� ��Ŷ�� ó���� �� ȣ��Ǵ� �Լ�
	virtual bool OnRecvImmediately(cConnection* lpConnection, DWORD dwSize, char* pRecvedMsg) = 0;

	// Client ���� ����� ȣ��Ǵ� �Լ�
	virtual bool OnClose(cConnection* lpConnection) = 0;

	// �������� ProcessThread�� �ƴ� �ٸ� �����忡�� �߻���Ų
	// �޽����� ���� ���ְ� ó���Ǿ� �Ѵٸ� �� �Լ��� ���.
	virtual bool OnSystemMsg(cConnection* lpConnection, DWORD dwMsgType, LPARAM lParam) = 0;

	/////////////////////////////////////////////////////////////////////////////////////
	static cIocpServer* GetIocpServer()
	{
		return cIocpServer::m_pIocpServer;
	}
	static cIocpServer* m_pIocpServer;
private:

	bool	CreateProcessThreads();
	bool	CreateWorkerThreads();
	void	GetProperThreadsCount();
	bool	CreateWorkerIOCP();
	bool	CreateProcessIOCP();
	bool	CreateListenSock();

	// ���� ������ ������ ����
	cIocpServer(const cIocpServer &rhs);
	cIocpServer &operator=(const cIocpServer &rhs);
};

inline cIocpServer* IocpServer()
{
	return cIocpServer::GetIocpServer();
}