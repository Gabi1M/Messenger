#pragma once

#include <Windows.h>

typedef int(*PFUNC_THREAD_POOL_WORKER)(PVOID);

typedef struct _THREAD_POOL
{
	unsigned int ThreadCount;
	HANDLE hSemaphore;
}THREAD_POOL;

typedef struct _THREAD_POOL_PARAM
{
	PFUNC_THREAD_POOL_WORKER WorkerFunction;
	PVOID WorkerParam;
	HANDLE hSemaphore;
}THREAD_POOL_PARAM;

DWORD ThreadPoolInit(THREAD_POOL* pPool, unsigned int ThreadCount);

DWORD ThreadPoolDestroy(THREAD_POOL* pPool);

DWORD ThreadPoolStartNewWorker(THREAD_POOL* pPool, PFUNC_THREAD_POOL_WORKER pFuncWorker, PVOID pWorkerParam);

DWORD WINAPI ThreadPoolFunc(_In_ LPVOID lpParam);