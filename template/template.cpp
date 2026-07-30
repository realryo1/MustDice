#include "template.h"
#include "define.h"
#include "font.h"
#include "input_manager.h"
#include "fade.h"
#include "scene.h"

using namespace DirectX;

static DrawFont* g_pTemplateText = nullptr;
static DrawFont* g_pHintText = nullptr;

void Template_Initialize(void)
{
	g_pTemplateText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f - 40.0f },
		48.0f,
		0.0f,
		{ 1.0f, 1.0f, 1.0f, 1.0f },
		"TEMPLATE"
	);

	g_pHintText = new DrawFont(
		{ SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f + 40.0f },
		28.0f,
		0.0f,
		{ 0.8f, 0.8f, 0.8f, 1.0f },
		"Press Decide"
	);
}

void Template_Update(void)
{
	if (Input_IsActionTrigger(INPUT_ACTION_DECIDE))
	{
		// 遷移先シーンは用途に合わせて変更する
		SetSceneFade(SCENE_TITLE);
	}
}

void Template_Draw(void)
{
	if (g_pTemplateText) g_pTemplateText->Draw();
	if (g_pHintText) g_pHintText->Draw();
}

void Template_Finalize(void)
{
	SAFE_DELETE(g_pTemplateText);
	SAFE_DELETE(g_pHintText);
}
