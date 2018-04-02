#include "Menu.h"
#include "Controls.h"
#include "Hooks.h" 
#include "Interfaces.h"
#include "CRC32.h"

#define WINDOW_WIDTH 700
#define WINDOW_HEIGHT 500

PhantomWindow Options::Menu;

void SaveLegitCallbk()
{
	switch (Options::Menu.ColorsTab.ConfigBox.GetIndex())
	{
	case 0:
		GUI.SaveWindowState(&Options::Menu, "legit.cfg");
		break;
	case 1:
		GUI.SaveWindowState(&Options::Menu, "rage.cfg");
		break;
	case 2:
		GUI.SaveWindowState(&Options::Menu, "mmhvh.cfg");
		break;
	case 3:
		GUI.SaveWindowState(&Options::Menu, "casual.cfg");
		break;
	}
}

void LoadLegitCallbk()
{
	switch (Options::Menu.ColorsTab.ConfigBox.GetIndex())
	{
	case 0:
		GUI.LoadWindowState(&Options::Menu, "legit.cfg");
		break;
	case 1:
		GUI.LoadWindowState(&Options::Menu, "rage.cfg");
		break;
	case 2:
		GUI.LoadWindowState(&Options::Menu, "mmhvh.cfg");
		break;
	case 3:
		GUI.LoadWindowState(&Options::Menu, "casual.cfg");
		break;
	}
}

void SaveRageCallbk()
{
	GUI.SaveWindowState(&Options::Menu, "rage.cfg");
}

void LoadRageCallbk()
{
	GUI.LoadWindowState(&Options::Menu, "rage.cfg");
}

void UnLoadCallbk()
{
	DoUnload = true;
}

void KnifeApplyCallbk()
{
	static ConVar* Meme = Interfaces::CVar->FindVar("cl_fullupdate");
	Meme->nFlags &= ~FCVAR_CHEAT;
	Interfaces::Engine->ClientCmd_Unrestricted("cl_fullupdate");
	bGlovesNeedUpdate = true;
}

void GlovesApplyCallbk()
{
	static ConVar* Meme = Interfaces::CVar->FindVar("cl_fullupdate");
	Meme->nFlags &= ~FCVAR_CHEAT;
	Interfaces::Engine->ClientCmd_Unrestricted("cl_fullupdate");
	bGlovesNeedUpdate = true;
}

void PhantomWindow::Setup()
{
	SetPosition(350, 50);
	SetSize(WINDOW_WIDTH, WINDOW_HEIGHT);
	SetTitle("");

	RegisterTab(&RageBotTab);
//	RegisterTab(&LegitBotTab);
	RegisterTab(&VisualsTab);
	RegisterTab(&MiscTab);
	RegisterTab(&ColorsTab);

	RECT Client = GetClientArea();
	Client.bottom -= 29;

	RageBotTab.Setup();
//	LegitBotTab.Setup();
	VisualsTab.Setup();
	MiscTab.Setup();
	ColorsTab.Setup();

#pragma region Bottom Buttons

	SaveButton.SetText("Save");
	SaveButton.SetCallback(SaveLegitCallbk);
	SaveButton.SetSize(200, 106);
	SaveButton.SetPosition(405, 400);

	LoadButton.SetText("Load");
	LoadButton.SetCallback(LoadLegitCallbk);
	LoadButton.SetSize(200, 106);
	LoadButton.SetPosition(580, 400);

	ColorsTab.RegisterControl(&SaveButton);
//	ColorsTab.RegisterControl(&ConfigBox);
	ColorsTab.RegisterControl(&LoadButton);

#pragma endregion
}

void CRageBotTab::Setup()
{
	SetTitle("b");

	//ActiveLabel.SetPosition(16, -15);
//	ActiveLabel.SetText("Active");
	//RegisterControl(&ActiveLabel);

	//Active.SetFileId("active");
//	Active.SetPosition(66, -15);
	//RegisterControl(&Active);

#pragma region Aimbot
	
	AimbotGroup.SetPosition(16, 30);
	AimbotGroup.SetText("Aimbot");
	AimbotGroup.SetSize(300, 370);
	AimbotGroup.AddTab(CGroupTab("Aimbot", 1));
	AimbotGroup.AddTab(CGroupTab("Accuracy", 2));
	AimbotGroup.AddTab(CGroupTab("Hitscan", 3));
	RegisterControl(&AimbotGroup);
	RegisterControl(&Active);

	Active.SetFileId("active");
	AimbotGroup.PlaceLabledControl(1, "Active Rage", this, &Active);

	AimbotEnable.SetFileId("aim_enable");
	AimbotGroup.PlaceLabledControl(1, "Enable", this, &AimbotEnable);

	AimbotAutoFire.SetFileId("aim_autofire");
	AimbotGroup.PlaceLabledControl(1, "Auto fire", this, &AimbotAutoFire);

	AimbotFov.SetFileId("aim_fov");
	AimbotFov.SetBoundaries(0.f, 180.f);
	AimbotFov.SetValue(39.f);
	AimbotGroup.PlaceLabledControl(1, "Maximum fov", this, &AimbotFov);

	AimbotSilentAim.SetFileId("aim_silent");
	AimbotGroup.PlaceLabledControl(1, "Silent aim", this, &AimbotSilentAim);

	AutoRevolver.SetFileId("aim_autorevolver");
	AimbotGroup.PlaceLabledControl(1, "Auto revolver", this, &AutoRevolver);

	AimbotAimStep.SetFileId("aim_aimstep");
	AimbotGroup.PlaceLabledControl(1, "Aim step", this, &AimbotAimStep);

	AimbotKeyPress.SetFileId("aim_usekey");
	AimbotGroup.PlaceLabledControl(1, "Aimbot on key", this, &AimbotKeyPress);

	AimbotKeyBind.SetFileId("aim_key");
	AimbotGroup.PlaceLabledControl(1, "Aimbot key", this, &AimbotKeyBind);

	AWPAtBody.SetFileId("aim_awpatbody");
	AimbotGroup.PlaceLabledControl(2, "AWP body aim", this, &AWPAtBody);

	AccuracyHitchance.SetFileId("acc_chance");
	AccuracyHitchance.SetBoundaries(0, 100);
	AccuracyHitchance.SetValue(0);
	AimbotGroup.PlaceLabledControl(2, "Hit chance", this, &AccuracyHitchance);

	BaimIfUnderXHealth.SetFileId("acc_BaimIfUnderXHealth");
	BaimIfUnderXHealth.SetBoundaries(0, 100);
	BaimIfUnderXHealth.SetValue(0);
	AimbotGroup.PlaceLabledControl(2, "Baim if hp lower than", this, &BaimIfUnderXHealth);

	AccuracyRecoil.SetFileId("acc_norecoil");
	AimbotGroup.PlaceLabledControl(2, "Remove recoil", this, &AccuracyRecoil);

	AccuracyAutoWall.SetFileId("acc_awall");
	AimbotGroup.PlaceLabledControl(2, "Auto wall", this, &AccuracyAutoWall);

	AccuracyMinimumDamage.SetFileId("acc_mindmg");
	AccuracyMinimumDamage.SetBoundaries(1.f, 99.f);
	AccuracyMinimumDamage.SetValue(1.f);
	AimbotGroup.PlaceLabledControl(2, "Autowall damage", this, &AccuracyMinimumDamage);

	AccuracyAutoScope.SetFileId("acc_scope");
	AimbotGroup.PlaceLabledControl(2, "Auto scope", this, &AccuracyAutoScope);

	AccuracyResolver.SetFileId("acc_aaa");
	AimbotGroup.PlaceLabledControl(2, "Resolver", this, &AccuracyResolver);

	LBYCorrection.SetFileId("acc_lbycorrect");
	AimbotGroup.PlaceLabledControl(2, "Correct lby", this, &LBYCorrection);

	Override.SetFileId("acc_overrideresolverenabble");
	AimbotGroup.PlaceLabledControl(2, "Override Resolver", this, &Override);
	
	OverrideKey.SetFileId("acc_overrridekey");
	AimbotGroup.PlaceLabledControl(2, "Override Key", this, &OverrideKey);


	TargetSelection.SetFileId("tgt_selection");
	TargetSelection.AddItem("Closest To Crosshair");
	TargetSelection.AddItem("Distance");
	TargetSelection.AddItem("Lowest Health");
	TargetSelection.AddItem("Threat");
	TargetSelection.AddItem("Next Shot");
	AimbotGroup.PlaceLabledControl(3, "Selection", this, &TargetSelection);

	TargetHitbox.SetFileId("tgt_hitbox");
	TargetHitbox.AddItem("Head");
	TargetHitbox.AddItem("Neck");
	TargetHitbox.AddItem("Chest");
	TargetHitbox.AddItem("Pelvis");
	TargetHitbox.AddItem("Shin");
	AimbotGroup.PlaceLabledControl(3, "Hitbox", this, &TargetHitbox);

	TargetHitscan.SetFileId("tgt_hitscan");
	TargetHitscan.AddItem("Off"); //0
	TargetHitscan.AddItem("Low"); // 1
	TargetHitscan.AddItem("Medium"); // 2
	TargetHitscan.AddItem("High"); // 3
	TargetHitscan.AddItem("Body only"); // 4
	AimbotGroup.PlaceLabledControl(3, "Hitscan", this, &TargetHitscan);

	TargetMultipoint.SetFileId("tgt_multipoint");
	AimbotGroup.PlaceLabledControl(3, "Multipoint", this, &TargetMultipoint);

	TargetPointscale.SetFileId("tgt_pointscale");
	TargetPointscale.SetBoundaries(0.f, 10.f);
	TargetPointscale.SetValue(5.f);
	AimbotGroup.PlaceLabledControl(3, "Aim height", this, &TargetPointscale);
#pragma endregion Targetting controls 

#pragma endregion Aimbot Controls Get Setup in here


	AntiAimGroup.SetPosition(370, 30);
	AntiAimGroup.SetText("Anti Aim");
	AntiAimGroup.SetSize(300, 280);
	AntiAimGroup.AddTab(CGroupTab("Main", 1));
	AntiAimGroup.AddTab(CGroupTab("Misc", 2));
	RegisterControl(&AntiAimGroup);

	AntiAimEnable.SetFileId("aa_enable");
	AntiAimGroup.PlaceLabledControl(1, "Enable", this, &AntiAimEnable);

	AntiAimPitch.SetFileId("aa_x");
	AntiAimPitch.AddItem("Off");
	AntiAimPitch.AddItem("Halfdown");
	AntiAimPitch.AddItem("Jitter");
	AntiAimPitch.AddItem("Emotion");
	AntiAimPitch.AddItem("Up");
	AntiAimPitch.AddItem("Zero");
	AntiAimPitch.AddItem("Custom");
	AntiAimGroup.PlaceLabledControl(1, "Pitch", this, &AntiAimPitch);

	AntiAimYaw.SetFileId("aa_y");
	AntiAimYaw.AddItem("Off");
	AntiAimYaw.AddItem("Backward");
	AntiAimYaw.AddItem("Backward jitter");
	AntiAimYaw.AddItem("Back spin");
	AntiAimYaw.AddItem("180z");
	AntiAimYaw.AddItem("Tick spin");
	AntiAimYaw.AddItem("Random");
	AntiAimYaw.AddItem("Automatic");
	AntiAimYaw.AddItem("Automatic random");
	AntiAimYaw.AddItem("Automatic spin");
	AntiAimGroup.PlaceLabledControl(1, "Real yaw", this, &AntiAimYaw);

	YawAdd.SetFileId("aa_yawadd");
	YawAdd.SetBoundaries(-180, 180);
	YawAdd.SetValue(0);
	AntiAimGroup.PlaceLabledControl(1, "", this, &YawAdd);

	AntiAimYawrun.SetFileId("aa_y");
	AntiAimYawrun.AddItem("Off");
	AntiAimYawrun.AddItem("Backward");
	AntiAimYawrun.AddItem("Backward jitter");
	AntiAimYawrun.AddItem("Back spin");
	AntiAimYawrun.AddItem("180z");
	AntiAimYawrun.AddItem("Tick spin");
	AntiAimYawrun.AddItem("Random");
	AntiAimYawrun.AddItem("Automatic");
	AntiAimYawrun.AddItem("Automatic random");
	AntiAimYawrun.AddItem("Automatic spin");
	AntiAimGroup.PlaceLabledControl(1, "Move real yaw", this, &AntiAimYawrun);

	YawRunningAdd.SetFileId("aa_yawrundadd");
	YawRunningAdd.SetBoundaries(-180, 180);
	YawRunningAdd.SetValue(0);
	AntiAimGroup.PlaceLabledControl(1, "", this, &YawRunningAdd);

	FakeYaw.SetFileId("fakeaa");
	FakeYaw.AddItem("Off");
	FakeYaw.AddItem("Backward");
	FakeYaw.AddItem("Opposite");
	FakeYaw.AddItem("Zero");
	FakeYaw.AddItem("Tick spin");
	FakeYaw.AddItem("180z");
	FakeYaw.AddItem("Random");
	FakeYaw.AddItem("Automatic");
	FakeYaw.AddItem("Automatic random");
	FakeYaw.AddItem("Automatic spin");
	AntiAimGroup.PlaceLabledControl(1, "Fake yaw", this, &FakeYaw);

	FakeYawAdd.SetFileId("aa_yawfakedadd");
	FakeYawAdd.SetBoundaries(-180, 180);
	FakeYawAdd.SetValue(0);
	AntiAimGroup.PlaceLabledControl(1, "", this, &FakeYawAdd);

	AntiAimTarget.SetFileId("aa_target");
	AntiAimGroup.PlaceLabledControl(1, "At targets", this, &AntiAimTarget);

	ManualAAEnable.SetFileId("manual_antiaim");
	AntiAimGroup.PlaceLabledControl(2, "Enable manual aa", this, &ManualAAEnable);

	ManualAASwitch.SetFileId("manual_aaswitchkey");
	AntiAimGroup.PlaceLabledControl(2, "Manual switch key", this, &ManualAASwitch);

	BreakLBY.SetFileId("r_lbybreaker");
	AntiAimGroup.PlaceLabledControl(2, "Break lby", this, &BreakLBY);

	BreakLBYDelta.SetFileId("r_lbydelta");
	BreakLBYDelta.SetBoundaries(-180, 180);
	BreakLBYDelta.SetValue(0);
	AntiAimGroup.PlaceLabledControl(2, "Break lby delta", this, &BreakLBYDelta);

	Freestand.SetFileId("freestand");
	AntiAimGroup.PlaceLabledControl(2, "Freestand", this, &Freestand);

	FreestandingDelta.SetFileId("aa_deltafreestand");
	FreestandingDelta.SetBoundaries(-180, 180);
	FreestandingDelta.SetValue(0);
	AntiAimGroup.PlaceLabledControl(2, "Freestand delta", this, &FreestandingDelta);

#pragma endregion  AntiAim controls get setup in here
}



void CVisualTab::Setup()
{
	SetTitle("c");

	

#pragma region Options
	OptionsGroup.SetText("Options");
	OptionsGroup.SetPosition(16, 30);
	OptionsGroup.SetSize(200, 346);
	OptionsGroup.AddTab(CGroupTab("Main", 1));
	OptionsGroup.AddTab(CGroupTab("Filters", 2));
	RegisterControl(&OptionsGroup);
	RegisterControl(&Active);

	Active.SetFileId("active");
	OptionsGroup.PlaceLabledControl(1, "Active Visuals", this, &Active);

	OptionsBox.SetFileId("opt_box");
	OptionsGroup.PlaceLabledControl(1, "Bounding box", this, &OptionsBox);

	OptionsName.SetFileId("opt_name");
	OptionsGroup.PlaceLabledControl(1, "Name", this, &OptionsName);

	OptionsWeapone.SetFileId("opt_weapone");
	OptionsGroup.PlaceLabledControl(1, "Weapon", this, &OptionsWeapone);

	Ammo.SetFileId("opt_ammobar");
	OptionsGroup.PlaceLabledControl(1, "Ammo", this, &Ammo);
	
	OptionsHealth.SetFileId("opt_hp");
	OptionsGroup.PlaceLabledControl(1, "Health", this, &OptionsHealth);

	OptionsInfo.SetFileId("opt_info");
	OptionsGroup.PlaceLabledControl(1, "Info", this, &OptionsInfo);

	OtherHitmarker.SetFileId("otr_hitmarker");
	OptionsGroup.PlaceLabledControl(1, "Hitmarker", this, &OtherHitmarker);
	
	OptionsSkeleton.SetFileId("opt_bone");
	OptionsGroup.PlaceLabledControl(1, "Skeleton", this, &OptionsSkeleton);
	
	OptionsCompRank.SetFileId("opt_comprank");
	OptionsGroup.PlaceLabledControl(1, "Show ranks", this, &OptionsCompRank);
	
	EntityGlow.SetFileId("opt_entityglow");
	OptionsGroup.PlaceLabledControl(1, "Entity glow", this, &EntityGlow);

	OptionsGlow.SetFileId("opt_glow");
	OptionsGroup.PlaceLabledControl(1, "Glow", this, &OptionsGlow);

	GlowZ.SetFileId("opt_glowz");
	GlowZ.SetBoundaries(0.f, 255.f);
	GlowZ.SetValue(255);
	OptionsGroup.PlaceLabledControl(1, "Glow opacity", this, &GlowZ);


	FiltersAll.SetFileId("ftr_all");
	OptionsGroup.PlaceLabledControl(2, "All", this, &FiltersAll);

	FiltersPlayers.SetFileId("ftr_players");
	OptionsGroup.PlaceLabledControl(2, "Players", this, &FiltersPlayers);

	FiltersEnemiesOnly.SetFileId("ftr_enemyonly");
	OptionsGroup.PlaceLabledControl(2, "Enemies only", this, &FiltersEnemiesOnly);

	FiltersWeapons.SetFileId("ftr_weaps");
	OptionsGroup.PlaceLabledControl(2, "Weapons", this, &FiltersWeapons);

	FiltersNades.SetFileId("ftr_nades");
	OptionsGroup.PlaceLabledControl(2, "Nades", this, &FiltersNades);

	FiltersC4.SetFileId("ftr_c4");
	OptionsGroup.PlaceLabledControl(2, "C4", this, &FiltersC4);

#pragma endregion Setting up the Options controls

#pragma region Filters

#pragma endregion Setting up the Filters controls
	ChamsGroup.SetText("Chams");
	ChamsGroup.SetPosition(230, 30);
	ChamsGroup.SetSize(200, 155);
	RegisterControl(&ChamsGroup);

	Chams.SetFileId("esp_chams");
	Chams.AddItem("None");
	Chams.AddItem("Material");
	Chams.AddItem("Dogtags");
	ChamsGroup.PlaceLabledControl(0, "Chams", this, &Chams);

	ChamsPlayers.SetFileId("esp_chamsplayers");
	ChamsGroup.PlaceLabledControl(0, "Material players", this, &ChamsPlayers);

	ChamsEnemyOnly.SetFileId("esp_chamsenemyonly");
	ChamsGroup.PlaceLabledControl(0, "Material enemy only", this, &ChamsEnemyOnly);


#pragma region Other
	
	OtherGroup.SetText("Other");
	OtherGroup.SetPosition(436, 30);
	OtherGroup.SetSize(210, 346);
	RegisterControl(&OtherGroup);

	AutowallCrosshair.SetFileId("otr_autowallcross");
	OtherGroup.PlaceLabledControl(0, "Autowall crosshair", this, &AutowallCrosshair);

	SpreadCross.SetFileId("otr_spreadcross");
	OtherGroup.PlaceLabledControl(0, "Spread crosshair", this, &SpreadCross);

	OtherThirdpersonAngle.SetFileId("aa_tpAngle");
	OtherThirdpersonAngle.AddItem("Show fake");
	OtherThirdpersonAngle.AddItem("Show real");
	OtherGroup.PlaceLabledControl(0, "Thirdperson angles", this, &OtherThirdpersonAngle);

	FakeAngleChams.SetFileId("otr_anglechams");
	FakeAngleChams.AddItem("Off");
	FakeAngleChams.AddItem("Fake angles");
	FakeAngleChams.AddItem("LBY angles");
	OtherGroup.PlaceLabledControl(0, "Angle chams", this, &FakeAngleChams);

	AAIndicators.SetFileId("otr_aaindicators");
	OtherGroup.PlaceLabledControl(0, "AA indicator", this, &AAIndicators);

	SpecList.SetFileId("otr_speclist");
	OtherGroup.PlaceLabledControl(0, "Spectators", this, &SpecList);

	OtherNoVisualRecoil.SetFileId("otr_visrecoil");
	OtherGroup.PlaceLabledControl(0, "No visual recoil", this, &OtherNoVisualRecoil);

	OtherNoFlash.SetFileId("otr_noflash");
	OtherGroup.PlaceLabledControl(0, "Remove flash effect", this, &OtherNoFlash);

	OtherNoSmoke.SetFileId("otr_nosmoke");
	OtherGroup.PlaceLabledControl(0, "Remove smoke effect", this, &OtherNoSmoke);

	OtherNoScope.SetFileId("otr_noscope");
	OtherGroup.PlaceLabledControl(0, "Remove scope", this, &OtherNoScope);

	OtherViewmodelFOV.SetFileId("otr_viewfov");
	OtherViewmodelFOV.SetBoundaries(0.f, 180.f);
	OtherViewmodelFOV.SetValue(0.f);
	OtherGroup.PlaceLabledControl(0, "Viewmodel fov", this, &OtherViewmodelFOV);

	OtherFOV.SetFileId("otr_fov");
	OtherFOV.SetBoundaries(0.f, 70.f);
	OtherFOV.SetValue(0.f);
	OtherGroup.PlaceLabledControl(0, "Override fov", this, &OtherFOV);

	BulletTracers.SetFileId("otr_btracers");
	OtherGroup.PlaceLabledControl(0, "Bullet tracers", this, &BulletTracers);


	DamageIndicator.SetFileId("otr_dmgindicator");
	OtherGroup.PlaceLabledControl(0, "Damage indicators", this, &DamageIndicator);



#pragma endregion Setting up the Other controls
}

void CMiscTab::Setup()
{
	SetTitle("d");

#pragma region Other
	
	OtherGroup.SetPosition(16, 30);
	OtherGroup.SetSize(300, 250);
	OtherGroup.SetText("Other");
	RegisterControl(&OtherGroup);

	OtherSafeMode.SetFileId("otr_safemode");
	OtherSafeMode.SetState(true);
	OtherGroup.PlaceLabledControl(0, "Anti untrusted", this, &OtherSafeMode);

	AutoPistol.SetFileId("otr_autopistol");
	OtherGroup.PlaceLabledControl(0, "Auto pistol", this, &AutoPistol);

	OtherAutoJump.SetFileId("otr_autojump");
	OtherGroup.PlaceLabledControl(0, "Auto jump", this, &OtherAutoJump);

	OtherAutoStrafe.SetFileId("otr_strafe");
	OtherAutoStrafe.AddItem("Off");
	OtherAutoStrafe.AddItem("Legit");
	OtherAutoStrafe.AddItem("Rage");
	OtherGroup.PlaceLabledControl(0, "Auto strafer", this, &OtherAutoStrafe);

	ClanTag.SetFileId("otr_clantag");
	OtherGroup.PlaceLabledControl(0, "Spam Clantag", this, &ClanTag);

	OtherThirdperson.SetFileId("aa_thirdpsr");
	OtherGroup.PlaceLabledControl(0, "Thirdperson", this, &OtherThirdperson);

	ThirdPersonKeyBind.SetFileId("aa_thirpbind");
	OtherGroup.PlaceLabledControl(0, "Thirdperson key", this, &ThirdPersonKeyBind);
	
	OtherAirStuck.SetFileId("otr_astuck");
	OtherGroup.PlaceLabledControl(0, "Air stuck", this, &OtherAirStuck);

	FakePingExploit.SetFileId("otr_fakepingexploit");
	OtherGroup.PlaceLabledControl(0, "Fake ping exploit", this, &FakePingExploit);

	FakePingValue.SetFileId("otr_fakepingspike");
	FakePingValue.SetBoundaries(0, 800);
	FakePingValue.SetValue(0);
	OtherGroup.PlaceLabledControl(0, "Fake ping spike", this, &FakePingValue);

#pragma endregion other random options

#pragma region FakeLag
	FakeLagGroup.SetPosition(16, 310);
	FakeLagGroup.SetSize(300, 105);
	FakeLagGroup.SetText("Fakelag");
	RegisterControl(&FakeLagGroup);

	FakeLagEnable.SetFileId("fakelag_enable");
	FakeLagGroup.PlaceLabledControl(0, "Enable", this, &FakeLagEnable);

	FakeLagChoke.SetFileId("fakelag_choke");
	FakeLagChoke.SetBoundaries(0, 14);
	FakeLagChoke.SetValue(0);
	FakeLagGroup.PlaceLabledControl(0, "Choke factor", this, &FakeLagChoke);

	FakelagOnground.SetFileId("fakelag_onground");
	FakeLagGroup.PlaceLabledControl(0, "On ground", this, &FakelagOnground);

	FakeLagTyp.SetFileId("fakelag_typ");
	FakeLagTyp.AddItem("Maximum");
	FakeLagTyp.AddItem("Adaptive");
	FakeLagTyp.AddItem("Fluctuate");
	FakeLagGroup.PlaceLabledControl(0, "Mode", this, &FakeLagTyp);

#pragma endregion fakelag shit
	/*
#pragma region Buybot
	BuyBotGroup.SetPosition(16, 105);
	BuyBotGroup.SetSize(360, 150);
	BuyBotGroup.SetText("Buybot");
	RegisterControl(&BuyBotGroup);

	EnableBuyBot.SetFileId("bb_enable");
	BuyBotGroup.PlaceLabledControl("Enable", this, &EnableBuyBot);

	BuyBot.SetFileId("buybot");
	BuyBot.AddItem("Off");
	BuyBot.AddItem("Ak/M4");
	BuyBot.AddItem("AWP");
	BuyBot.AddItem("Auto + Duals");
	BuyBotGroup.PlaceLabledControl("Buy Bot", this, &BuyBot);

	BuyBotGrenades.SetFileId("buybot_grenades");
	BuyBotGrenades.AddItem("Off");
	BuyBotGrenades.AddItem("Flash + Smoke + HE");
	BuyBotGrenades.AddItem("Flash + Smoke + HE + Molo");
	BuyBotGroup.PlaceLabledControl("Buy Grenades", this, &BuyBotGrenades);

	BuyBotKevlar.SetFileId("buybot_kevlar");
	BuyBotGroup.PlaceLabledControl("Buy Armor", this, &BuyBotKevlar);

	BuyBotDefuser.SetFileId("buybot_defuser");
	BuyBotGroup.PlaceLabledControl("Buy Defuse Kit", this, &BuyBotDefuser);
	*/

#pragma endregion Buybot
}

void CColorTab::Setup()
{
	SetTitle("i");
#pragma region Visual Colors
	
	 ColorsGroup.SetText("Colors");
	 ColorsGroup.SetPosition(16, 30);
	 ColorsGroup.SetSize(300, 346);
	 RegisterControl(&ColorsGroup);


	 BoxR.SetFileId("BoxRed");
	 BoxR.SetBoundaries(0.f, 255.f);
	 BoxR.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Bounding box red", this, &BoxR);

	 BoxG.SetFileId("BoxGreen");
	 BoxG.SetBoundaries(0.f, 255.f);
	 BoxG.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Bounding box green", this, &BoxG);

	 BoxB.SetFileId("BoxBlue");
	 BoxB.SetBoundaries(0.f, 255.f);
	 BoxB.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Bounding box blue", this, &BoxB);



	 GlowR.SetFileId("GlowR");
	 GlowR.SetBoundaries(0.f, 255.f);
	 GlowR.SetValue(230.f);
	 ColorsGroup.PlaceLabledControl(0, "Glow red", this, &GlowR);

	 GlowG.SetFileId("GlowG");
	 GlowG.SetBoundaries(0.f, 255.f);
	 GlowG.SetValue(0.f);
	 ColorsGroup.PlaceLabledControl(0, "Glow green", this, &GlowG);

	 GlowB.SetFileId("GlowB");
	 GlowB.SetBoundaries(0.f, 255.f);
	 GlowB.SetValue(0.f);
	 ColorsGroup.PlaceLabledControl(0, "Glow blue", this, &GlowB);



	 ChamsVisRed.SetFileId("chamsvisred");
	 ChamsVisRed.SetBoundaries(0.f, 255.f);
	 ChamsVisRed.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Chams visible red", this, &ChamsVisRed);

	 ChamsVisGreen.SetFileId("chamsvisgreen");
	 ChamsVisGreen.SetBoundaries(0.f, 255.f);
	 ChamsVisGreen.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Chams visible green", this, &ChamsVisGreen);

	 ChamsVisBlue.SetFileId("chamsvisblue");
	 ChamsVisBlue.SetBoundaries(0.f, 255.f);
	 ChamsVisBlue.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Chams visible blue", this, &ChamsVisBlue);



	 ChamsNotVisRed.SetFileId("chamsnotvisred");
	 ChamsNotVisRed.SetBoundaries(0.f, 255.f);
	 ChamsNotVisRed.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Chams not visible red", this, &ChamsNotVisRed);

	 ChamsNotVisGreen.SetFileId("chamsnotvisgreen");
	 ChamsNotVisGreen.SetBoundaries(0.f, 255.f);
	 ChamsNotVisGreen.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Chams not visible green", this, &ChamsNotVisGreen);

	 ChamsNotVisBlue.SetFileId("chamsnotvisblue");
	 ChamsNotVisBlue.SetBoundaries(0.f, 255.f);
	 ChamsNotVisBlue.SetValue(255.f);
	 ColorsGroup.PlaceLabledControl(0, "Chams not visible blue", this, &ChamsNotVisBlue);

	
	
	ConfigGroup.SetPosition(350, 200);
	ConfigGroup.SetText("Configs");
	ConfigGroup.SetSize(300, 100);
	RegisterControl(&ConfigGroup);

	ConfigBox.SetFileId("cfg_box");
	ConfigBox.AddItem("legit");
	ConfigBox.AddItem("rage");
	ConfigBox.AddItem("mm hvh");
	ConfigBox.AddItem("casual");
	ConfigGroup.PlaceLabledControl(0, "Config", this, &ConfigBox);
	
#pragma endregion

#pragma region GloveModel
	Gloves.SetPosition(350, 30);
	Gloves.SetText("Glove Changer");
	Gloves.SetSize(300, 100);
	RegisterControl(&Gloves);

	EnableGloves.SetFileId("gloves_enable");
	Gloves.PlaceLabledControl(0, "Enable", this, &EnableGloves);

	GloveModel.SetFileId("glove_model");
	GloveModel.AddItem("Off");
	GloveModel.AddItem("Bloodhound");
	GloveModel.AddItem("Handwrap");
	GloveModel.AddItem("Driver");
	GloveModel.AddItem("Sport");
	GloveModel.AddItem("Motorcycle");
	GloveModel.AddItem("Specialist");
	Gloves.PlaceLabledControl(0, "Glove Model", this, &GloveModel);

	GloveSkin.SetFileId("glove_skin");
	GloveSkin.AddItem("Off");
	GloveSkin.AddItem("Bloodhound - Snakebite");
	GloveSkin.AddItem("Bloodhound - Charred");
	GloveSkin.AddItem("Bloodhound - Guerrilla");
	GloveSkin.AddItem("Bloodhound - Bronzed");
	GloveSkin.AddItem("Handwrap - Slaughter");
	GloveSkin.AddItem("Handwrap - Badlands");
	GloveSkin.AddItem("Handwrap - Leather");
	GloveSkin.AddItem("Handwrap - Spruce DDPAT");
	GloveSkin.AddItem("Driver - Crimson Weave");
	GloveSkin.AddItem("Driver - Lunar Weave");
	GloveSkin.AddItem("Driver - Diamondback");
	GloveSkin.AddItem("Driver - Convoy");
	GloveSkin.AddItem("Sport - Pandoras Box");
	GloveSkin.AddItem("Sport - Hedge Naze");
	GloveSkin.AddItem("Sport - Superconductor");
	GloveSkin.AddItem("Sport - Arid");
	GloveSkin.AddItem("Motorcycle - Spearmint");
	GloveSkin.AddItem("Motorcycle - Cool Mint");
	GloveSkin.AddItem("Motorcycle - Boom");
	GloveSkin.AddItem("Motorcycle - Eclipse");
	GloveSkin.AddItem("Specialist - Crimson Kimono");
	GloveSkin.AddItem("Specialist - Emerald Web");
	GloveSkin.AddItem("Specialist - Foundation");
	GloveSkin.AddItem("Specialist - Forest DDPAT");
	Gloves.PlaceLabledControl(0, "Glove Skin", this, &GloveSkin);

	GlovesApply.SetText("Apply Gloves");
	GlovesApply.SetCallback(GlovesApplyCallbk);
	GlovesApply.SetPosition(350, 150);
	GlovesApply.SetSize(200, 106);
	RegisterControl(&GlovesApply);

	

#pragma endregion
}

void Options::SetupMenu()
{
	Menu.Setup();

	GUI.RegisterWindow(&Menu);
	GUI.BindWindow(VK_INSERT, &Menu);
}

void Options::DoUIFrame()
{
	if (Menu.VisualsTab.FiltersAll.GetState())
	{
		Menu.VisualsTab.FiltersC4.SetState(true);
		Menu.VisualsTab.FiltersPlayers.SetState(true);
		Menu.VisualsTab.FiltersWeapons.SetState(true);
		Menu.VisualsTab.FiltersNades.SetState(true);
	}

	GUI.Update();
	GUI.Draw();
	
}