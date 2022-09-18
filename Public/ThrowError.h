#pragma once
#include "..\Public\headers.h"

std::wstring AnsiToWString(const std::string& str);

class HrException
{
public:
	HrException() = default;
	HrException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber);
	std::wstring ToString()const;

	HRESULT ErrorCode = S_OK;
	std::wstring FunctionName;
	std::wstring Filename;
	int LineNumber = -1;
};
