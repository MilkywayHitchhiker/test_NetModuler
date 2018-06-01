#pragma once
#include<Windows.h>
#define dfBuffSize 4086
class CRingbuffer
{
protected:

	SRWLOCK cs;

	char *pBuffer;

	//���� �� ������
	int BufferSize;

	//�б���ġ
	int Front;

	//������ġ
	int Rear;
	

public : 
	
	//������
	CRingbuffer (void);
	CRingbuffer (int iBufferSize);

	//�ı���
	~CRingbuffer (void);

	//ũ��Ƽ�� ���� ��
	void Lock (void);
	//ũ��Ƽ�� ���� �� ����
	void Free (void);

	//�ʱ�ȭ
	void Initial (int iBufferSize);
	
	//���� ���� ũ��(��� ���ϴ� 1byte����) ��ȯ.
	int GetBufferSize (void);


	//���� ������� ���� ũ�� ��ȯ
	int GetUseSize (void);

	
	//���� ��밡���� ���� ũ�� ��ȯ
	int GetFreeSize (void);

	//���� �����ͷ� �ܺο��� �ѹ濡 ������ �ִ� ����
	int GetNotBrokenGetSize (void);


	//���� �����ͷ� �ܺο��� �ѹ濡 ���� �ִ� ����
	int GetNotBrokenPutSize (void);


	//WritePos�� ����Ÿ ����
	int Put (char *chpData, int iSize);

	//ReadPos���� ������ ������. ReadPos�̵�.
	int Get (char *chpDest, int iSize);


	//ReadPos���� ������ ������. ReadPos����.
	int Peek (char *chpDest, int iSize);


	//���ϴ� ���̸�ŭ �б� ��ġ���� ����/���� ��ġ �̵�

	void RemoveData (int iSize);
	int MoveWritePos (int iSize);

	//������ ��� ������ ����
	void ClearBuffer (void);

	//������ ������ ����.
	char *GetBufferPtr (void);

	//������ ReadPos ������ ����
	char *GetReadBufferPtr (void);

	//������ WritePos ������ ����
	char *GetWriteBufferPtr (void);

};