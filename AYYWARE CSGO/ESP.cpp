#include "ESP.h"
#include "Interfaces.h"
#include "RenderManager.h"
#include "GlowManager.h"
#include "Autowall.h"
#include <stdio.h>
#include <stdlib.h>

DWORD GlowManager = *(DWORD*)(Utilities::Memory::FindPatternV2("client.dll", "0F 11 05 ?? ?? ?? ?? 83 C8 01 C7 05 ?? ?? ?? ?? 00 00 00 00") + 3);

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

void CEsp::Init()
{
	BombCarrier = nullptr;
}

void CEsp::Move(CUserCmd *pCmd,bool &bSendPacket) 
{

}

void CEsp::Draw()
{
	if (!Interfaces::Engine->IsConnected() || !Interfaces::Engine->IsInGame())
		return;

	IClientEntity *pLocal = hackManager.pLocal();

	for (int i = 0; i < Interfaces::EntList->GetHighestEntityIndex(); i++)
	{
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		player_info_t pinfo;

		if (pEntity &&  pEntity != pLocal && !pEntity->IsDormant())
		{
			if (Options::Menu.VisualsTab.OtherRadar.GetState())
			{
				DWORD m_bSpotted = NetVar.GetNetVar(0x839EB159);
				*(char*)((DWORD)(pEntity)+m_bSpotted) = 1;
			}

			if (Options::Menu.VisualsTab.FiltersPlayers.GetState() && Interfaces::Engine->GetPlayerInfo(i, &pinfo) && pEntity->IsAlive())
			{
				DrawPlayer(pEntity, pinfo);
			}

			ClientClass* cClass = (ClientClass*)pEntity->GetClientClass();

			if (Options::Menu.VisualsTab.FiltersNades.GetState() && strstr(cClass->m_pNetworkName, "Projectile"))
			{
				DrawThrowable(pEntity);
			}

			if (Options::Menu.VisualsTab.FiltersWeapons.GetState() && cClass->m_ClassID != (int)CSGOClassID::CBaseWeaponWorldModel && ((strstr(cClass->m_pNetworkName, "Weapon") || cClass->m_ClassID == (int)CSGOClassID::CDEagle || cClass->m_ClassID == (int)CSGOClassID::CAK47)))
			{
				DrawDrop(pEntity, cClass);
			}

			if (Options::Menu.VisualsTab.FiltersC4.GetState())
			{
				if (cClass->m_ClassID == (int)CSGOClassID::CPlantedC4)
					DrawBombPlanted(pEntity, cClass);

				if (cClass->m_ClassID == (int)CSGOClassID::CPlantedC4)
					BombTimer(pEntity, cClass);

				if (cClass->m_ClassID == (int)CSGOClassID::CC4)
					DrawBomb(pEntity, cClass);
			}
		}
	}

	if (Options::Menu.VisualsTab.OtherNoFlash.GetState())
	{
		DWORD m_flFlashMaxAlpha = NetVar.GetNetVar(0xFE79FB98);
		*(float*)((DWORD)pLocal + m_flFlashMaxAlpha) = 0;
	}

	if (Options::Menu.VisualsTab.OptionsGlow.GetState())
	{
		DrawGlow();
	}
	if (Options::Menu.VisualsTab.EntityGlow.GetState())
	{
		EntityGlow();
	}
}

void CEsp::DrawPlayer(IClientEntity* pEntity, player_info_t pinfo)
{
	Box box;
	ESPBox Box;
	Color Color;

	Vector max = pEntity->GetCollideable()->OBBMaxs();
	Vector pos, pos3D;
	Vector top, top3D;
	pos3D = pEntity->GetOrigin();
	top3D = pos3D + Vector(0, 0, max.z);

	if (!Render::TransformScreen(pos3D, pos) || !Render::TransformScreen(top3D, top))
		return;

	if (Options::Menu.VisualsTab.FiltersEnemiesOnly.GetState() && (pEntity->GetTeamNum() == hackManager.pLocal()->GetTeamNum()))
		return;

	if (get_box(pEntity, box, Options::Menu.VisualsTab.Active.GetState()))
	{
		Color = GetPlayerColor(pEntity);

		if (Options::Menu.VisualsTab.OptionsBox.GetState())
			DrawBoxx(box, Color);

		if (Options::Menu.VisualsTab.OptionsName.GetState())
			DrawName(pinfo, box);

		if (Options::Menu.VisualsTab.OptionsHealth.GetState())
			DrawHealth(pEntity, box);

		if (Options::Menu.VisualsTab.OptionsInfo.GetState() || Options::Menu.VisualsTab.OptionsWeapone.GetState())
			DrawInfo(pEntity, box);

		if (Options::Menu.VisualsTab.OptionsSkeleton.GetState())
			DrawSkeleton(pEntity);

		if (Options::Menu.VisualsTab.Ammo.GetState())
			AmmoBar(pEntity, box);
	}

	//if (GetBox(pEntity, Box))
//	{
//		Color = GetPlayerColor(pEntity);
//	}
}
float dot_product_t(const float* a, const float* b) {
	return (a[0] * b[0] + a[1] * b[1] + a[2] * b[2]);
}

void vector_transform_a(const float *in1, const matrix3x4& in2, float *out) {
	out[0] = dot_product_t(in1, in2[0]) + in2[0][3];
	out[1] = dot_product_t(in1, in2[1]) + in2[1][3];
	out[2] = dot_product_t(in1, in2[2]) + in2[2][3];
}

inline void vector_transform_z(const Vector& in1, const matrix3x4 &in2, Vector &out) {
	vector_transform_a(&in1.x, in2, &out.x);
}

bool CEsp::get_box(IClientEntity* m_entity, Box& box, bool dynamic) {
	DWORD m_rgflCoordinateFrame = (DWORD)0x470 - 0x30;
	const matrix3x4& trnsf = *(matrix3x4*)((DWORD)m_entity + (DWORD)m_rgflCoordinateFrame);

	Vector  vOrigin, min, max, sMin, sMax, sOrigin,
		flb, brt, blb, frt, frb, brb, blt, flt;

	float left, top, right, bottom;

	vOrigin = m_entity->GetOrigin();
	min = m_entity->collisionProperty()->GetMins();
	max = m_entity->collisionProperty()->GetMaxs();

	if (!dynamic) {
		min += vOrigin;
		max += vOrigin;
	}

	Vector points[] = { Vector(min.x, min.y, min.z),
		Vector(min.x, max.y, min.z),
		Vector(max.x, max.y, min.z),
		Vector(max.x, min.y, min.z),
		Vector(max.x, max.y, max.z),
		Vector(min.x, max.y, max.z),
		Vector(min.x, min.y, max.z),
		Vector(max.x, min.y, max.z) };

	Vector vector_transformed[8];

	if (dynamic)
	{
		for (int i = 0; i < 8; i++)
		{
			vector_transform_z(points[i], trnsf, vector_transformed[i]);
			points[i] = vector_transformed[i];
		}
	}
	if (!Render::TransformScreen(points[3], flb) || !Render::TransformScreen(points[5], brt)
		|| !Render::TransformScreen(points[0], blb) || !Render::TransformScreen(points[4], frt)
		|| !Render::TransformScreen(points[2], frb) || !Render::TransformScreen(points[1], brb)
		|| !Render::TransformScreen(points[6], blt) || !Render::TransformScreen(points[7], flt))
		return false;

	Vector arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };
	left = flb.x;
	top = flb.y;
	right = flb.x;
	bottom = flb.y;

	for (int i = 1; i < 8; i++) {
		if (left > arr[i].x)
			left = arr[i].x;
		if (bottom < arr[i].y)
			bottom = arr[i].y;
		if (right < arr[i].x)
			right = arr[i].x;
		if (top > arr[i].y)
			top = arr[i].y;
	}

	box.x = left;
	box.y = top;
	box.w = right - left;
	box.h = bottom - top;

	return true;
}
bool CEsp::GetBox(IClientEntity* pEntity, CEsp::ESPBox &result)
{
	Vector  vOrigin, min, max, sMin, sMax, sOrigin,
		flb, brt, blb, frt, frb, brb, blt, flt;
	float left, top, right, bottom;

	vOrigin = pEntity->GetOrigin();
	min = pEntity->collisionProperty()->GetMins() + vOrigin;
	max = pEntity->collisionProperty()->GetMaxs() + vOrigin;

	Vector points[] = { Vector(min.x, min.y, min.z),
		Vector(min.x, max.y, min.z),
		Vector(max.x, max.y, min.z),
		Vector(max.x, min.y, min.z),
		Vector(max.x, max.y, max.z),
		Vector(min.x, max.y, max.z),
		Vector(min.x, min.y, max.z),
		Vector(max.x, min.y, max.z) };

	if (!Render::TransformScreen(points[3], flb) || !Render::TransformScreen(points[5], brt)
		|| !Render::TransformScreen(points[0], blb) || !Render::TransformScreen(points[4], frt)
		|| !Render::TransformScreen(points[2], frb) || !Render::TransformScreen(points[1], brb)
		|| !Render::TransformScreen(points[6], blt) || !Render::TransformScreen(points[7], flt))
		return false;

	Vector arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };

	left = flb.x;
	top = flb.y;
	right = flb.x;
	bottom = flb.y;

	for (int i = 1; i < 8; i++)
	{
		if (left > arr[i].x)
			left = arr[i].x;
		if (bottom < arr[i].y)
			bottom = arr[i].y;
		if (right < arr[i].x)
			right = arr[i].x;
		if (top > arr[i].y)
			top = arr[i].y;
	}

	result.x = left;
	result.y = top;
	result.w = right - left;
	result.h = bottom - top;

	return true;
}

Color CEsp::GetPlayerColor(IClientEntity* pEntity)
{
	int TeamNum = pEntity->GetTeamNum();
	bool IsVis = GameUtils::IsVisible(hackManager.pLocal(), pEntity, (int)CSGOHitboxID::Head);

	Color color;

	if (TeamNum == TEAM_CS_T)
	{
		if (IsVis)
			color = Color(Options::Menu.ColorsTab.BoxR.GetValue(), Options::Menu.ColorsTab.BoxG.GetValue(), Options::Menu.ColorsTab.BoxB.GetValue(), 255);
		else
			color = Color(Options::Menu.ColorsTab.BoxR.GetValue(), Options::Menu.ColorsTab.BoxG.GetValue(), Options::Menu.ColorsTab.BoxB.GetValue(), 255);
	}
	else
	{
		if (IsVis)
			color = Color(Options::Menu.ColorsTab.BoxR.GetValue(), Options::Menu.ColorsTab.BoxG.GetValue(), Options::Menu.ColorsTab.BoxB.GetValue(), 255);
		else
			color = Color(Options::Menu.ColorsTab.BoxR.GetValue(), Options::Menu.ColorsTab.BoxG.GetValue(), Options::Menu.ColorsTab.BoxB.GetValue(), 255);
	}

	return color;
}

void CEsp::Corners(CEsp::ESPBox size, Color color, IClientEntity* pEntity)
{
	int VertLine = (((float)size.w) * (0.20f));
	int HorzLine = (((float)size.h) * (0.30f));

	Render::Clear(size.x, size.y - 1, VertLine, 1, Color(0, 0, 0, 255));
	Render::Clear(size.x + size.w - VertLine, size.y - 1, VertLine, 1, Color(0, 0, 0, 255));
	Render::Clear(size.x, size.y + size.h - 1, VertLine, 1, Color(0, 0, 0, 255));
	Render::Clear(size.x + size.w - VertLine, size.y + size.h - 1, VertLine, 1, Color(0, 0, 0, 255));

	Render::Clear(size.x - 1, size.y, 1, HorzLine, Color(0, 0, 0, 255));
	Render::Clear(size.x - 1, size.y + size.h - HorzLine, 1, HorzLine, Color(0, 0, 0, 255));
	Render::Clear(size.x + size.w - 1, size.y, 1, HorzLine, Color(0, 0, 0, 255));
	Render::Clear(size.x + size.w - 1, size.y + size.h - HorzLine, 1, HorzLine, Color(0, 0, 0, 255));

	Render::Clear(size.x, size.y, VertLine, 1, color);
	Render::Clear(size.x + size.w - VertLine, size.y, VertLine, 1, color);
	Render::Clear(size.x, size.y + size.h, VertLine, 1, color);
	Render::Clear(size.x + size.w - VertLine, size.y + size.h, VertLine, 1, color);

	Render::Clear(size.x, size.y, 1, HorzLine, color);
	Render::Clear(size.x, size.y + size.h - HorzLine, 1, HorzLine, color);
	Render::Clear(size.x + size.w, size.y, 1, HorzLine, color);
	Render::Clear(size.x + size.w, size.y + size.h - HorzLine, 1, HorzLine, color);
}

void CEsp::FilledBox(CEsp::ESPBox size, Color color)
{
	int VertLine = (((float)size.w) * (0.20f));
	int HorzLine = (((float)size.h) * (0.20f));

	Render::Clear(size.x + 1, size.y + 1, size.w - 2, size.h - 2, Color(0, 0, 0, 40));
	Render::Clear(size.x + 1, size.y + 1, size.w - 2, size.h - 2, Color(0, 0, 0, 40));
	Render::Clear(size.x, size.y, VertLine, 1, color);
	Render::Clear(size.x + size.w - VertLine, size.y, VertLine, 1, color);
	Render::Clear(size.x, size.y + size.h, VertLine, 1, color);
	Render::Clear(size.x + size.w - VertLine, size.y + size.h, VertLine, 1, color);
	Render::Clear(size.x + 1, size.y + 1, size.w - 2, size.h - 2, Color(0, 0, 0, 40));
	Render::Clear(size.x, size.y, 1, HorzLine, color);
	Render::Clear(size.x, size.y + size.h - HorzLine, 1, HorzLine, color);
	Render::Clear(size.x + size.w, size.y, 1, HorzLine, color);
	Render::Clear(size.x + size.w, size.y + size.h - HorzLine, 1, HorzLine, color);
	Render::Clear(size.x + 1, size.y + 1, size.w - 2, size.h - 2, Color(0, 0, 0, 40));
}

void CEsp::DrawBoxx(Box box, Color color)
{
	Render::Outline(box.x, box.y, box.w, box.h, color);
	Render::Outline(box.x - 1, box.y - 1, box.w + 2, box.h + 2, Color(21, 21, 21, 150));
	Render::Outline(box.x + 1, box.y + 1, box.w - 2, box.h - 2, Color(21, 21, 21, 150));
}


std::string CleanItemName(std::string name)
{
	std::string Name = name;
	if (Name[0] == 'C')
		Name.erase(Name.begin());

	auto startOfWeap = Name.find("Weapon");
	if (startOfWeap != std::string::npos)
		Name.erase(Name.begin() + startOfWeap, Name.begin() + startOfWeap + 6);

	return Name;
}



 
static wchar_t* CharToWideChar(const char* text)
{
	size_t size = strlen(text) + 1;
	wchar_t* wa = new wchar_t[size];
	mbstowcs_s(NULL, wa, size/4, text, size);
	return wa;
}

void CEsp::BombTimer(IClientEntity* pEntity, ClientClass* cClass)
{
	BombCarrier = nullptr;

	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	CCSBomb* Bomb = (CCSBomb*)pEntity;

	if (Render::TransformScreen(vOrig, vScreen))
	{

		
		float flBlow = Bomb->GetC4BlowTime();
		float TimeRemaining = flBlow - (Interfaces::Globals->interval_per_tick * hackManager.pLocal()->GetTickBase());
		float TimeRemaining2;
		bool exploded = true;
		if (TimeRemaining < 0)
		{
			!exploded;

			TimeRemaining2 = 0;
		}
		else
		{
			exploded = true;
			TimeRemaining2 = TimeRemaining;
		}
		char buffer[64];
		if (exploded)
		{
			sprintf_s(buffer, "Bomb: %.1f", TimeRemaining2);
		}
		else
		{
			sprintf_s(buffer, "Bomb Undefusable", TimeRemaining2);
		}
		Render::Text(vScreen.x, vScreen.y, Color(0, 255, 50, 255), Render::Fonts::ESP, buffer);
	}
}

void CEsp::DrawName(player_info_t pinfo, Box size)
{
	if (strlen(pinfo.name) > 16)
	{
		pinfo.name[12] = 0;
		strcat(pinfo.name, "...");
		RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, pinfo.name);
		Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y - 11, Color(255, 255, 255, 255), Render::Fonts::ESP, pinfo.name);
	}
	else
	{
		RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, pinfo.name);
		Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y - 11, Color(255, 255, 255, 255), Render::Fonts::ESP, pinfo.name);
	}
}

void CEsp::DrawHealth(IClientEntity* pEntity, Box size)
{



	int HPEnemy = 100;
	HPEnemy = pEntity->GetHealth();
	char nameBuffer[512];
	sprintf_s(nameBuffer, "%d", HPEnemy);
	float h = (size.h);
	float health = pEntity->GetHealth();
	UINT hp = h - (UINT)((h * health) / 100);

	int Health = pEntity->GetHealth();
	if (Health > 100)
		Health = 100;
	int healthG = Health * 2.55;
	int healthR = 255 - healthG;
	Interfaces::Surface->DrawSetColor(0, 0, 0, 150);
	Interfaces::Surface->DrawOutlinedRect(size.x - 5, size.y - 1, size.x - 1, size.y + size.h + 1);
	//Interfaces::Surface->DrawOutlinedRect(BOX.x + BOX.w + 1, BOX.y - 1, BOX.x + BOX.w + 5, BOX.y + BOX.h + 1);

	int hpBarH = Health * size.h / 100;

	if (Health > 0)
	{

		Interfaces::Surface->DrawSetColor(healthR, healthG, 0, 255);
		Interfaces::Surface->DrawFilledRect(size.x - 4, size.y + size.h - hpBarH, size.x - 2, size.y + size.h);
	}

	if (Health < 100)
	{

		Render::Text(size.x - 10, size.y + hp - 2, Color(255, 255, 255, 255), Render::Fonts::ESP, nameBuffer);
	}

	/*
    float flBoxes = std::ceil(pEntity->GetHealth() / 10.f);
    float flX = size.x - 5;
    float flY = size.y - 1;
    float flHeight = size.h / 10.f;
    float flMultiplier = 12 / 360.f;
    flMultiplier *= flBoxes - 1;
    Color ColHealth = Color::FromHSB(flMultiplier, 1, 1);

    Render::DrawRect(flX, flY, 4, size.h + 2, Color(80, 80, 80, 125));
    Render::DrawOutlinedRect(flX, flY, 4, size.h + 2, Color(0, 0, 0, 245));
    Render::DrawRect(flX + 1, flY, 2, flHeight * flBoxes + 1, ColHealth);

    for (int i = 0; i < 10; i++)
    {
        Render::DrawLine(flX, flY + i * flHeight, flX + 4, flY + i * flHeight, Color(0, 0, 0, 245));
    }

	float health = pEntity->GetHealth();

	if (health > 100)
		health = 100;

	int colors[2] = { 255 - (health * 2.55), health * 2.55 };

	Interfaces::Surface->DrawSetColor(0, 0, 0, 220);
	if (Options::Menu.VisualsTab.OptionsHealth.GetState())
	{
		Interfaces::Surface->DrawOutlinedRect(size.x - 5, size.y - 1, size.x - 1, size.y + size.h + 1);

		int height = health * size.h / 100;

		if (health > 0)
		{
			Interfaces::Surface->DrawSetColor(colors[0], colors[1], 0, 255);
			Interfaces::Surface->DrawFilledRect(size.x - 4,
				size.y + size.h - height,
				size.x - 2,
				size.y + size.h);
		}
	}
    */
}
void CEsp::DrawInfo(IClientEntity* pEntity, Box size)
{
	RECT defSize = Render::GetTextSize(Render::Fonts::ESP, "");
	std::vector<std::string> Info;

	// Player Weapon ESP
	IClientEntity* pWeapon = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)pEntity->GetActiveWeaponHandle());

	if (Options::Menu.VisualsTab.OptionsWeapone.GetState())
	{

		if (Options::Menu.VisualsTab.Ammo.GetState())
		{
			RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, pWeapon->GetWeaponName());
			Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y + size.h + 5,
				Color(255, 255, 255, 255), Render::Fonts::ESP, pWeapon->GetWeaponName());
		}
		else
		{
			RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, pWeapon->GetWeaponName());
			Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y + size.h,
				Color(255, 255, 255, 255), Render::Fonts::ESP, pWeapon->GetWeaponName());
		}
	}
	if (Options::Menu.VisualsTab.OptionsInfo.GetState())
	{

		if (pEntity->ArmorValue() > 0)
		{
			char hp[50];
			sprintf_s(hp, sizeof(hp), "%i", pEntity->ArmorValue());

			if (pEntity->HasHelmet())
				Render::Text(size.x + size.w + 1, size.y + 0, Color(255, 255, 255, 255), Render::Fonts::ESP, "HK");
			else
				Render::Text(size.x + size.w + 1, size.y + 0, Color(255, 255, 255, 255), Render::Fonts::ESP, "K");
		}
	}
	if (Options::Menu.VisualsTab.OptionsInfo.GetState())
	{

		if (pEntity->IsScoped())
		{
			Render::Text(size.x + size.w + 1, size.y + 10, Color(0, 255, 255, 255), Render::Fonts::ESP, "ZOOM");
		}
		
	}
	static RECT Size = Render::GetTextSize(Render::Fonts::Default, "Hi");
	int i = 0;
	for (auto Text : Info)
	{
		Render::Text(size.x + size.w + 3, size.y + (i*(Size.bottom + 2)), Color(255, 255, 255, 255), Render::Fonts::ESP, Text.c_str());
		i++;
	}
}
void CEsp::DrawCross(IClientEntity* pEntity)
{
	Vector cross = pEntity->GetHeadPos(), screen;
	static int Scale = 2;
	if (Render::TransformScreen(cross, screen))
	{
		Render::Clear(screen.x - Scale, screen.y - (Scale * 2), (Scale * 2), (Scale * 4), Color(20, 20, 20, 160));
		Render::Clear(screen.x - (Scale * 2), screen.y - Scale, (Scale * 4), (Scale * 2), Color(20, 20, 20, 160));
		Render::Clear(screen.x - Scale - 1, screen.y - (Scale * 2) - 1, (Scale * 2) - 2, (Scale * 4) - 2, Color(250, 250, 250, 160));
		Render::Clear(screen.x - (Scale * 2) - 1, screen.y - Scale - 1, (Scale * 4) - 2, (Scale * 2) - 2, Color(250, 250, 250, 160));
	}
}
void CEsp::AmmoBar(IClientEntity* pEntity, Box size)
{
	IClientEntity* pWeapon = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)pEntity->GetActiveWeaponHandle());
	CSWeaponInfo* weapInfo = ((C_BaseCombatWeapon*)Interfaces::EntList->GetClientEntityFromHandle(pEntity->GetActiveWeaponHandle()))->GetCSWpnData();


	Box ArBar = size;
	ArBar.y += (ArBar.h + 3);
	ArBar.h = 6;

	float ArValue = pWeapon->GetAmmoInClip();
	float ArPerc = ArValue / weapInfo->iMaxClip1;
	float Width = (size.w * ArPerc);
	ArBar.w = Width;
	

	//int player_armor = pEntity->ArmorValue() > 100 ? 100 : pEntity->ArmorValue();

	if (pWeapon) {
		Color arc = Color(86, 123, 190, 255);


		Render::Outline(size.x - 1, size.y + size.h + 2, size.w + 2, 4, Color(21, 21, 21, 200));

		int armor_width = Width;

		Render::DrawRect(size.x, size.y + size.h + 3, size.w, 2, Color(21, 21, 21, 240));
		Render::DrawRect(size.x, size.y + size.h + 3, armor_width, 2, arc);
	}
}
void CEsp::DrawDrop(IClientEntity* pEntity, ClientClass* cClass)
{
	Color color;
	Box Box;
	IClientEntity* Weapon = (IClientEntity*)pEntity;
	IClientEntity* plr = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)Weapon->GetOwnerHandle());
	if (Weapon && !plr)
	{
		if (get_box(pEntity, Box, Options::Menu.VisualsTab.FiltersWeapons.GetState()))
		{
			if (Options::Menu.VisualsTab.FiltersWeapons.GetState())
			{
				DrawBoxx(Box, Color(255, 255, 255, 255));
			}

			std::string ItemName = CleanItemName(cClass->m_pNetworkName);
			RECT TextSize = Render::GetTextSize(Render::Fonts::ESP, Weapon->GetWeaponName());
			Render::Text(Box.x + (Box.w / 2) - (TextSize.right / 2), Box.y + Box.h, Color(255, 255, 255, 255), Render::Fonts::ESP, Weapon->GetWeaponName());
		}
	}
}

void CEsp::DrawBombPlanted(IClientEntity* pEntity, ClientClass* cClass)
{
	BombCarrier = nullptr;

	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	CCSBomb* Bomb = (CCSBomb*)pEntity;

	float flBlow = Bomb->GetC4BlowTime();
	float TimeRemaining = flBlow - (Interfaces::Globals->interval_per_tick * hackManager.pLocal()->GetTickBase());
	char buffer[64];
	sprintf_s(buffer, "%.1fs", TimeRemaining);
	float TimeRemaining2;
	bool exploded = true;
	if (TimeRemaining < 0)
	{
		!exploded;

		TimeRemaining2 = 0;
	}
	else
	{
		exploded = true;
		TimeRemaining2 = TimeRemaining;
	}
	if (exploded)
	{
		sprintf_s(buffer, "Bomb: %.1f", TimeRemaining2);
	}
	else
	{
		sprintf_s(buffer, "Bomb Undefusable", TimeRemaining2);
	}

	Render::Text(10, 45, Color(0, 255, 0, 255), Render::Fonts::Clock, buffer);

}

void CEsp::DrawBomb(IClientEntity* pEntity, ClientClass* cClass)
{
	BombCarrier = nullptr;
	C_BaseCombatWeapon *BombWeapon = (C_BaseCombatWeapon *)pEntity;
	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	bool adopted = true;
	HANDLE parent = BombWeapon->GetOwnerHandle();
	if (parent || (vOrig.x == 0 && vOrig.y == 0 && vOrig.z == 0))
	{
		IClientEntity* pParentEnt = (Interfaces::EntList->GetClientEntityFromHandle(parent));
		if (pParentEnt && pParentEnt->IsAlive())
		{
			BombCarrier = pParentEnt;
			adopted = false;
		}
	}

	if (adopted)
	{
		if (Render::TransformScreen(vOrig, vScreen))
		{
			Render::Text(vScreen.x, vScreen.y, Color(112, 230, 20, 255), Render::Fonts::ESP, "Bomb");
		}
	}
}

void DrawBoneArray(int* boneNumbers, int amount, IClientEntity* pEntity, Color color)
{
	Vector LastBoneScreen;
	for (int i = 0; i < amount; i++)
	{
		Vector Bone = pEntity->GetBonePos(boneNumbers[i]);
		Vector BoneScreen;

		if (Render::TransformScreen(Bone, BoneScreen))
		{
			if (i>0)
			{
				Render::Line(LastBoneScreen.x, LastBoneScreen.y, BoneScreen.x, BoneScreen.y, color);
			}
		}
		LastBoneScreen = BoneScreen;
	}
}

void DrawBoneTest(IClientEntity *pEntity)
{
	for (int i = 0; i < 127; i++)
	{
		Vector BoneLoc = pEntity->GetBonePos(i);
		Vector BoneScreen;
		if (Render::TransformScreen(BoneLoc, BoneScreen))
		{
			char buf[10];
			_itoa_s(i, buf, 10);
			Render::Text(BoneScreen.x, BoneScreen.y, Color(255, 255, 255, 180), Render::Fonts::ESP, buf);
		}
	}
}

void CEsp::DrawSkeleton(IClientEntity* pEntity)
{
	studiohdr_t* pStudioHdr = Interfaces::ModelInfo->GetStudiomodel(pEntity->GetModel());

	if (!pStudioHdr)
		return;

	Vector vParent, vChild, sParent, sChild;

	for (int j = 0; j < pStudioHdr->numbones; j++)
	{
		mstudiobone_t* pBone = pStudioHdr->GetBone(j);

		if (pBone && (pBone->flags & BONE_USED_BY_HITBOX) && (pBone->parent != -1))
		{
			vChild = pEntity->GetBonePos(j);
			vParent = pEntity->GetBonePos(pBone->parent);

			if (Render::TransformScreen(vParent, sParent) && Render::TransformScreen(vChild, sChild))
			{
				Render::Line(sParent[0], sParent[1], sChild[0], sChild[1], Color(255,255,255,255));
			}
		}
	}
}
void CEsp::DrawHealthText(IClientEntity* pEntity, CEsp::ESPBox size)
{

	ESPBox HealthBar = size;
	HealthBar.y += (HealthBar.h + 6);
	HealthBar.h = 4;

	float HealthValue = pEntity->GetHealth();
	float HealthPerc = HealthValue / 100.f;
	float Width = (size.w * HealthPerc);
	HealthBar.w = Width;

	int HPEnemy = 100;
	HPEnemy = pEntity->GetHealth();
	char nameBuffer[512];
	sprintf_s(nameBuffer, "%d HP", HPEnemy);

	Render::Text(size.x - 36, size.y + 0, Color(255, 255, 255, 255), Render::Fonts::ESP, nameBuffer);
}


void CEsp::BoxAndText(IClientEntity* entity, std::string text)
{
	Box Box;
	std::vector<std::string> Info;
	RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, "");
	if (get_box(entity, Box, Options::Menu.VisualsTab.FiltersNades.GetState()))
	{
		Info.push_back(text);
		if (Options::Menu.VisualsTab.FiltersNades.GetState())
		{
			int i = 0;
			for (auto kek : Info)
			{
				DrawBoxx(Box, Color(255, 255, 255, 255));
				Render::Text(Box.x + (Box.w / 2) - (nameSize.right / 2), Box.y - 11, Color(255, 255, 255, 255), Render::Fonts::ESP, kek.c_str());
				i++;
			}
		}
	}
}

void CEsp::DrawThrowable(IClientEntity* throwable)
{
	model_t* nadeModel = (model_t*)throwable->GetModel();

	if (!nadeModel)
		return;

	studiohdr_t* hdr = Interfaces::ModelInfo->GetStudiomodel(nadeModel);

	if (!hdr)
		return;

	if (!strstr(hdr->name, "thrown") && !strstr(hdr->name, "dropped"))
		return;

	std::string nadeName = "Unknown Grenade";

	IMaterial* mats[32];
	Interfaces::ModelInfo->GetModelMaterials(nadeModel, hdr->numtextures, mats);

	for (int i = 0; i < hdr->numtextures; i++)
	{
		IMaterial* mat = mats[i];
		if (!mat)
			continue;

		if (strstr(mat->GetName(), "flashbang"))
		{
			nadeName = "flash";
			break;
		}
		else if (strstr(mat->GetName(), "m67_grenade") || strstr(mat->GetName(), "hegrenade"))
		{
			nadeName = "hegrenade";
			break;
		}
		else if (strstr(mat->GetName(), "smoke"))
		{
			nadeName = "smoke";
			break;
		}
		else if (strstr(mat->GetName(), "decoy"))
		{
			nadeName = "decoy";
			break;
		}
		else if (strstr(mat->GetName(), "incendiary") || strstr(mat->GetName(), "molotov"))
		{
			nadeName = "fire";
			break;
		}
	}
	BoxAndText(throwable, nadeName);
}
void CEsp::DrawGlow()
{
	int GlowR = Options::Menu.ColorsTab.GlowR.GetValue();
	int GlowG = Options::Menu.ColorsTab.GlowG.GetValue();
	int GlowB = Options::Menu.ColorsTab.GlowB.GetValue();
	int GlowZ = Options::Menu.VisualsTab.GlowZ.GetValue();

	CGlowObjectManager* GlowObjectManager = (CGlowObjectManager*)GlowManager;

	for (int i = 0; i < GlowObjectManager->size; ++i)
	{
		CGlowObjectManager::GlowObjectDefinition_t* glowEntity = &GlowObjectManager->m_GlowObjectDefinitions[i];
		IClientEntity* Entity = glowEntity->getEntity();

		if (glowEntity->IsEmpty() || !Entity)
			continue;

		switch (Entity->GetClientClass()->m_ClassID)
		{
		case 35:
			if (Options::Menu.VisualsTab.OptionsGlow.GetState())
			{
				if (!Options::Menu.VisualsTab.FiltersPlayers.GetState() && !(Entity->GetTeamNum() == hackManager.pLocal()->GetTeamNum()))
					break;
				if (Options::Menu.VisualsTab.FiltersEnemiesOnly.GetState() && (Entity->GetTeamNum() == hackManager.pLocal()->GetTeamNum()))
					break;

				if (GameUtils::IsVisible(hackManager.pLocal(), Entity, 0))
				{
					glowEntity->set((Entity->GetTeamNum() == hackManager.pLocal()->GetTeamNum()) ? Color(GlowR, GlowG, GlowB, GlowZ) : Color(GlowR, GlowG, GlowB, GlowZ));
				}

				else
				{
					glowEntity->set((Entity->GetTeamNum() == hackManager.pLocal()->GetTeamNum()) ? Color(GlowR, GlowG, GlowB, GlowZ) : Color(GlowR, GlowG, GlowB, GlowZ));
				}
			}
		}
	}
}
void CEsp::EntityGlow()
{
	int GlowR = Options::Menu.ColorsTab.GlowR.GetValue();
	int GlowG = Options::Menu.ColorsTab.GlowG.GetValue();
	int GlowB = Options::Menu.ColorsTab.GlowB.GetValue();
	int GlowZ = Options::Menu.VisualsTab.GlowZ.GetValue();

	CGlowObjectManager* GlowObjectManager = (CGlowObjectManager*)GlowManager;

	for (int i = 0; i < GlowObjectManager->size; ++i)
	{
		CGlowObjectManager::GlowObjectDefinition_t* glowEntity = &GlowObjectManager->m_GlowObjectDefinitions[i];
		IClientEntity* Entity = glowEntity->getEntity();

		if (glowEntity->IsEmpty() || !Entity)
			continue;

		switch (Entity->GetClientClass()->m_ClassID)
		{
		case 1:
			if (Options::Menu.VisualsTab.EntityGlow.GetState())
			{
				if (Options::Menu.VisualsTab.EntityGlow.GetState())
					glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 9:
			if (Options::Menu.VisualsTab.FiltersNades.GetState())
			{
				if (Options::Menu.VisualsTab.EntityGlow.GetState())
					glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 29:
			if (Options::Menu.VisualsTab.EntityGlow.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 39:
			if (Options::Menu.VisualsTab.EntityGlow.GetState())
			{
				if (Options::Menu.VisualsTab.FiltersC4.GetState())
					glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 41:
			if (Options::Menu.VisualsTab.EntityGlow.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 66:
			if (Options::Menu.VisualsTab.EntityGlow.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 87:
			if (Options::Menu.VisualsTab.FiltersNades.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 98:
			if (Options::Menu.VisualsTab.FiltersNades.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 108:
			if (Options::Menu.VisualsTab.FiltersC4.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 130:
			if (Options::Menu.VisualsTab.FiltersNades.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		case 134:
			if (Options::Menu.VisualsTab.FiltersNades.GetState())
			{
				glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		default:
			if (Options::Menu.VisualsTab.EntityGlow.GetState())
			{
				if (strstr(Entity->GetClientClass()->m_pNetworkName, "Weapon"))
					glowEntity->set(Color(GlowR, GlowG, GlowB, GlowZ));
			}
		}
	}
}