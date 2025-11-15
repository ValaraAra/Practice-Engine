#include "app.h"

int main()
{
	App app;
	app.run();

	return 0;
}

#ifdef _WIN32
#include <windows.h>

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
	return main();
}
#endif