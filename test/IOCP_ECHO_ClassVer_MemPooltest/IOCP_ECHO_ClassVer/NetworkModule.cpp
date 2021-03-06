#include"stdafx.h"
#include "NetworkModule.h"

/*======================================================================
//생성자
//인자 : 없음
======================================================================*/
CLanServer::CLanServer (void)
{
	bServerOn = false;
	Packet::Initialize ();
}


/*======================================================================
//파괴자
//인자 : 없음
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
//설명 : NetworkServer On
//인자 : WCHAR * 서버IP, int PORT번호, int 동시접속 Session수, int 워커스레드 최대숫자
//리턴 : 서버 작동 여부. true false;
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

	//소켓 초기화 및 Listen작업.
	if ( InitializeNetwork (ServerIP, PORT) == false)
	{
		return false;
	}

	//세션 배열 생성
	_Session_Max = Session_Max;
	Session_Array = new Session[Session_Max];

	//비어있는 세션배열 번호 전부 저장.
	for ( int Cnt = 0; Cnt < Session_Max; Cnt++ )
	{
		if ( Session_Array[Cnt].p_IOChk.UseFlag == false )
		{
			emptySession.Push (Cnt);
		}
	}

	//IOCP 포트 생성 및 스레드 생성.
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
//설명 : NetworkServer Off
//인자 : 없음
//리턴 : 서버 중지 여부. true false;
======================================================================*/
bool CLanServer::Stop (void)
{
	if ( bServerOn == false )
	{
		return false;
	}

	
	//AcceptThread 종료
	closesocket (_ListenSock);

	//세션 정리
	for ( int Cnt = 0; Cnt < _Session_Max; Cnt++ )
	{
		if ( Session_Array[Cnt].p_IOChk.UseFlag == true )
		{
			shutdown (Session_Array[Cnt].sock, SD_BOTH);
		}
	}



	//PQCS로 워커 스레드 끄기.
	WaitForSingleObject (&Thread[0], INFINITE);

	PostQueuedCompletionStatus (_IOCP, 0, 0, 0);

	//스레드 종료 대기.
	WaitForMultipleObjects (_WorkerThread_Num + 1, Thread, TRUE, INFINITE);

	wprintf (L"\nNetworkModule End \n");

	LOG_LOG (L"Network", LOG_SYSTEM, L" NetworkStop");
	bServerOn = false;
	return true;
}




/*======================================================================
//SendPacket
//설명 : Packet을 SendQ에 넣고 WSASend함수 호출
//인자 : UINT64 SessionID, Packet *
//리턴 : 없음
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

	//Send버퍼 초과로 해당 세션을 강제로 끊어줘야 된다.
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
//설명 : 해당 세션의 TCP 연결 해제 요청 함수.
//인자 : UINT64 SessionID
//리턴 : 없음
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
//설명 : 인자로 들어온 해당 세션의 IOCount 차감 및 IOCount가 0일시 Session Release함수호출
//인자 : Session *
//리턴 : 없음
======================================================================*/
void CLanServer::IODecrement (Session * p)
{
	int Num = InterlockedDecrement (( volatile long * )&p->p_IOChk.IOCount);
	if ( Num == 0 )
	{
		SessionRelease (p);
	}
	//세션카운터가 0이하면 잘못만들은것이므로 크래쉬 일으켜서 확인.
	else if ( Num < 0 )
	{
		CCrashDump::Crash ();
	}
}




/*======================================================================
//AcceptThread
//설명 : AcceptThread() 함수 랩핑.
//인자 : LPVOID pParam; = CLanServer this pointer 를 일로 넘겨받음.
//리턴 : 0
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
//설명 : 실제 AcceptThread. 
//인자 : 없음
//리턴 : 없음
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
		//Accept 대기
		hClientSock = accept (_ListenSock, ( sockaddr * )&ClientAddr, &addrLen);


		if ( hClientSock == INVALID_SOCKET )
		{
			break;
		}

		//비어있는 세션찾기.
		//비어있는 세션이 없다면 로그로 남기고 서버 종료.

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

		//IOCP 포트에 등록
		CreateIoCompletionPort (( HANDLE )p->sock, _IOCP, ( ULONG_PTR )p, 0);



		//OnClientJoin으로 새로운 접속자 알림.
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
//설명 : WorkerThread() 함수 랩핑.
//인자 : LPVOID pParam; = CLanServer this pointer 를 일로 넘겨받음.
//리턴 : 0
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
//설명 : 실제 WorkerThread. GQCS로 Recv와 Send 완료통지를 받음.
//인자 : 없음
//리턴 : 없음
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

		//IOCP 자체 에러부
		if ( GQCS_Return == false && pOver == NULL )
		{
			LOG_LOG (L"Network", LOG_WARNING, L"IOCP GQCS ERROR");
			break;
		}


		//Transferred가 0이 나왔을 경우 처리부
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
					//Transferred가 0 일 경우 해당 세션이 파괴된것이므로 종료절차를 밟아나감.
				shutdown (pSession->sock, SD_BOTH);

				IODecrement (pSession);
			}


		}
		
		//실제 Recv,Send 처리부
		else
		{
			//Recv일 경우
			if ( pOver == &pSession->RecvOver )
			{

				PROFILE_BEGIN (L"recv");
				pSession->RecvQ.MoveWritePos (Transferred);

				//패킷 처리.
				while ( 1 )
				{
					short Header;

					int Size = pSession->RecvQ.GetUseSize ();

					if ( Size < sizeof (Header) )
					{
						break;
					}

					pSession->RecvQ.Peek (( char * )&Header, sizeof (Header));

					//헤더가 맞지 않는다. shutdown걸고 빠짐.
					if ( Header != 8 )
					{
						LOG_LOG (L"Network", LOG_DEBUG, L"SessionID 0x%p, Header %d", pSession->SessionID,Header);
						shutdown (pSession->sock, SD_BOTH);
						break;
					}
					
					//데이터가 전부 오지 않았다.
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
			//Send일 경우
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
//설명 : Start로 서버 On시 Listen소켓 초기화 및 bind, listen 함수
//인자 : WCHAR * IP, int 포트번호
//리턴 : 성공여부
======================================================================*/
bool CLanServer::InitializeNetwork (WCHAR *IP, int PORT)
{
	int retval;
	WSADATA wsaData;

	//윈속초기화
	if ( WSAStartup (MAKEWORD (2, 2), &wsaData) != 0 )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"WSA Start Up Failed");
		return false;
	}


	//소켓초기화
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
//설명 : 세션검색 및 해당세션에 대한 IO카운트 증가로 락을 거는 작업.
//인자 : UINT64 SessionID
//리턴 : Session *, NULL리턴시 실패.
======================================================================*/
CLanServer::Session *CLanServer::FindLockSession (UINT64 SessionID)
{
	int Cnt;
	
	Cnt = indexSessionID (SessionID);

	//IO증가. 1이라면 이전에 어디선가 Release를 타고 있을수 있으므로 IO차감하고 나감.
	if ( InterlockedIncrement (( volatile long * )&Session_Array[Cnt].p_IOChk.IOCount) == 1)
	{
		IODecrement (&Session_Array[Cnt]);
		return NULL;
	}
	//index로 찾은 해당 세션의 id가 내가 찾던 세션이 아닐 경우
	if ( Session_Array[Cnt].p_IOChk.UseFlag == false  )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"Delete Session search = 0x%p, Array = 0x%p", SessionID, Session_Array[Cnt].SessionID);
		IODecrement (&Session_Array[Cnt]);
		return NULL;
	}

	//세션 ID가 일치하지 않을 경우.
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
//설명 : RecvQ를 WSARecv로 등록하는 작업. 
//인자 : Session *
//리턴 : 없음
======================================================================*/
void CLanServer::PostRecv (Session * p)
{
	int Cnt = 0;
	DWORD RecvByte;
	DWORD dwFlag = 0;
	int retval;

	InterlockedIncrement ((volatile long *)&p->p_IOChk.IOCount);

	//RecvQ 버퍼 사이즈 체크 0 이라면 정상진행이 불가이므로 해당 세션을 종료시키고 빠져나온다.
	if ( p->RecvQ.GetFreeSize () <= 0 )
	{
		LOG_LOG (L"Network", LOG_WARNING, L"SessionID = Ox%p, WSABuffer 0 NotRecv", p->SessionID);

		shutdown (p->sock, SD_BOTH);
		IODecrement (p);

		return;
	}

	//WSARecv 등록

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


	//에러체크
	if ( retval == SOCKET_ERROR )
	{
		DWORD Errcode = GetLastError ();

		//IO_PENDING이라면 문제없이 진행중이므로 그냥 빠져나옴.
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
//설명 : SendQ에 있는 Packet을 WSASend로 POST
//인자 : Session *
//리턴 : 없음
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

	//WSASend 셋팅 및 등록부
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
	//에러체크
	if ( retval == SOCKET_ERROR )
	{
		DWORD Errcode = GetLastError ();

		//IO_PENDING이라면 문제없이 진행중이므로 그냥 빠져나옴.
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
//설명 : IOCount가 0이면 진입. UseFlag가 false면 SessionRelease작업 진행.
//인자 : Session *
//리턴 : 없음
======================================================================*/
void CLanServer::SessionRelease (Session * p)
{
	IOChk ComChk;
	ComChk.IOCount = 0;
	ComChk.UseFlag = true;
	IOChk ExChk;
	ExChk.IOCount = 0;
	ExChk.UseFlag = false;
	
	//IOCount와 UseFlag를 동시에 비교해서 IOCount가 0이고 UseFlag가 true일때만 Release진행. 
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