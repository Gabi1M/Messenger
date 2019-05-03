#include "stdafx.h"
#include "Persistance.h"

#define MAX_MESSAGE_SIZE 255

TCHAR** splitString(TCHAR* message, DWORD* numberOfWords, TCHAR* token)
{
	TCHAR** result = (TCHAR * *)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR*));
	for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
	{
		result[i] = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	}

	*numberOfWords = 0;

	TCHAR* word = _tcstok(message, token);
	while (word != NULL)
	{
		_tcscpy(result[*numberOfWords], word);
		(*numberOfWords)++;
		word = _tcstok(NULL, token);
	}

	return result;
}

DWORD addUserToFile(TCHAR* username)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFile(TEXT("D:\\registration.txt"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		hFile = CreateFile(TEXT("D:\\registration.txt"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
	}
	if (hFile == INVALID_HANDLE_VALUE)
	{
		return ERROR_ERRORS_ENCOUNTERED;
	}

	BOOL userExists = FALSE;
	TCHAR* lineRead = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	TCHAR** readArray;
	DWORD bytesRead = 0;

	while (TRUE)
	{
		if (ReadFile(hFile, lineRead, 30, &bytesRead, NULL) == 0)
		{
			CloseHandle(hFile);
			free(lineRead);
			return ERROR_ERRORS_ENCOUNTERED;
		}

		if (bytesRead == 0)
		{
			break;
		}

		DWORD numberOfWords = 0;
		readArray = splitString(lineRead, &numberOfWords, TEXT(","));

		if (_tcscmp(readArray[0], username) == 0)
		{
			userExists = TRUE;
			break;
		}
		free(readArray);
	}
	free(lineRead);

	DWORD bytesWritten = 0;

	if (userExists == FALSE)
	{
		if (WriteFile(hFile, username, 30, &bytesWritten, NULL) == 0)
		{
			CloseHandle(hFile);
			return ERROR_ERRORS_ENCOUNTERED;
		}
		if (bytesWritten != 30)
		{
			CloseHandle(hFile);
			return ERROR_ERRORS_ENCOUNTERED;
		}
	}

	CloseHandle(hFile);
	return ERROR_SUCCESS;
}