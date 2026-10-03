#pragma once
#include <Windows.h>

class GameTimer;

class WindowClass 
{
private:
	HINSTANCE hInstance_;
	HINSTANCE hPrevInstance_;
	WNDCLASSEX wc_;
	HWND hWnd_ = nullptr;
	RAWINPUTDEVICE rid_[2];
public:
	WindowClass(HINSTANCE hInstance, HINSTANCE hPrevInstance) : hInstance_(hInstance), hPrevInstance_(hPrevInstance) {}
	void initWindow(WNDPROC WndProc);
	void CreateWnd(int windowWidth, int windowHeight);
	void ShowWnd();
	void UpdateWnd();
	bool CheckRegister();
	void RegisterRawInputDevice();
	int WRun(GameTimer* gt);
	bool CheckCreation();
	HWND getHWND() const { return hWnd_; }
};
