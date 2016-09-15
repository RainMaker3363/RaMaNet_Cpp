#include "PreCompile.h"
#include "cRingBuffer.h"

cRingBuffer::cRingBuffer()
{
	m_pRingBuffer = NULL;
	m_pBeginMark = NULL;
	m_pEndMark = NULL;
	
	m_pCurrentMark = NULL;
	m_pGettedBufferMark = NULL;
	m_pLastMoveMark = NULL;

	m_nUserBufferSize = 0;
	m_nBufferSize = 0;
}

cRingBuffer::~cRingBuffer()
{
	m_nUserBufferSize = 0;

	m_pCurrentMark = m_pBeginMark;
	m_pGettedBufferMark = m_pBeginMark;
	m_pLastMoveMark = m_pBeginMark;

	m_uiAllUserBufSize = 0;
}

bool cRingBuffer::Initalize()
{
	cMonitor::Owner lock(m_csRingBuffer);
	{
		m_nUserBufferSize = 0;
		m_pCurrentMark = m_pBeginMark;
		m_pGettedBufferMark = m_pBeginMark;
		m_pEndMark = m_pBeginMark;

		m_uiAllUserBufSize = 0;
	}

	return true;
}

bool cRingBuffer::Create(int nBufferSize)
{
	if (m_pBeginMark != NULL)
		delete[] m_pBeginMark;

	m_pBeginMark = new char[nBufferSize];

	if (m_pBeginMark == NULL)
		return false;

	m_pEndMark = m_pBeginMark + nBufferSize - 1;
	m_nBufferSize = nBufferSize;

	Initalize();

	return true;
}

char* cRingBuffer::ForwardMark(int nForwardLength)
{
	char* pPreCurrentMark = NULL;

	cMonitor::Owner lock(m_csRingBuffer);
	{
		// 링 버퍼 오버플로 체크
		if (m_nUserBufferSize + nForwardLength > m_nBufferSize)
		{
			return NULL;
		}

		if (m_pEndMark - m_pCurrentMark > nForwardLength)
		{
			pPreCurrentMark = m_pCurrentMark;
			m_pCurrentMark += nForwardLength;
		}
		else
		{
			// 순환되기 전 마지막 좌표를 저장
			m_pLastMoveMark = m_pCurrentMark;
			m_pCurrentMark = m_pBeginMark + nForwardLength;
			pPreCurrentMark = m_pBeginMark;
		}

		m_nUserBufferSize += nForwardLength;
		m_uiAllUserBufSize += nForwardLength;
	}

	return pPreCurrentMark;
}

char* cRingBuffer::ForwardMark(int nForwardLength, int nNextLength, DWORD dwRemainLength)
{
	cMonitor::Owner lock(m_csRingBuffer);
	{
		// 링 버퍼 오버플로 체크
		if ((m_nUserBufferSize + nForwardLength + nNextLength) > m_nBufferSize)
		{
			return NULL;
		}

		if ((m_pEndMark - m_pCurrentMark) > (nNextLength + nForwardLength))
		{
			m_pCurrentMark += nForwardLength;
		}
		else
		{
			// 순환되기 전 마지막 좌표를 저장
			m_pLastMoveMark = m_pCurrentMark;
			CopyMemory(m_pBeginMark,
				m_pCurrentMark - (dwRemainLength - nForwardLength),
				dwRemainLength);

			m_pCurrentMark = m_pBeginMark + dwRemainLength;
		}
		m_nUserBufferSize += nForwardLength;
		m_uiAllUserBufSize += nForwardLength;
	}

	return m_pCurrentMark;
}

void cRingBuffer::ReleaseBuffer(int nRleaseSize)
{
	cMonitor::Owner lock(m_csRingBuffer);
	{
		m_nUserBufferSize -= nRleaseSize;
	}

}

//사용된 버퍼양 설정(이것을 하는 이유는 SendPost()함수가 멀티 쓰레드에서 돌아가기때문에
//PrepareSendPacket()에서(ForwardMark()에서) 사용된 양을 늘려버리면 PrepareSendPacket한다음에 데이터를
//채워 넣기전에 바로 다른 쓰레드에서 SendPost()가 불린다면 엉뚱한 쓰레기 데이터가 갈 수 있다.
//그걸 방지하기 위해 데이터를 다 채운 상태에서만 사용된 버퍼 사이즈를 설정할 수 있어야한다.
//이함수는 sendpost함수에서 불리게 된다.
void cRingBuffer::SetUsedBufferSize(int nUsedBufferSize)
{
	cMonitor::Owner lock(m_csRingBuffer);
	{
		m_nUserBufferSize += nUsedBufferSize;
		m_uiAllUserBufSize += nUsedBufferSize;
	}
}

char* cRingBuffer::GetBuffer(int nReadSize, int* pReadSize)
{
	char* pRet = NULL;

	cMonitor::Owner lock(m_csRingBuffer);
	{
		// 마지막까지 다 읽었다면 그 읽어드릴 버퍼의 포인터는 맨 앞으로 옮긴다.
		if (m_pLastMoveMark == m_pBeginMark)
		{
			m_pGettedBufferMark = m_pBeginMark;
			m_pLastMoveMark = m_pEndMark;
		}

		// 현재 버퍼에 있는 size가 읽어드릴 size보다 크다면
		if (m_nUserBufferSize > nReadSize)
		{
			// 링 버퍼의 끝인지 판단.
			if ((m_pLastMoveMark - m_pGettedBufferMark) >= nReadSize)
			{
				*pReadSize = nReadSize;
				pRet = m_pGettedBufferMark;
				m_pGettedBufferMark += nReadSize;
			}
			else
			{
				*pReadSize = (int)(m_pLastMoveMark - m_pGettedBufferMark);
				pRet = m_pGettedBufferMark;
				m_pGettedBufferMark += *pReadSize;
			}
		}
		else if (m_nUserBufferSize > 0)
		{
			// 링 버퍼의 끝인지 판단
			if ((m_pLastMoveMark - m_pGettedBufferMark) >= nReadSize)
			{
				*pReadSize = m_nUserBufferSize;
				pRet = m_pGettedBufferMark;
				m_pGettedBufferMark += m_nUserBufferSize;
			}
			else
			{
				*pReadSize = (int)(m_pLastMoveMark - m_pGettedBufferMark);
				pRet = m_pGettedBufferMark;
				m_pGettedBufferMark += *pReadSize;
			}
		}

	}

	return pRet;
}