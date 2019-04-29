#include"stdafx.h"
#include "ThreadPool.h"
#include <stdio.h>

DWORD ThreadPoolInit(THREAD_POOL* pPool, unsigned int ThreadCount)
{
	pPool->ThreadCount = ThreadCount;
	pPool->hSemaphore = NULL;
	pPool->hSemaphore = CreateSemaphore(NULL, pPool->ThreadCount, pPool->ThreadCount, NULL);

	if (pPool->hSemaphore == NULL)
	{
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

DWORD ThreadPoolDestroy(THREAD_POOL* pPool)
{

	pPool->ThreadCount = 0;
	if (NULL != pPool->hSemaphore)
	{
		CloseHandle(pPool->hSemaphore);
		pPool->hSemaphore = NULL;
	}

	return 0;
}

DWORD ThreadPoolStartNewWorker(THREAD_POOL* pPool, PFUNC_THREAD_POOL_WORKER pFuncWorker, PVOID pWorkerParam)
{
	THREAD_POOL_PARAM* param = NULL;

	if (WaitForSingleObject(pPool->hSemaphore, INFINITE) != WAIT_OBJECT_0)
	{
		_tprintf_s(TEXT("Waiting for semaphore failed with error code: 0x%X\n"), GetLastError());
		return GetLastError();
	}

	param = (THREAD_POOL_PARAM*)malloc(sizeof(THREAD_POOL_PARAM));
	param->hSemaphore = pPool->hSemaphore;
	param->WorkerFunction = pFuncWorker;
	param->WorkerParam = pWorkerParam;

	if (CreateThread(NULL, 0, ThreadPoolFunc, param, 0, NULL) == NULL)
	{
		free(param);
		return GetLastError();
	}

	return ERROR_SUCCESS;
}

DWORD WINAPI ThreadPoolFunc(_In_ LPVOID lpParameter)
{
	THREAD_POOL_PARAM* param = (THREAD_POOL_PARAM*)lpParameter;
	DWORD result = param->WorkerFunction(param->WorkerParam);
	if (ReleaseSemaphore(param->hSemaphore, 1, NULL) == FALSE)
	{
		_tprintf_s(TEXT("ReleaseSemaphore failed with error code: 0x%X\n"), GetLastError());
	}
	return result;
}