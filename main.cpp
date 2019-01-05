#include "nvse/nvse/PluginAPI.h"
#include "nvse/nvse/nvse_version.h"
#include "nvse/nvse/SafeWrite.h"
#include "GameObjects.h"

void patchOpenPipboy();
bool versionCheck(const NVSEInterface* nvse);
void handleIniOptions();

HMODULE stewieTweakHandle;
int g_bShowMessage = 1;
int g_bPipboyUseActionPoints = 0;
int g_iPipboyAPCost = 50;

/* credits to JazzIsParis */
enum ActorValueCode
{
	kAVCode_Aggression,
	kAVCode_Confidence,
	kAVCode_Energy,
	kAVCode_Responsibility,
	kAVCode_Mood,
	kAVCode_Strength,
	kAVCode_Perception,
	kAVCode_Endurance,
	kAVCode_Charisma,
	kAVCode_Intelligence,
	kAVCode_Agility,
	kAVCode_Luck,
	kAVCode_ActionPoints,
	kAVCode_CarryWeight,
	kAVCode_CritChance,
	kAVCode_HealRate,
	kAVCode_Health,
	kAVCode_MeleeDamage,
	kAVCode_DamageResist,
	kAVCode_PoisonResist,
	kAVCode_RadResist,
	kAVCode_SpeedMult,
	kAVCode_Fatigue,
	kAVCode_Karma,
	kAVCode_XP,
	kAVCode_PerceptionCondition,
	kAVCode_EnduranceCondition,
	kAVCode_LeftAttackCondition,
	kAVCode_RightAttackCondition,
	kAVCode_LeftMobilityCondition,
	kAVCode_RightMobilityCondition,
	kAVCode_BrainCondition,
	kAVCode_Barter,
	kAVCode_BigGuns,
	kAVCode_EnergyWeapons,
	kAVCode_Explosives,
	kAVCode_Lockpick,
	kAVCode_Medicine,
	kAVCode_MeleeWeapons,
	kAVCode_Repair,
	kAVCode_Science,
	kAVCode_Guns,
	kAVCode_Sneak,
	kAVCode_Speech,
	kAVCode_Survival,
	kAVCode_Unarmed,
	kAVCode_InventoryWeight,
	kAVCode_Paralysis,
	kAVCode_Invisibility,
	kAVCode_Chameleon,
	kAVCode_NightEye,
	kAVCode_Turbo,
	kAVCode_FireResist,
	kAVCode_WaterBreathing,
	kAVCode_RadiationRads,
	kAVCode_BloodyMess,
	kAVCode_UnarmedDamage,
	kAVCode_Assistance,
	kAVCode_ElectricResist,
	kAVCode_FrostResist,
	kAVCode_EnergyResist,
	kAVCode_EmpResist,
	kAVCode_Variable01,
	kAVCode_Variable02,
	kAVCode_Variable03,
	kAVCode_Variable04,
	kAVCode_Variable05,
	kAVCode_Variable06,
	kAVCode_Variable07,
	kAVCode_Variable08,
	kAVCode_Variable09,
	kAVCode_Variable10,
	kAVCode_IgnoreCrippledLimbs,
	kAVCode_Dehydration,
	kAVCode_Hunger,
	kAVCode_SleepDeprivation,
	kAVCode_DamageThreshold,
};


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
	g_bPipboyUseActionPoints = GetPrivateProfileIntA("Main", "bAllowPipboyUsingActionPoints", 0, filename);
	g_iPipboyAPCost = GetPrivateProfileIntA("Main", "bPipboyAPCost", 50, filename);
}

bool __stdcall playerInDanger() {
	PlayerCharacter* g_thePlayer = *(PlayerCharacter**)0x11DEA3C;
	return g_thePlayer->pcInCombat && !g_thePlayer->pcUnseen;
}

/* credits to JazzIsParis */
__declspec(naked) float GetPCActionPoints() {
	static const UInt32 kAddr_GetActorValue = 0x66EF50;
	static PlayerCharacter* g_thePlayer = *(PlayerCharacter**)0x11DEA3C;
	__asm
	{
		push	kAVCode_ActionPoints
		mov		ecx, g_thePlayer
		add		ecx, 0xA4
		mov		eax, kAddr_GetActorValue
		call	eax
		retn
	}
}


bool __stdcall tryDecreaseAP() {
	PlayerCharacter* g_thePlayer = *(PlayerCharacter**)0x11DEA3C;
	if (GetPCActionPoints() >= g_iPipboyAPCost) {
		g_thePlayer->DamageActorValue(kAVCode_ActionPoints, -g_iPipboyAPCost, NULL);
		return true;
	}
	return false;
}

__declspec(naked) void hookCheckTabKey() {
	static const UInt32 retnAddr = 0x70E913;
	static const UInt32 skipAddr = 0x70E91D;

	if (playerInDanger() && !(g_bPipboyUseActionPoints && tryDecreaseAP())) {
		if (g_bShowMessage) {
			if (g_bPipboyUseActionPoints) QueueUIMessage("Not enough AP to use pipboy in combat!", pain, NULL, NULL, 1.0F, false);
			else QueueUIMessage("You cannot use your PipBoy in combat!", pain, NULL, NULL, 1.0F, false);
		}
		_asm {
			jmp  skipAddr
		}
	}
	else {
		_asm {
		originalcode:
			push 00
			push 00
			mov ecx, [ebp - 0xCC]
			jmp retnAddr
		}
	}	
}

__declspec(naked) void hookCheckFunctionKeys() {
	static const UInt32 retnAddr = 0x70EA2E;
	static const UInt32 skipAddr = 0x70EA39;

	if (playerInDanger() && !(g_bPipboyUseActionPoints && tryDecreaseAP())) {
		if (g_bShowMessage) {
			if(g_bPipboyUseActionPoints) QueueUIMessage("Not enough AP to use pipboy in combat!", pain, NULL, NULL, 1.0F, false);
			else QueueUIMessage("You cannot use your PipBoy in combat!", pain, NULL, NULL, 1.0F, false);
		}
		_asm {
			jmp skipAddr 
		}
	}
	else {
		_asm {
		originalcode:
			mov ecx, [ebp - 0x28]
			push ecx
			push 00
			jmp retnAddr 
		}
	}
}

void patchOpenPipboy() {
	WriteRelJump(0x70E909, (UInt32)hookCheckTabKey);
	WriteRelJump(0x70EA28, (UInt32)hookCheckFunctionKeys);
}

bool versionCheck(const NVSEInterface* nvse) {
	if (nvse->isEditor) return false;
	if (nvse->nvseVersion < NVSE_VERSION_INTEGER) {
		_ERROR("NVSE version too old (got %08X expected at least %08X)", nvse->nvseVersion, NVSE_VERSION_INTEGER);
		return false;
	}
	return true;
}
