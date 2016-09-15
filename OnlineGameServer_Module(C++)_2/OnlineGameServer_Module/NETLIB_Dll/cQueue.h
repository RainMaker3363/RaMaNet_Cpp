#pragma once

template<class TYPE>
class cQueue : public cMonitor
{
private:
	// 실제 데이터를 저장하는 데이터 배열
	TYPE*	m_arrQueue;
	int		m_nQueueMaxSize;

	// 큐의 동기화를 위한 변수
	cMonitor	m_csQueue;

	int		m_nCurSize;
	// 큐의 들어있는 데이터 중 가장 마지막을 가리키고 있는 변수
	int		m_nEndMark;

	// 큐에 들어있는 데이터 중 가장 처음을 가리키고 있는 변수
	int		m_nBeginMark;

public:
	cQueue(int nMaxSize = MAX_QUEUESIZE);
	~cQueue();

	// 큐에 데이터를 넣는다.
	bool PushQueue(TYPE typeQueueItem);

	// 큐에 크기를 하나 감소 시키고
	void PopQueue();

	// 큐가 비어있는지 확인한다.
	bool IsEmptyQueue();

	// 큐에 들어있는 데이터중 가장 처음에 있는 데이터를 가져온다.
	TYPE GetFrontQueue();

	// 큐의 현재 크기를 반환한다.
	int GetQueueSize();

	// 큐의 최대 크기를 반환한다.
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