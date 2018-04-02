#pragma once

#include "Hacks.h"
extern int missedLogHits[65];
struct Box
{
	int x = -1, y = -1, w = -1, h = -1;
};
class CEsp : public CHack
{
public:
	void Init();
	void Draw();
	void Move(CUserCmd *pCmd, bool &bSendPacket);
private:

	IClientEntity *BombCarrier;

	struct ESPBox
	{
		int x, y, w, h;
	};
	
	void DrawPlayer(IClientEntity* pEntity, player_info_t pinfo);
	Color GetPlayerColor(IClientEntity* pEntity);
	void Corners(CEsp::ESPBox size, Color color, IClientEntity * pEntity);
	void FilledBox(CEsp::ESPBox size, Color color);
	void DrawBoxx(Box size, Color color);
	bool get_box(IClientEntity * m_entity, Box & box, bool dynamic);
	bool GetBox(IClientEntity* pEntity, ESPBox &result);
	void DrawBox(ESPBox size, Color color);
	void Barrel(CEsp::ESPBox size, Color color, IClientEntity * pEntity);
	void DrawWeapon(IClientEntity * pEntity, CEsp::ESPBox size);
	void Ammo(IClientEntity * pEntity, player_info_t pinfo, CEsp::ESPBox size);
	void DrawGlow();
	void EntityGlow();
	void BombTimer(IClientEntity * pEntity, ClientClass * cClass);
	void DrawName(player_info_t pinfo, Box size);
	void DrawHealth(IClientEntity * pEntity, Box size);
	void DrawInfo(IClientEntity * pEntity, Box size);
	//void DrawName(player_info_t pinfo, ESPBox size);
	//void DrawHealth(IClientEntity* pEntity, ESPBox size);
	//void DrawInfo(IClientEntity* pEntity, ESPBox size);
	void DrawCross(IClientEntity* pEntity);
	void AmmoBar(IClientEntity * pEntity, Box size);
	void DrawSkeleton(IClientEntity* pEntity);
	void DrawHealthText(IClientEntity * pEntity, CEsp::ESPBox size);
	void IsPlayerDefusing(player_info_t pinfo, CEsp::ESPBox size, IClientEntity * pEntity);
	void DrawMoney(IClientEntity * pEntity, CEsp::ESPBox size);
	void Armor(IClientEntity * pEntity, CEsp::ESPBox size);
	void DrawMolotov(IClientEntity * pEntity, ClientClass * cClass);
	void DrawSmoke(IClientEntity * pEntity, ClientClass * cClass);
	void DrawDecoy(IClientEntity * pEntity, ClientClass * cClass);
	void DrawHE(IClientEntity * pEntity, ClientClass * cClass);
	void Info(IClientEntity * pEntity, CEsp::ESPBox size);
	void GrenadeTrace();
	void BoxAndText(IClientEntity * entity, std::string text);
	void DrawThrowable(IClientEntity * throwable);
	void DrawDistance(CEsp::ESPBox size, IClientEntity * pEntity);
	float DistanceTo(Vector vecSrc, Vector vecDst);
	void DrawChicken(IClientEntity* pEntity, ClientClass* cClass);
	void DrawDrop(IClientEntity* pEntity, ClientClass* cClass);
	void DrawBombPlanted(IClientEntity* pEntity, ClientClass* cClass);
	void DrawBomb(IClientEntity* pEntity, ClientClass* cClass);
};

