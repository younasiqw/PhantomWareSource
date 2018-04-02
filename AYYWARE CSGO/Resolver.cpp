#include "Resolver.h"
#include "Ragebot.h"
#include "Hooks.h"
#ifdef NDEBUG
#define XorStr( s ) ( XorCompileTime::XorString< sizeof( s ) - 1, __COUNTER__ >( s, std::make_index_sequence< sizeof( s ) - 1>() ).decrypt() )
#else
#define XorStr( s ) ( s )
#endif
float get_average_lby_standing_update_delta(IClientEntity* player) {
	static float last_update_time[64];
	static float second_laste_update_time[64];
	static float oldlowerbody[64];
	float lby = static_cast<int>(fabs(player->GetEyeAnglesPointer()->y - player->GetLowerBodyYaw()));

	if (lby != oldlowerbody[player->GetIndex()]) {
		second_laste_update_time[player->GetIndex()] = last_update_time[player->GetIndex()];
		last_update_time[player->GetIndex()] = Interfaces::Globals->curtime;
		oldlowerbody[player->GetIndex()] = lby;
	}

	return last_update_time[player->GetIndex()] - second_laste_update_time[player->GetIndex()];
}

bool lby_keeps_updating() {
	return get_average_lby_standing_update_delta;
}

void PitchCorrection()
{
	CUserCmd* pCmd;
	for (int i = 0; i < Interfaces::Engine->GetMaxClients(); ++i)
	{
		IClientEntity* pLocal = hackManager.pLocal();
		IClientEntity *player = (IClientEntity*)Interfaces::EntList->GetClientEntity(i);

		if (!player || player->IsDormant() || player->GetHealth() < 1 || (DWORD)player == (DWORD)pLocal)
			continue;

		if (!player)
			continue;

		if (pLocal)
			continue;

		if (pLocal && player && pLocal->IsAlive())
		{
			if (Options::Menu.RageBotTab.AdvancedResolver.GetState())
			{
				Vector* eyeAngles = player->GetEyeAnglesXY();
				if (eyeAngles->x < -179.f) eyeAngles->x += 360.f;
				else if (eyeAngles->x > 90.0 || eyeAngles->x < -90.0) eyeAngles->x = 89.f;
				else if (eyeAngles->x > 89.0 && eyeAngles->x < 91.0) eyeAngles->x -= 90.f;
				else if (eyeAngles->x > 179.0 && eyeAngles->x < 181.0) eyeAngles->x -= 180;
				else if (eyeAngles->x > -179.0 && eyeAngles->x < -181.0) eyeAngles->x += 180;
				else if (fabs(eyeAngles->x) == 0) eyeAngles->x = std::copysign(89.0f, eyeAngles->x);
			}
		}
	}
}
float Bolbilize(float Yaw)
{
	if (Yaw > 180)
	{
		Yaw -= (round(Yaw / 360) * 360.f);
	}
	else if (Yaw < -180)
	{
		Yaw += (round(Yaw / 360) * -360.f);
	}
	return Yaw;
}

float GetCurTime(CUserCmd* ucmd) {
	IClientEntity* local_player = hackManager.pLocal();
	static int g_tick = 0;
	static CUserCmd* g_pLastCmd = nullptr;
	if (!g_pLastCmd || g_pLastCmd->hasbeenpredicted) {
		g_tick = (float)local_player->GetTickBase();
	}
	else {
		// Required because prediction only runs on frames, not ticks
		// So if your framerate goes below tickrate, m_nTickBase won't update every tick
		++g_tick;
	}
	g_pLastCmd = ucmd;
	float curtime = g_tick * Interfaces::Globals->interval_per_tick;
	return curtime;
}

Vector CalcAngle69(Vector dst, Vector src)
{
	Vector angles;

	double delta[3] = { (src.x - dst.x), (src.y - dst.y), (src.z - dst.z) };
	double hyp = sqrt(delta[0] * delta[0] + delta[1] * delta[1]);
	angles.x = (float)(atan(delta[2] / hyp) * 180.0 / 3.14159265);
	angles.y = (float)(atanf(delta[1] / delta[0]) * 57.295779513082f);
	angles.z = 0.0f;

	if (delta[0] >= 0.0)
	{
		angles.y += 180.0f;
	}

	return angles;
}

bool HasFakeHead(IClientEntity* pEntity) {
	//lby should update if distance from lby to eye angles exceeds 35 degrees
	return abs(pEntity->GetEyeAnglesXY()->y - pEntity->GetLowerBodyYaw()) > 35;
}
bool Lbywithin35(IClientEntity* pEntity) {
	//lby should update if distance from lby to eye angles less than 35 degrees
	return abs(pEntity->GetEyeAnglesXY()->y - pEntity->GetLowerBodyYaw()) < 35;
}
bool IsMovingOnGround(IClientEntity* pEntity) {
	//Check if player has a velocity greater than 0 (moving) and if they are onground.
	return pEntity->GetVelocity().Length2D() > 45.f && pEntity->GetFlags() & FL_ONGROUND;
}
bool IsMovingOnInAir(IClientEntity* pEntity) {
	//Check if player has a velocity greater than 0 (moving) and if they are onground.
	return !(pEntity->GetFlags() & FL_ONGROUND);
}
bool OnGround(IClientEntity* pEntity) {
	//Check if player has a velocity greater than 0 (moving) and if they are onground.
	return pEntity->GetFlags() & FL_ONGROUND;
}
bool IsFakeWalking(IClientEntity* pEntity) {
	//Check if a player is moving, but at below a velocity of 36
	return IsMovingOnGround(pEntity) && pEntity->GetVelocity().Length2D() < 36.0f;
}
inline float RandomFloat(float min, float max)
{
	static auto fn = (decltype(&RandomFloat))(GetProcAddress(GetModuleHandle("vstdlib.dll"), "RandomFloat"));
	return fn(min, max);
}
void BETA_LBYBreakerCorrections(IClientEntity* pEntity)
{
	float movinglby[64];
	float lbytomovinglbydelta[64];
	bool onground = pEntity->GetFlags() & FL_ONGROUND;

	if (Options::Menu.RageBotTab.LBYCorrection.GetState())
	{
		lbytomovinglbydelta[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw() - lbytomovinglbydelta[pEntity->GetIndex()];

		if (pEntity->GetVelocity().Length2D() > 6 && pEntity->GetVelocity().Length2D() < 42)
		{
			pEntity->GetEyeAnglesXY()->y = pEntity->GetLowerBodyYaw() + 120;
		}
		else if (pEntity->GetVelocity().Length2D() < 6 || pEntity->GetVelocity().Length2D() > 42) // they are moving
		{
			pEntity->GetEyeAnglesXY()->y = pEntity->GetLowerBodyYaw();
			movinglby[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
		}
		else if (lbytomovinglbydelta[pEntity->GetIndex()] > 50 && lbytomovinglbydelta[pEntity->GetIndex()] < -50 &&
			lbytomovinglbydelta[pEntity->GetIndex()] < 112 && lbytomovinglbydelta[pEntity->GetIndex()] < -112) // the 50 will allow you to have a 30 degree margin of error (do the math :))
		{
			pEntity->GetEyeAnglesXY()->y = movinglby[pEntity->GetIndex()];
		}
		else pEntity->GetEyeAnglesXY()->y = pEntity->GetLowerBodyYaw();
	}
}
void ResolverSetup::Resolve(IClientEntity* pEntity)
{

	float angletolerance;
	angletolerance = pEntity->GetEyeAnglesXY()->y + 180.0;
	float v23; float v24;
	double v20;
	
	std::vector<int> HitBoxesToScan;
	bool MeetsLBYReq;
	if (pEntity->GetFlags() & FL_ONGROUND)
		MeetsLBYReq = true;
	else
		MeetsLBYReq = false;

	bool IsMoving;
	if (pEntity->GetVelocity().Length2D() >= 0.5)
		IsMoving = true;
	else
		IsMoving = false;

	
	static float StoredYaw[16];
	static float StoredVelo[16];
	bool HasMoved;
	bool DoesntMeet;


	ResolverSetup::NewANgles[pEntity->GetIndex()] = *pEntity->GetEyeAnglesXY();
	ResolverSetup::newlby[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
	ResolverSetup::newsimtime = pEntity->GetSimulationTime();
	ResolverSetup::newdelta[pEntity->GetIndex()] = pEntity->GetEyeAnglesXY()->y;
	ResolverSetup::newlbydelta[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
	ResolverSetup::finaldelta[pEntity->GetIndex()] = ResolverSetup::newdelta[pEntity->GetIndex()] - ResolverSetup::storeddelta[pEntity->GetIndex()];
	ResolverSetup::finallbydelta[pEntity->GetIndex()] = ResolverSetup::newlbydelta[pEntity->GetIndex()] - ResolverSetup::storedlbydelta[pEntity->GetIndex()];
	if (newlby == storedlby)
		ResolverSetup::lbyupdated = false;
	else
		ResolverSetup::lbyupdated = true;

	static float time_at_update[65];
	float kevin[64];
	static bool bLowerBodyIsUpdated = false;
	static float LatestLowerBodyYawUpdateTime[55];
	static float LatestLowerBodyYawUpdateTime1[55];
	if (pEntity->GetLowerBodyYaw() != kevin[pEntity->GetIndex()])
		bLowerBodyIsUpdated = true;
	else
		bLowerBodyIsUpdated = false;

	if (pEntity->GetVelocity().Length2D() > 0.1)
	{
		kevin[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();//storing their moving lby for later
		LatestLowerBodyYawUpdateTime[pEntity->GetIndex()] = pEntity->GetSimulationTime() + 0.22;

	}
	IClientEntity *pLocal = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());
	DWORD eyeangles = NetVar.GetNetVar(0xBFEA4E7B);

	for (int i = 0; i < Interfaces::Engine->GetMaxClients(); ++i)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		if (!pEntity || pEntity->IsDormant() || !pEntity->IsAlive())
			continue;

		player_info_t pTemp;
		if (!Interfaces::Engine->GetPlayerInfo(i, &pTemp))
			continue;

		int index = pEntity->GetIndex();
		Vector* eyeAngles = pEntity->GetEyeAnglesXY();

		if (Options::Menu.RageBotTab.AccuracyResolver.GetState())
		{
			static float last_moving_lby[64];
			float interpellslationrationz = Interfaces::CVar->FindVar(XorStr("cl_interp_ratio"))->fValue;
			float updaterlaterrater = Interfaces::CVar->FindVar(XorStr("cl_updaterate"))->fValue;
			int skeet = interpellslationrationz / updaterlaterrater;
			const bool is_moving = pEntity->GetVelocity().Length2D() > 0.1;
			float ismoving = pEntity->GetVelocity().Length2D() > 0.1f;
			float isstanding = pEntity->GetVelocity().Length2D() <= 0.0f;
			static float lastlby[65];
			static float last_moving_time[65];
			float servercalc = Interfaces::Globals->curtime * Interfaces::Globals->tickcount;

			float bodyeyedelta = eyeAngles->y - pEntity->GetLowerBodyYaw();
			float lby = pEntity->GetLowerBodyYaw();

			if (ismoving) {
				eyeAngles->y = lby;
			}
			if (IsFakeWalking) {
				eyeAngles->y - lby >= lby + 180;
			}
			else if (is_moving && !IsFakeWalking)
			{
				last_moving_lby[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
				lastlby[pEntity->GetIndex()] = last_moving_lby[pEntity->GetIndex()];
				last_moving_time[pEntity->GetIndex()] = Interfaces::Globals->curtime;
				*(float*)((DWORD)pEntity + eyeangles) = pEntity->GetLowerBodyYaw();
			}
			else if (isstanding)
			{
				if (lby_keeps_updating() < 35) {
					switch (Globals::Shots % 2) {
					case 0:
						pEntity->GetAbsOrigin(), pLocal->GetAbsOrigin().y + 35;
						break;
					case 1:
						pEntity->GetAbsOrigin(), pLocal->GetAbsOrigin().y - 170;
						break;
					}
				}
				else if (lby_keeps_updating() > 35) {
					switch (Globals::Shots % 2) {
					case 0:
						pEntity->GetAbsOrigin(), lby;
						break;
					case 1:
						pEntity->GetAbsOrigin(), pLocal->GetAbsOrigin().y - rand() % 35;
						break;
					}
				}
				else if (lby_keeps_updating() > 2.2f) {
					static_cast<DWORD>(eyeAngles->y = eyeAngles->y + 180);
				}
			}
			//test
			else if (skeet) {
				if (lby_keeps_updating() >= 35) {
					eyeAngles->y - lby >= lby + 180;
				}
			}
			else
			{
				switch (Globals::Shots % 4)
				{
				case 0: *(float*)((DWORD)pEntity + eyeangles) = last_moving_lby[pEntity->GetIndex()] + 90; break;
				case 1: *(float*)((DWORD)pEntity + eyeangles) = last_moving_lby[pEntity->GetIndex()] - 90; break;
				case 2: *(float*)((DWORD)pEntity + eyeangles) = last_moving_lby[pEntity->GetIndex()] + 180; break;
				case 3: *(float*)((DWORD)pEntity + eyeangles) = last_moving_lby[pEntity->GetIndex()] - 180; break; //new
				}
			}
		}
		/*
		float v1, v2;

		v1 = newlbydelta[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
		if (v1 <= 89.f)
		{
		if (v1 < -89.0)
		v1 = v1 + 89.0;
		}
		else
		{
		v1 = v1 - 128.0;
		newlbydelta[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
		}

		v2 = newlbydelta[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
		if (v2 <= -128.0)
		{
		if (v2 < 89.0)
		v2 = v2 - 89.0;
		}
		else
		{
		v2 = v2 + 128.0;
		newlbydelta[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
		}*/

		
		BETA_LBYBreakerCorrections(pEntity);

	}
}
void ResolverSetup::OverrideResolver(IClientEntity* pEntity)
{

	int OverrideYaw = Options::Menu.RageBotTab.OverrideCount.GetValue();
	int OverrideKey = Options::Menu.RageBotTab.OverrideKey.GetKey();

	if (Options::Menu.RageBotTab.Override.GetState())
	{
		if (GetAsyncKeyState(Options::Menu.RageBotTab.OverrideKey.GetKey()))
		{
			pEntity->GetEyeAnglesXY()->y = pEntity->GetLowerBodyYaw() - 180.f;
		}
	}
}
// xluq tu byl i sie zmyl
void ResolverSetup::StoreFGE(IClientEntity* pEntity)
{
	ResolverSetup::storedanglesFGE = pEntity->GetEyeAnglesXY()->y;
	ResolverSetup::storedlbyFGE = pEntity->GetLowerBodyYaw();
	ResolverSetup::storedsimtimeFGE = pEntity->GetSimulationTime();
}

void ResolverSetup::StoreThings(IClientEntity* pEntity)
{
	ResolverSetup::StoredAngles[pEntity->GetIndex()] = *pEntity->GetEyeAnglesXY();
	ResolverSetup::storedlby[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
	ResolverSetup::storedsimtime = pEntity->GetSimulationTime();
	ResolverSetup::storeddelta[pEntity->GetIndex()] = pEntity->GetEyeAnglesXY()->y;
	ResolverSetup::storedlby[pEntity->GetIndex()] = pEntity->GetLowerBodyYaw();
}

void ResolverSetup::CM(IClientEntity* pEntity)
{
	for (int x = 1; x < Interfaces::Engine->GetMaxClients(); x++)
	{

		pEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(x);

		if (!pEntity
			|| pEntity == hackManager.pLocal()
			|| pEntity->IsDormant()
			|| !pEntity->IsAlive())
			continue;

		ResolverSetup::StoreThings(pEntity);
	}
}

void ResolverSetup::FSN(IClientEntity* pEntity, ClientFrameStage_t stage)
{
	if (stage == ClientFrameStage_t::FRAME_NET_UPDATE_POSTDATAUPDATE_START)
	{
		for (int i = 1; i < Interfaces::Engine->GetMaxClients(); i++)
		{

			pEntity = (IClientEntity*)Interfaces::EntList->GetClientEntity(i);

			if (!pEntity
				|| pEntity == hackManager.pLocal()
				|| pEntity->IsDormant()
				|| !pEntity->IsAlive())
				continue;

			ResolverSetup::Resolve(pEntity);
			ResolverSetup::OverrideResolver(pEntity);
		}
	}
}