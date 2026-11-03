#pragma once


#include <Windows.h>
#include <ffx_api.h>

class FSRManager
{
public:
	bool Load();
	void Unload();

	bool IsLoaded() const;

private:
	HMODULE loader = nullptr;
	PfnFfxCreateContext createContext = nullptr;
	PfnFfxDestroyContext destroyContext = nullptr;
	PfnFfxDispatch dispatch = nullptr;
	PfnFfxQuery query = nullptr;
	PfnFfxConfigure configure = nullptr;
};