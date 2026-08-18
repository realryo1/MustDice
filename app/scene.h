#pragma once

enum SCENE {
	SCENE_TITLE = 0,
	SCENE_GAME,
	SCENE_SHOP,
	SCENE_RESULT,
	SCENE_MULTILOBBY,
	SCENE_MULTIGAME,
	SCENE_MULTIRESULT,
	SCENE_MAX,
	SCENE_NONE,
	SCENE_DEBUG,
};

void Init(void);
void Update(void);
void Draw(void);
void Finalize(void);

// 内部用。シーン遷移は SetSceneFade（fade.h）を使うこと
void ApplySceneInternal(SCENE id);
SCENE GetScene(void);
