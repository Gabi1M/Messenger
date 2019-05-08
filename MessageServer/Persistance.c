#include "stdafx.h"
#include "Persistance.h"

#define MAX_MESSAGE_SIZE 255

TCHAR** splitString(TCHAR* message, DWORD* numberOfWords, TCHAR* token)
{
	TCHAR** result = (TCHAR * *)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR*));
	if (result == NULL)
	{
		return NULL;
	}

	for (int i = 0; i < MAX_MESSAGE_SIZE; i++)
	{
		result[i] = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
		if (result[i] == NULL)
		{
			return NULL;
		}
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
	if (lineRead == NULL)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		return (DWORD)-1;
	}

	TCHAR** readArray;
	DWORD bytesRead = 0;

	while (TRUE)
	{
		if (ReadFile(hFile, lineRead, 30, &bytesRead, NULL) == 0)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			free(lineRead);
			lineRead = NULL;

			return ERROR_ERRORS_ENCOUNTERED;
		}

		if (bytesRead == 0)
		{
			break;
		}

		DWORD numberOfWords = 0;
		readArray = splitString(lineRead, &numberOfWords, TEXT(","));
		if (readArray == NULL)
		{
			free(lineRead);
			lineRead = NULL;
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			return (DWORD)-1;
		}

		if (_tcscmp(readArray[0], username) == 0)
		{
			userExists = TRUE;
			break;
		}
		free(readArray);
		readArray = NULL;
	}
	free(lineRead);
	lineRead = NULL;

	DWORD bytesWritten = 0;

	if (userExists == FALSE)
	{
		if (WriteFile(hFile, username, 30, &bytesWritten, NULL) == 0)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			return ERROR_ERRORS_ENCOUNTERED;
		}
		if (bytesWritten != 30)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			return ERROR_ERRORS_ENCOUNTERED;
		}
	}

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;
	return ERROR_SUCCESS;
}

DWORD checkIfUserExists(TCHAR* username)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFile(TEXT("D:\\registration.txt"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		return ERROR_ERRORS_ENCOUNTERED;
	}

	TCHAR* lineRead = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	if (lineRead == NULL)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		return (DWORD)-1;
	}

	TCHAR** readArray;
	DWORD bytesRead = 0;

	while (TRUE)
	{
		if (ReadFile(hFile, lineRead, 30, &bytesRead, NULL) == 0)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			free(lineRead);
			lineRead = NULL;
			return ERROR_ERRORS_ENCOUNTERED;
		}

		if (bytesRead == 0)
		{
			break;
		}

		DWORD numberOfWords = 0;
		readArray = splitString(lineRead, &numberOfWords, TEXT(","));
		if (readArray == NULL)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			free(lineRead);
			lineRead = NULL;
			return (DWORD)-1;
		}

		if (_tcscmp(readArray[0], username) == 0)
		{
			free(lineRead);
			lineRead = NULL;
			free(readArray);
			readArray = NULL;
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;
			return ERROR_SUCCESS;
		}
		free(readArray);
		readArray = NULL;
	}
	free(lineRead);
	lineRead = NULL;
	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	return ERROR_NOT_FOUND;
}

DWORD addMessageToFile(TCHAR* originName, TCHAR* destinationName, TCHAR* message)
{
	TCHAR* fileName = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	_tcscpy(fileName, TEXT("D:\\"));
	_tcscat(fileName, destinationName);
	_tcscat(fileName, TEXT(".txt"));

	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, NULL);
	}
	if (hFile == INVALID_HANDLE_VALUE)
	{
		free(fileName);
		fileName = NULL;
		return ERROR_ERRORS_ENCOUNTERED;
	}
	free(fileName);
	fileName = NULL;

	TCHAR* fileEntry = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	_tcscpy(fileEntry, originName);
	_tcscat(fileEntry, TEXT(" "));
	_tcscat(fileEntry, message);

	DWORD bytesWritten = 0;
	DWORD bytesRead = 0;

	TCHAR* readMsg = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));

	while (TRUE)
	{
		if (ReadFile(hFile, readMsg, MAX_MESSAGE_SIZE, &bytesRead, NULL) == 0)
		{
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;

			return ERROR_ERRORS_ENCOUNTERED;
		}

		if (bytesRead == 0)
		{
			break;
		}

		//SetFilePointer(hFile, MAX_MESSAGE_SIZE, NULL, FILE_CURRENT);
	}

	if (WriteFile(hFile, fileEntry, MAX_MESSAGE_SIZE, &bytesWritten, NULL) == 0)
	{
		CloseHandle(hFile);
		hFile = INVALID_HANDLE_VALUE;
		free(fileEntry);
		fileEntry = NULL;

		return ERROR_ERRORS_ENCOUNTERED;
	}

	CloseHandle(hFile);
	hFile = INVALID_HANDLE_VALUE;

	free(fileEntry);
	fileEntry = NULL;

	if (bytesWritten == 0)
	{
		return ERROR_ERRORS_ENCOUNTERED;
	}

	return ERROR_SUCCESS;

}

TCHAR** getMessagesFromFile(TCHAR* name, DWORD desiredMessageCount, DWORD* actualMessageCount)
{
	TCHAR* fileName = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	_tcscpy(fileName, TEXT("D:\\"));
	_tcscat(fileName, name);
	_tcscat(fileName, TEXT(".txt"));

	HANDLE hFile = INVALID_HANDLE_VALUE;
	hFile = CreateFile(fileName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE || GetLastError() == ERROR_FILE_NOT_FOUND)
	{
		free(fileName);
		fileName = NULL;
		return NULL;
	}
	free(fileName);
	fileName = NULL;

	TCHAR** result = (TCHAR * *)malloc(100 * sizeof(TCHAR*));
	for (int i = 0; i < 100; i++)
	{
		result[i] = (TCHAR*)malloc(MAX_MESSAGE_SIZE * sizeof(TCHAR));
	}

	DWORD bytesRead = 0;
	int i = 0;

	while (TRUE)
	{
		if (ReadFile(hFile, result[i], MAX_MESSAGE_SIZE, &bytesRead, NULL) == 0)
		{
			for (i = 0; i < 100; i++)
			{
				free(result[i]);
				result[i] = NULL;
			}
			free(result);
			result = NULL;
			CloseHandle(hFile);
			hFile = INVALID_HANDLE_VALUE;

			return NULL;
		}

		if (bytesRead == 0 || ((int)desiredMessageCount == i-1))
		{
			break;
		}
		i++;
	}

	*actualMessageCount = i;
	return result;
}