#pragma once
#include "Scene.h"

class GameScene : public Scene {
public:
	void BuildObjects() override;
	void OnEnterScene() override;
	void OnLeaveScene() override;
	void ProcessInput() override;
	void Update() override;
	void SyncSceneWithServer() override;

private:
	Vector3 v3TerrainPos;
	Vector3 v3TerrainRotation = Vector3{ 0,0,0 };

	Vector3 v3PlayerPos;
};

//Old_Rotten_Wood_vlzhfekn_2K_Normal.dds
//Old_Concrete_Barrier_vksrdes_Mid_2K_Normal.dds
//TX_PaintedWood_A_NRM.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Fine_American_Road_sjfnch0a_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Street_Curbs_sepxW_Mid_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
//Dirty_Sidewalk_Tiles_ugxjcdpn_2K_Normal.dds
