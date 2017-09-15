#pragma once
#include "mindwave/thinkgear.h"
#include "mindwave/NSK_Algo.h"
#include <Windows.h>
#include <thread>

#define MWM_COM					"COM3" // The COM number may be different, check system

#ifdef _WIN64
#define ALGO_SDK_DLL			L"AlgoSdkDll64.dll"
#else
#define ALGO_SDK_DLL			L"AlgoSdkDll.dll"
#endif

#define NSK_ALGO_CDECL(ret, func, args)		typedef ret (__cdecl *func##_Dll) args; func##_Dll func##Addr = NULL; char func##Str[] = #func;

static int connectionId = -1;

class MindwaveManager
{
public:
	MindwaveManager();
	~MindwaveManager();

private:
	int errCode;
	char *comPortName = NULL;
	int packetsRead;
	static void AlgoSdkCallback(sNSK_ALGO_CB_PARAM param);
	int _init();
	static void ReadPacket();
};

