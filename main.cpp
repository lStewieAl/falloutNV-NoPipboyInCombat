#include "nvse/nvse/PluginAPI.h"
#include "nvse/nvse/nvse_version.h"
#include "nvse/nvse/SafeWrite.h"
#include "GameObjects.h"

void patchOpenPipboy();
bool versionCheck(const NVSEInterface* nvse);

extern "C" {

	BOOL WINAPI DllMain(HANDLE hDllHandle, DWORD dwReason, LPVOID lpreserved) {
		return TRUE;
	}

	bool NVSEPlugin_Query(const NVSEInterface *nvse, PluginInfo *info) {
		/* fill out the info structure */
		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "No Pipboy In Combat";
		info->version = 1;

		return versionCheck(nvse);
	}

	bool NVSEPlugin_Load(const NVSEInterface *nvse) {
		patchOpenPipboy();
		return true;
	}

};

bool playerInDanger() {
	PlayerCharacter* g_thePlayer = *(PlayerCharacter**)0x11DEA3C;
	return g_thePlayer->pcInCombat && !g_thePlayer->pcUnseen;
}

__declspec(naked) void hookCheckDanger() {
	static const UInt32 retnAddr = 0x0096766F;
	if (playerInDanger()) {
		QueueUIMessage("You cannot use your pipboy in combat!", 3, NULL, NULL, 1.0F, false);
		_asm {
			mov edx, [ebp - 0x2C]
			jmp  retnAddr
		}
	}
	else {
		_asm {
			mov edx, [ebp - 0x2C]
			mov byte ptr[edx + 0x00000D54], 01
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
