#include "PreCompile.h"

unsigned int WINAPI CallWorkerThread(LPVOID p);
unsigned int WINAPI CallProcessThread(LPVOID p);

cIocpServer* cIocpServer::m_pIocpServer = NULL;

unsigned int WINAPI CallWorkerThread(LPVOID p)
{
	cIocpServer* pServerSock = (cIocpServer*)p;
	pServerSock->WorkerThread();

	return 1;
}

unsigned int WINAPI CallProcessThread(LPVOID p)
{
	cIocpServer* pServerSock = (cIocpServer*)p;
	pServerSock->ProcessThread();

	return 1;
}

cIocpServer::cIocpServer()
{
	m_bWorkThreadFlag = true;
	m_bProcessThreadFlag = true;
	m_dwProcessThreadCount = 0;
	m_dwWorkerThreadCount = 0;
	m_lpProcessPacket = NULL;
}

cIocpServer::~cIocpServer()
{
	if (m_lpProcessPacket)
		delete[] m_lpProcessPacket;

	WSACleanup();
}

bool cIocpServer::ServerStart(INITCONFIG &initConfig)
{
	m_usPort = initConfig.nServerPort;
	m_dwWorkerThreadCount = initConfig.nWorkerThreadCnt;
	m_dwProcessThreadCount = initConfig.nProcessThreadCnt;

	if (InitializeSocket() == false)
		return false;
	if (!CreateWorkerIOCP())
		return false;
	if (!CreateProcessIOCP())
		return false;
	if (!CreateWorkerThreads())
		return false;
	if (!CreateProcessThreads())
		return false;
	if (!CreateListenSock())
		return false;

	initConfig.sockListner = GetListenSocket();

	if (m_lpProcessPacket)
	{
		delete[] m_lpProcessPacket;
	}

	m_lpProcessPacket = new PROCESSPACKET[initConfig.nProcessPacketCnt];
	m_dwProcessPacketCnt = initConfig.nProcessPacketCnt;

	return true;

}

bool cIocpServer::ServerOff()
{
	LOG(LOG_INFO_LOW, "SYSTEM | cIocpServer::ServerOff() | 서버 종료를 시작합니다.");

	// 모든 쓰레드를 멈축.. 완전히 종료 하기위해 뒷 처리를 해준다.
	if (m_hWorkerIOCP)
	{
		m_bWorkThreadFlag = false;

		for (DWORD i = 0; i < m_dwWorkerThreadCount; i++)
		{
			// WorkerThread에 종료 메세지를 보낸다.
			PostQueuedCompletionStatus(m_hWorkerIOCP, 0, 0, NULL);
		}
		CloseHandle(m_hWorkerIOCP);
		m_hWorkerIOCP = NULL;
	}

	if (m_hProcessIOCP)
	{
		m_bProcessThreadFlag = false;

		for (DWORD i = 0; i < m_dwProcessThreadCount; i++)
		{
			// ProcessThread에 종료 메세지를 보낸다.
			PostQueuedCompletionStatus(m_hProcessIOCP, 0, 0, NULL);
		}
		CloseHandle(m_hProcessIOCP);
		m_hProcessIOCP = NULL;
	}

	// 핸들을 닫는다.
	for (unsigned int i = 0; i < m_dwWorkerThreadCount; i++)
	{
		if (m_hWorkerThread[i] != INVALID_HANDLE_VALUE)
			CloseHandle(m_hWorkerThread[i]);
		m_hWorkerThread[i] = INVALID_HANDLE_VALUE;
	}

	// 핸들을 닫는다.
	for (unsigned int i = 0; i < m_dwProcessThreadCount; i++)
	{
		if (m_hProcessThread[i] != INVALID_HANDLE_VALUE)
			CloseHandle(m_hProcessThread[i]);
		m_hProcessThread[i] = INVALID_HANDLE_VALUE;
	}

	if (m_ListenSock != INVALID_SOCKET)
	{
		closesocket(m_ListenSock);
		m_ListenSock = INVALID_SOCKET;
	}

	LOG(LOG_INFO_LOW,
		"SYSTEM | cIocpServer::ServerOff() | 서버가 완전히 종료 되었습니다.");

	return true;
}

bool cIocpServer::InitializeSocket()
{
	for (DWORD i = 0; i < m_dwWorkerThreadCount; i++)
	{
		m_hWorkerThread[i] = INVALID_HANDLE_VALUE;
	}

	m_hWorkerIOCP = INVALID_HANDLE_VALUE;
	m_hProcessIOCP = INVALID_HANDLE_VALUE;
	m_ListenSock = INVALID_SOCKET;

	WSADATA WsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &WsaData);

	if (nRet)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cIocpServer::InitializeSocket() | WSAStartup() Failed..");
		return false;
	}

	return true;
}

// CPU의 개수를 파악하여 적당한 WorkerThread의 개수를 얻어온다 (CPU*2 + 1)
void cIocpServer::GetProperThreadsCount()
{
	SYSTEM_INFO SystemInfo;
	DWORD		ProperCount = 0;
	DWORD		DispatcherCount = 0;

	GetSystemInfo(&SystemInfo);

	ProperCount = SystemInfo.dwNumberOfProcessors * 2 + 1;

	if (ProperCount > MAX_WORKER_THREAD)
	{
		ProperCount = (DWORD)MAX_WORKER_THREAD;
	}

	m_dwWorkerThreadCount = ProperCount;
}

bool cIocpServer::CreateListenSock()
{
	sockaddr_in si_addr;
	int			nRet;
	int			nZero = 0;

	// Create listen socket
	m_ListenSock = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (m_ListenSock == INVALID_SOCKET)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cIocpServer::CreateListenSock() | Socket Creation Failed : (%u)",
			GetLastError());

		return false;
	}

	// bind listen socket with si_addr struct
	si_addr.sin_family = AF_INET;
	si_addr.sin_port = htons(m_usPort);
	si_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);

	nRet = bind(m_ListenSock, (struct sockaddr*)&si_addr, sizeof(si_addr));

	if (nRet == SOCKET_ERROR)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cIocpServer::CreateListenSock() | bind() Failed : (%u)",
			GetLastError());
		return false;
	}

	// 리스닝 시작..
	nRet = listen(m_ListenSock, 50);

	if (nRet == SOCKET_ERROR)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cIocpServer::CreateListenSock() | listen() Failed : (%u)",
			GetLastError());
		return false;
	}

	HANDLE hIOCPHandle;
	hIOCPHandle = CreateIoCompletionPort((HANDLE)m_ListenSock, m_hWorkerIOCP, (DWORD)0, 0);

	if (hIOCPHandle == NULL || m_hWorkerIOCP != hIOCPHandle)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cIocpServer::CreateListenSock() | CreateIoCompletionPort() Failed : (%u)",
			GetLastError());
		return false;
	}
	return true;
}

bool cIocpServer::CreateProcessThreads()
{
	HANDLE hThread;
	UINT   uiThreadId;

	// 워커 스레드를 생성한다.
	for (DWORD dwCount = 0; dwCount < m_dwProcessThreadCount; dwCount++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, &CallProcessThread, this, CREATE_SUSPENDED, &uiThreadId);

		if (hThread == NULL)
		{
			LOG(LOG_ERROR_LOW,
				"SYSTEM | cIocpServer::CreateProcessThreads() | _beginthreadex() Failed : (%u)",
				GetLastError());

			return false;
		}

		m_hProcessThread[dwCount] = hThread;

		ResumeThread(hThread);

		SetThreadPriority(hThread, THREAD_PRIORITY_TIME_CRITICAL);
	}

	return true;
}

bool cIocpServer::CreateWorkerThreads()
{
	HANDLE hThread;
	UINT   uiThreadId;

	// 프로세스 스레드를 생성한다.
	for (DWORD dwCount = 0; m_dwWorkerThreadCount; dwCount++)
	{
		hThread = (HANDLE)_beginthreadex(NULL, 0, &CallWorkerThread, this, CREATE_SUSPENDED, &uiThreadId);

		if (hThread == NULL)
		{
			LOG(LOG_ERROR_LOW,
				"SYSTEM | cIocpServer::CreateWorkerThreads() | _beginthreadex() Failed : (%u)",
				GetLastError());

			return false;
		}

		m_hWorkerThread[dwCount] = hThread;

		ResumeThread(hThread);
	}
}

bool cIocpServer::CreateWorkerIOCP()
{
	m_hWorkerIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

	if (m_hWorkerIOCP == NULL)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cIocpServer::CreateWorkerIOCP() | CreateIoCompletionPort() Failed : (%u)",
			GetLastError());

		return false;
	}
	return true;
}

bool cIocpServer::CreateProcessIOCP()
{
	m_hProcessIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);

	if (m_hProcessIOCP)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cIocpServer::CreateProcessIOCP() | CreateIoCompletionPort() Failed : (%u)",
			GetLastError());
		return false;
	}
	return true;
}

void cIocpServer::WorkerThread()
{
	BOOL			bSuccess = false;
	LPOVERLAPPED	lpOverlapped = NULL;
	cConnection*	lpConnection = NULL;
	LPOVERLAPPED_EX	lpOverlappedEx = NULL;
	DWORD			dwIoSize = 0;

	while (m_bWorkThreadFlag)
	{
		dwIoSize = 0;
		lpOverlapped = NULL;

		bSuccess = GetQueuedCompletionStatus(m_hWorkerIOCP,
			&dwIoSize,
			(LPDWORD)&lpConnection,
			&lpOverlapped,
			INFINITE);

		// Client가 접속을 끊었을때...
		if (!bSuccess)
		{
			if (lpOverlapped == NULL && lpConnection == NULL)
			{
				LOG(LOG_ERROR_LOW,
					"SYSTEM | cIocpServer::WorkerThread() | GetQueuedCompletionStatus() Failed : (%u)",
					GetLastError());
				continue;
			}

			LPOVERLAPPED_EX lpOverlappedEx = (LPOVERLAPPED_EX)lpOverlapped;
			lpConnection = (cConnection*)lpOverlappedEx->s_lpConnection;

			if (lpConnection == NULL)
			{
				continue;
			}

			// Overlapped I/O 요청 되어있던 작업의 카운트를 줄인다.
			if (lpOverlappedEx->s_eOperation == OP_ACCEPT)
				lpConnection->DecrementAcceptIoRefCount();
			else if (lpOverlappedEx->s_eOperation == OP_RECV)
				lpConnection->DecrementRecvIoRefCount();
			else if (lpOverlappedEx->s_eOperation == OP_SEND)
				lpConnection->DecrementSendIoRefCount();

			CloseConnection(lpConnection);

			continue;
		}

		LPOVERLAPPED_EX lpOverlappedEx = (LPOVERLAPPED_EX)lpOverlapped;

		if (NULL == lpOverlappedEx)
			continue;

		switch (lpOverlappedEx->s_eOperation)
		{
			case OP_ACCEPT:
			{
				DoAccept(lpOverlappedEx);
			}
			break;

			case OP_RECV:
			{
				DoRecv(lpOverlappedEx, dwIoSize);
			}
			break;

			case OP_SEND:
			{
				DoSend(lpOverlappedEx, dwIoSize);
			}
			break;
		}
	}
}

void cIocpServer::DoAccept(LPOVERLAPPED_EX lpOverlappedEx)
{
	SOCKADDR *lpLocalSocAddr = NULL, *lpRemoteSockAddr = NULL;

	int			nLocalSockAddrLen = 0, nRemoteSocaddrLen = 0;

	cConnection* lpConnection = (cConnection*)lpOverlappedEx->s_lpConnection;

	if (lpConnection == NULL)
	{
		return;
	}

	lpConnection->DecrementAcceptIoRefCount();

	// Remote Address를 알아낸다.
	// GetAcceptExSockaddrs는 AcceptEx와 짝을 이루는 함수로
	// AcceptEx 호출로 리턴된 버퍼에서 로컬, 리모트 어드레스를 얻어 온다.
	GetAcceptExSockaddrs(lpConnection->m_szAddressBuf, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, &lpLocalSocAddr, &nLocalSockAddrLen, &lpRemoteSockAddr, &nRemoteSocaddrLen);

	if (nRemoteSocaddrLen != 0)
	{
		lpConnection->SetConnectionIp(inet_ntoa(((SOCKADDR_IN*)lpRemoteSockAddr)->sin_addr));
	}
	else
	{
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | cIocpServer::WorkerThread() | GetAcceptExSockaddrs() Failed : (%u)",
			GetLastError());

		CloseConnection(lpConnection);

		return;
	}

	//bind Completion key & connection context..
	if (lpConnection->BindIOCP(m_hWorkerIOCP) == false)
	{
		CloseConnection(lpConnection);
		return;
	}

	lpConnection->m_bIsConnect = true;

	if (lpConnection->RecvPost(lpConnection->m_ringRecvBuffer.GetBeginMark(), 0) == false)
	{
		CloseConnection(lpConnection);
		return;
	}

	OnAccept(lpConnection);
}

void cIocpServer::DoRecv(LPOVERLAPPED_EX lpOverlappedEx, DWORD dwIoSize)
{
	cConnection* lpConnection = (cConnection*)lpOverlappedEx->s_lpConnection;

	if (lpConnection == NULL)
		return;

	lpConnection->DecrementRecvIoRefCount();

	int nMsgSize = 0, nRemain = 0;
	char *pCurrent = NULL, *pNext = NULL;

	nRemain = lpOverlappedEx->s_dwRemain;
	lpOverlappedEx->s_WsaBuf.buf = lpOverlappedEx->s_lpSocketMsg;
	lpOverlappedEx->s_dwRemain += dwIoSize;

	if (lpOverlappedEx->s_dwRemain >= PACKET_SIZE_LENGTH)
	{
		CopyMemory(&nMsgSize, &(lpOverlappedEx->s_WsaBuf.buf[0]), PACKET_SIZE_LENGTH);
	}
	else
	{
		nMsgSize = 0;
	}

	// 잘못된 패킷이 도착했을때..
	if (nMsgSize <= 0 || nMsgSize > lpConnection->m_ringRecvBuffer.GetBufferSize())
	{
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | cIocpServer::WorkerThread() | arrived wrong packet : (%u)",
			GetLastError());

		CloseConnection(lpConnection);
		return;
	}

	lpOverlappedEx->s_TotalBytes = nMsgSize;

	// 메세지가 전부 도착하지 않았을때
	if ((lpOverlappedEx->s_dwRemain) < ((DWORD)nMsgSize))
	{
		nRemain = lpOverlappedEx->s_dwRemain;
		pNext = lpOverlappedEx->s_WsaBuf.buf;
	}
	else // 하나 이상의 패킷이 데이터를 모두 받았다면
	{
		pCurrent = &(lpOverlappedEx->s_WsaBuf.buf[0]);
		int dwCurrentSize = nMsgSize;

		nRemain = lpOverlappedEx->s_dwRemain;

		if (ProcessPacket(lpConnection, pCurrent, dwCurrentSize) == false)
		{
			return;
		}

		nRemain -= dwCurrentSize;
		pNext = pCurrent + dwCurrentSize;

		while (true)
		{
			if (nRemain >= PACKET_SIZE_LENGTH)
			{
				CopyMemory(&nMsgSize, pNext, PACKET_SIZE_LENGTH);
				dwCurrentSize = nMsgSize;

				// 잘못된 패킷이 도착했을때..
				if (nMsgSize <= 0 || nMsgSize > lpConnection->m_ringRecvBuffer.GetBufferSize())
				{
					LOG(LOG_ERROR_NORMAL,
						"SYSTEM | cIocpServer::WorkerThread() | arrived wrong packet : (%u)",
						GetLastError());

					CloseConnection(lpConnection);

					return;
				}

				lpOverlappedEx->s_TotalBytes = dwCurrentSize;

				if (nRemain >= dwCurrentSize)
				{
					if (ProcessPacket(lpConnection, pNext, dwCurrentSize) == false)
					{
						return;
					}

					nRemain -= dwCurrentSize;
					pNext += dwCurrentSize;
				}
				else
					break;
			}
			else
				break;
		}
	}

	lpConnection->RecvPost(pNext, nRemain);
}

void cIocpServer::DoSend(LPOVERLAPPED_EX lpOverlappedEx, DWORD dwIoSize)
{
	cConnection* lpConnection = (cConnection*)lpOverlappedEx->s_lpConnection;

	if (lpConnection == NULL)
	{
		return;
	}

	lpConnection->DecrementSendIoRefCount();

	lpOverlappedEx->s_dwRemain += dwIoSize;

	// 만약 모든 메시지를 다 보내지 못했다면.
	if ((DWORD)lpOverlappedEx->s_TotalBytes > lpOverlappedEx->s_dwRemain)
	{
		DWORD dwFlag = 0;
		DWORD dwSendNumBytes = 0;

		lpOverlappedEx->s_WsaBuf.buf += dwIoSize;
		lpOverlappedEx->s_WsaBuf.len -= dwIoSize;

		memset(&lpOverlappedEx->s_Overlapped, 0, sizeof(OVERLAPPED));

		lpConnection->IncrementSendIoRefCount();

		int nRet = WSASend(lpConnection->GetSocket(),
			&(lpOverlappedEx->s_WsaBuf),
			1,
			&dwSendNumBytes,
			dwFlag,
			&(lpOverlappedEx->s_Overlapped),
			NULL);

		if (nRet == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			lpConnection->DecrementSendIoRefCount();

			LOG(LOG_ERROR_NORMAL,
				"SYSTEM | cIocpServer::WorkerThread() | WSASend Failed : (%u)",
				GetLastError());

			CloseConnection(lpConnection);

			return;
		}
	}
	else  // 모든 메세지를 다 보냈다면..
	{
		lpConnection->m_ringSendBuffer.ReleaseBuffer(lpOverlappedEx->s_TotalBytes);
		InterlockedExchange((LPLONG)&lpConnection->m_bIsSend, true);
		lpConnection->SendPost(0);
	}
}

LPPROCESSPACKET cIocpServer::GetProcessPacket(eOperationType operationType, LPARAM lPARAM, WPARAM wParam)
{
	DWORD dwProcessPacketCnt = InterlockedDecrement((LPLONG)&m_dwProcessPacketCnt);

	if ((int)-1 == (int)dwProcessPacketCnt)
	{
		InterlockedIncrement((LPLONG)&m_dwProcessPacketCnt);

		LOG(LOG_ERROR_HIGH,
			"SYSTEM | cIocpServer::GetProcessPacket() | ProcessPacket Empty..");
		return NULL;
	}

	LPPROCESSPACKET lpProcessPacket = &m_lpProcessPacket[m_dwProcessPacketCnt];

	lpProcessPacket->s_eOperationType = operationType;
	lpProcessPacket->s_lPARAM = lPARAM;
	lpProcessPacket->s_wParam = wParam;

	return lpProcessPacket;
}

void cIocpServer::ClearProcessPacket(LPPROCESSPACKET lpProcessPacket)
{
	lpProcessPacket->Init();
	InterlockedIncrement((LPLONG)&m_dwProcessPacketCnt);
}

bool cIocpServer::ProcessPacket(cConnection* lpConnection, char* pCurrent, DWORD dwCurrentSize)
{
	int nUserBufSize = lpConnection->m_ringRecvBuffer.GetUserBufferSize();

	// 만약 단일로 처리 하지 않아도 되는것이면 곧바로 처리한다.
	if (!OnRecvImmediately(lpConnection, dwCurrentSize, pCurrent))
	{
		LPPROCESSPACKET lpProcessPacket = GetProcessPacket(OP_RECVPACKET, (LPARAM)pCurrent, NULL);

		if (lpProcessPacket == NULL)
		{
			return false;
		}

		if (PostQueuedCompletionStatus(m_hProcessIOCP, dwCurrentSize, (ULONG_PTR)lpConnection, (LPOVERLAPPED)lpProcessPacket) == 0)
		{

			ClearProcessPacket(lpProcessPacket);

			LOG(LOG_ERROR_NORMAL,
				"SYSTEM | cIocpServer::ProcessPacket() | PostQueuedCompletionStatus Failed : [%d], socket[%d]"
				, GetLastError(), lpConnection->GetSocket());
		}
	}
	else   // 처리된 패킷은 링 버퍼에서 할당된 버퍼를 해제한다.
	{
		lpConnection->m_ringRecvBuffer.ReleaseBuffer(dwCurrentSize);
	}

	return true;
}

bool cIocpServer::CloseConnection(cConnection* lpConnection)
{
	// 래퍼런스 카운트가 남아있다면 소켓을 끊고 Iocp에서 Completion될때까지 기다려야한다.
	if (lpConnection->GetAcceptIoRefCount != 0 ||
		lpConnection->GetRecvIoRefCount != 0 ||
		lpConnection->GetSendIorefCount != 0)
	{
		// 소켓 초기화
		shutdown(lpConnection->GetSocket(), SD_BOTH);
		closesocket(lpConnection->GetSocket());
		lpConnection->SetSocket(INVALID_SOCKET);

		return true;
	}

	if (InterlockedCompareExchange((LPLONG)&lpConnection->m_bIsClosed, TRUE, FALSE) == FALSE)
	{
		LPPROCESSPACKET lpProcessPacket = GetProcessPacket(OP_CLOSE, NULL, NULL);

		if (lpProcessPacket == NULL)
		{
			return false;
		}

		if (PostQueuedCompletionStatus(m_hProcessIOCP, 0, (ULONG_PTR)lpConnection, (LPOVERLAPPED)lpProcessPacket))
		{
			ClearProcessPacket(lpProcessPacket);

			LOG(LOG_ERROR_NORMAL,
				"SYSTEM | cIocpServer::CloseConnection() | PostQueuedCompletionStatus Failed : [%d], socket[%d]"
				, GetLastError(), lpConnection->GetSocket());
			lpConnection->CloseConnection(true);

		}
	}

	return true;
}

void cIocpServer::ProcessThread()
{
	BOOL				bSuccess = FALSE;
	int					nRet = 0;
	LPPROCESSPACKET		lpProcessPacket = NULL;
	LPOVERLAPPED		lpOverlapped = NULL;
	cConnection*		lpConnection = NULL;
	LPOVERLAPPED_EX		lpOverlappedEx = NULL;
	DWORD				dwIoSize = 0;

	while (m_bProcessThreadFlag)
	{
		dwIoSize = 0;

		bSuccess = GetQueuedCompletionStatus(m_hProcessIOCP,
			&dwIoSize,
			(LPDWORD)&lpConnection,
			(LPOVERLAPPED*)&lpProcessPacket,
			INFINITE);

		// 쓰레드 종료
		if (bSuccess == TRUE && lpConnection == NULL)
		{
			break;
		}

		switch (lpProcessPacket->s_eOperationType)
		{
			case OP_CLOSE:
			{
				lpConnection->CloseConnection(true);
			}
			case OP_RECVPACKET:
			{
				if (lpProcessPacket->s_lPARAM == NULL)
				{
					continue;
				}

				OnRecv(lpConnection, dwIoSize, (char*)lpProcessPacket->s_lPARAM);
				lpConnection->m_ringRecvBuffer.ReleaseBuffer(dwIoSize);

				break;
			}
			case OP_SYSTEM:
			{
				OnSystemMsg(lpConnection, (DWORD)lpProcessPacket->s_lPARAM, lpProcessPacket->s_wParam);
				break;
			}

		}
		ClearProcessPacket(lpProcessPacket);
	}
}