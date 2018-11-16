/**
 * @Author:      joe
 * @Version:     1.0
 * @DateTime:    2018-11-15 23:38:09
 * @Description: ...
 */

#include <windows.h>

int CALLBACK 
WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
	MessageBox(0, "This is Handmade Hero", "Handmade Hero",
			   MB_OK | MB_ICONINFORMATION);
	return 0;
}
