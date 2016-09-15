#pragma once

template<class TYPE>
class cQueue : public cMonitor
{
private:
	// ���� �����͸� �����ϴ� ������ �迭
	TYPE*	m_arrQueue;
	int		m_nQueueMaxSize;

	// ť�� ����ȭ�� ���� ����
	cMonitor	m_csQueue;

	int		m_nCurSize;
	// ť�� ����ִ� ������ �� ���� �������� ����Ű�� �ִ� ����
	int		m_nEndMark;

	// ť�� ����ִ� ������ �� ���� ó���� ����Ű�� �ִ� ����
	int		m_nBeginMark;

public:
	cQueue(int nMaxSize = MAX_QUEUESIZE);
	~cQueue();

	// ť�� �����͸� �ִ´�.
	bool PushQueue(TYPE typeQueueItem);

	// ť�� ũ�⸦ �ϳ� ���� ��Ű��
	void PopQueue();

	// ť�� ����ִ��� Ȯ���Ѵ�.
	bool IsEmptyQueue();

	// ť�� ����ִ� �������� ���� ó���� �ִ� �����͸� �����´�.
	TYPE GetFrontQueue();

	// ť�� ���� ũ�⸦ ��ȯ�Ѵ�.
	int GetQueueSize();

	// ť�� �ִ� ũ�⸦ ��ȯ�Ѵ�.
	int GetQueueMaxSize() { return m_nQueueMaxSize; }

	void SetQueueMaxSize(int nMaxSize) { m_nQueueMaxSize = nMaxSize; }
	void ClearQueue();


};

template<class TYPE>
cQueue<TYPE>::cQueue<TYPE>(int nMaxSize)
{
	m_arrQueue = new TYPE[nMaxSize];
	m_nQueueMaxSize = nMaxSize;

	ClearQueue();
}

template<class TYPE>
bool cQueue<TYPE>::PushQueue(TYPE typeQueueItem)
{
	cMonitor::Owner lock(m_csQueue);
	{
		if (m_nCurSize >= m_nQueueMaxSize)
		{
			return false;
		}

		m_nCurSize++;

		if (m_nEndMark == m_nQueueMaxSize)
		{
			m_nEndMark = 0;
		}

		m_arrQueue[m_nEndMark++] = typeQueueItem;
	}

	return true;
}

template<class TYPE>
TYPE cQueue<TYPE>::GetFrontQueue()
{
	TYPE typeQueueItem;

	cMonitor::Owner lock(m_csQueue);
	{
		if (m_nCurSize <= 0)
		{
			return NULL;
		}

		if (m_nBeginMark == m_nQueueMaxSize)
		{
			m_nBeginMark = 0;
		}

		typeQueueItem = m_arrQueue[m_nBeginMark];
	}

	return typeQueueItem;
}

template<class TYPE>
void cQueue<TYPE>::PopQueue()
{
	cMonitor::Owner lock(m_csQueue);
	{
		m_nCurSize--;
		m_nBeginMark++;
	}

}

template<class TYPE>
bool cQueue<TYPE>::IsEmptyQueue()
{
	bool bFlag = false;

	cMonitor::Owner lock(m_csQueue);
	{
		bFlag = (m_nCurSize > 0) ? false : true;
	}

	return bFlag;
}

template<class TYPE>
int cQueue<TYPE>::GetQueueSize()
{
	int nSize;

	cMonitor::Owner lock(m_csQueue);
	{
		nSize = m_nCurSize;
	}

	return nSize;
}

template<class TYPE>
void cQueue<TYPE>::ClearQueue()
{
	cMonitor::Owner lock(m_csQueue);
	{
		m_nCurSize = 0;
		m_nEndMark = 0;
		m_nBeginMark = 0;
	}
}