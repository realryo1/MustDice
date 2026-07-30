#pragma execution_character_set("utf-8")
/*==============================================================================
   デバッグモデルビューアシーン [debug_model_scene.cpp]
   asset\model フォルダ内の .fbx / .glb を自動列挙し、グリッド状に配置して表示する。
==============================================================================*/

#include <windows.h>
#include <string>
#include <vector>
#include <DirectXMath.h>
#include "debug_model_scene.h"
#include "renderer.h"
#include "light.h"
#include "camera.h"
#include "sprite3d.h"
#include "keyboard.h"
#include "debugcamera.h"

using namespace DirectX;

// ======================================================
// 内部構造体
// ======================================================

struct DebugModelEntry
{
	std::string fileName;   // "bathtub.fbx" など
	std::string filePath;   // "asset\\model\\bathtub.fbx"
	Sprite3D*   pModel;     // 読み込んだモデル
};

// ======================================================
// 静的変数
// ======================================================
static std::vector<DebugModelEntry> g_Entries;
static AmbientLight* g_pAmbientLight = nullptr;
static PointLight* g_pPointLight = nullptr;

static const float MODEL_SPACING = 5.0f;
static const int MODEL_COLS = 6;

// ======================================================
// asset\model フォルダを列挙して .fbx / .glb を集める
// ======================================================
static void EnumerateModels()
{
	const char* patterns[] = {
		"asset\\model\\*.fbx",
		"asset\\model\\*.glb",
	};

	for (const char* pattern : patterns)
	{
		WIN32_FIND_DATAA fd;
		HANDLE hFind = FindFirstFileA(pattern, &fd);
		if (hFind == INVALID_HANDLE_VALUE) continue;

		do
		{
			if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

			DebugModelEntry entry;
			entry.fileName = fd.cFileName;
			entry.filePath = "asset\\model\\" + entry.fileName;
			entry.pModel = nullptr;
			g_Entries.push_back(entry);
		} while (FindNextFileA(hFind, &fd));

		FindClose(hFind);
	}
}

// ======================================================
// 列挙結果をグリッド配置して読み込む
// ======================================================
static void LoadAllModels()
{
	for (int i = 0; i < (int)g_Entries.size(); i++)
	{
		int row = i / MODEL_COLS;
		int col = i % MODEL_COLS;
		float x = (col - MODEL_COLS / 2.0f + 0.5f) * MODEL_SPACING;
		float z = (float)row * MODEL_SPACING;

		g_Entries[i].pModel = new Sprite3D(
			{ x, 0.0f, z },
			{ 1.0f, 1.0f, 1.0f },
			{ 0.0f, 0.0f, 0.0f },
			g_Entries[i].filePath.c_str(),
			S_PHONG
		);
	}
}

static void ReleaseAllModels()
{
	for (int i = 0; i < (int)g_Entries.size(); i++)
	{
		if (g_Entries[i].pModel)
		{
			delete g_Entries[i].pModel;
			g_Entries[i].pModel = nullptr;
		}
	}
	g_Entries.clear();
}

static void ReloadAllModels()
{
	ReleaseAllModels();
	EnumerateModels();
	LoadAllModels();
}

// ======================================================
// Initialize
// ======================================================
void DebugModelScene_Initialize(void)
{
	g_pAmbientLight = new AmbientLight(XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f));
	g_pPointLight = new PointLight(
		TRUE,
		XMFLOAT4(0.0f, 5.0f, -5.0f, 1.0f),
		XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f),
		50.0f,
		1.0f
	);

	EnumerateModels();
	LoadAllModels();

	DebugCamera_Initialize();
}

// ======================================================
// Update
// ======================================================
void DebugModelScene_Update(void)
{
	if (Keyboard_IsKeyDownTrigger(KK_R))
	{
		ReloadAllModels();
	}

	DebugCamera_Update();
	SetCameraPosition(GetCamera()->GetPos());
}

// ======================================================
// Draw
// ======================================================
void DebugModelScene_Draw(void)
{
	SetDepthEnable(true);
	if (g_pPointLight && g_pAmbientLight)
	{
		g_pPointLight->Apply(*g_pAmbientLight);
	}

	for (int i = 0; i < (int)g_Entries.size(); i++)
	{
		if (g_Entries[i].pModel)
		{
			g_Entries[i].pModel->Draw();
		}
	}

	SetDepthEnable(false);
	DebugCamera_Draw();
}

// ======================================================
// Finalize
// ======================================================
void DebugModelScene_Finalize(void)
{
	ReleaseAllModels();

	DebugCamera_Finalize();
	Camera_Finalize();

	if (g_pAmbientLight) { delete g_pAmbientLight; g_pAmbientLight = nullptr; }
	if (g_pPointLight) { delete g_pPointLight; g_pPointLight = nullptr; }
}
