#include "stdafx.h"
#include "MindwaveManager.h"

NSK_ALGO_CDECL(eNSK_ALGO_RET, NSK_ALGO_Init, (eNSK_ALGO_TYPE type, const NS_STR dataPath));
NSK_ALGO_CDECL(eNSK_ALGO_RET, NSK_ALGO_Uninit, (NS_VOID));
NSK_ALGO_CDECL(eNSK_ALGO_RET, NSK_ALGO_RegisterCallback, (NskAlgo_Callback cbFunc, NS_VOID *userData));
NSK_ALGO_CDECL(NS_STR, NSK_ALGO_SdkVersion, (NS_VOID));
NSK_ALGO_CDECL(NS_STR, NSK_ALGO_AlgoVersion, (eNSK_ALGO_TYPE type));
NSK_ALGO_CDECL(eNSK_ALGO_RET, NSK_ALGO_Start, (NS_BOOL bBaseline));
NSK_ALGO_CDECL(eNSK_ALGO_RET, NSK_ALGO_Pause, (NS_VOID));
NSK_ALGO_CDECL(eNSK_ALGO_RET, NSK_ALGO_Stop, (NS_VOID));
NSK_ALGO_CDECL(eNSK_ALGO_RET, NSK_ALGO_DataStream, (eNSK_ALGO_DATA_TYPE type, NS_INT16 *data, NS_INT dataLenght));

MindwaveManager::MindwaveManager()
{
	errCode = 0;
	packetsRead = 0;
	if (_init() < 0) {
		printf("Mindwave initialization fails!");
	}
}


MindwaveManager::~MindwaveManager()
{
}

void *getFuncAddr(HINSTANCE hinstLib, char *funcName, bool *bError) {
	void *funcPtr = (void*)GetProcAddress(hinstLib, funcName);
	*bError = true;
	if (NULL == funcPtr) {
		printf("Failed ot get %s function address\n.", (char*)funcName);
		*bError = false;
	}
	return funcPtr;
}

bool getFuncAddrs(HINSTANCE hinstLib) {
	bool bError;

	NSK_ALGO_InitAddr = (NSK_ALGO_Init_Dll)getFuncAddr(hinstLib, NSK_ALGO_InitStr, &bError);
	NSK_ALGO_UninitAddr = (NSK_ALGO_Uninit_Dll)getFuncAddr(hinstLib, NSK_ALGO_UninitStr, &bError);
	NSK_ALGO_RegisterCallbackAddr = (NSK_ALGO_RegisterCallback_Dll)getFuncAddr(hinstLib, NSK_ALGO_RegisterCallbackStr, &bError);
	NSK_ALGO_SdkVersionAddr = (NSK_ALGO_SdkVersion_Dll)getFuncAddr(hinstLib, NSK_ALGO_SdkVersionStr, &bError);
	NSK_ALGO_AlgoVersionAddr = (NSK_ALGO_AlgoVersion_Dll)getFuncAddr(hinstLib, NSK_ALGO_AlgoVersionStr, &bError);
	NSK_ALGO_StartAddr = (NSK_ALGO_Start_Dll)getFuncAddr(hinstLib, NSK_ALGO_StartStr, &bError);
	NSK_ALGO_PauseAddr = (NSK_ALGO_Pause_Dll)getFuncAddr(hinstLib, NSK_ALGO_PauseStr, &bError);
	NSK_ALGO_StopAddr = (NSK_ALGO_Stop_Dll)getFuncAddr(hinstLib, NSK_ALGO_StopStr, &bError);
	NSK_ALGO_DataStreamAddr = (NSK_ALGO_DataStream_Dll)getFuncAddr(hinstLib, NSK_ALGO_DataStreamStr, &bError);

	return bError;
}

void MindwaveManager::AlgoSdkCallback(sNSK_ALGO_CB_PARAM param) {

	switch (param.cbType)
	{
	case NSK_ALGO_CB_TYPE_SIGNAL_LEVEL:
	{
		eNSK_ALGO_SIGNAL_QUALITY sq = (eNSK_ALGO_SIGNAL_QUALITY)param.param.sq;
		switch (sq)
		{
		case NSK_ALGO_SQ_GOOD:
			printf("Good Signal\n");
			break;
		case NSK_ALGO_SQ_MEDIUM:
			printf("Medium Signal\n");
			break;
		case NSK_ALGO_SQ_NOT_DETECTED:
			printf("Signal Not Detected\n");
			break;
		case NSK_ALGO_SQ_POOR:
			printf("Poor Signal\n");
			break;
		}
	}

	default:
		break;
	}
}

int MindwaveManager::_init()
{
	HINSTANCE hinstLib = LoadLibrary(ALGO_SDK_DLL);

	//try to load Algo SDK library
	if (hinstLib == NULL) {
		printf("Failed to load library AlgoSdkDll.dll\n");
		return -1;
	}
	else {
		printf("AlgoSdkDll.dll library loaded\n");
		if (getFuncAddrs(hinstLib) == false) {
			FreeLibrary(hinstLib);
			printf("Functions in Algo SDK fail to load\n");
			return -1;
		}
		else {
			printf("All Algo SDK functions are loaded successfully\n");
		}
	}

	// Generate a connection id
	connectionId = TG_GetNewConnectionId();
	//connect fails
	if (connectionId < 0) {
		printf("Connection ID fails to generate!\n");
		return -1;
	}
	else {
		printf("Successfully generated connection ID\n");
	}

	// Connecting to headset through USB with MWM_COM
	comPortName = "\\\\.\\" MWM_COM;
	errCode = TG_Connect(connectionId, comPortName, TG_BAUD_1200, TG_STREAM_PACKETS);
	if (errCode < 0) {
		printf("TG_Connect() failed with error code %d\n.", errCode);

		return -1;
	}
	else {
		printf("Successfully connected to Mindwave USB via %s.\n", MWM_COM);
	}


	int lSelectedAlgos = 0;
	lSelectedAlgos |= NSK_ALGO_TYPE_ATT;
	lSelectedAlgos |= NSK_ALGO_TYPE_MED;
	lSelectedAlgos |= NSK_ALGO_TYPE_BLINK;
	lSelectedAlgos |= NSK_ALGO_TYPE_BP;

	wchar_t ReadBuffer[1024] = { 0 };
	char readBuf[1024] = { 0 };

	GetCurrentDirectory(1024, ReadBuffer);
	wcstombs_s(NULL, readBuf, ReadBuffer, 1024);

	eNSK_ALGO_RET ret = (NSK_ALGO_RegisterCallbackAddr)(&AlgoSdkCallback, NULL);
	ret = (NSK_ALGO_InitAddr)((eNSK_ALGO_TYPE)(lSelectedAlgos), readBuf);
	if (NSK_ALGO_RET_SUCCESS == ret) {
		printf("Algo SDK inited successfully\n");
	}
	else {
		printf("Algo SDK initializtion fails\n");
		return -1;
	}


	std::thread readPacketThread(ReadPacket);
	readPacketThread.join();
	return 0;
}

// Reading packet process, dispatch packet to AlgoSdkCallback
void MindwaveManager::ReadPacket() {
	int rawCount = 0;
	short rawData[512] = { 0 };
	int lastRawCount = 0;
	int packetsRead = 0;
	printf("Reading packets from headset...... \n");
	while (TRUE) {

		// Read a single Packet from the connection
		packetsRead = TG_ReadPackets(connectionId, 1);

		// If TG_ReadPackets is able to read one packet
		if (packetsRead > 0) {

			if (packetsRead == 1) {
				// If the Packet containted a new raw wave value...
				if (TG_GetValueStatus(connectionId, TG_DATA_RAW) != 0) {
					// Get and print out the new raw value */
					rawData[rawCount++] = (short)TG_GetValue(connectionId, TG_DATA_RAW);
					lastRawCount = rawCount;
					if (rawCount == 512) {
						(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_EEG, rawData, rawCount);
						rawCount = 0;
					}
				}

				if (TG_GetValueStatus(connectionId, TG_DATA_POOR_SIGNAL) != 0) {
					short pq = (short)TG_GetValue(connectionId, TG_DATA_POOR_SIGNAL);
					rawCount = 0;
					(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_PQ, &pq, 1);
				}

				if (TG_GetValueStatus(connectionId, TG_DATA_ATTENTION) != 0) {
					short att = (short)TG_GetValue(connectionId, TG_DATA_ATTENTION);
					(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_ATT, &att, 1);
				}

				if (TG_GetValueStatus(connectionId, TG_DATA_MEDITATION) != 0) {
					short med = (short)TG_GetValue(connectionId, TG_DATA_MEDITATION);
					(NSK_ALGO_DataStreamAddr)(NSK_ALGO_DATA_TYPE_MED, &med, 1);
				}
			}
		}
	}
}