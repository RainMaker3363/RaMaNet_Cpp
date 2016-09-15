#include "PreCompile.h"

IMPLEMENT_SINGLETON(cLog)

cLog::cLog()
{
	ZeroMemory(m_nLogInfoTypes, MAX_STORAGE_TYPE * sizeof(int));
	ZeroMemory(m_szLogFileName, MAX_FILENAME_LENGTH);
	ZeroMemory(m_szIP, MAX_IP_LENGTH);
	ZeroMemory(m_szDSNNAME, MAX_DSN_NAME);
	ZeroMemory(m_szDSNID, MAX_DSN_ID);
	ZeroMemory(m_szDSNPW, MAX_DSN_PW);

	m_eLogFileType = FILETYPE_NONE;
	m_hWnd = NULL;
	m_hLogFile = NULL;
	m_sockUDP = INVALID_SOCKET;
	m_nMsgBufferidx = 0;
	m_nUDPPort = DEFAULT_UDPPORT;
	m_nTCPPort = DEFAULT_TCPPORT;
	m_nServerType = 0;
	m_dwFileMaxSize = 0;
}

cLog::~cLog()
{

}

void cLog::OnProcess()
{
	size_t nLogCount = m_queueLogMsg.GetQueueSize();

	for (unsigned int i = 0; i < nLogCount; i++)
	{
		sLogMsg* pLogMsg = m_queueLogMsg.GetFrontQueue();

		// �α׸� ��´�.
		LogOutput(pLogMsg->s_eLogInfoType, pLogMsg->s_szOutputString);
		m_queueLogMsg.PopQueue();
	}
}

bool cLog::Init(sLogConfig &LogConfig)
{
	// ���� �ʱ�ȭ���� ������ �Ǿ��־��ٸ� ��� ������ CLOSE ��Ų��.
	CLoseAllLog();

	char strtime[100];

	// �� ������ ����
	CopyMemory(m_nLogInfoTypes, LogConfig.s_nLogInfoTypes, MAX_STORAGE_TYPE * sizeof(int));

	time_t curTime;
	struct tm *locTime;

	curTime = time(NULL);
	locTime = localtime(&curTime);
	strftime(strtime, 100, "%m��%d��%H��%M��", locTime);

	// Log ���丮 ����
	CreateDirectory("./LOG", NULL);

	sprintf(m_szLogFileName, "./Log/%s_%s.log", LogConfig.s_szLogFileName, strtime);
	strncpy(m_szIP, LogConfig.s_szIP, MAX_IP_LENGTH);
	strncpy(m_szDSNNAME, LogConfig.s_szIP, MAX_IP_LENGTH);
	strncpy(m_szDSNID, LogConfig.s_szDSNID, MAX_DSN_ID);
	strncpy(m_szDSNPW, LogConfig.s_szDNSPW, MAX_DSN_PW);

	m_eLogFileType = LogConfig.s_eLogFileType;
	m_nTCPPort = LogConfig.s_nTCPPort;
	m_nUDPPort = LogConfig.s_nUDPPort;
	m_nServerType = LogConfig.s_nServerType;
	m_dwFileMaxSize = LogConfig.s_dwFileMaxSize;

	m_hWnd = LogConfig.s_hWnd;
	bool bRet = false;

	// ���Ϸα׸� �����ߴٸ�
	if (LOG_NONE != m_nLogInfoTypes[STORAGE_FILE])
		bRet = InitFile();

	if (bRet == false)
	{
		goto error;
	}

	// DB �α׸� �����ߴٸ�
	if (LOG_NONE != m_nLogInfoTypes[STORAGE_DB])
		bRet = InitDB();

	if (bRet == false)
	{
		goto error;
	}

	// UDP �α׸� �����ߴٸ�
	if (LOG_NONE != m_nLogInfoTypes[STORAGE_UDP])
		bRet = InitUDP();

	if (bRet == false)
	{
		goto error;
	}

	m_nLogInfoTypes[STORAGE_TCP] = LOG_NONE;

	// TCP �α׸� �����ߴٸ�
	if (LOG_NONE != m_nLogInfoTypes[STORAGE_TCP])
		bRet = InitTCP();

	if (bRet == false)
	{
		goto error;
	}

	CreateThread(LogConfig.s_dwProcessTick);
	Run();

	return true;

//������ ����ٸ�..
error:
	CLoseAllLog();
	return false;
}

void cLog::CLoseAllLog()
{
	ZeroMemory(m_nLogInfoTypes, MAX_STORAGE_TYPE * sizeof(int));
	ZeroMemory(m_szLogFileName, MAX_FILENAME_LENGTH);
	ZeroMemory(m_szIP, MAX_IP_LENGTH);
	ZeroMemory(m_szDSNNAME, MAX_DSN_NAME);
	ZeroMemory(m_szDSNID, MAX_DSN_ID);
	ZeroMemory(m_szDSNPW, MAX_DSN_PW);
	m_nUDPPort = DEFAULT_UDPPORT;
	m_nTCPPort = DEFAULT_TCPPORT;
	m_eLogFileType = FILETYPE_NONE;
	m_hWnd = NULL;
	m_nMsgBufferidx = 0;

	// ���� �α׸� ������.
	if (m_hLogFile)
	{
		CloseHandle(m_hLogFile);
		m_hLogFile = NULL;
	}

	// UDP ������ �ʱ�ȭ ��Ų��.
	if (m_sockUDP != INVALID_SOCKET)
	{
		closesocket(m_sockUDP);
		m_sockUDP = INVALID_SOCKET;
	}

	// TCP ������ �ʱ�ȭ ��Ų��.
	if (m_sockTCP != INVALID_SOCKET)
	{
		shutdown(m_sockTCP, SD_BOTH);
		closesocket(m_sockTCP);
		m_sockTCP = INVALID_SOCKET;
	}

	// ������ ����
	Stop();
}

bool cLog::InitDB()
{
	// ��� ������ ���⼭ �����ϸ� �ȴ�...

	return true;
}

bool cLog::InitFile()
{
	m_hLogFile = CreateFile(m_szLogFileName,
		GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL,
		OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (m_hLogFile == NULL)
		return false;

	return true;
}

bool cLog::InitUDP()
{
	WSADATA WsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &WsaData);

	if (nRet)
		return false;

	return true;
}

bool cLog::InitTCP()
{
	WSADATA WsaData;

	int nRet = WSAStartup(MAKEWORD(2, 2), &WsaData);

	if (nRet)
		return false;
	if (INVALID_SOCKET != m_sockTCP)
		return false;

	sockaddr_in Addr;

	memset((char*)&Addr, 0x00, sizeof(Addr));
	
	Addr.sin_family = AF_INET;
	Addr.sin_addr.S_un.S_addr = inet_addr(m_szIP);
	Addr.sin_port = htons(m_nTCPPort);

	// ���� ��Ŷ����
	m_sockTCP = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	nRet = connect(m_sockTCP, (sockaddr*)&Addr, sizeof(Addr));

	if (nRet == SOCKET_ERROR)
		return false;

	return true;
}

void cLog::LogOutput(enumLogInfoType eLogInfo, char *szOutputString)
{
	// ��� �������ݷ� �ٸ� ���α׷����� �α׸� ��������
	// ���� Ÿ�԰� �޽����� �������Ѵ�.
	if (m_nLogInfoTypes[STORAGE_UDP] & eLogInfo)
	{
		OutputUDP(eLogInfo, szOutputString);
	}

	if (m_nLogInfoTypes[STORAGE_TCP] & eLogInfo)
	{
		OutputTCP(eLogInfo, szOutputString);
	}

	// �α�, �ð� : �������� : ���� ���� : Ŭ���� : �Լ� : ��������
	// ���� �ð��� ���´�.
	char szTime[25];
	time_t curTime;
	struct tm *locTime;

	// LOG ENUM �� StringTable ���� ������ ��ġ ��Ų��.
	int nIdx = (int)eLogInfo;

	if ((eLogInfo >> 8) != 0)
		nIdx = (eLogInfo >> 8) + 0x20 - 3;
	else if ((eLogInfo >> 4) != 0)
		nIdx = (eLogInfo >> 4) + 0x10 - 1;
	if (nIdx < 0 || nIdx > 31)
		return;

	curTime = time(NULL);
	locTime = localtime(&curTime);
	strftime(szTime, 25, "%Y/%m/%d(%H:%M:%S)", locTime);
	sprintf(m_szOutStr, "%s | %s | %s | %s%c%c"
		, szTime, (eLogInfo >> 4) ? "����" : "����"
		, szLogInfoType_StringTable[nIdx]
		, szOutputString, 0x0d, 0x0a);

	if (m_nLogInfoTypes[STORAGE_FILE] & eLogInfo)
	{
		OutputFile(m_szOutStr);
	}
	if (m_nLogInfoTypes[STORAGE_DB] & eLogInfo)
	{
		OutputDB(m_szOutStr);
	}
	if (m_nLogInfoTypes[STORAGE_WINDOW] & eLogInfo)
	{
		OutputWindow(eLogInfo, m_szOutStr);
	}
	if (m_nLogInfoTypes[STORAGE_OUTPUTWND] & eLogInfo)
	{
		OutputDebugWnd(m_szOutStr);
	}
}

void cLog::OutputFile(char* szOutputString)
{
	if (m_hLogFile == NULL)
		return;

	DWORD dwWrittenBytes = 0;
	DWORD dwSize = 0;
	dwSize = GetFileSize(m_hLogFile, NULL);

	// ���� �뷮�� ���ѿ� �ɷȴٸ�
	if (dwSize > m_dwFileMaxSize || dwSize > MAX_LOGFILE_SIZE)
	{
		char strTime[100];
		time_t curTime;
		struct tm *locTime;

		curTime = time(NULL);
		locTime = localtime(&curTime);
		strftime(strTime, 100, "%m��%d��%H��%M��", locTime);
		m_szLogFileName[strlen(m_szLogFileName) - 21] = NULL;
		sprintf(m_szLogFileName, "%s_%s.log", m_szLogFileName, strTime);
		CloseHandle(m_hLogFile);
		m_hLogFile = NULL;
		InitFile();
	}

	// ���Ͽ� ������ ���� �����͸� �ű��.
	SetFilePointer(m_hLogFile, 0, 0, FILE_END);
	
	BOOL bRet = WriteFile(m_hLogFile, szOutputString, (DWORD)strlen(szOutputString), &dwWrittenBytes, NULL);
}

void cLog::OutputDB(char* szOutputString)
{
	// DB�� �α׸� ����� �ڵ带 ���⿡ �ۼ�
}

void cLog::OutputWindow(enumLogInfoType eLogInfo, char* szOutputString)
{
	if (m_hWnd == NULL)
	{
		return;
	}

	SendMessage(m_hWnd, WM_DEBUGMSG, (WPARAM)szOutputString, (LPARAM)eLogInfo);
}

void cLog::OutputDebugWnd(char* szOutputString)
{
	// ����� â�� ������ش�.
	OutputDebugString(szOutputString);
}

void cLog::OutputUDP(enumLogInfoType eLogInfo, char* szOutputString)
{
	// UDP Packet�� ���� �� ����
	sockaddr_in Addr;
	memset((char*)&Addr, 0x00, sizeof(Addr));
	Addr.sin_family = AF_INET;
	Addr.sin_addr.S_un.S_addr = inet_addr(m_szIP);
	Addr.sin_port = htons(m_nUDPPort);

	// ���� ��Ŷ ����
	int nBufLen = (int)strlen(szOutputString);

	if (m_sockUDP == INVALID_SOCKET)
	{
		m_sockUDP = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
	}

	int iResult = sendto(m_sockUDP, (char*)szOutputString, nBufLen, 0, (const struct sockaddr*)&Addr, sizeof(Addr));
}

void cLog::OutputTCP(enumLogInfoType eLogInfo, char* szOutputString)
{
	int nLen = (int)strlen(szOutputString);
	int iResult = send(m_sockTCP, (char*)szOutputString, nLen, 0);
}

void cLog::LogOutputLastErrorToMsgBox(char* szOutputString)
{
	int nLastError = GetLastError();

	if (nLastError == 0)
		return;

	LPVOID pDump;
	DWORD result;

	result = FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		nLastError,
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
		(LPTSTR)&pDump,
		0,
		NULL);

	sprintf(g_szOutStr, "������ġ : %s \n������ȣ : %d\n���� : %s",
		szOutputString, nLastError, pDump);
	MessageBox(NULL, g_szOutStr, "GetLastError", MB_OK);

	if (result)
		LocalFree(pDump);
}

// �ʱ�ȭ �Լ�
bool INIT_LOG(sLogConfig &LogConfig)
{
	return Log()->Init(LogConfig);
}

// �α׸� ����� �Լ�
void LOG(enumLogInfoType eLogInfoType, char *szOutputString, ...)
{
	cMonitor::Owner lock(g_csLog);

	size_t nQueueCnt = Log()->GetQueueSize();

	// ���� ť  size�� �ʰ��ߴٸ�
	if (nQueueCnt == MAX_QUEUE_CNT)
	{
		return;
	}

	va_list argptr;
	va_start(argptr, szOutputString);
	vsprintf(g_stLogMsg[nQueueCnt].s_szOutputString, szOutputString, argptr);
	va_end(argptr);

	g_stLogMsg[nQueueCnt].s_eLogInfoType = eLogInfoType;

	Log()->InsertMsgToQueue(&g_stLogMsg[nQueueCnt]);
}

// ��Ʈ������ �޽��� �ڽ��� ��� �Լ�
void LOG_LASTERROR(char *szOutputString, ...)
{
	va_list argptr;
	va_start(argptr, szOutputString);
	vsprintf(g_szOutStr, szOutputString, argptr);
	va_end(argptr);

	Log()->LogOutputLastErrorToMsgBox(g_szOutStr);
}

// �α׸� ������.
void CLOSE_LOG()
{
	Log()->CLoseAllLog();
}