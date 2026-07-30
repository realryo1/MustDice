/*==============================================================================

   日本語フォント描画システム実装 [font.cpp]
										  Author : Copilot
										  Date   : 2025/01/10
--------------------------------------------------------------------------------

==============================================================================*/
#include "font.h"
#include "renderer.h"
#include "shadermanager.h"

#include "define.h"
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <d3d11.h>
#include <d3dcompiler.h>

#include <ft2build.h>
#include FT_FREETYPE_H

using namespace DirectX;

#pragma comment(lib, "d3dcompiler.lib")
#ifdef _DEBUG
#pragma comment(lib, "freetype_d.lib")
#else
#pragma comment(lib, "freetype.lib")
#endif

// ==========================================
// グローバルフォントデータ（全 FontRenderer で共有）
// ==========================================
static unsigned char* g_pGlobalFontData = nullptr;
static int g_GlobalFontDataSize = 0;
static FT_Library g_FtLibrary = nullptr;
static FT_Face g_FtFace = nullptr;
static FT_UInt g_FtCurrentPixelSize = 0;

void Font_InitializeGlobalData()
{
	if (g_FtFace != nullptr) {
		return; // 既に初期化済み
	}

	FILE* f = nullptr;
	fopen_s(&f, "asset/font/KaiseiDecol-Medium.ttf", "rb");
	if (!f) {
		return;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	fseek(f, 0, SEEK_SET);

	g_pGlobalFontData = (unsigned char*)malloc(size);
	if (!g_pGlobalFontData) {
		fclose(f);
		return;
	}

	fread(g_pGlobalFontData, 1, size, f);
	fclose(f);
	g_GlobalFontDataSize = (int)size;

	if (FT_Init_FreeType(&g_FtLibrary) != 0) {
		free(g_pGlobalFontData);
		g_pGlobalFontData = nullptr;
		g_GlobalFontDataSize = 0;
		g_FtLibrary = nullptr;
		return;
	}

	if (FT_New_Memory_Face(g_FtLibrary, g_pGlobalFontData, g_GlobalFontDataSize, 0, &g_FtFace) != 0) {
		FT_Done_FreeType(g_FtLibrary);
		free(g_pGlobalFontData);
		g_pGlobalFontData = nullptr;
		g_GlobalFontDataSize = 0;
		g_FtLibrary = nullptr;
		g_FtFace = nullptr;
		return;
	}

	g_FtCurrentPixelSize = 0;
}

void Font_FinalizeGlobalData()
{
	if (g_FtFace) {
		FT_Done_Face(g_FtFace);
		g_FtFace = nullptr;
	}
	if (g_FtLibrary) {
		FT_Done_FreeType(g_FtLibrary);
		g_FtLibrary = nullptr;
	}
	if (g_pGlobalFontData) {
		free(g_pGlobalFontData);
		g_pGlobalFontData = nullptr;
		g_GlobalFontDataSize = 0;
	}
	g_FtCurrentPixelSize = 0;
}

// ==========================================
// FontRenderer クラス実装
// ==========================================

FontRenderer::FontRenderer(XMFLOAT2 pos, float fontSize, float rotation,
	XMFLOAT4 color, const std::string& text, TextAlignment align)
	: Transform2D(pos, rotation, { 1.0f, 1.0f }), m_Color(color), m_Text(text),
	m_FontSize(fontSize), m_Alignment(align),
	m_pTexture(nullptr), m_pSRV(nullptr),
	m_pVertexBuffer(nullptr),
	m_VertexCount(0), m_AtlasWidth(0), m_AtlasHeight(0),
	m_AtlasNextX(0), m_AtlasNextY(0), m_AtlasRowHeight(0),
	m_pAtlasData(nullptr),
	m_FontAscender(0), m_FontDescender(0),
	m_Ready(false)
{
	if (!CreateShaders()) {
		return;
	}

	if (!BakeAtlas()) {
		return;
	}
}

FontRenderer::~FontRenderer() {
	if (m_pVertexBuffer) m_pVertexBuffer->Release();
	if (m_pSRV) m_pSRV->Release();
	if (m_pTexture) m_pTexture->Release();
	if (m_pAtlasData) free(m_pAtlasData);
}

bool FontRenderer::CreateShaders() {
	return true;
}

bool FontRenderer::EnsurePixelSize() {
	if (!g_FtFace) {
		return false;
	}

	FT_UInt pixelHeight = (FT_UInt)(m_FontSize * DRAW_SCALE_X + 0.5f);
	if (pixelHeight < 1) {
		pixelHeight = 1;
	}

	if (g_FtCurrentPixelSize != pixelHeight) {
		if (FT_Set_Pixel_Sizes(g_FtFace, 0, pixelHeight) != 0) {
			return false;
		}
		g_FtCurrentPixelSize = pixelHeight;
	}
	return true;
}

float FontRenderer::GetKerningPx(int prevGlyph, int glyphIndex) const {
	if (!g_FtFace || prevGlyph <= 0 || glyphIndex <= 0) {
		return 0.0f;
	}

	// TT_CONFIG_OPTION_GPOS_KERNING 有効時は GPOS ペアも FT_Get_Kerning 経由で取得できる
	FT_Vector delta;
	if (FT_Get_Kerning(g_FtFace, (FT_UInt)prevGlyph, (FT_UInt)glyphIndex, FT_KERNING_DEFAULT, &delta) != 0) {
		return 0.0f;
	}
	return (float)delta.x / 64.0f;
}

float FontRenderer::GetGlyphAdvancePx(int glyphIndex) {
	if (!g_FtFace || !EnsurePixelSize()) {
		return 0.0f;
	}

	if (FT_Load_Glyph(g_FtFace, (FT_UInt)glyphIndex, FT_LOAD_DEFAULT) != 0) {
		return 0.0f;
	}
	return (float)g_FtFace->glyph->advance.x / 64.0f;
}

bool FontRenderer::BakeAtlas() {
	ID3D11Device* pDevice = GetDevice();

	if (!pDevice) {
		return false;
	}

	if (!g_FtFace) {
		return false;
	}

	m_Ready = true;
	m_FontAscender = (int)g_FtFace->ascender;
	m_FontDescender = (int)g_FtFace->descender;

	m_AtlasWidth = FONT_ATLAS_WIDTH;
	m_AtlasHeight = FONT_ATLAS_HEIGHT;
	m_pAtlasData = (unsigned char*)calloc(m_AtlasWidth * m_AtlasHeight, 1);

	if (!m_pAtlasData) {
		m_Ready = false;
		return false;
	}

	m_AtlasNextX = 0;
	m_AtlasNextY = 0;
	m_AtlasRowHeight = 0;

	D3D11_TEXTURE2D_DESC desc = {};
	desc.Width = m_AtlasWidth;
	desc.Height = m_AtlasHeight;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	unsigned char* atlasRGBA = (unsigned char*)malloc(m_AtlasWidth * m_AtlasHeight * 4);
	if (!atlasRGBA) {
		free(m_pAtlasData);
		m_pAtlasData = nullptr;
		m_Ready = false;
		return false;
	}

	for (int i = 0; i < m_AtlasWidth * m_AtlasHeight; i++) {
		atlasRGBA[i * 4 + 0] = 255;
		atlasRGBA[i * 4 + 1] = 255;
		atlasRGBA[i * 4 + 2] = 255;
		atlasRGBA[i * 4 + 3] = 255;
	}

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = atlasRGBA;
	initData.SysMemPitch = m_AtlasWidth * 4;

	HRESULT hr = pDevice->CreateTexture2D(&desc, &initData, &m_pTexture);
	if (FAILED(hr)) {
		free(atlasRGBA);
		free(m_pAtlasData);
		m_pAtlasData = nullptr;
		m_Ready = false;
		return false;
	}

	hr = pDevice->CreateShaderResourceView(m_pTexture, nullptr, &m_pSRV);
	if (FAILED(hr)) {
		m_pTexture->Release();
		m_pTexture = nullptr;
		free(atlasRGBA);
		free(m_pAtlasData);
		m_pAtlasData = nullptr;
		m_Ready = false;
		return false;
	}

	struct Vertex {
		XMFLOAT3 position;
		XMFLOAT3 normal;
		XMFLOAT4 color;
		XMFLOAT2 texCoord;
	};

	Vertex vertices[] = {
		{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f } },
		{ { 1.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 0.0f } },
		{ { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f } },
		{ { 1.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f } }
	};

	D3D11_BUFFER_DESC vbDesc = {};
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.ByteWidth = sizeof(vertices);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	D3D11_SUBRESOURCE_DATA vbData = {};
	vbData.pSysMem = vertices;

	hr = pDevice->CreateBuffer(&vbDesc, &vbData, &m_pVertexBuffer);
	if (FAILED(hr)) {
		m_pSRV->Release();
		m_pTexture->Release();
		free(atlasRGBA);
		free(m_pAtlasData);
		m_pSRV = nullptr;
		m_pTexture = nullptr;
		m_pAtlasData = nullptr;
		m_Ready = false;
		return false;
	}

	m_VertexCount = 4;
	free(atlasRGBA);
	return true;
}

int FontRenderer::UTF8ToCodePoint(const std::string& text, size_t& index) {
	unsigned char c = (unsigned char)text[index];

	if ((c & 0x80) == 0) {
		index++;
		return (int)c;
	}

	if ((c & 0xE0) == 0xC0) {
		int codepoint = ((c & 0x1F) << 6) | ((unsigned char)text[index + 1] & 0x3F);
		index += 2;
		return codepoint;
	}

	if ((c & 0xF0) == 0xE0) {
		int codepoint = ((c & 0x0F) << 12) | (((unsigned char)text[index + 1] & 0x3F) << 6) | ((unsigned char)text[index + 2] & 0x3F);
		index += 3;
		return codepoint;
	}

	if ((c & 0xF8) == 0xF0) {
		int codepoint = ((c & 0x07) << 18) | (((unsigned char)text[index + 1] & 0x3F) << 12) | (((unsigned char)text[index + 2] & 0x3F) << 6) | ((unsigned char)text[index + 3] & 0x3F);
		index += 4;
		return codepoint;
	}

	index++;
	return 0;
}

bool FontRenderer::AddGlyphToAtlas(int glyphIndex) {
	if (!m_Ready || !g_FtFace) {
		return false;
	}

	if (m_CharCache.find(glyphIndex) != m_CharCache.end()) {
		return true;
	}

	if ((int)m_CharCache.size() >= FONT_MAX_CACHE_GLYPHS) {
		EvictLRUGlyph();
	}

	if (!EnsurePixelSize()) {
		return false;
	}

	if (FT_Load_Glyph(g_FtFace, (FT_UInt)glyphIndex, FT_LOAD_RENDER) != 0) {
		return false;
	}

	FT_GlyphSlot slot = g_FtFace->glyph;
	FT_Bitmap& bitmap = slot->bitmap;

	int glyph_width = (int)bitmap.width;
	int glyph_height = (int)bitmap.rows;

	CharInfo info = {};
	info.glyphIndex = glyphIndex;
	info.xadvance = (float)slot->advance.x / 64.0f;
	info.bitmapLeft = (float)slot->bitmap_left;
	info.bitmapTop = (float)slot->bitmap_top;

	if (glyph_width == 0 || glyph_height == 0) {
		info.x0 = info.y0 = info.x1 = info.y1 = 0.0f;
		m_CharCache[glyphIndex] = info;
		m_CacheLRU.push_back(glyphIndex);
		return true;
	}

	if (m_AtlasNextX + glyph_width > m_AtlasWidth) {
		m_AtlasNextX = 0;
		m_AtlasNextY += m_AtlasRowHeight;
		m_AtlasRowHeight = 0;
	}

	if (m_AtlasNextY + glyph_height > m_AtlasHeight) {
		return false;
	}

	const int pitch = bitmap.pitch;
	for (int y = 0; y < glyph_height; y++) {
		const unsigned char* srcRow = bitmap.buffer + y * pitch;
		for (int x = 0; x < glyph_width; x++) {
			int atlas_idx = (m_AtlasNextY + y) * m_AtlasWidth + (m_AtlasNextX + x);
			m_pAtlasData[atlas_idx] = srcRow[x];
		}
	}

	info.x0 = (float)m_AtlasNextX;
	info.y0 = (float)m_AtlasNextY;
	info.x1 = (float)(m_AtlasNextX + glyph_width);
	info.y1 = (float)(m_AtlasNextY + glyph_height);

	m_CharCache[glyphIndex] = info;
	m_CacheLRU.push_back(glyphIndex);

	m_AtlasNextX += glyph_width;
	m_AtlasRowHeight = (std::max)(m_AtlasRowHeight, glyph_height);

	UpdateAtlasTexture();
	return true;
}

bool FontRenderer::AddGlyphToAtlasBatch(int glyphIndex) {
	if (!m_Ready || !g_FtFace) {
		return false;
	}

	if (m_CharCache.find(glyphIndex) != m_CharCache.end()) {
		return false;
	}

	if ((int)m_CharCache.size() >= FONT_MAX_CACHE_GLYPHS) {
		EvictLRUGlyph();
	}

	if (!EnsurePixelSize()) {
		return false;
	}

	if (FT_Load_Glyph(g_FtFace, (FT_UInt)glyphIndex, FT_LOAD_RENDER) != 0) {
		return false;
	}

	FT_GlyphSlot slot = g_FtFace->glyph;
	FT_Bitmap& bitmap = slot->bitmap;

	int glyph_width = (int)bitmap.width;
	int glyph_height = (int)bitmap.rows;

	CharInfo info = {};
	info.glyphIndex = glyphIndex;
	info.xadvance = (float)slot->advance.x / 64.0f;
	info.bitmapLeft = (float)slot->bitmap_left;
	info.bitmapTop = (float)slot->bitmap_top;

	if (glyph_width == 0 || glyph_height == 0) {
		info.x0 = info.y0 = info.x1 = info.y1 = 0.0f;
		m_CharCache[glyphIndex] = info;
		m_CacheLRU.push_back(glyphIndex);
		return false;
	}

	if (m_AtlasNextX + glyph_width > m_AtlasWidth) {
		m_AtlasNextX = 0;
		m_AtlasNextY += m_AtlasRowHeight;
		m_AtlasRowHeight = 0;
	}

	if (m_AtlasNextY + glyph_height > m_AtlasHeight) {
		return false;
	}

	const int pitch = bitmap.pitch;
	for (int y = 0; y < glyph_height; y++) {
		const unsigned char* srcRow = bitmap.buffer + y * pitch;
		for (int x = 0; x < glyph_width; x++) {
			int atlas_idx = (m_AtlasNextY + y) * m_AtlasWidth + (m_AtlasNextX + x);
			m_pAtlasData[atlas_idx] = srcRow[x];
		}
	}

	info.x0 = (float)m_AtlasNextX;
	info.y0 = (float)m_AtlasNextY;
	info.x1 = (float)(m_AtlasNextX + glyph_width);
	info.y1 = (float)(m_AtlasNextY + glyph_height);

	m_CharCache[glyphIndex] = info;
	m_CacheLRU.push_back(glyphIndex);

	m_AtlasNextX += glyph_width;
	m_AtlasRowHeight = (std::max)(m_AtlasRowHeight, glyph_height);

	return true;
}

void FontRenderer::EvictLRUGlyph() {
	if (m_CacheLRU.empty()) {
		return;
	}

	int lru_glyph = m_CacheLRU.front();
	m_CacheLRU.pop_front();
	m_CharCache.erase(lru_glyph);
}

void FontRenderer::UpdateAtlasTexture() {
	ID3D11DeviceContext* pContext = GetDeviceContext();
	if (!pContext || !m_pTexture) {
		return;
	}

	unsigned char* atlasRGBA = (unsigned char*)malloc(m_AtlasWidth * m_AtlasHeight * 4);
	for (int i = 0; i < m_AtlasWidth * m_AtlasHeight; i++) {
		atlasRGBA[i * 4 + 0] = (unsigned char)(m_Color.x * 255.0f);
		atlasRGBA[i * 4 + 1] = (unsigned char)(m_Color.y * 255.0f);
		atlasRGBA[i * 4 + 2] = (unsigned char)(m_Color.z * 255.0f);
		atlasRGBA[i * 4 + 3] = m_pAtlasData[i];
	}

	D3D11_MAPPED_SUBRESOURCE msr;
	HRESULT hr = pContext->Map(m_pTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);
	if (SUCCEEDED(hr)) {
		memcpy(msr.pData, atlasRGBA, m_AtlasWidth * m_AtlasHeight * 4);
		pContext->Unmap(m_pTexture, 0);
	}

	free(atlasRGBA);
}

void FontRenderer::Draw() {
	ID3D11DeviceContext* pContext = GetDeviceContext();
	ShaderManager* shader = GetShader(S_UNLIT);

	if (!pContext || !shader || !shader->GetVertexLayout() || !shader->GetVertexShader() || !shader->GetPixelShader()) {
		return;
	}

	if (!m_pVertexBuffer || !m_pSRV || !m_Ready || !g_FtFace) {
		return;
	}

	if (!EnsurePixelSize()) {
		return;
	}

	SetWorldMatrix(XMMatrixIdentity());
	SetViewMatrix(XMMatrixIdentity());
	SetProjectionMatrix(XMMatrixOrthographicOffCenterLH(0.0f, DRAW_SCREEN_WIDTH, DRAW_SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f));

	SetDepthEnable(false);

	pContext->IASetInputLayout(shader->GetVertexLayout());
	pContext->VSSetShader(shader->GetVertexShader(), NULL, 0);
	pContext->PSSetShader(shader->GetPixelShader(), NULL, 0);

	pContext->PSSetShaderResources(0, 1, &m_pSRV);
	SetBlendState(BLENDSTATE_ALFA);

	// テキスト全体の幅を計算（4K単位）
	float text_width = 0.0f;
	size_t temp_i = 0;
	int prev_glyph = 0;
	while (temp_i < m_Text.length()) {
		int codepoint = UTF8ToCodePoint(m_Text, temp_i);
		if (codepoint <= 0) continue;

		FT_UInt glyph_index = FT_Get_Char_Index(g_FtFace, (FT_ULong)codepoint);
		if (glyph_index == 0) continue;

		float kerning = GetKerningPx(prev_glyph, (int)glyph_index);

		if (codepoint == 0x0020 || codepoint == 0x3000) {
			text_width += kerning + GetGlyphAdvancePx((int)glyph_index);
			prev_glyph = (int)glyph_index;
			continue;
		}

		if (!AddGlyphToAtlas((int)glyph_index)) continue;

		CharInfo& info = m_CharCache[(int)glyph_index];
		float actual_glyph_width = info.x1 - info.x0;
		float margin = actual_glyph_width * FONT_MARGIN_RATIO;

		text_width += kerning + info.xadvance + margin;
		prev_glyph = (int)glyph_index;
	}

	float scX = GetScaleX();
	float scY = GetScaleY();

	float draw_start_x = m_Position.x * DRAW_SCALE_X;
	if (m_Alignment == TA_MIDDLE) {
		draw_start_x -= (text_width * scX) / 2.0f;
	} else if (m_Alignment == TA_END) {
		draw_start_x -= (text_width * scX);
	}
	float draw_current_x = draw_start_x;
	float draw_current_y = m_Position.y * DRAW_SCALE_Y;

	size_t i = 0;
	int prev_glyph_draw = 0;
	while (i < m_Text.length()) {
		int codepoint = UTF8ToCodePoint(m_Text, i);
		if (codepoint <= 0) continue;

		FT_UInt glyph_index = FT_Get_Char_Index(g_FtFace, (FT_ULong)codepoint);
		if (glyph_index == 0) continue;

		float kerning = GetKerningPx(prev_glyph_draw, (int)glyph_index);
		draw_current_x += kerning * scX;

		if (codepoint == 0x0020 || codepoint == 0x3000) {
			draw_current_x += GetGlyphAdvancePx((int)glyph_index) * scX;
			prev_glyph_draw = (int)glyph_index;
			continue;
		}

		if (!AddGlyphToAtlas((int)glyph_index)) continue;

		CharInfo& info = m_CharCache[(int)glyph_index];

		float actual_glyph_width = info.x1 - info.x0;
		float actual_glyph_height = info.y1 - info.y0;
		float margin = actual_glyph_width * FONT_MARGIN_RATIO;

		float u0 = info.x0 / (float)m_AtlasWidth;
		float v0 = info.y0 / (float)m_AtlasHeight;
		float u1 = info.x1 / (float)m_AtlasWidth;
		float v1 = info.y1 / (float)m_AtlasHeight;

		// FreeType: bitmap_top はベースラインからの上方向。
		// 既存の FONT_OFFSET_Y を足しつつ、stb の y0 相当（= -bitmap_top）で配置する。
		float y0 = -info.bitmapTop;
		float y_offset = draw_current_y + (y0 + m_FontSize * FONT_OFFSET_Y * DRAW_SCALE_Y) * scY;
		float glyph_x = draw_current_x + info.bitmapLeft * scX;

		float char_pixel_width = actual_glyph_width * scX;
		float char_pixel_height = actual_glyph_height * scY;

		D3D11_MAPPED_SUBRESOURCE msr;
		pContext->Map(m_pVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &msr);

		struct Vertex {
			XMFLOAT3 position;
			XMFLOAT3 normal;
			XMFLOAT4 color;
			XMFLOAT2 texCoord;
		};

		Vertex* v = (Vertex*)msr.pData;

		v[0].position = { glyph_x, y_offset, 0.0f };
		v[0].texCoord = { u0, v0 };
		v[0].normal = { 0.0f, 0.0f, 0.0f };
		v[0].color = m_Color;

		v[1].position = { glyph_x + char_pixel_width, y_offset, 0.0f };
		v[1].texCoord = { u1, v0 };
		v[1].normal = { 0.0f, 0.0f, 0.0f };
		v[1].color = m_Color;

		v[2].position = { glyph_x, y_offset + char_pixel_height, 0.0f };
		v[2].texCoord = { u0, v1 };
		v[2].normal = { 0.0f, 0.0f, 0.0f };
		v[2].color = m_Color;

		v[3].position = { glyph_x + char_pixel_width, y_offset + char_pixel_height, 0.0f };
		v[3].texCoord = { u1, v1 };
		v[3].normal = { 0.0f, 0.0f, 0.0f };
		v[3].color = m_Color;

		pContext->Unmap(m_pVertexBuffer, 0);

		UINT stride = sizeof(Vertex);
		UINT offset = 0;
		pContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &stride, &offset);
		pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
		pContext->Draw(m_VertexCount, 0);

		draw_current_x += (info.xadvance + margin) * scX;
		prev_glyph_draw = (int)glyph_index;
	}
}

void FontRenderer::SetText(const std::string& text) {
	m_Text = text;
}

void FontRenderer::PreCacheGlyphs() {
	if (!m_Ready || !g_FtFace) return;
	if (!EnsurePixelSize()) return;

	bool atlasUpdated = false;
	size_t idx = 0;
	while (idx < m_Text.length()) {
		int codepoint = UTF8ToCodePoint(m_Text, idx);
		if (codepoint <= 0) continue;
		if (codepoint == 0x0020 || codepoint == 0x3000) continue;

		FT_UInt glyph_index = FT_Get_Char_Index(g_FtFace, (FT_ULong)codepoint);
		if (glyph_index == 0) continue;

		if (m_CharCache.find((int)glyph_index) != m_CharCache.end()) continue;

		if (AddGlyphToAtlasBatch((int)glyph_index)) {
			atlasUpdated = true;
		}
	}

	if (atlasUpdated) {
		UpdateAtlasTexture();
	}
}
