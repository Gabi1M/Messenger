#include "stdafx.h"

// communication library
#include "communication_api.h"
#include <Windows.h>

#define MAX_MESSAGE_SIZE 255

DWORD sendData(CM_CLIENT* client, CM_DATA_BUFFER* dataToSend, TCHAR* messageToSend)
{
	SIZE_T messageToSendSize = _tcslen(messageToSend);
	CM_SIZE sendByteCount;
	CM_ERROR error;

	char* aux = malloc(MAX_MESSAGE_SIZE);
	wcstombs(aux, messageToSend, MAX_MESSAGE_SIZE);

	error = CopyDataIntoBuffer(dataToSend, (const CM_BYTE*)aux, (CM_SIZE)messageToSendSize);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("CopyDataIntoBuffer failed with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}

	error = SendDataToServer(client, dataToSend, &sendByteCount);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("SendDataToServer failed with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}

	return ERROR_SUCCESS;
}

DWORD receiveData(CM_CLIENT* client, CM_DATA_BUFFER* dataToReceive, TCHAR* bufferedMessage)
{
	CM_SIZE receivedByteCount;
	CM_ERROR error;

	error = ReceiveDataFormServer(client, dataToReceive, &receivedByteCount);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("ReceiveDataFromClient failed with err-code=0x%X!\n"), error);
		return ERROR_ERRORS_ENCOUNTERED;
	}

	_tcscpy(bufferedMessage, (TCHAR*)dataToReceive->DataBuffer);
	return ERROR_SUCCESS;
}

DWORD initDataBuffer(CM_DATA_BUFFER** buffer, CM_SIZE size)
{
	CM_ERROR error;
	error = CreateDataBuffer(buffer, size);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("Failed to create data buffer with error code: 0x%X\n"), GetLastError());
		return ERROR_ERRORS_ENCOUNTERED;
	}
	return ERROR_SUCCESS;
}

int _tmain(int argc, TCHAR* argv[])
{
	(void)argc;
	(void)argv;

	EnableCommunicationModuleLogger();

	CM_ERROR error = InitCommunicationModule();
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("InitCommunicationModule failed with err-code=0x%X!\n"), error);
		return -1;
	}

	CM_CLIENT* client = NULL;
	error = CreateClientConnectionToServer(&client);
	if (CM_IS_ERROR(error))
	{
		_tprintf_s(TEXT("CreateClientConnectionToServer failed with err-code=0x%X!\n"), error);
		UninitCommunicationModule();
		return -1;
	}

	_tprintf_s(TEXT("We are connected to the server...\n"));

	TCHAR* message = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));

	while (TRUE)
	{
		_tprintf_s(TEXT("Enter text: "));
		_getts_s(message, MAX_MESSAGE_SIZE);

		CM_DATA_BUFFER* dataToSend = NULL;
		if (initDataBuffer(&dataToSend, MAX_MESSAGE_SIZE) != ERROR_SUCCESS)
		{
			return -1;
		}

		if (sendData(client, dataToSend, message) != ERROR_SUCCESS)
		{
			break;
		}

		DestroyDataBuffer(dataToSend);
		dataToSend = NULL;
	}

	_tprintf_s(TEXT("Client is shutting down now...\n"));

	DestroyClient(client);
	UninitCommunicationModule();

	return 0;
}

