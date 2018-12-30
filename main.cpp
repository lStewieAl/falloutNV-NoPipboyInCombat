#include "nvse/nvse/PluginAPI.h"
#include "nvse/nvse/nvse_version.h"
#include "nvse/nvse/SafeWrite.h"
#include "GameObjects.h"

void patchOpenPipboy();
bool versionCheck(const NVSEInterface* nvse);
void handleIniOptions();

HMODULE stewieTweakHandle;
int g_bShowMessage = 1;

extern "C" {

	BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
		if (dwReason == DLL_PROCESS_ATTACH)
			stewieTweakHandle = (HMODULE)hDllHandle;
		return TRUE;
	}


	bool NVSEPlugin_Query(const NVSEInterface *nvse, PluginInfo *info) {
		/* fill out the info structure */
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "No Pipboy In Combat";
		info->version = 1;

		handleIniOptions();
		return versionCheck(nvse);
	}

	bool NVSEPlugin_Load(const NVSEInterface *nvse) {
		patchOpenPipboy();
		return true;
	}

};

void handleIniOptions() {
	char filename[MAX_PATH];
	GetModuleFileNameA(stewieTweakHandle, filename, MAX_PATH);
	strcpy((char *)(strrchr(filename, '\\') + 1), "nvse_no_pipboy_in_combat.ini");
	g_bShowMessage = GetPrivateProfileIntA("Main", "bShowMessage", 1, filename);
}

bool playerInDanger() {
	PlayerCharacter* g_thePlayer = *(PlayerCharacter**)0x11DEA3C;
	return g_thePlayer->pcInCombat && !g_thePlayer->pcUnseen;
}

__declspec(naked) void hookCheckDanger() {
	static const UInt32 retnAddr = 0x0096766F;
	if (playerInDanger()) {
		if (g_bShowMessage) QueueUIMessage("You cannot use your pipboy in combat!", pain, NULL, NULL, 1.0F, false);
		_asm {
			mov edx, [ebp - 0x2C]
			jmp  retnAddr
		}
	}
	else {
		_asm {
			mov edx, [ebp - 0x2C]
			mov byte ptr[edx + 0xD54], 01
			jmp retnAddr
		}
	}
}

void patchOpenPipboy() {
	WriteRelJump(0x96743F, (UInt32)hookCheckDanger);
}

bool versionCheck(const NVSEInterface* nvse) {
	if (nvse->isEditor) return false;
	if (nvse->nvseVersion < NVSE_VERSION_INTEGER) {
		_ERROR("NVSE version too old (got %08X expected at least %08X)", nvse->nvseVersion, NVSE_VERSION_INTEGER);
		return false;
	}
	return true;
}
