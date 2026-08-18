#include "scene.h"
#include "game.h"
#include "shop.h"
#include "renderer.h"
#include "keyboard.h"
#include "texture.h"
#include "title.h"
#include "result.h"
#include "multilobby.h"
#include "multigame.h"
#include "multiresult.h"
#if defined(_DEBUG)
#include "debugscene.h"
#endif
#include "define.h"
#include "main.h"
#include <Windows.h>
using namespace DirectX;

static SCENE scene = SCENE_TITLE;

#if !defined(_DEBUG)
static void AlertDebugSceneUnavailable(const wchar_t* detail)
{
	MessageBoxW(
		NULL,
		detail,
		L"Scene Transition Cancelled",
		MB_OK | MB_ICONWARNING);
}
#endif

void Init( void )
{
#if !defined(_DEBUG)
	// 初期シーンが SCENE_DEBUG のまま Release ビルドされた場合のフォールバック
	if (scene == SCENE_DEBUG)
	{
		AlertDebugSceneUnavailable(
			L"初期シーンが SCENE_DEBUG に設定されていますが、\n"
			L"Release ビルドでは利用できません。\n"
			L"SCENE_TITLE にフォールバックします。");
		scene = SCENE_TITLE;
	}
#endif

	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Initialize();
		break;
		case SCENE_GAME:
		Game_Initialize();
		break;
		case SCENE_SHOP:
		Shop_Initialize();
		break;
		case SCENE_RESULT:
		Result_Initialize();
		break;
#if defined(_DEBUG)
		case SCENE_DEBUG:
		DebugScene_Initialize();
		break;
#endif
		case SCENE_MULTILOBBY:
		Multilobby_Initialize();
		break;
		case SCENE_MULTIGAME:
		Multigame_Initialize();
		break;
		case SCENE_MULTIRESULT:
		Multiresult_Initialize();
		break;
		default:
		break;
	}
}

void Update( void )
{
	if (Keyboard_IsKeyDownTrigger(KK_F2))
	{
		TakeScreenshot();
	}

	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Update();
		break;
		case SCENE_GAME:
		Game_Update();
		break;
		case SCENE_SHOP:
		Shop_Update();
		break;
		case SCENE_RESULT:
		Result_Update();
		break;
#if defined(_DEBUG)
		case SCENE_DEBUG:
		DebugScene_Update();
		break;
#endif
		case SCENE_MULTILOBBY:
		Multilobby_Update();
		break;
		case SCENE_MULTIGAME:
		Multigame_Update();
		break;
		case SCENE_MULTIRESULT:
		Multiresult_Update();
		break;
		default:
		break;
	}
}

void Draw( void )
{
	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Draw();
		break;
		case SCENE_GAME:
		Game_Draw();
		break;
		case SCENE_SHOP:
		Shop_Draw();
		break;
		case SCENE_RESULT:
		Result_Draw();
		break;
#if defined(_DEBUG)
		case SCENE_DEBUG:
		DebugScene_Draw();
		break;
#endif
		case SCENE_MULTILOBBY:
		Multilobby_Draw();
		break;
		case SCENE_MULTIGAME:
		Multigame_Draw();
		break;
		case SCENE_MULTIRESULT:
		Multiresult_Draw();
		break;
		default:
		break;
	}
}

void Finalize( void )
{
	switch ( scene )
	{
		case SCENE_TITLE:
		Title_Finalize();
		break;
		case SCENE_GAME:
		Game_Finalize();
		break;
		case SCENE_SHOP:
		Shop_Finalize();
		break;
		case SCENE_RESULT:
		Result_Finalize();
		break;
#if defined(_DEBUG)
		case SCENE_DEBUG:
		DebugScene_Finalize();
		break;
#endif
		case SCENE_MULTILOBBY:
		Multilobby_Finalize();
		break;
		case SCENE_MULTIGAME:
		Multigame_Finalize();
		break;
		case SCENE_MULTIRESULT:
		Multiresult_Finalize();
		break;
		default:
		break;
	}
}

// 内部用。シーン遷移は SetSceneFade（fade.h）を使うこと
void ApplySceneInternal( SCENE id )
{
#if !defined(_DEBUG)
	if (id == SCENE_DEBUG)
	{
		AlertDebugSceneUnavailable(
			L"SCENE_DEBUG は Release ビルドでは利用できません。\n"
			L"シーン遷移をキャンセルしました。");
		return;
	}
#endif

	Finalize();

	scene = id;

	Init();
	RequestRedraw();
}

SCENE GetScene( void )
{
	return scene;
}
