#include"stdafx.h"
#include "NetworkModule.h"

/*======================================================================
//������
//���� : ����
======================================================================*/
CLanServer::CLanServer (void)
{
	bServerOn = false;
	Packet::Initialize ();
}


/*======================================================================
//�ı���
//���� : ����
======================================================================*/
CLanServer::~CLanServer (void)
{
	if ( bServerOn )
	{
		Stop ();
	}
}


/*======================================================================
//Start
//���� : NetworkServer On
//���� : WCHAR * ����IP, int PORT��ȣ, int �������� Session��, int ��Ŀ������ �ִ����
//���� : ���� �۵� ����. true false;
======================================================================*/
bool CLanServer::Start (WCHAR * ServerIP, int PORT, int Session_Max, int WorkerThread_Num)
{
	if ( bServerOn == true )
	{
		return false;
	}

	LOG_DIRECTORY (L"LOG_FILE");
	LOG_LEVEL (LOG_SYSTEM, false);
	wprintf (L"\n NetworkModule Start \n");

	//���� �ʱ�ȭ �� Listen�۾�.
	if ( InitializeNetwork (ServerIP, PORT) == false)
	{
		return false;
	}

	//���� �迭 ����
	_Session_Max = Session_Max;
	Session_Array = new Session[Session_Max];

	//����ִ� ���ǹ迭 ��ȣ ���� ����.
	for ( int Cnt = 0; Cnt < Session_Max; Cnt++ )
	{
		if ( Session_Array[Cnt].p_IOChk.UseFlag == false )
		{
			emptySession.Push (Cnt);
		}
	}

	//IOCP ��Ʈ ���� �� ������ ����.
	_WorkerThread_Num = WorkerThread_Num;
	_IOCP = CreateIoCompletionPort (INVALID_HANDLE_VALUE, NULL, 0, WorkerThread_Num);

	Thread = new HANDLE[WorkerThread_Num + 1];

	Thread[0] = ( HANDLE )_beginthreadex (NULL,0,AcceptThread,(void *)this,NULL,NULL );

	for ( int Cnt = 0; Cnt < WorkerThread_Num; Cnt++ )
	{
		Thread[Cnt + 1] = ( HANDLE )_beginthreadex (NULL, 0, WorkerThread, (void *)this, NULL, NULL);
	}

	LOG_LOG (L"Network", LOG_SYSTEM, L" NetworkStart IP = %s, PORT = %d, SessionMax = %d, WorkerThreadNum = %d", ServerIP, PORT, Session_Max, WorkerThread_Num);
	bServerOn = true;
	return true;
}



/*======================================================================
//Stop
//���� : NetworkServer Off
//���� : ����
//���� : ���� ���� ����. true false;
======================================================================*/
bool CLanServer::Stop (void)
{
	if ( bServerOn == false )
	{
		return false;
	}

	
	//AcceptThread ����
	closesocket (_ListenSock);

	//���� ����
	for ( int Cnt = 0; Cnt < _Session_Max; Cnt++ )
	{
		if ( Session_Array[Cnt].p_IOChk.UseFlag == true )
		{
			shutdown (Session_Array[Cnt].sock, SD_BOTH);
		}
	}



	//PQCS�� ��Ŀ ������ ����.
	WaitForSingleObject (&Thread[0], INFINITE);

	PostQueuedCompletionStatus (_IOCP, 0, 0, 0);

	//������ ���� ���.
	WaitForMultipleObjects (_WorkerThread_Num + 1, Thread, TRUE, INFINITE);

	wprintf (L"\nNetworkModule End \n");

	LOG_LOG (L"Network", LOG_SYSTEM, L" NetworkStop");
	bServerOn = false;
	return true;
}




/*======================================================================
//SendPacket
//���� : Packet�� SendQ�� �ְ� WSASend�Լ� ȣ��
//���� : UINT64 SessionID, Packet *
//���� : ����
======================================================================*/
void CLanServer::SendPacket (UINT64 SessionID, Packet *pack)
{

	Session *p = FindLockSession (SessionID);
	if ( p == NULL )
	{
		return;
	}

	short Header = sizeof (INT64);

	pack->PutHeader (( char * )&Header, sizeof (Header));

	//Send���� �ʰ��� �ش� ������ ������ ������� �ȴ�.
	pack->Add ();

	if ( p->SendQ.Enqueue (pack) == false )
	{
		LOG_LOG (L"Network", LOG_ERROR, L"SendBuffer Overflow SessionID = 0x%p ", p->SessionID);
		shutdown (p->sock, SD_BOTH);
		return;
	}

	PostSend (p);

	IODecrement (p);
	return;
}



/*======================================================================
//Disconnect
//���� : �ش� ������ TCP ���� ���� ��û �Լ�.
//���� : UINT64 SessionID
//���� : ����
======================================================================*/
void CLanServer::Disconnect (UINT64 SessionID)
{
	Session *p = FindLockSession (SessionID);
	if ( p == NULL )
	{
		return;
	}

	shutdown (p->sock, SD_BOTH);

	IODecrement (p);
	return;
}



/*======================================================================
//IODeCrement
//���� : ���ڷ� ���� �ش� ������ IOCount ���� �� IOCount�� 0�Ͻ� Session Release�Լ�ȣ��
//���� : Session *
//���� : ����
======================================================================*/
void CLanServer::IODecrement (Session * p)
{
	int Num = InterlockedDecrement (( volatile long * )&p->p_IOChk.IOCount);
	if ( Num == 0 )
	{
		SessionRelease (p);
	}
	//����ī���Ͱ� 0���ϸ� �߸����������̹Ƿ� ũ���� �����Ѽ� Ȯ��.
	else if ( Num < 0 )
	{
		CCrashDump::Crash ();
	}
}




/*======================================================================
//AcceptThread
//���� : AcceptThread() �Լ� ����.
//���� : LPVOID pParam; = CLanServer this pointer �� �Ϸ� �Ѱܹ���.
//���� : 0
======================================================================*/
unsigned int CLanServer::AcceptThread (LPVOID pParam)
{
	CLanServer *p = (CLanServer * )pParam;

	wprintf (L"Accept_thread_Start\n");
	LOG_LOG (L"Network", LOG_SYSTEM, L"AcceptThread_Start");

	p->AcceptThread ();

	wprintf (L"\n\n\nAccept Thread End\n\n\n");
	LOG_LOG (L"Network", LOG_SYSTEM, L"AcceptThread_End");

	return 0;
}




/*======================================================================
//AcceptThread
//���� : ���� AcceptThread. 
//���� : ����
//���� : ����
======================================================================*/
void CLanServer::AcceptThread (void)
{	
	SOCKET hClientSock = 0;
	SOCKADDR_IN ClientAddr;
	Session *p;
	int addrLen;


	while ( 1 )
	{
		addrLen = sizeof (ClientAddr);
		//Accept ���
		hClientSock = accept (_ListenSock, ( sockaddr * )&ClientAddr, &addrLen);


		if ( hClientSock == INVALID_SOCKET )
		{
			break;
		}

		//����ִ� ����ã��.
		//����ִ� ������ ���ٸ� �α׷� ����� ���� ����.

		if ( emptySession.isEmpty () )
		{
			closesocket (hClientSock);
			LOG_LOG (L"Network", LOG_SYSTEM, L"Session %d Use Full ", _Session_Max);
			break;
		}
		int Cnt;
		emptySession.Pop (&Cnt);

		InterlockedIncrement ((volatile long *)&Session_Array[Cnt].p_IOChk.IOCount);

		p = &Session_Array[Cnt];
		p->sock = hClientSock;
		p->SessionID = CreateSessionID (Cnt, InterlockedIncrement64 (( volatile LONG64 * )&_SessionID_Count));
		p->p_IOChk.UseFlag = true;
		p->SendFlag = false;

		InterlockedIncrement (( volatile long * )&_Use_Session_Cnt);

		//IOCP ��Ʈ�� ���
		CreateIoCompletionPort (( HANDLE )p->sock, _IOCP, ( ULONG_PTR )p, 0);



		//OnClientJoin���� ���ο� ������ �˸�.
		WCHAR IP[36];
		WSAAddressToString (( SOCKADDR * )&ClientAddr, sizeof (ClientAddr), NULL, IP, (DWORD *)&addrLen);

		if ( OnClientJoin (p->SessionID, IP, ntohs (ClientAddr.sin_port)) == false )
		{
			SessionRelease (p);
			continue;
		}

		PostRecv (p);

		InterlockedIncrement (( volatile long * )&_AcceptTPS);
		InterlockedIncrement (( volatile long * )&_AcceptTotal);
		
		IODecrement (p);

	}
	return;
}



/*======================================================================
//WorkerThread
//���� : WorkerThread() �Լ� ����.
//���� : LPVOID pParam; = CLanServer this pointer �� �Ϸ� �Ѱܹ���.
//���� : 0
======================================================================*/
unsigned int CLanServer::WorkerThread (LPVOID pParam)
{
	CLanServer *p = ( CLanServer * )pParam;
	wprintf (L"worker_thread_Start\n");
	LOG_LOG (L"Network", LOG_SYSTEM, L"Worker Thread_Start");

	p->WorkerThread ();

	wprintf (L"\nWorker Thread End\n");
	LOG_LOG (L"Network", LOG_SYSTEM, L"Worker Thread_End");
	return 0;
}



/*======================================================================
//WorkerThread
//���� : ���� WorkerThread. GQCS�� Recv�� Send �Ϸ������� ����.
//���� : ����
//���� : ����
======================================================================*/
void CLanServer::WorkerThread (void)
{

	BOOL GQCS_Return;

	DWORD Transferred;
	OVERLAPPED *pOver;
	Session *pSession;

	while ( 1 )
	{
		Transferred = 0;
		pSession = NULL;
		pOver = NULL;

		GQCS_Return = GetQueuedCompletionStatus (_IOCP, &Transferred, ( PULONG_PTR )&pSession, &pOver, INFINITE);

		PROFILE_BEGIN (L"WorKerThread");

		//IOCP ��ü ������
		if ( GQCS_Return == false && pOver == NULL )
		{
			LOG_LOG (L"Network", LOG_WARNING, L"IOCP GQCS ERROR");
			break;
		}


		//Transferred�� 0�� ������ ��� ó����
		if ( Transferred == 0 )
		{

			if ( pOver == NULL && pSession == 0 )
			{
				PostQueuedCompletionStatus (_IOCP, 0, 0, NULL);
				break;
			}
			else
			{
				if ( pOver == &pSession->RecvOver )
				{
					LOG_LOG (L"Network", LOG_DEBUG, L"Session 0x%p, Transferred 0 RecvOver", pSession->SessionID);
				}
				else if ( pOver == &pSession->SendOver )
				{
					LOG_LOG (L"Network", LOG_DEBUG, L"Session 0x%p, Transferred 0 SendOver", pSession->SessionID);
				}
					//Transferred�� 0 �� ��� �ش� ������ �ı��Ȱ��̹Ƿ� ���������� ��Ƴ���.
				shutdown (pSession->sock, SD_BOTH);

				IODecrement (pSession);
			}


		}
		
		//���� Recv,Send ó����
		else
		{
			//Recv�� ���
			if ( pOver == &pSession->RecvOver )
			{

				PROFILE_BEGIN (L"recv");
				pSession->RecvQ.MoveWritePos (Transferred);

				//��Ŷ ó��.
				while ( 1 )
				{
					short Header;

					int Size = pSession->RecvQ.GetUseSize ();

					if ( Size < sizeof (Header) )
					{
						break;
					}

					pSession->RecvQ.Peek (( char * )&Header, sizeof (Header));

					//����� ���� �ʴ´�. shutdown�ɰ� ����.
					if ( Header != 8 )
					{
						LOG_LOG (L"Network", LOG_DEBUG, L"SessionID 0x%p, Header %d", pSession->SessionID,Header);
						shutdown (pSession->sock, SD_BOTH);
						break;
					}
					
					//�����Ͱ� ���� ���� �ʾҴ�.
					if ( Size < sizeof (Header) + Header )
					{
						break;
					}

					pSession->RecvQ.RemoveData (sizeof (Header));

					Packet *Pack = Packet::Alloc();

					pSession->RecvQ.Get (Pack->GetBufferPtr(),Header);

					Pack->MoveWritePos (Header);


					OnRecv (pSession->SessionID, Pack);

					Packet::Free (Pack);
					InterlockedIncrement (( volatile LONG * )&_RecvPacketTPS);
				}

				PostRecv (pSession);

				PROFILE_END (L"recv");

			}
			//Send�� ���
			else if ( pOver == &pSession->SendOver )
			{
				PROFILE_BEGIN (L"send");
				OnSend (pSession->SessionID, Transferred);

				Packet *Pack;
				while ( 1 )
				{
					if ( pSession->SendPack.Pop (&Pack) == false )
					{
						break;
					}
					Packet::Free(Pack);
				}

				
				pSession->SendFlag = FALSE;


				if ( pSession->SendQ.GetUseSize () > 0 )
				{
					PostSend (pSession);
				}
				
				InterlockedIncrement (( volatile LONG * )&_SendPacketTPS);

				PROFILE_END (L"send");
			}

			IODecrement (pSession);
		}
		PROFILE_END (L"WorKerThread");
	}

	PROFILE_END (L"WorKerThread");
}






/*======================================================================
//InitializeNetwork
//���� : Start�� ���� On�� Listen���� �ʱ�ȭ �� bind, listen �Լ�
//���� : WCHAR * IP, int ��Ʈ��ȣ
//���� : ��������
======================================================================*/
bool CLanServer::InitializeNetwork (WCHAR *IP, int PORT)
{
	int retval;
	WSADATA wsaData;

	//�����ʱ�ȭ
	if ( WSAStartup (MAKEWORD (2, 2), &wsaData) != 0 )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"WSA Start Up Failed");
		return false;
	}


	//�����ʱ�ȭ
	_ListenSock = socket (AF_INET, SOCK_STREAM, 0);
	if ( _ListenSock == SOCKET_ERROR )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"Listen_Sock Failed");
		return false;
	}



	//bind
	SOCKADDR_IN addr;

	addr.sin_family = AF_INET;
	InetPton (AF_INET, IP, &addr.sin_addr);
	addr.sin_port = htons (PORT);

	retval = bind (_ListenSock, ( SOCKADDR * )&addr, sizeof (addr));
	if ( retval == SOCKET_ERROR )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"bind Failed");
		return false;
	}


	//listen
	retval = listen (_ListenSock, SOMAXCONN);
	if ( retval == SOCKET_ERROR )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"Listen Failed");
		return false;
	}


	return true;

}


/*======================================================================
//FindLockSession
//���� : ���ǰ˻� �� �ش缼�ǿ� ���� IOī��Ʈ ������ ���� �Ŵ� �۾�.
//���� : UINT64 SessionID
//���� : Session *, NULL���Ͻ� ����.
======================================================================*/
CLanServer::Session *CLanServer::FindLockSession (UINT64 SessionID)
{
	int Cnt;
	
	Cnt = indexSessionID (SessionID);

	//IO����. 1�̶�� ������ ��𼱰� Release�� Ÿ�� ������ �����Ƿ� IO�����ϰ� ����.
	if ( InterlockedIncrement (( volatile long * )&Session_Array[Cnt].p_IOChk.IOCount) == 1)
	{
		IODecrement (&Session_Array[Cnt]);
		return NULL;
	}
	//index�� ã�� �ش� ������ id�� ���� ã�� ������ �ƴ� ���
	if ( Session_Array[Cnt].p_IOChk.UseFlag == false  )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"Delete Session search = 0x%p, Array = 0x%p", SessionID, Session_Array[Cnt].SessionID);
		IODecrement (&Session_Array[Cnt]);
		return NULL;
	}

	//���� ID�� ��ġ���� ���� ���.
	if ( Session_Array[Cnt].SessionID != SessionID )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"SessionID not match search = 0x%p, Array = 0x%p", SessionID, Session_Array[Cnt].SessionID);
		IODecrement (&Session_Array[Cnt]);
		return NULL;
	}

	return &Session_Array[Cnt];
}


/*======================================================================
//PostRecv
//���� : RecvQ�� WSARecv�� ����ϴ� �۾�. 
//���� : Session *
//���� : ����
======================================================================*/
void CLanServer::PostRecv (Session * p)
{
	int Cnt = 0;
	DWORD RecvByte;
	DWORD dwFlag = 0;
	int retval;

	InterlockedIncrement ((volatile long *)&p->p_IOChk.IOCount);

	//RecvQ ���� ������ üũ 0 �̶�� ���������� �Ұ��̹Ƿ� �ش� ������ �����Ű�� �������´�.
	if ( p->RecvQ.GetFreeSize () <= 0 )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"SessionID = Ox%p, WSABuffer 0 NotRecv", p->SessionID);

		shutdown (p->sock, SD_BOTH);
		IODecrement (p);

		return;
	}

	//WSARecv ���

	WSABUF buf[2];
	buf[0].buf = p->RecvQ.GetWriteBufferPtr ();
	buf[0].len = p->RecvQ.GetNotBrokenPutSize ();
	Cnt++;
	if ( p->RecvQ.GetFreeSize () > p->RecvQ.GetNotBrokenPutSize () )
	{
		buf[1].buf = p->RecvQ.GetBufferPtr ();
		buf[1].len = p->RecvQ.GetFreeSize () - p->RecvQ.GetNotBrokenPutSize ();
		Cnt++;
	}

	memset (&p->RecvOver, 0, sizeof (p->RecvOver));

	retval = WSARecv (p->sock, buf, Cnt, &RecvByte, &dwFlag, &p->RecvOver, NULL);


	//����üũ
	if ( retval == SOCKET_ERROR )
	{
		DWORD Errcode = GetLastError ();

		//IO_PENDING�̶�� �������� �������̹Ƿ� �׳� ��������.
		if ( Errcode != WSA_IO_PENDING )
		{

			if ( Errcode == WSAENOBUFS )
			{
				LOG_LOG (L"Network", LOG_WARNING, L"SessionID = 0x%p, ErrorCode = %ld WSAENOBUFS ERROR ", p->SessionID, Errcode);
			}
			else
			{
				LOG_LOG (L"Network", LOG_ERROR, L"SessionID = 0x%p, ErrorCode = %ld PostRecv", p->SessionID, Errcode);
			}

			shutdown (p->sock, SD_BOTH);
			IODecrement (p);
		}
	}

	return;
}



/*======================================================================
//PostSend
//���� : SendQ�� �ִ� Packet�� WSASend�� POST
//���� : Session *
//���� : ����
======================================================================*/
void CLanServer::PostSend (Session *p)
{
	int Cnt = 0;
	DWORD SendByte;
	DWORD dwFlag = 0;
	int retval;
	InterlockedIncrement (( volatile long * )&p->p_IOChk.IOCount);

	if ( InterlockedCompareExchange (( volatile long * )&p->SendFlag, TRUE, FALSE) == TRUE )
	{
		IODecrement (p);
		return;
	}

	//WSASend ���� �� ��Ϻ�
	WSABUF buf[SendbufMax];

	Packet *pack;
	while ( 1 )
	{

		if ( p->SendQ.Dequeue (&pack) == false || Cnt >= SendbufMax )
		{
			break;
		}

		buf[Cnt].buf = pack->GetBufferPtr();
		buf[Cnt].len = pack->GetDataSize ();
		Cnt++;
		
		p->SendPack.Push (pack);
	}

	if ( Cnt == 0 )
	{
		IODecrement (p);
		p->SendFlag = FALSE;
		return;
	}

	memset (&p->SendOver, 0, sizeof (p->SendOver));
	retval = WSASend (p->sock, buf, Cnt, &SendByte, dwFlag, &p->SendOver, NULL);
	//����üũ
	if ( retval == SOCKET_ERROR )
	{
		DWORD Errcode = GetLastError ();

		//IO_PENDING�̶�� �������� �������̹Ƿ� �׳� ��������.
		if ( Errcode != WSA_IO_PENDING )
		{

			if ( Errcode == WSAENOBUFS )
			{
				LOG_LOG (L"Network", LOG_WARNING, L"SessionID = 0x%p, ErrorCode = %ld WSAENOBUFS ERROR ", p->SessionID, Errcode);
			}
			else
			{
				LOG_LOG (L"Network", LOG_ERROR, L"SessionID = 0x%p, ErrorCode = %ld PostSend", p->SessionID, Errcode);
			}

			shutdown (p->sock, SD_BOTH);
			IODecrement (p);
			PROFILE_END (L"postsend");
		}
	}
	return;
}



/*======================================================================
//SessionRelease
//���� : IOCount�� 0�̸� ����. UseFlag�� false�� SessionRelease�۾� ����.
//���� : Session *
//���� : ����
======================================================================*/
void CLanServer::SessionRelease (Session * p)
{
	IOChk ComChk;
	ComChk.IOCount = 0;
	ComChk.UseFlag = true;
	IOChk ExChk;
	ExChk.IOCount = 0;
	ExChk.UseFlag = false;
	
	//IOCount�� UseFlag�� ���ÿ� ���ؼ� IOCount�� 0�̰� UseFlag�� true�϶��� Release����. 
	if ( !InterlockedCompareExchange64 (( volatile LONG64 * )&p->p_IOChk, ( LONG64 )&ExChk, ( LONG64 )&ComChk))
	{
		return;
	}

	OnClientLeave (p->SessionID);

	closesocket (p->sock);

	p->RecvQ.ClearBuffer ();

	Packet *pack;

	while ( 1 )
	{
		

		if ( p->SendQ.Dequeue (&pack) == false )
		{
			break;
		}
		Packet::Free (pack);
	}


	while ( 1 )
	{
		if ( p->SendPack.Pop (&pack) == false )
		{
			break;
		}		

		Packet::Free (pack);
	}
	

	InterlockedDecrement (( volatile long * )&_Use_Session_Cnt);


	emptySession.Push (indexSessionID (p->SessionID));

	return;
}