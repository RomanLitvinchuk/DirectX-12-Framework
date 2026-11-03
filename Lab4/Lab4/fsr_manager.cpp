#include "fsr_manager.h"

bool FSRManager::IsLoaded() const
{
	return loader != nullptr &&
		   createContext != nullptr &&
		   destroyContext != nullptr && 
		   dispatch != nullptr &&
		   query != nullptr &&
		   configure != nullptr;
}

bool FSRManager::Load()
{
	loader = LoadLibraryW(L"FSR\\amd_fidelityfx_loader_dx12.dll");
	if (!loader)
	{
		return false;
	}

	createContext = reinterpret_cast<PfnFfxCreateContext>(GetProcAddress(loader, "ffxCreateContext"));
	destroyContext = reinterpret_cast<PfnFfxDestroyContext>(GetProcAddress(loader, "ffxDestroyContext"));
	dispatch = reinterpret_cast<PfnFfxDispatch>(GetProcAddress(loader, "ffxDispatch"));
	query = reinterpret_cast<PfnFfxQuery>(GetProcAddress(loader, "ffxQuery"));
	configure = reinterpret_cast<PfnFfxConfigure>(GetProcAddress(loader, "ffxConfigure"));

	if (!IsLoaded())
	{
		Unload();
		return false;
	}

	return true;
}

void FSRManager::Unload()
{
	if (loader)
	{
		FreeLibrary(loader);
		loader = nullptr;
	}
	createContext = nullptr;
	destroyContext = nullptr;
	dispatch = nullptr;
	query = nullptr;
	configure = nullptr;
}