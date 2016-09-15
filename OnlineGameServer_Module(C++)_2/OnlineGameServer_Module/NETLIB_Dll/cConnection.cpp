#include "PreCompile.h"

cConnection::cConnection()
{
	m_sockListener = INVALID_SOCKET;
	m_socket = INVALID_SOCKET;

	m_lpRecvOverlappedEx = NULL;
	m_lpSendOverlappedEx = NULL;

	m_nSendBufSize = 0;
	m_nRecvBufSize = 0;

	InitializeConnection();
}

void cConnection::InitializeConnection()
{
	ZeroMemory(m_szIp, MAX_IP_LENGTH);

	m_socket = INVALID_SOCKET;
	m_bIsConnect = FALSE;
	m_bIsClosed = FALSE;
	m_bIsSend = TRUE;

	m_dwSendIoRefCount = 0;
	m_dwRecvIoRefCount = 0;
	m_dwAcceptIoRefCount = 0;

	m_ringRecvBuffer.Initalize();
	m_ringSendBuffer.Initalize();
}

cConnection::~cConnection()
{
	m_sockListener = INVALID_SOCKET;
	m_socket = INVALID_SOCKET;
}

bool cConnection::CreateConnection(INITCONFIG &initConfig)
{
	m_nIndex = initConfig.nIndex;
	m_sockListener = initConfig.sockListner;

	m_lpRecvOverlappedEx = new OVERLAPPED_EX(this);
	m_lpSendOverlappedEx = new OVERLAPPED_EX(this);
	m_ringRecvBuffer.Create(initConfig.nRecvBufSize * initConfig.nRecvBufCnt);
	m_ringSendBuffer.Create(initConfig.nSendBufSize * initConfig.nSendBufCnt);

	m_nRecvBufSize = initConfig.nRecvBufSize;
	m_nSendBufSize = initConfig.nSendBufSize;

	return BindAceeptExSock();
}

bool cConnection::ConnectTo(char* szIp, unsigned short usPort)
{
	SOCKADDR_IN si_addr;
	int			nRet;
	int			nZero = 0;

	// 리슨(Listen) 소켓 생성
	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (m_socket == INVALID_SOCKET)
	{
		LOG(LOG_ERROR_LOW, "SYSTEM | cConnection::ConnectTo() | WSASocket() , Socket Creation Failed : LastError(%u)", GetLastError());
		return false;
	}

	// 소켓을 si_addr 구조체로 bind 한다.
	si_addr.sin_family = AF_INET;
	si_addr.sin_port = htons(usPort);
	si_addr.sin_addr.s_addr = inet_addr(szIp);

	nRet = WSAConnect(m_socket, (sockaddr*)&si_addr, sizeof(sockaddr), NULL, NULL, NULL, NULL);

	if (SOCKET_ERROR == nRet)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cConnection::ConnectTo() | WSAConnect() , WSAConnect Failed : LastError(%u)",
			GetLastError());

		return false;
	}

	HANDLE	hIOCP = IocpServer()->GetWorkerIOCP();

	if (BindIOCP(hIOCP) == false)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cConnection::ConnectTo() | BindIOCP() , BindIOCP Failed : LastError(%u)",
			GetLastError());

		return false;
	}

	m_bIsConnect = TRUE;

	if (RecvPost(m_ringRecvBuffer.GetBeginMark(), 0) == false)
	{
		LOG(LOG_ERROR_LOW,
			"SYSTEM | cConnection::ConnectTo() | RecvPost() , BindRecv Failed : LastError(%u)",
			GetLastError());

		return false;
	}

	return true;
}

bool cConnection::BindAceeptExSock()
{
	// 리슨 소켓이 없다면 acceptex에 bind하지 않는다.
	if (m_sockListener == 0)
		return true;

	DWORD dwBytes = 0;
	memset(&m_lpRecvOverlappedEx->s_eOperation, 0, sizeof(OVERLAPPED));
	m_lpRecvOverlappedEx->s_WsaBuf.buf = m_szAddressBuf;
	m_lpRecvOverlappedEx->s_lpSocketMsg = &m_lpRecvOverlappedEx->s_WsaBuf.buf[0];
	m_lpRecvOverlappedEx->s_WsaBuf.len = m_nRecvBufSize;
	m_lpRecvOverlappedEx->s_eOperation = OP_ACCEPT;
	m_lpRecvOverlappedEx->s_lpConnection = this;

	m_socket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

	if (m_socket == INVALID_SOCKET)
	{
		LOG(LOG_ERROR_NORMAL, "SYSTEM | cConnection::BindAcceptExSock() | WSASocket() Failed: error[%u]", GetLastError());
		return false;
	}

	IncrementAcceptIoRefCount();

	// AcceptEx는 기존 Accept함수와는 다르게
	// 비동기 처리를 해준다.
	// 접속을 수락할때 미리 소켓을 만들어 놓기에 속도가 빠르다
	// 접속 수락 패킷과 함께 데이터를 전달 받을 수 있기에 효율성이 좋다.
	// DisConnectEx()함수나 TransmitFile() 함수와 함께 사용하여 소켓을 재사용할 수 있기에 좋다.
	BOOL bRet = AcceptEx(m_sockListener, m_socket, m_lpRecvOverlappedEx->s_WsaBuf.buf,
		0,
		sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16,
		&dwBytes,
		(LPOVERLAPPED)m_lpRecvOverlappedEx
		);

	if (!bRet && WSAGetLastError() != WSA_IO_PENDING)
	{
		DecrementAcceptIoRefCount();

		LOG(LOG_ERROR_NORMAL, "SYSTEM | cConnection::BindAcceptExSock() | AcceptEx() Failed: error[%u]", GetLastError());
		return false;
	}

	return true;
}

bool cConnection::CloseConnection(bool bForce)
{
	cMonitor::Owner lock(m_csConnection);

	{
		// Defaullt = SO_DONTLINGER
		struct linger li = { 0, 0 };

		if (bForce)
			li.l_onoff = 1;	// SO_LINGER, timeout = 0
		if (IocpServer() != NULL && m_bIsConnect == TRUE)
		{
			IocpServer()->OnClose(this);
		}

		shutdown(m_socket, SD_BOTH);

		setsockopt(m_socket, SOL_SOCKET, SO_LINGER, (char*)&li, sizeof(li));
		
		closesocket(m_socket);

		m_socket = INVALID_SOCKET;

		if (m_lpRecvOverlappedEx != NULL)
		{
			m_lpRecvOverlappedEx->s_dwRemain = 0;
			m_lpRecvOverlappedEx->s_TotalBytes = 0;
		}

		if (m_lpSendOverlappedEx != NULL)
		{
			m_lpSendOverlappedEx->s_dwRemain = 0;
			m_lpSendOverlappedEx->s_TotalBytes = 0;
		}

		// Connection을 다시 초기화 시켜준다.
		InitializeConnection();
		BindAceeptExSock();
	}

	return true;
}

char* cConnection::PrepareSendPacket(int slen)
{
	if (m_bIsConnect == false)
	{
		return NULL;
	}

	char* pBuf = m_ringSendBuffer.ForwardMark(slen);

	if (pBuf = NULL)
	{
		IocpServer()->CloseConnection(this);

		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | cConnection::PrepareSendPacket() | Socket[%d] SendRingBuffer overflow",
			m_socket);
		return NULL;
	}

	ZeroMemory(pBuf, slen);
	CopyMemory(pBuf, &slen, PACKET_SIZE_LENGTH);

	return pBuf;
}

bool cConnection::ReleaseSendPacket(LPOVERLAPPED_EX IpSendOverlappedEx)
{
	if (IpSendOverlappedEx == NULL)
	{
		return false;
	}

	m_ringSendBuffer.ReleaseBuffer(m_lpSendOverlappedEx->s_WsaBuf.len);
	IpSendOverlappedEx = NULL;

	return true;
}

bool cConnection::BindIOCP(HANDLE& hIOCP)
{
	HANDLE hIOCPHandle;
	cMonitor::Owner lock(m_csConnection);

	hIOCPHandle = CreateIoCompletionPort((HANDLE)m_socket,
		hIOCP, reinterpret_cast<DWORD>(this), 0);

	if (hIOCPHandle != hIOCP || hIOCPHandle == NULL)
	{
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | cConnection::BindIOCP() | CreateIoCompletionPort() Failed : %d",
			GetLastError());

		return false;
	}

	m_hIOCP = hIOCP;
	return true;
}

bool cConnection::RecvPost(char* pNext, DWORD dwRemain)
{
	int		nRet = 0;
	DWORD	dwFlag = 0;
	DWORD	dwRecvNumBytes = 0;

	if (m_lpRecvOverlappedEx == NULL || m_bIsConnect == false)
	{
		return false;
	}

	m_lpRecvOverlappedEx->s_eOperation = OP_RECV;
	m_lpRecvOverlappedEx->s_dwRemain = dwRemain;

	int nMoveMark = dwRemain - (m_ringRecvBuffer.GetCurrentMark() - pNext);

	m_lpRecvOverlappedEx->s_WsaBuf.len = m_nRecvBufSize;
	m_lpRecvOverlappedEx->s_WsaBuf.buf = m_ringRecvBuffer.ForwardMark(nMoveMark, m_nRecvBufSize, dwRemain);

	if (m_lpRecvOverlappedEx->s_WsaBuf.buf == NULL)
	{
		IocpServer()->CloseConnection(this);
		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | cConnection::RecvPost() | Socket[%d] RecvRingBuffer overflow..",
			m_socket);
		return false;
	}

	m_lpRecvOverlappedEx->s_lpSocketMsg = m_lpRecvOverlappedEx->s_WsaBuf.buf - dwRemain;

	memset(&m_lpRecvOverlappedEx->s_Overlapped, 0, sizeof(OVERLAPPED));
	IncrementRecvIoRefCount();

	int ret = WSARecv(m_socket, &m_lpRecvOverlappedEx->s_WsaBuf, 1, &dwRecvNumBytes, &dwFlag, &m_lpRecvOverlappedEx->s_Overlapped, NULL);

	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		DecrementRecvIoRefCount();
		IocpServer()->CloseConnection(this);

		LOG(LOG_ERROR_NORMAL,
			"SYSTEM | cConnection::RecvPost() | WSARecv() Failed : %u",
			GetLastError());

		return false;
	}
	return true;
}

bool cConnection::SendPost(int nSendSize)
{
	DWORD dwBytes;

	// 보내는 양이 있다면, 즉 IocpServer Class의 DoSend()에서 불린게 아니라
	// PrepareSendPacket() 함수를 부르고
	// SendPost가 불렸다면 보내는 양이 있고 DoSend에서 불렸다면 0이 온다.

	if (nSendSize > 0)
	{
		m_ringSendBuffer.SetUsedBufferSize(nSendSize);
	}

	if (InterlockedCompareExchange((LPLONG)&m_bIsSend, FALSE, TRUE) == TRUE)
	{
		int nReadSize;
		char* pBuf = m_ringSendBuffer.GetBuffer(m_nSendBufSize, &nReadSize);

		if (pBuf == NULL)
		{
			InterlockedExchange((LPLONG)&m_bIsSend, TRUE);
			return false;
		}
		m_lpSendOverlappedEx->s_dwRemain = 0;
		m_lpSendOverlappedEx->s_eOperation = OP_SEND;
		m_lpSendOverlappedEx->s_TotalBytes = nReadSize;

		ZeroMemory(&m_lpSendOverlappedEx->s_Overlapped, sizeof(OVERLAPPED));

		m_lpSendOverlappedEx->s_WsaBuf.len = nReadSize;
		m_lpSendOverlappedEx->s_WsaBuf.buf = pBuf;
		m_lpSendOverlappedEx->s_lpConnection = this;

		IncrementSendIoRefCount();

		int ret = WSASend(m_socket, &m_lpSendOverlappedEx->s_WsaBuf, 1, &dwBytes, 0, &m_lpSendOverlappedEx->s_Overlapped, NULL);

		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			DecrementSendIoRefCount();
			IocpServer()->CloseConnection(this);

			LOG(LOG_ERROR_NORMAL,
				"[ERROR] socket[%u] WSASend(): SOCKET_ERROR, %u\n",
				m_socket, WSAGetLastError());

			InterlockedExchange((LPLONG)&m_bIsSend, FALSE);

			return false;
		}
	}
	return true;
}