#include "texture.h"
#include <Windows.h>
#include <cstdio>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

// シーンごとのテクスチャキャッシュ
static std::unordered_map<std::wstring, ID3D11ShaderResourceView*> g_TextureCache[SCENE_MAX];
static std::unordered_map<std::wstring, ID3D11ShaderResourceView*> g_TextureCacheLinear[SCENE_MAX];

// 共通テクスチャキャッシュ（シーン切り替え時に破棄されない）
static std::unordered_map<std::wstring, ID3D11ShaderResourceView*> g_CommonTextureCache;
static std::unordered_map<std::wstring, ID3D11ShaderResourceView*> g_CommonTextureCacheLinear;

namespace
{
bool IsCommonTexture(const std::wstring& path)
{
	// "fade.png" などシーンをまたいで永続利用されるテクスチャを共通キャッシュにする
	return path.find(L"fade.png") != std::wstring::npos;
}

std::string ToString(const std::wstring& wstr)
{
	std::string str;
	for (wchar_t c : wstr) str += static_cast<char>(c);
	return str;
}

ID3D11ShaderResourceView* LoadTextureWithFlags(const wchar_t* texpass, WIC_FLAGS flags)
{
	TexMetadata metadata;
	ScratchImage image;
	ID3D11ShaderResourceView* g_Texture = nullptr;

	// flagsで「色テクスチャとして読むか」「データテクスチャとして読むか」を切り替える。
	HRESULT hr = LoadFromWICFile(texpass, flags, &metadata, image);
	if (FAILED(hr))
	{
		return nullptr;
	}

	// 標準的に SRV を作成（戻り値をチェック）
	hr = CreateShaderResourceView(
		GetDevice(),
		image.GetImages(),
		image.GetImageCount(),
		metadata,
		&g_Texture
	);

	if (FAILED(hr) || g_Texture == nullptr)
	{
		// 失敗時は NULL を返す（呼び出し側でフォールバック処理を行う）
		return nullptr;
	}

	return g_Texture;
}
}

ID3D11ShaderResourceView* LoadTexture(const wchar_t* texpass)
{
	std::wstring path = texpass;
	std::string pathStr = ToString(path);

	if (IsCommonTexture(path))
	{
		auto it = g_CommonTextureCache.find(path);
		if (it != g_CommonTextureCache.end())
		{
			hal::dout << "[Texture Cache] COMMON HIT: " << pathStr << std::endl;
			return it->second;
		}
		hal::dout << "[Texture Cache] COMMON MISS (Load): " << pathStr << std::endl;
		ID3D11ShaderResourceView* srv = LoadTextureWithFlags(texpass, WIC_FLAGS_FORCE_SRGB);
		if (srv)
		{
			g_CommonTextureCache[path] = srv;
		}
		return srv;
	}

	SCENE currentScene = GetScene();
	if (currentScene < 0 || currentScene >= SCENE_MAX)
	{
		hal::dout << "[Texture Cache] BYPASS (Invalid Scene, Load): " << pathStr << std::endl;
		return LoadTextureWithFlags(texpass, WIC_FLAGS_FORCE_SRGB);
	}

	auto it = g_TextureCache[currentScene].find(path);
	if (it != g_TextureCache[currentScene].end())
	{
		hal::dout << "[Texture Cache] SCENE " << currentScene << " HIT: " << pathStr << std::endl;
		return it->second;
	}

	hal::dout << "[Texture Cache] SCENE " << currentScene << " MISS (Load): " << pathStr << std::endl;
	ID3D11ShaderResourceView* srv = LoadTextureWithFlags(texpass, WIC_FLAGS_FORCE_SRGB);
	if (srv)
	{
		g_TextureCache[currentScene][path] = srv;
	}
	return srv;
}

ID3D11ShaderResourceView* LoadTexture(const std::wstring& texpass)
{
	return LoadTexture(texpass.c_str());
}

ID3D11ShaderResourceView* LoadTextureLinear(const wchar_t* texpass)
{
	std::wstring path = texpass;
	std::string pathStr = ToString(path);

	if (IsCommonTexture(path))
	{
		auto it = g_CommonTextureCacheLinear.find(path);
		if (it != g_CommonTextureCacheLinear.end())
		{
			hal::dout << "[Texture Cache] COMMON LINEAR HIT: " << pathStr << std::endl;
			return it->second;
		}
		hal::dout << "[Texture Cache] COMMON LINEAR MISS (Load): " << pathStr << std::endl;
		ID3D11ShaderResourceView* srv = LoadTextureWithFlags(texpass, WIC_FLAGS_IGNORE_SRGB);
		if (srv)
		{
			g_CommonTextureCacheLinear[path] = srv;
		}
		return srv;
	}

	SCENE currentScene = GetScene();
	if (currentScene < 0 || currentScene >= SCENE_MAX)
	{
		hal::dout << "[Texture Cache] BYPASS LINEAR (Invalid Scene, Load): " << pathStr << std::endl;
		return LoadTextureWithFlags(texpass, WIC_FLAGS_IGNORE_SRGB);
	}

	auto it = g_TextureCacheLinear[currentScene].find(path);
	if (it != g_TextureCacheLinear[currentScene].end())
	{
		hal::dout << "[Texture Cache] SCENE " << currentScene << " LINEAR HIT: " << pathStr << std::endl;
		return it->second;
	}

	hal::dout << "[Texture Cache] SCENE " << currentScene << " LINEAR MISS (Load): " << pathStr << std::endl;
	ID3D11ShaderResourceView* srv = LoadTextureWithFlags(texpass, WIC_FLAGS_IGNORE_SRGB);
	if (srv)
	{
		g_TextureCacheLinear[currentScene][path] = srv;
	}
	return srv;
}

ID3D11ShaderResourceView* LoadTextureLinear(const std::wstring& texpass)
{
	return LoadTextureLinear(texpass.c_str());
}

void ReleaseTexturesForScene(SCENE scene)
{
	if (scene < 0 || scene >= SCENE_MAX) return;

	hal::dout << "[Texture Cache] --- Releasing Scene " << scene << " Textures ---" << std::endl;

	for (auto& pair : g_TextureCache[scene])
	{
		if (pair.second)
		{
			hal::dout << "  Released: " << ToString(pair.first) << std::endl;
			pair.second->Release();
		}
	}
	g_TextureCache[scene].clear();

	for (auto& pair : g_TextureCacheLinear[scene])
	{
		if (pair.second)
		{
			hal::dout << "  Released (Linear): " << ToString(pair.first) << std::endl;
			pair.second->Release();
		}
	}
	g_TextureCacheLinear[scene].clear();
}

void ReleaseAllTextures()
{
	hal::dout << "[Texture Cache] --- Releasing All Textures (Shutdown) ---" << std::endl;

	for (int i = 0; i < SCENE_MAX; ++i)
	{
		ReleaseTexturesForScene(static_cast<SCENE>(i));
	}

	// 共通キャッシュの解放
	for (auto& pair : g_CommonTextureCache)
	{
		if (pair.second)
		{
			hal::dout << "  Released (Common): " << ToString(pair.first) << std::endl;
			pair.second->Release();
		}
	}
	g_CommonTextureCache.clear();

	for (auto& pair : g_CommonTextureCacheLinear)
	{
		if (pair.second)
		{
			hal::dout << "  Released (Common Linear): " << ToString(pair.first) << std::endl;
			pair.second->Release();
		}
	}
	g_CommonTextureCacheLinear.clear();
}

void UpdateTextureCache()
{
	SCENE currentScene = GetScene();
	static SCENE prevScene = SCENE_NONE;
	if (currentScene != prevScene)
	{
		if (prevScene >= 0 && prevScene < SCENE_MAX)
		{
			ReleaseTexturesForScene(prevScene);
		}
		prevScene = currentScene;
	}
}
