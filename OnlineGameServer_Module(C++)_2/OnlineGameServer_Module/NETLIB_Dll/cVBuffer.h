#pragma once

class NETLIB_API cVBuffer : public cSingleton
{
	DECLEAR_SINGLETON(cVBuffer);

private:
	char* m_pszVBuffer;	// 실제 버퍼
	char* m_pCurMark;	// 현재 버퍼 위치
	int m_nMaxBufSize;	// 최대 버퍼 사이즈

	int m_nCurBufSize;	// 현재 사용된 버퍼 사이즈

	cVBuffer(const cVBuffer &rhs);
	cVBuffer &operator=(const cVBuffer &rhs);

public:
	cVBuffer(int nMaxBufSize = 1024 * 50);
	virtual ~cVBuffer();

	void GetChar(char& cCh);
	void GetShort(short& sNum);
	void GetInterger(int& nNum);
	void GetString(char* pszBuffer);
	void GetStream(char* pszBuffer, short sLen);
	
	void SetInterger(int nI);
	void SetShort(short sShort);
	void SetChar(char cCh);
	void SetString(char* pszBuffer);
	void SetSteream(char* pszBuffer, short sLen);

	void SetBuffer(char* pVBuffer);

	inline int GetMaxBufSize() { return m_nMaxBufSize; }
	inline int GetCurBufSize() { return m_nCurBufSize; }
	inline char* GetCurMark() { return m_pCurMark; }
	inline char* GetBeginMark() { return m_pszVBuffer; }

	bool CopyBuffer(char* pDestBuffer);
	void Init();
	



};

CREATE_FUNCTION(cVBuffer, VBuffer)