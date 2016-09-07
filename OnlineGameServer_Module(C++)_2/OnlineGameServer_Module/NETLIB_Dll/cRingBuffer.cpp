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
		// �� ���� �����÷� üũ
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
			// ��ȯ�Ǳ� �� ������ ��ǥ�� ����
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
		// �� ���� �����÷� üũ
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
			// ��ȯ�Ǳ� �� ������ ��ǥ�� ����
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

char* cRingBuffer::GetBuffer(int nReadSize, int* pReadSize)
{
	char* pRet = NULL;

	cMonitor::Owner lock(m_csRingBuffer);
	{
		// ���������� �� �о��ٸ� �� �о�帱 ������ �����ʹ� �� ������ �ű��.
		if (m_pLastMoveMark == m_pBeginMark)
		{
			m_pGettedBufferMark = m_pBeginMark;
			m_pLastMoveMark = m_pEndMark;
		}

		// ���� ���ۿ� �ִ� size�� �о�帱 size���� ũ�ٸ�
		if (m_nUserBufferSize > nReadSize)
		{
			// �� ������ ������ �Ǵ�.
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
			// �� ������ ������ �Ǵ�
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