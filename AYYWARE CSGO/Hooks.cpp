#include "Hooks.h"
#include "Hacks.h"
#include "Chams.h"
#include "Menu.h"
#include "CBulletListener.h"
#include "Interfaces.h"
#include "RenderManager.h"
#include "MiscHacks.h"
#include "CRC32.h"
#include "Resolver.h"
#include "hitmarker.h"
#include <intrin.h>
#include "DamageIndicator.h"


#define MakePtr(cast, ptr, addValue) (cast)( (DWORD)(ptr) + (DWORD)(addValue))

#ifdef NDEBUG
#define strenc( s ) std::string( cx_make_encrypted_string( s ) )
#define charenc( s ) strenc( s ).c_str()
#define wstrenc( s ) std::wstring( strenc( s ).begin(), strenc( s ).end() )
#define wcharenc( s ) wstrenc( s ).c_str()
#else
#define strenc( s ) ( s )
#define charenc( s ) ( s )
#define wstrenc( s ) ( s )
#define wcharenc( s ) ( s )
#endif

#ifdef NDEBUG
#define XorStr( s ) ( XorCompileTime::XorString< sizeof( s ) - 1, __COUNTER__ >( s, std::make_index_sequence< sizeof( s ) - 1>() ).decrypt() )
#else
#define XorStr( s ) ( s )
#endif
std::vector<trace_info> trace_logs;
int currentfov;
Vector LastAngleAA;
Vector LastAngleAAFake;
bool Resolver::didhitHS;
CUserCmd* Globals::UserCmd;
IClientEntity* Globals::Target;
int Globals::Shots;
bool Globals::change;
int Globals::TargetID;
bool Globals::Up2date;
std::map<int, QAngle>Globals::storedshit;
int Globals::missedshots;
static int missedLogHits[65];
float fakeangle;
typedef void(__thiscall* DrawModelEx_)(void*, void*, void*, const ModelRenderInfo_t&, matrix3x4*);
typedef void(__thiscall* PaintTraverse_)(PVOID, unsigned int, bool, bool);
typedef bool(__thiscall* InPrediction_)(PVOID);
typedef void(__stdcall *FrameStageNotifyFn)(ClientFrameStage_t);
typedef bool(__thiscall *FireEventClientSideFn)(PVOID, IGameEvent*);
typedef void(__thiscall* RenderViewFn)(void*, CViewSetup&, CViewSetup&, int, int);
using OverrideViewFn = void(__fastcall*)(void*, void*, CViewSetup*);
typedef float(__stdcall *oGetViewModelFOV)();
typedef void(__thiscall *SceneEnd_t)(void *pEcx);

SceneEnd_t pSceneEnd;
PaintTraverse_ oPaintTraverse;
DrawModelEx_ oDrawModelExecute;
FrameStageNotifyFn oFrameStageNotify;
OverrideViewFn oOverrideView;
FireEventClientSideFn oFireEventClientSide;
RenderViewFn oRenderView;


void __fastcall PaintTraverse_Hooked(PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce);
bool __stdcall Hooked_InPrediction();
bool __fastcall Hooked_FireEventClientSide(PVOID ECX, PVOID EDX, IGameEvent *Event);
void __fastcall Hooked_DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t &pInfo, matrix3x4 *pCustomBoneToWorld);
bool __stdcall CreateMoveClient_Hooked(float frametime, CUserCmd* pCmd);
void  __stdcall Hooked_FrameStageNotify(ClientFrameStage_t curStage);
void __fastcall Hooked_OverrideView(void* ecx, void* edx, CViewSetup* pSetup);
float __stdcall GGetViewModelFOV();
void __fastcall Hooked_RenderView(void* ecx, void* edx, CViewSetup &setup, CViewSetup &hudViewSetup, int nClearFlags, int whatToDraw);
void __fastcall	hkSceneEnd(void *pEcx, void *pEdx);



namespace Hooks
{
	Utilities::Memory::VMTManager VMTPanel; 
	Utilities::Memory::VMTManager VMTClient;
	Utilities::Memory::VMTManager VMTClientMode;
	Utilities::Memory::VMTManager VMTModelRender; 
	Utilities::Memory::VMTManager VMTPrediction; 
	Utilities::Memory::VMTManager VMTRenderView;
	Utilities::Memory::VMTManager VMTEventManager;
};

void Hooks::UndoHooks()
{
	VMTPanel.RestoreOriginal();
	VMTPrediction.RestoreOriginal();
	VMTModelRender.RestoreOriginal();
	VMTClientMode.RestoreOriginal();
}

void Hooks::Initialise()
{
	Interfaces::Engine->ExecuteClientCmd("clear");
	Interfaces::CVar->ConsoleColorPrintf(Color(0, 255, 0, 255), ("\n PhantomWare Injected! \n"));
	Interfaces::CVar->ConsoleColorPrintf(Color(0, 255, 255, 255), ("\n Date : %s \n"), __DATE__);

	VMTPanel.Initialise((DWORD*)Interfaces::Panels);
	oPaintTraverse = (PaintTraverse_)VMTPanel.HookMethod((DWORD)&PaintTraverse_Hooked, Offsets::VMT::Panel_PaintTraverse);

	VMTPrediction.Initialise((DWORD*)Interfaces::Prediction);
	VMTPrediction.HookMethod((DWORD)&Hooked_InPrediction, 14);

	VMTModelRender.Initialise((DWORD*)Interfaces::ModelRender);
	oDrawModelExecute = (DrawModelEx_)VMTModelRender.HookMethod((DWORD)&Hooked_DrawModelExecute, Offsets::VMT::ModelRender_DrawModelExecute);
	
	VMTClientMode.Initialise((DWORD*)Interfaces::ClientMode);
	VMTClientMode.HookMethod((DWORD)CreateMoveClient_Hooked, 24);

	oOverrideView = (OverrideViewFn)VMTClientMode.HookMethod((DWORD)&Hooked_OverrideView, 18);
	VMTClientMode.HookMethod((DWORD)&GGetViewModelFOV, 35);

	VMTClient.Initialise((DWORD*)Interfaces::Client);
	oFrameStageNotify = (FrameStageNotifyFn)VMTClient.HookMethod((DWORD)&Hooked_FrameStageNotify, 36);

	VMTEventManager.Initialise((DWORD*)Interfaces::EventManager);
	oFireEventClientSide = (FireEventClientSideFn)VMTEventManager.HookMethod((DWORD)&Hooked_FireEventClientSide, 9);

	VMTRenderView.Initialise((DWORD*)Interfaces::RenderView);
	pSceneEnd = (SceneEnd_t)VMTRenderView.HookMethod((DWORD)&hkSceneEnd, 9);




}

void MovementCorrection(CUserCmd* pCmd)
{
}

float clip(float n, float lower, float upper)
{
	return (std::max)(lower, (std::min)(n, upper));
}

const char* clantaganimation[21] =
{
	"P ",
	"Ph ",
	"Pha ",
	"Phan ",
	"Phant ",
	"Phanto ",
	"Phantom ",
	"PhantomW ",
	"PhantomWa ",
	"PhantomWar ",
	"PhantomWare ",
	"PhantomWar ",
	"PhantomWa ",
	"PhantomW ",
	"Phantom ",
	"Phanto ",
	"Phant ",
	"Phan ",
	"Pha ",
	"Ph ",
	"P "
};

int LagCompBreak() {
	IClientEntity *pLocalPlayer = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	Vector velocity = pLocalPlayer->GetVelocity();
	velocity.z = 0;
	float speed = velocity.Length();
	if (speed > 0.f) {
		auto distance_per_tick = speed *
			Interfaces::Globals->interval_per_tick;
		int choked_ticks = std::ceilf(65.f / distance_per_tick);
		return std::min<int>(choked_ticks, 14);
	}
	return 1;
}

BYTE bMoveData[0x200];
void Prediction(CUserCmd* pCmd, IClientEntity* LocalPlayer)
{
	if (Interfaces::MoveHelper && Options::Menu.RageBotTab.AimbotEnable.GetState() && LocalPlayer->IsAlive())
	{
		float curtime = Interfaces::Globals->curtime;
		float frametime = Interfaces::Globals->frametime;
		int iFlags = LocalPlayer->GetFlags();

		Interfaces::Globals->curtime = (float)LocalPlayer->GetTickBase() * Interfaces::Globals->interval_per_tick;
		Interfaces::Globals->frametime = Interfaces::Globals->interval_per_tick;

		Interfaces::MoveHelper->SetHost(LocalPlayer);

		Interfaces::Prediction1->SetupMove(LocalPlayer, pCmd, nullptr, bMoveData);
		Interfaces::GameMovement->ProcessMovement(LocalPlayer, bMoveData);
		Interfaces::Prediction1->FinishMove(LocalPlayer, pCmd, bMoveData);

		Interfaces::MoveHelper->SetHost(0);

		Interfaces::Globals->curtime = curtime;
		Interfaces::Globals->frametime = frametime;
		*LocalPlayer->GetPointerFlags() = iFlags;
	}
}

int kek = 0;
int autism = 0;
void NasaExploit(CUserCmd*pCmd)
{
	float manipulation;     float zAngle;     int command;     bool flip;     static int ChokedPackets = -1;     if (ChokedPackets >= 0) 
	{
		if (flip) 
		{   
			//well thoughtout specific number *very important*     manipulation = 1996544567442.176672818763728;     else     //delta z angle manipulation calculation     manipulation = command + manipulation /zAngle;     //make the z angle a jitter constant     zAngle = RandomFloat(-100000, 100000);     //angle xy delta divide by 100 to find the absolute constant     command = (abs(viewangles.y - viewangles.x) / 100)     //set the angles     pCmd->viewangle.z = zAngle + manipulation - command;     //stop for the interval     Sleep(command);     //flip the command     flip = !flip;     ChokedPackets - -;     }     else      ChokedPackets++;     }
		}
	}
}
int speed = 0;
void Moveexploit()
{
	void* baseptr;
	_asm mov baseptr, ebp;

	int i = 4;

	while (i--)
		baseptr = *(void**)baseptr;

	if (speed)
	{
		*((char*)baseptr + 0x4) -= 0x5;
		--speed;
	}
	else
		speed = 10;
}
static float testtimeToTick;
static float testServerTick;
static float testTickCount64 = 1;
bool __stdcall CreateMoveClient_Hooked(float frametime, CUserCmd* pCmd)
{
	if (!pCmd->command_number)
		return true;

	IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && pLocal)
	{

		PVOID pebp;
		__asm mov pebp, ebp;
		bool* pbSendPacket = (bool*)(*(DWORD*)pebp - 0x1C);
		bool& bSendPacket = *pbSendPacket;

		Vector origView = pCmd->viewangles;
		Vector viewforward, viewright, viewup, aimforward, aimright, aimup;
		Vector qAimAngles;
		qAimAngles.Init(0.0f, pCmd->viewangles.y, 0.0f);
		AngleVectors(qAimAngles, &viewforward, &viewright, &viewup);

		IClientEntity* pEntity;
		if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && pLocal && pLocal->IsAlive())
		{
			Hacks::MoveHacks(pCmd, bSendPacket);
			ResolverSetup::GetInst().CM(pEntity);
		}

		//if (Options::Menu.MiscTab.FakeLagEnable.GetState())
			//g_Misc->FakeLag();

		if (Options::Menu.MiscTab.FakeLagEnable.GetState())
		{
			
		}
		/*
		if (Options::Menu.MiscTab.FakeLagEnable.GetState())
		{
			static int ticks = 0;
			static int ticks1 = 0;
			static int iTick = 0;
			static int iTick1 = 0;
			static int iTick2 = 0;
			int ticksMax = 16;
			int value = Options::Menu.MiscTab.FakeLagChoke.GetValue();
			if (Options::Menu.MiscTab.FakeLagEnable.GetState() && value > 0 && Options::Menu.MiscTab.FakeLagTyp.GetIndex() == 2)
			{
				if (ticks >= ticksMax)
				{
					bSendPacket = true;
					ticks = 0;
				}
				else
				{
					int packetsToChoke;
					if (pLocal->GetVelocity().Length() > 0.f)
					{
						packetsToChoke = (int)((128.f / Interfaces::Globals->interval_per_tick) / pLocal->GetVelocity().Length()) + 1;
						if (packetsToChoke >= 15)
							packetsToChoke = 14;
						if (packetsToChoke < value)
							packetsToChoke = value;
					}
					else
						packetsToChoke = 0;

					bSendPacket = ticks < 18 - packetsToChoke;;
				}
				ticks++;
			}

			if (Options::Menu.MiscTab.FakeLagEnable.GetState() && value > 0 && Options::Menu.MiscTab.FakeLagTyp.GetIndex() == 4)
			{
				if (!(pLocal->GetFlags() & FL_ONGROUND))
				{
					if (ticks1 >= ticksMax)
					{
						bSendPacket = true;
						ticks1 = 0;
					}
					else
					{
						int packetsToChoke;
						if (pLocal->GetVelocity().Length() > 0.f)
						{
							packetsToChoke = (int)((128.f / Interfaces::Globals->interval_per_tick) / pLocal->GetVelocity().Length()) + 1;
							if (packetsToChoke >= 15)
								packetsToChoke = 14;
							if (packetsToChoke < value)
								packetsToChoke = value;
						}
						else
							packetsToChoke = 0;

						bSendPacket = ticks1 < 18 - packetsToChoke;;
					}
					ticks1++;
				}
			}
			if (Options::Menu.MiscTab.FakeLagEnable.GetState() && value > 0 && Options::Menu.MiscTab.FakeLagTyp.GetIndex() == 1)
			{

				if (iTick < value) {
					bSendPacket = false;
					iTick++;
				}
				else {
					bSendPacket = true;
					iTick = 0;
				}
			}
			if (Options::Menu.MiscTab.FakeLagEnable.GetState() && value > 0 && Options::Menu.MiscTab.FakeLagTyp.GetIndex() == 3)
			{
				if (!(pLocal->GetFlags() & FL_ONGROUND))
				{
					if (iTick1 < value) {
						bSendPacket = false;
						iTick1++;
					}
					else {
						bSendPacket = true;
						iTick1 = 0;
					}
				}
			}
			if (Options::Menu.MiscTab.FakeLagEnable.GetState() && value > 0 && Options::Menu.MiscTab.FakeLagTyp.GetIndex() == 5)
			{
				value = LagCompBreak();
				if (iTick2 < value) {
					bSendPacket = false;
					iTick2++;
				}
				else {
					bSendPacket = true;
					iTick2 = 0;
				}
			}
		}
		*/


		if (Interfaces::Engine->IsInGame() && Interfaces::Engine->IsConnected())
		{
			static auto SetClanTag = reinterpret_cast<void(__fastcall*)(const char*, const char*)>(((DWORD)(Utilities::Memory::FindPatternV2("engine.dll", "53 56 57 8B DA 8B F9 FF 15"))));
			if (Options::Menu.MiscTab.ClanTag.GetState())
			{
				static size_t lastTime = 0;

				if (GetTickCount() > lastTime)
				{
					kek++;
					if (kek > 10)
					{
						autism = autism + 1;

						if (autism >= 21) // number of clantaganimation
							autism = 0;

						char random[255];
						SetClanTag(clantaganimation[autism], clantaganimation[autism]);
						lastTime = GetTickCount() + 500;
					}

					if (kek > 10)
						kek = 0;
				}
			}
		}
#pragma region Timer4LBY
		static float myOldLby;
		static float myoldTime;
		testtimeToTick = TIME_TO_TICKS(0.1);
		testServerTick = TIME_TO_TICKS(1);
		static int timerino;
		static float oneTickMinues;

		if (testServerTick == 128) {
			oneTickMinues = testServerTick / 128;
		}
		else {
			oneTickMinues = testServerTick / 64;
		}
		//IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());


#pragma endregion
		IClientEntity* LocalPlayer = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
		float flServerTime = (float)(LocalPlayer->GetTickBase()  * Interfaces::Globals->interval_per_tick);
		static float next_time = 0;
		if (!bSendPacket && LocalPlayer->IsAlive() && LocalPlayer->GetVelocity().Length2D() == 0) {

			float TickStuff = TICKS_TO_TIME(LocalPlayer->GetTickBase());
			Globals::Up2date = false;
			//flServerTime = next_time;

			if (next_time - TICKS_TO_TIME(LocalPlayer->GetTickBase()) > 1.1)
			{
				next_time = 0;
			}

			if (TickStuff > next_time + 1.1f)
			{
				next_time = TickStuff + TICKS_TO_TIME(1);
				Globals::Up2date = true;
			}
		}
		if (Options::Menu.VisualsTab.NightSky.GetState())
		{
			ConVar* sky = Interfaces::CVar->FindVar("sv_skyname");
			SpoofedConvar* sky_spoofed = new SpoofedConvar(sky);
			sky_spoofed->SetString("sky_csgo_night02");
		}

		qAimAngles.Init(0.0f, GetAutostrafeView().y, 0.0f); 
		AngleVectors(qAimAngles, &viewforward, &viewright, &viewup);
		qAimAngles.Init(0.0f, pCmd->viewangles.y, 0.0f);
		AngleVectors(qAimAngles, &aimforward, &aimright, &aimup);
		Vector vForwardNorm;		Normalize(viewforward, vForwardNorm);
		Vector vRightNorm;			Normalize(viewright, vRightNorm);
		Vector vUpNorm;				Normalize(viewup, vUpNorm);

		float forward = pCmd->forwardmove;
		float right = pCmd->sidemove;
		float up = pCmd->upmove;
		if (forward > 450) forward = 450;
		if (right > 450) right = 450;
		if (up > 450) up = 450;
		if (forward < -450) forward = -450;
		if (right < -450) right = -450;
		if (up < -450) up = -450;
		pCmd->forwardmove = DotProduct(forward * vForwardNorm, aimforward) + DotProduct(right * vRightNorm, aimforward) + DotProduct(up * vUpNorm, aimforward);
		pCmd->sidemove = DotProduct(forward * vForwardNorm, aimright) + DotProduct(right * vRightNorm, aimright) + DotProduct(up * vUpNorm, aimright);
		pCmd->upmove = DotProduct(forward * vForwardNorm, aimup) + DotProduct(right * vRightNorm, aimup) + DotProduct(up * vUpNorm, aimup);

		if (Options::Menu.MiscTab.OtherSafeMode.GetState())
		{
			GameUtils::NormaliseViewAngle(pCmd->viewangles);

			if (pCmd->viewangles.z != 0.0f)
			{
				pCmd->viewangles.z = 0.00;
			}

			if (pCmd->viewangles.x < -89 || pCmd->viewangles.x > 89 || pCmd->viewangles.y < -180 || pCmd->viewangles.y > 180)
			{
				Utilities::Log("Having to re-normalise!");
				GameUtils::NormaliseViewAngle(pCmd->viewangles);
				Beep(750, 800); 
				if (pCmd->viewangles.x < -89 || pCmd->viewangles.x > 89 || pCmd->viewangles.y < -180 || pCmd->viewangles.y > 180)
				{
					pCmd->viewangles = origView;
					pCmd->sidemove = right;
					pCmd->forwardmove = forward;
				}
			}
		}

		if (pCmd->viewangles.x > 90)
		{
			pCmd->forwardmove = -pCmd->forwardmove;
		}

		if (pCmd->viewangles.x < -90)
		{
			pCmd->forwardmove = -pCmd->forwardmove;
		}

		if (bSendPacket == true) {
			fakeangle = pCmd->viewangles.y;
		}
		if (bSendPacket)
			LastAngleAA = pCmd->viewangles;

		if (!bSendPacket)
			LastAngleAAFake = pCmd->viewangles;
	}
	return false;
}

std::string GetTimeString()
{
	time_t current_time;
	struct tm *time_info;
	static char timeString[10];
	time(&current_time);
	time_info = localtime(&current_time);
	strftime(timeString, sizeof(timeString), "%X", time_info);
	return timeString;
}

void __fastcall PaintTraverse_Hooked(PVOID pPanels, int edx, unsigned int vguiPanel, bool forceRepaint, bool allowForce)
{
	if (Options::Menu.VisualsTab.Active.GetState() && Options::Menu.VisualsTab.OtherNoScope.GetState() && strcmp("HudZoom", Interfaces::Panels->GetName(vguiPanel)) == 0)
		return;

	static unsigned int FocusOverlayPanel = 0;
	static bool FoundPanel = false;

	if (!FoundPanel)
	{
		PCHAR szPanelName = (PCHAR)Interfaces::Panels->GetName(vguiPanel);
		if (strstr(szPanelName, XorStr("MatSystemTopPanel")))
		{
			FocusOverlayPanel = vguiPanel;
			FoundPanel = true;
		}
	}
	else if (FocusOverlayPanel == vguiPanel)
	{
		if (Options::Menu.VisualsTab.AAIndicators.GetState())
		{
			int W, H, cW, cH;
			Interfaces::Engine->GetScreenSize(W, H);
			cW = W / 2;
			cH = H / 2;

			if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
			{
				if (Options::Menu.RageBotTab.ManualAAEnable.GetState())
				{
					if (GetKeyState(Options::Menu.RageBotTab.ManualAASwitch.GetKey()))
					{
						Render::Text(cW + 34, cH - 20, Color(0, 128, 255, 160), Render::Fonts::LBY, L"▶");
						Render::Text(cW - 50, cH - 20, Color(255, 255, 255, 160), Render::Fonts::LBY, L"◀");
					}
					else
					{
						Render::Text(cW + 34, cH - 20, Color(255, 255, 255, 160), Render::Fonts::LBY, L"▶");
						Render::Text(cW - 50, cH - 20, Color(0, 128, 255, 160), Render::Fonts::LBY, L"◀");
					}
				}
				else
				{
					if (GetKeyState(Options::Menu.RageBotTab.ManualAASwitch.GetKey()))
					{
						Render::Text(cW + 34, cH - 20, Color(255, 255, 255, 160), Render::Fonts::LBY, L"▶");
						Render::Text(cW - 50, cH - 20, Color(255, 255, 255, 160), Render::Fonts::LBY, L"◀");
					}
					else
					{
						Render::Text(cW + 34, cH - 20, Color(255, 255, 255, 160), Render::Fonts::LBY, L"▶");
						Render::Text(cW - 50, cH - 20, Color(255, 255, 255, 160), Render::Fonts::LBY, L"◀");
					}
				}
			}
		}
		/*
		if (Options::Menu.VisualsTab.LBYIndicator.GetState())
		{
			CUserCmd* cmdlist = *(CUserCmd**)((DWORD)Interfaces::pInput + 0xEC);
			CUserCmd* pCmd = cmdlist;
			IClientEntity* pLocal = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
			if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && pLocal->IsAlive())
			{

				RECT TextSize = Render::GetTextSize(Render::Fonts::LBYIndicator, "LBY");
				RECT scrn = Render::GetViewport();

				if (pCmd->viewangles.y - *pLocal->GetLowerBodyYawTarget() >= -35 && pCmd->viewangles.y - *pLocal->GetLowerBodyYawTarget() <= 35)
				{
					Render::Text(10, scrn.bottom - 60, Color(255, 0, 30, 255), Render::Fonts::LBY, "LBY");
				}
				else
				{
					Render::Text(10, scrn.bottom - 60, Color(30, 255, 0, 255), Render::Fonts::LBY, "LBY");
				}
			}
		}
		*/
			if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
				Hacks::DrawHacks();

			Options::DoUIFrame();

			if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && Options::Menu.VisualsTab.OtherHitmarker.GetState())
				hitmarker::singleton()->on_paint();

			if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && Options::Menu.VisualsTab.DamageIndicator.GetState())
				damage_indicators.paint();
	}
	oPaintTraverse(pPanels, vguiPanel, forceRepaint, allowForce);
}

bool __stdcall Hooked_InPrediction()
{
	bool result;
	static InPrediction_ origFunc = (InPrediction_)Hooks::VMTPrediction.GetOriginalFunction(14);
	static DWORD *ecxVal = Interfaces::Prediction;
	result = origFunc(ecxVal);

	if (Options::Menu.VisualsTab.OtherNoVisualRecoil.GetState() && (DWORD)(_ReturnAddress()) == Offsets::Functions::dwCalcPlayerView)
	{
		IClientEntity* pLocalEntity = NULL;

		float* m_LocalViewAngles = NULL;

		__asm
		{
			MOV pLocalEntity, ESI
			MOV m_LocalViewAngles, EBX
		}

		Vector viewPunch = pLocalEntity->localPlayerExclusive()->GetViewPunchAngle();
		Vector aimPunch = pLocalEntity->localPlayerExclusive()->GetAimPunchAngle();

		m_LocalViewAngles[0] -= (viewPunch[0] + (aimPunch[0] * 2 * 0.4499999f));
		m_LocalViewAngles[1] -= (viewPunch[1] + (aimPunch[1] * 2 * 0.4499999f));
		m_LocalViewAngles[2] -= (viewPunch[2] + (aimPunch[2] * 2 * 0.4499999f));
		return true;
	}

	return result;
}

int Kills2 = 0;
int Kills = 0;
bool RoundInfo = false;
size_t Delay = 0;
bool flipAA;
bool __fastcall Hooked_FireEventClientSide(PVOID ECX, PVOID EDX, IGameEvent *Event)
{
	CBulletListener::singleton()->OnStudioRender();


	if (Options::Menu.MiscTab.EnableBuyBot.GetState())
	{
		if (Options::Menu.MiscTab.BuyBot.GetIndex() == 1)
		{
			if (strcmp(Event->GetName(), "round_start") == 0)
				Interfaces::Engine->ClientCmd_Unrestricted("buy ak47; buy m4a1;");
		}
		else if (Options::Menu.MiscTab.BuyBot.GetIndex() == 2)
		{
			if (strcmp(Event->GetName(), "round_start") == 0)
				Interfaces::Engine->ClientCmd_Unrestricted("buy awp;");
		}
		else if (Options::Menu.MiscTab.BuyBot.GetIndex() == 3)
		{
			if (strcmp(Event->GetName(), "round_start") == 0) 
				Interfaces::Engine->ClientCmd_Unrestricted("buy scar20; buy g3sg1;buy elite;");
		}
	}

	if (Options::Menu.MiscTab.EnableBuyBot.GetState())
	{
		if (Options::Menu.MiscTab.BuyBotGrenades.GetIndex() == 1)
		{
			if (strcmp(Event->GetName(), "round_start") == 0)
				Interfaces::Engine->ClientCmd_Unrestricted("buy flashbang; buy flashbang; buy smokegrenade; buy hegrenade;");
		}
		else if (Options::Menu.MiscTab.BuyBotGrenades.GetIndex() == 2)
		{
			if (strcmp(Event->GetName(), "round_start") == 0)
				Interfaces::Engine->ClientCmd_Unrestricted("buy flashbang; buy smokegrenade; buy hegrenade; buy molotov; buy incgrenade;");
		}
	}

	if (Options::Menu.MiscTab.EnableBuyBot.GetState())
	{
		if (Options::Menu.MiscTab.BuyBotKevlar.GetState())
		{
			if (strcmp(Event->GetName(), "round_start") == 0)
				Interfaces::Engine->ClientCmd_Unrestricted("buy vest; buy vesthelm;");
		}
	}

	if (Options::Menu.MiscTab.EnableBuyBot.GetState())
	{
		if (Options::Menu.MiscTab.BuyBotDefuser.GetState())
		{
			if (strcmp(Event->GetName(), "round_start") == 0)
				Interfaces::Engine->ClientCmd_Unrestricted("buy defuser;");
		}
	}
	if (Options::Menu.VisualsTab.DamageIndicator.GetState())
	{
		if (strcmp(Event->GetName(), "player_hurt") == 0)
		{
			IClientEntity* pLocalPlayer = hackManager.pLocal();
			IClientEntity* hurt = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetPlayerForUserID(Event->GetInt("userid")));
			IClientEntity* attacker = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetPlayerForUserID(Event->GetInt("attacker")));

			if (hurt != pLocalPlayer && attacker == pLocalPlayer)
			{
				DamageIndicator_t DmgIndicator;
				DmgIndicator.iDamage = Event->GetInt("dmg_health");
				DmgIndicator.Player = hurt;
				DmgIndicator.flEraseTime = pLocalPlayer->GetTickBase() * Interfaces::Globals->interval_per_tick + 3.f;
				DmgIndicator.bInitialized = false;
				damage_indicators.data.push_back(DmgIndicator);
			}
		}
	}
	if (Options::Menu.RageBotTab.AimbotResolver.GetIndex() == 1)
	{
		if (!strcmp(Event->GetName(), "player_hurt"))
		{
			int deadfag = Event->GetInt("userid");
			int attackingfag = Event->GetInt("attacker");
			IClientEntity* pLocal = hackManager.pLocal();
			if (Interfaces::Engine->GetPlayerForUserID(deadfag) != Interfaces::Engine->GetLocalPlayer() && Interfaces::Engine->GetPlayerForUserID(attackingfag) == Interfaces::Engine->GetLocalPlayer())
			{
				IClientEntity* hittedplayer = (IClientEntity*)(Interfaces::Engine->GetPlayerForUserID(deadfag));
				int hit = Event->GetInt("hitgroup");
				if (hit == 1 && hittedplayer && deadfag && attackingfag)
				{
					Resolver::didhitHS = true;
					Globals::missedshots = 0;
				}
				else
				{
					Resolver::didhitHS = false;
					Globals::missedshots++;
				}
			}
		}
	}
	return oFireEventClientSide(ECX, Event);
}
#define TEXTURE_GROUP_LIGHTMAP                      "Lightmaps"
#define TEXTURE_GROUP_WORLD                         "World textures"
#define TEXTURE_GROUP_MODEL                         "Model textures"
#define TEXTURE_GROUP_VGUI                          "VGUI textures"
#define TEXTURE_GROUP_PARTICLE                      "Particle textures"
#define TEXTURE_GROUP_DECAL                         "Decal textures"
#define TEXTURE_GROUP_SKYBOX                        "SkyBox textures"
#define TEXTURE_GROUP_CLIENT_EFFECTS                "ClientEffect textures"
#define TEXTURE_GROUP_OTHER                         "Other textures"
#define TEXTURE_GROUP_PRECACHED                     "Precached"             // TODO: assign texture groups to the precached materials
#define TEXTURE_GROUP_CUBE_MAP                      "CubeMap textures"
#define TEXTURE_GROUP_RENDER_TARGET                 "RenderTargets"
#define TEXTURE_GROUP_UNACCOUNTED                   "Unaccounted textures"  // Textures that weren't assigned a texture group.
//#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER        "Static Vertex"
#define TEXTURE_GROUP_STATIC_INDEX_BUFFER           "Static Indices"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_DISP     "Displacement Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_COLOR    "Lighting Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_WORLD    "World Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_MODELS   "Model Verts"
#define TEXTURE_GROUP_STATIC_VERTEX_BUFFER_OTHER    "Other Verts"
#define TEXTURE_GROUP_DYNAMIC_INDEX_BUFFER          "Dynamic Indices"
#define TEXTURE_GROUP_DYNAMIC_VERTEX_BUFFER         "Dynamic Verts"
#define TEXTURE_GROUP_DEPTH_BUFFER                  "DepthBuffer"
#define TEXTURE_GROUP_VIEW_MODEL                    "ViewModel"
#define TEXTURE_GROUP_PIXEL_SHADERS                 "Pixel Shaders"
#define TEXTURE_GROUP_VERTEX_SHADERS                "Vertex Shaders"
#define TEXTURE_GROUP_RENDER_TARGET_SURFACE         "RenderTarget Surfaces"
#define TEXTURE_GROUP_MORPH_TARGETS                 "Morph Targets"
void __fastcall  hkSceneEnd(void *pEcx, void *pEdx) {
	Hooks::VMTRenderView.GetMethod<SceneEnd_t>(9)(pEcx);
	pSceneEnd(pEcx);
	static float vis_col[3] = { 0.f, 0.f, 0.f };
	static float hid_col[3] = { 0.f, 0.f, 0.f };
	
	IClientEntity* pLocalPlayer = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	if (Options::Menu.VisualsTab.Chams.GetIndex() == 0)
	{

	}
	else if (Options::Menu.VisualsTab.Chams.GetIndex() == 1 && Options::Menu.VisualsTab.ChamsPlayers.GetState())
	{

		for (int i = 1; i <= Interfaces::Globals->maxClients; ++i) {
			auto ent = Interfaces::EntList->GetClientEntity(i);
			IMaterial *material;
			if (!ent) continue;

			if (pLocalPlayer && (!Options::Menu.VisualsTab.ChamsEnemyOnly.GetState() || ent->GetTeamNum() != pLocalPlayer->GetTeamNum()))
			{

				if (ent)
				{	
					material = Interfaces::MaterialSystem->FindMaterial(("chams_ignorez"), TEXTURE_GROUP_MODEL);
					if (material)
					{

						hid_col[0] = Options::Menu.ColorsTab.ChamsNotVisRed.GetValue() / 255.f;
						hid_col[1] = Options::Menu.ColorsTab.ChamsNotVisGreen.GetValue() / 255.f;
						hid_col[2] = Options::Menu.ColorsTab.ChamsNotVisBlue.GetValue() / 255.f;

						Interfaces::RenderView->SetColorModulation(hid_col);
						Interfaces::ModelRender->ForcedMaterialOverride(material);
						ent->draw_model(1, 255);
					}
					material = Interfaces::MaterialSystem->FindMaterial(("chams"), TEXTURE_GROUP_MODEL);
					if (material)
					{

						vis_col[0] = Options::Menu.ColorsTab.ChamsVisRed.GetValue() / 255.f;
						vis_col[1] = Options::Menu.ColorsTab.ChamsVisGreen.GetValue() / 255.f;
						vis_col[2] = Options::Menu.ColorsTab.ChamsVisBlue.GetValue() / 255.f;

						Interfaces::RenderView->SetColorModulation(vis_col);
						Interfaces::ModelRender->ForcedMaterialOverride(material);
						ent->draw_model(1, 255);
					}

					Interfaces::ModelRender->ForcedMaterialOverride(nullptr);
				}
			}
		}
	}
	else if (Options::Menu.VisualsTab.Chams.GetIndex() == 2 && Options::Menu.VisualsTab.ChamsPlayers.GetState())
	{

		for (auto i = 1; i <= Interfaces::EntList->GetHighestEntityIndex(); ++i)
		{
			auto ent = Interfaces::EntList->GetClientEntity(i);

			if (ent && ent->IsAlive() && ent->IsPlayer()) {
				IMaterial* mat = Interfaces::MaterialSystem->FindMaterial("models/inventory_items/dogtags/dogtags_outline", TEXTURE_GROUP_MODEL);
				if (mat) {


					vis_col[0] = Options::Menu.ColorsTab.ChamsVisRed.GetValue() / 255.f;
					vis_col[1] = Options::Menu.ColorsTab.ChamsVisGreen.GetValue() / 255.f;
					vis_col[2] = Options::Menu.ColorsTab.ChamsVisBlue.GetValue() / 255.f;

					Interfaces::RenderView->SetColorModulation(vis_col);
					Interfaces::ModelRender->ForcedMaterialOverride(mat);
					ent->draw_model(0x1/*STUDIO_RENDER*/, 255);
					Interfaces::ModelRender->ForcedMaterialOverride(nullptr);
				}
			}
		}
	}
	
	
	IClientEntity* pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	if (Options::Menu.VisualsTab.FakeAngleChams.GetIndex() == 0)
	{

	}
	else if (Options::Menu.VisualsTab.FakeAngleChams.GetIndex() == 1)
	{
		if (pLocal)
		{
			static  IMaterial* CoveredLit = CreateMaterial(false, true, false);
			if (CoveredLit)
			{
				Vector OrigAng;
				OrigAng = pLocal->GetEyeAngles();
				pLocal->SetAngle2(Vector(0, fakeangle, 0));

				bool LbyColor = false; // u can make LBY INDICATOR. When LbyColor is true. Color will be Green , if false it will be White
				float NormalColor[3] = { 1, 1, 1 };
				float lbyUpdateColor[3] = { 0, 1, 0 };
				Interfaces::RenderView->SetColorModulation(LbyColor ? lbyUpdateColor : NormalColor);
				Interfaces::ModelRender->ForcedMaterialOverride(CoveredLit);
				pLocal->draw_model(1, 0);
				Interfaces::ModelRender->ForcedMaterialOverride(nullptr);
				pLocal->SetAngle2(OrigAng);
			}
		}
	}
	else if (Options::Menu.VisualsTab.FakeAngleChams.GetIndex() == 2)
	{
		if (pLocal)
		{
			static  IMaterial* CoveredLit = CreateMaterial(false, true, false);
			if (CoveredLit)
			{
				Vector OrigAng;
				OrigAng = pLocal->GetEyeAngles();
				pLocal->SetAngle2(Vector(0, pLocal->GetLowerBodyYaw(), 0));

				bool LbyColor = false; // u can make LBY INDICATOR. When LbyColor is true. Color will be Green , if false it will be White
				float NormalColor[3] = { 1, 1, 1 };
				float lbyUpdateColor[3] = { 0, 1, 0 };
				Interfaces::RenderView->SetColorModulation(LbyColor ? lbyUpdateColor : NormalColor);
				Interfaces::ModelRender->ForcedMaterialOverride(CoveredLit);
				pLocal->draw_model(1, 0);
				Interfaces::ModelRender->ForcedMaterialOverride(nullptr);
				pLocal->SetAngle2(OrigAng);
			}
		}
	}
}


void Hooks::DrawBeamd(Vector src, Vector end, Color color)
{

	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMPOINTS;
	beamInfo.m_pszModelName = "sprites/physbeam.vmt";
	beamInfo.m_nModelIndex = -1; // will be set by CreateBeamPoints if its -1
	beamInfo.m_flHaloScale = 0.0f;
	beamInfo.m_flLife = 1.5f;
	beamInfo.m_flWidth = 1.0f;
	beamInfo.m_flEndWidth = 1.0f;
	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = 2.0f;
	beamInfo.m_flBrightness = color.a();
	beamInfo.m_flSpeed = 0.2f;
	beamInfo.m_nStartFrame = 0;
	beamInfo.m_flFrameRate = 0.f;
	beamInfo.m_flRed = color.r();
	beamInfo.m_flGreen = color.g();
	beamInfo.m_flBlue = color.b();
	beamInfo.m_nSegments = 2;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = 0;

	/*
	BeamInfo_t beamInfo;
	beamInfo.m_nType = TE_BEAMPOINTS;
	beamInfo.m_pszModelName = "sprites/physbeam.vmt";
	beamInfo.m_nModelIndex = -1; // will be set by CreateBeamPoints if its -1
	beamInfo.m_flHaloScale = 0.0f;
	beamInfo.m_flLife = 1.0f;
	beamInfo.m_flWidth = 1.0f;
	beamInfo.m_flEndWidth = 1.0f;
	beamInfo.m_flFadeLength = 0.0f;
	beamInfo.m_flAmplitude = 0.0f;
	beamInfo.m_flBrightness = color.a();
	beamInfo.m_flSpeed = 0.2f;
	beamInfo.m_nStartFrame = 0;
	beamInfo.m_flFrameRate = 0.f;
	beamInfo.m_flRed = color.r();
	beamInfo.m_flGreen = color.g();
	beamInfo.m_flBlue = color.b();
	beamInfo.m_nSegments = 2;
	beamInfo.m_bRenderable = true;
	beamInfo.m_nFlags = 0;
	*/
	beamInfo.m_vecStart = src;
	beamInfo.m_vecEnd = end;

	Beam_t* myBeam = Interfaces::g_pViewRenderBeams->CreateBeamPoints(beamInfo);

	if (myBeam)
		Interfaces::g_pViewRenderBeams->DrawBeam(myBeam);
}
void __fastcall Hooked_DrawModelExecute(void* thisptr, int edx, void* ctx, void* state, const ModelRenderInfo_t &pInfo, matrix3x4 *pCustomBoneToWorld)
{
	if (!Interfaces::Engine->IsConnected() || !Interfaces::Engine->IsInGame())
		return;

	Color color;
	float flColor[3] = { 0.f };
	static IMaterial* CoveredLit = CreateMaterial(true);
	static IMaterial* OpenLit = CreateMaterial(false);
	static IMaterial* CoveredFlat = CreateMaterial(true, false);
	static IMaterial* OpenFlat = CreateMaterial(false, false);
	bool DontDraw = false;

	const char* ModelName = Interfaces::ModelInfo->GetModelName((model_t*)pInfo.pModel);
	IClientEntity* pModelEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(pInfo.entity_index);
	IClientEntity* pLocal = (IClientEntity*)Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (Options::Menu.VisualsTab.Active.GetState())
	{
		int ChamsStyle = Options::Menu.VisualsTab.OptionsChams.GetIndex();
		int HandsStyle = Options::Menu.VisualsTab.OtherNoHands.GetIndex();
		int WeaponsChams = Options::Menu.VisualsTab.WeaponChams.GetState();
		if (ChamsStyle != 0 && Options::Menu.VisualsTab.FiltersPlayers.GetState() && strstr(ModelName, "models/player"))
		{
			if (pModelEntity == nullptr)
				return;

			if (pLocal && (!Options::Menu.VisualsTab.FiltersEnemiesOnly.GetState() ||
				pModelEntity->GetTeamNum() != pLocal->GetTeamNum()))
			{
				IMaterial *covered = ChamsStyle == 1 ? CoveredLit : CoveredFlat;
				IMaterial *open = ChamsStyle == 1 ? OpenLit : OpenFlat;

				IClientEntity* pModelEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(pInfo.entity_index);
				if (pModelEntity)
				{
					if (pModelEntity == nullptr)
						return;

					IClientEntity *local = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
					if (local)
					{
						if (local == nullptr)
							return;

						if (pModelEntity->IsAlive() && pModelEntity->GetHealth() > 0 && pModelEntity->GetTeamNum() != local->GetTeamNum())
						{
							float alpha = 1.f;

							if (pModelEntity->HasGunGameImmunity())
								alpha = 0.5f;

							if (pModelEntity->GetTeamNum() == 2)
							{
								flColor[0] = 255.f;
								flColor[1] = 255.f;
								flColor[2] = 255.f;
							}
							else
							{
								flColor[0] = 255.f;
								flColor[1] = 255.f;
								flColor[2] = 255.f;
							}

							if (Options::Menu.VisualsTab.ChamsVisibleOnly.GetState())
							{
								Interfaces::RenderView->SetColorModulation(flColor);
								Interfaces::RenderView->SetBlend(1);
								Interfaces::ModelRender->ForcedMaterialOverride(covered);
								oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
							}
							else
							{
								Interfaces::RenderView->SetColorModulation(flColor);
								Interfaces::RenderView->SetBlend(0);
								Interfaces::ModelRender->ForcedMaterialOverride(covered);
								oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
							}

							if (pModelEntity->GetTeamNum() == 2)
							{
								flColor[0] = 255.f;
								flColor[1] = 255.f;
								flColor[2] = 255.f;
							}
							else
							{
								flColor[0] = 255.f;
								flColor[1] = 255.f;
								flColor[2] = 255.f;
							}

							Interfaces::RenderView->SetColorModulation(flColor);
							Interfaces::RenderView->SetBlend(alpha);
							Interfaces::ModelRender->ForcedMaterialOverride(open);
						}
						else
						{
							color.SetColor(255, 255, 255, 255);
							ForceMaterial(color, open);
						}
					}
				}
			}
		}
		else if (HandsStyle != 0 && strstr(ModelName, XorStr("arms")))
		{
			if (HandsStyle == 1)
			{
				DontDraw = true;
			}
			else if (HandsStyle == 2)
			{
				Interfaces::RenderView->SetBlend(0.3);
			}

			else if (HandsStyle == 3)
			{
				flColor[0] = 10.f / 255.f;
				flColor[1] = 10.f / 255.f;
				flColor[2] = 10.f / 255.f;

				Interfaces::RenderView->SetColorModulation(flColor);
				static IMaterial* wire = CreateMaterial(true, false, true);
				IMaterial *handy = wire;
				handy->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true);
				Interfaces::ModelRender->ForcedMaterialOverride(handy);
			}

			else if (HandsStyle == 4)
			{
				IMaterial *covered = ChamsStyle == 1 ? CoveredLit : CoveredFlat;
				IMaterial *open = ChamsStyle == 1 ? OpenLit : OpenFlat;
				if (pLocal)
				{
					if (pLocal->IsAlive())
					{
						int alpha = pLocal->HasGunGameImmunity() ? 150 : 255;

						if (pLocal->GetTeamNum() == 2)
							color.SetColor(240, 30, 35, alpha);
						else
							color.SetColor(63, 72, 205, alpha);

						ForceMaterial(color, covered);
						oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

						if (pLocal->GetTeamNum() == 2)
							color.SetColor(247, 180, 20, alpha);
						else
							color.SetColor(32, 180, 57, alpha);
					}
					else
					{
						color.SetColor(255, 255, 255, 255);
					}

					ForceMaterial(color, open);
				}
			}
			else
			{
				static int counter = 0;
				static float colors[3] = { 1.f, 0.f, 0.f };

				if (colors[counter] >= 1.0f)
				{
					colors[counter] = 1.0f;
					counter += 1;
					if (counter > 2)
						counter = 0;
				}
				else
				{
					int prev = counter - 1;
					if (prev < 0) prev = 2;
					colors[prev] -= 0.05f;
					colors[counter] += 0.05f;
				}

				Interfaces::RenderView->SetColorModulation(colors);
				Interfaces::RenderView->SetBlend(0.5);
				Interfaces::ModelRender->ForcedMaterialOverride(OpenLit);
			}
		}
		else if (WeaponsChams != 0 && strstr(ModelName, "models/weapons/v_") && !strstr(ModelName, "arms"))
		{
			IMaterial *covered = ChamsStyle == 1 ? CoveredLit : CoveredFlat;
			IMaterial *open = ChamsStyle == 1 ? OpenLit : OpenFlat;
			if (pLocal)
			{
				if (pLocal->IsAlive())
				{
					int alpha = pLocal->HasGunGameImmunity() ? 150 : 255;

					if (pLocal->GetTeamNum() == 2)
						color.SetColor(220, 220, 220, alpha);
					else
						color.SetColor(220, 220, 220, alpha);

					ForceMaterial(color, covered);
					oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);

					if (pLocal->GetTeamNum() == 2)
						color.SetColor(220, 220, 220, alpha);
					else
						color.SetColor(220, 220, 220, alpha);
				}
				else
				{
					color.SetColor(255, 255, 255, 255);
				}

				ForceMaterial(color, open);
			}
		}
	}

	if (!DontDraw)
		oDrawModelExecute(thisptr, ctx, state, pInfo, pCustomBoneToWorld);
	Interfaces::ModelRender->ForcedMaterialOverride(NULL);
}


int RandomInt(int min, int max)
{
	return rand() % max + min;
}

bool bGlovesNeedUpdate;
void ApplyCustomGloves(IClientEntity* pLocal)
{
	if (Options::Menu.ColorsTab.EnableGloves.GetState())
	{
		if (!Interfaces::Engine->IsConnected() || !Interfaces::Engine->IsInGame())
			return;

		if (bGlovesNeedUpdate || !pLocal->IsAlive())
		{
			DWORD* hMyWearables = (DWORD*)((size_t)pLocal + 0x2EF4);

			if (!Interfaces::EntList->GetClientEntity(hMyWearables[0] & 0xFFF))
			{
				for (ClientClass* pClass = Interfaces::Client->GetAllClasses(); pClass; pClass = pClass->m_pNext)
				{
					if (pClass->m_ClassID != (int)CSGOClassID::CEconWearable)
						continue;

					int iEntry = (Interfaces::EntList->GetHighestEntityIndex() + 1);
					int	iSerial = RandomInt(0x0, 0xFFF);

					pClass->m_pCreateFn(iEntry, iSerial);
					hMyWearables[0] = iEntry | (iSerial << 16);

					break;
				}
			}

			player_info_t LocalPlayerInfo;
			Interfaces::Engine->GetPlayerInfo(Interfaces::Engine->GetLocalPlayer(), &LocalPlayerInfo);

			C_BaseCombatWeapon* glovestochange = (C_BaseCombatWeapon*)Interfaces::EntList->GetClientEntity(hMyWearables[0] & 0xFFF);

			if (!glovestochange)
				return;

			switch (Options::Menu.ColorsTab.GloveModel.GetIndex())
			{
			case 1:
			{
				*glovestochange->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 5027;
				glovestochange->SetModelIndex(Interfaces::ModelInfo->GetModelIndex("models/weapons/v_models/arms/glove_bloodhound/v_glove_bloodhound.mdl"));
				break;
			}
			case 2:
			{
				*glovestochange->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 5032;
				glovestochange->SetModelIndex(Interfaces::ModelInfo->GetModelIndex("models/weapons/v_models/arms/glove_handwrap_leathery/v_glove_handwrap_leathery.mdl"));
				break;
			}
			case 3:
			{
				*glovestochange->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 5031;
				glovestochange->SetModelIndex(Interfaces::ModelInfo->GetModelIndex("models/weapons/v_models/arms/glove_slick/v_glove_slick.mdl"));
				break;
			}
			case 4:
			{
				*glovestochange->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 5030;
				glovestochange->SetModelIndex(Interfaces::ModelInfo->GetModelIndex("models/weapons/v_models/arms/glove_sporty/v_glove_sporty.mdl"));
				break;
			}
			case 5:
			{
				*glovestochange->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 5033;
				glovestochange->SetModelIndex(Interfaces::ModelInfo->GetModelIndex("models/weapons/v_models/arms/glove_motorcycle/v_glove_motorcycle.mdl"));
				break;
			}
			case 6:
			{
				*glovestochange->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 5034;
				glovestochange->SetModelIndex(Interfaces::ModelInfo->GetModelIndex("models/weapons/v_models/arms/glove_specialist/v_glove_specialist.mdl"));
				break;
			}
			default:
				break;
			}

			switch (Options::Menu.ColorsTab.GloveSkin.GetIndex())
			{
			case 0:
				break;
			case 1:
				*glovestochange->FallbackPaintKit() = 10007;
				break;
			case 2:
				*glovestochange->FallbackPaintKit() = 10006;
				break;
			case 3:
				*glovestochange->FallbackPaintKit() = 10039;
				break;
			case 4:
				*glovestochange->FallbackPaintKit() = 10008;
				break;
			case 5:
				*glovestochange->FallbackPaintKit() = 10021;
				break;
			case 6:
				*glovestochange->FallbackPaintKit() = 10036;
				break;
			case 7:
				*glovestochange->FallbackPaintKit() = 10009;
				break;
			case 8:
				*glovestochange->FallbackPaintKit() = 10010;
				break;
			case 9:
				*glovestochange->FallbackPaintKit() = 10016;
				break;
			case 10:
				*glovestochange->FallbackPaintKit() = 10013;
				break;
			case 11:
				*glovestochange->FallbackPaintKit() = 10040;
				break;
			case 12:
				*glovestochange->FallbackPaintKit() = 10015;
				break;
			case 13:
				*glovestochange->FallbackPaintKit() = 10037;
				break;
			case 14:
				*glovestochange->FallbackPaintKit() = 10038;
				break;
			case 15:
				*glovestochange->FallbackPaintKit() = 10018;
				break;
			case 16:
				*glovestochange->FallbackPaintKit() = 10019;
				break;
			case 17:
				*glovestochange->FallbackPaintKit() = 10026;
				break;
			case 18:
				*glovestochange->FallbackPaintKit() = 10028;
				break;
			case 19:
				*glovestochange->FallbackPaintKit() = 10027;
				break;
			case 20:
				*glovestochange->FallbackPaintKit() = 10024;
				break;
			case 21:
				*glovestochange->FallbackPaintKit() = 10033;
				break;
			case 22:
				*glovestochange->FallbackPaintKit() = 10034;
				break;
			case 23:
				*glovestochange->FallbackPaintKit() = 10035;
				break;
			case 24:
				*glovestochange->FallbackPaintKit() = 10030;
				break;
			}

			*glovestochange->m_AttributeManager()->m_Item()->ItemIDHigh() = -1;
			*glovestochange->FallbackWear() = 0.001f;
			*glovestochange->m_AttributeManager()->m_Item()->AccountID() = LocalPlayerInfo.xuidlow;


			glovestochange->PreDataUpdate(0);
			bGlovesNeedUpdate = false;
		}
	}
}

std::vector<const char*> vistasmoke_mats =
{
	"particle/vistasmokev1/vistasmokev1_fire",
	"particle/vistasmokev1/vistasmokev1_smokegrenade",
	"particle/vistasmokev1/vistasmokev1_emods",
	"particle/vistasmokev1/vistasmokev1_emods_impactdust",
};

void  __stdcall Hooked_FrameStageNotify(ClientFrameStage_t curStage)
{
	DWORD eyeangles = NetVar.GetNetVar(0xBFEA4E7B);
	IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	IClientEntity* pEntity = nullptr;


	static auto linegoesthrusmoke = Utilities::Memory::FindPattern("client.dll", (PBYTE)"\x55\x8B\xEC\x83\xEC\x08\x8B\x15\x00\x00\x00\x00\x0F\x57\xC0", "xxxxxxxx????xxx");
	static auto smokecout = *(DWORD*)(linegoesthrusmoke + 0x8);

	if (Options::Menu.VisualsTab.OtherNoSmoke.GetState())
	{
		std::vector<const char*> vistasmoke_mats =
		{
			"particle/vistasmokev1/vistasmokev1_fire",
			"particle/vistasmokev1/vistasmokev1_smokegrenade",
			"particle/vistasmokev1/vistasmokev1_emods",
			"particle/vistasmokev1/vistasmokev1_emods_impactdust",
		};

		for (auto matName : vistasmoke_mats) {
			IMaterial* mat = Interfaces::MaterialSystem->FindMaterial(matName, "Other textures");
			mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true);
		}


		if (Options::Menu.VisualsTab.OtherNoSmoke.GetState()) {
			*(int*)(smokecout) = 0;
		}
	}
	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && curStage == FRAME_RENDER_START)
	{
		

		if (Options::Menu.MiscTab.OtherThirdperson.GetState() || Options::Menu.VisualsTab.OtherNoSmoke.GetState())
		{
			static bool rekt = false;
			if (!rekt)
			{
				ConVar* sv_cheats = Interfaces::CVar->FindVar("sv_cheats");
				SpoofedConvar* sv_cheats_spoofed = new SpoofedConvar(sv_cheats);
				sv_cheats_spoofed->SetInt(1);
				rekt = true;
			}
		}

		if (pLocal->IsAlive() && Options::Menu.MiscTab.OtherThirdperson.GetState())
		{

			Vector thirdpersonMode;

			switch (Options::Menu.VisualsTab.OtherThirdpersonAngle.GetIndex())
			{
			case 0:
				thirdpersonMode = LastAngleAA;
				break;
			case 1:
				thirdpersonMode = LastAngleAAFake;
				break;
			}


			static bool kek = false;

			if (!kek)
			{
				Interfaces::Engine->ClientCmd_Unrestricted("thirdperson");
				kek = true;
			}

			static bool toggleThirdperson;
			static float memeTime;
			int ThirdPersonKey = Options::Menu.MiscTab.ThirdPersonKeyBind.GetKey();
			if (ThirdPersonKey >= 0 && GetAsyncKeyState(ThirdPersonKey) && abs(memeTime - Interfaces::Globals->curtime) > 0.5)
			{
				toggleThirdperson = !toggleThirdperson;
				memeTime = Interfaces::Globals->curtime;
			}


			if (toggleThirdperson)
			{
				Interfaces::pInput->m_fCameraInThirdPerson = true;
				if (*(bool*)((DWORD)Interfaces::pInput + 0xA5))
					*(Vector*)((DWORD)pLocal + 0x31C8) = thirdpersonMode;
			}
			else {
				// No Thirdperson
				static Vector vecAngles;
				Interfaces::Engine->GetViewAngles(vecAngles);
				Interfaces::pInput->m_fCameraInThirdPerson = false;
				Interfaces::pInput->m_vecCameraOffset = Vector(vecAngles.x, vecAngles.y, 0);
			}


		}
		else if (pLocal->IsAlive() == 0)
		{
			kek = false;
			Interfaces::Engine->ClientCmd_Unrestricted("firstperson");

		}

		if (!Options::Menu.MiscTab.OtherThirdperson.GetState()) {

			// No Thirdperson
			static Vector vecAngles;
			Interfaces::Engine->GetViewAngles(vecAngles);
			Interfaces::pInput->m_fCameraInThirdPerson = false;
			Interfaces::pInput->m_vecCameraOffset = Vector(vecAngles.x, vecAngles.y, 0);
		}
		else if (pLocal->GetHealth() <= 0)
		{
			Interfaces::Engine->ClientCmd_Unrestricted("firstperson");
			kek = false;
		}
	

		if (curStage == FRAME_RENDER_START)
		{
			for (int i = 1; i <= Interfaces::Globals->maxClients; i++)
			{
				if (i == Interfaces::Engine->GetLocalPlayer()) continue;

				IClientEntity* pEnt = Interfaces::EntList->GetClientEntity(i);
				if (!pEnt) continue;

				*(int*)((uintptr_t)pEnt + 0xA30) = Interfaces::Globals->framecount; //we'll skip occlusion checks now
				*(int*)((uintptr_t)pEnt + 0xA28) = 0;//clear occlusion flags
			}
			for (auto matName : vistasmoke_mats)
			{
				IMaterial* mat = Interfaces::MaterialSystem->FindMaterial(matName, "Other textures");
				mat->SetMaterialVarFlag(MATERIAL_VAR_WIREFRAME, true);
			}
		}
	}

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame() && curStage == FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
		ResolverSetup::GetInst().FSN(pEntity, curStage);
		int iBayonet = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_bayonet.mdl");
		int iButterfly = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_butterfly.mdl");
		int iFlip = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_flip.mdl");
		int iGut = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_gut.mdl");
		int iKarambit = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_karam.mdl");
		int iM9Bayonet = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_m9_bay.mdl");
		int iHuntsman = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_tactical.mdl");
		int iFalchion = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_falchion_advanced.mdl");
		int iDagger = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_push.mdl");
		int iBowie = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_survival_bowie.mdl");
		int iGunGame = Interfaces::ModelInfo->GetModelIndex("models/weapons/v_knife_gg.mdl");

		for (int i = 0; i <= Interfaces::EntList->GetHighestEntityIndex(); i++)
		{
			IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);

			if (pEntity)
			{
				if (pEntity == nullptr)
					return;

				if (pEntity && pLocal && Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
				ApplyCustomGloves(pLocal);

				
				/*
				ULONG hOwnerEntity = *(PULONG)((DWORD)pEntity + 0x148);

				IClientEntity* pOwner = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)hOwnerEntity);

				if (pOwner)
				{

					if (pOwner == nullptr)
						return;

					if (pOwner == pLocal)
					{
						C_BaseCombatWeapon* pWeapon = (C_BaseCombatWeapon*)pEntity;

						ClientClass *pClass = Interfaces::Client->GetAllClasses();

						
						if (Options::Menu.ColorsTab.SkinEnable.GetState())
						{
							int Model = Options::Menu.ColorsTab.KnifeModel.GetIndex();
							if (pEntity->GetClientClass()->m_ClassID == (int)CSGOClassID::CKnife)
							{
								if (Model == 0) // Bayonet
								{
									*pWeapon->ModelIndex() = iBayonet; 
									*pWeapon->ViewModelIndex() = iBayonet;
									*pWeapon->WorldModelIndex() = iBayonet + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 500;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 1) // Bowie
								{
									*pWeapon->ModelIndex() = iBowie; 
									*pWeapon->ViewModelIndex() = iBowie;
									*pWeapon->WorldModelIndex() = iBowie + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 514;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 2) // Butterfly 
								{
									*pWeapon->ModelIndex() = iButterfly;
									*pWeapon->ViewModelIndex() = iButterfly;
									*pWeapon->WorldModelIndex() = iButterfly + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 515;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 3) // Falchion
								{
									*pWeapon->ModelIndex() = iFalchion;
									*pWeapon->ViewModelIndex() = iFalchion;
									*pWeapon->WorldModelIndex() = iFalchion + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 512;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 4) // Flip Knife
								{
									*pWeapon->ModelIndex() = iFlip;
									*pWeapon->ViewModelIndex() = iFlip;
									*pWeapon->WorldModelIndex() = iFlip + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 505;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 5) // Gut Knife
								{
									*pWeapon->ModelIndex() = iGut; 
									*pWeapon->ViewModelIndex() = iGut;
									*pWeapon->WorldModelIndex() = iGut + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 506;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 6) // Huntsman 
								{
									*pWeapon->ModelIndex() = iHuntsman; 
									*pWeapon->ViewModelIndex() = iHuntsman;
									*pWeapon->WorldModelIndex() = iHuntsman + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 509;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 7) // Karambit
								{
									*pWeapon->ModelIndex() = iKarambit; 
									*pWeapon->ViewModelIndex() = iKarambit;
									*pWeapon->WorldModelIndex() = iKarambit + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 507;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 8) // M9 Bayonet
								{
									*pWeapon->ModelIndex() = iM9Bayonet; 
									*pWeapon->ViewModelIndex() = iM9Bayonet;
									*pWeapon->WorldModelIndex() = iM9Bayonet + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 508;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
								else if (Model == 10) // Shadow Daggers
								{
									*pWeapon->ModelIndex() = iDagger; 
									*pWeapon->ViewModelIndex() = iDagger;
									*pWeapon->WorldModelIndex() = iDagger + 1;
									*pWeapon->m_AttributeManager()->m_Item()->ItemDefinitionIndex() = 516;
									*pWeapon->GetEntityQuality() = 3;
									*pWeapon->FallbackPaintKit() = atoi(Options::Menu.ColorsTab.KnifeSkin.getText().c_str());
								}
							}

							*pWeapon->OwnerXuidLow() = 0;
							*pWeapon->OwnerXuidHigh() = 0;
							*pWeapon->FallbackWear() = 0.001f;
							*pWeapon->m_AttributeManager()->m_Item()->ItemIDHigh() = 1;
						}
						
					}
				}
				*/
			}
		}
	}
	oFrameStageNotify(curStage);
}

void __fastcall Hooked_OverrideView(void* ecx, void* edx, CViewSetup* pSetup)
{
	IClientEntity* localplayer1 = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	if (!localplayer1)
		return;

	if (!Interfaces::Engine->IsConnected())
		return;
	if (!Interfaces::Engine->IsInGame())
		return;

	if (localplayer1)
	{
		if (!localplayer1->IsScoped() && Options::Menu.VisualsTab.OtherFOV.GetValue() > 0)
			pSetup->fov += Options::Menu.VisualsTab.OtherFOV.GetValue();
	}
	currentfov = pSetup->fov;

	oOverrideView(ecx, edx, pSetup);
}

void GetViewModelFOV(float& fov)
{
	IClientEntity* localplayer = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame())
	{

		if (!localplayer)
			return;


		if (Options::Menu.VisualsTab.Active.GetState())
		fov += Options::Menu.VisualsTab.OtherViewmodelFOV.GetValue();
	}
}

float __stdcall GGetViewModelFOV()
{
	float fov = Hooks::VMTClientMode.GetMethod<oGetViewModelFOV>(35)();

	GetViewModelFOV(fov);

	return fov;
}

void __fastcall Hooked_RenderView(void* ecx, void* edx, CViewSetup &setup, CViewSetup &hudViewSetup, int nClearFlags, int whatToDraw)
{
	static DWORD oRenderView = Hooks::VMTRenderView.GetOriginalFunction(6);

	IClientEntity* pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	__asm
	{
		PUSH whatToDraw
		PUSH nClearFlags
		PUSH hudViewSetup
		PUSH setup
		MOV ECX, ecx
		CALL oRenderView
	}
}
