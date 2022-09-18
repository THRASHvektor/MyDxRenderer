#include "..\Public\Initialize.h"
#include "..\Public\headers.h"

using namespace Microsoft::WRL;



int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR pCmdLine, int nCmdShow)
{
	try
	{
		if (!Init(hInstance, nCmdShow))
			return 1;
		return Run();
	}
	catch (HrException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR FAILED", MB_OK);
		return 0;
	}

}