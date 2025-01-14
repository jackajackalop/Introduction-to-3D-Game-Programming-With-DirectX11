//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#ifndef D3DUTIL_H
#define D3DUTIL_H

#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
 
#include <d3d11.h>
#include "d3dx11Effect.h"
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <cassert>
#include <ctime>
#include <algorithm>
#include <string>
#include <sstream>
#include <fstream>
#include <vector>
#include "MathHelper.h"
#include "LightHelper.h"

//---------------------------------------------------------------------------------------
// Simple d3d error checker for book demos.
//---------------------------------------------------------------------------------------

#define HR(x) (x)
#define D3DX11_DEFAULT            ((UINT) -1)
#define D3DX11_FROM_FILE          ((UINT) -3)
#define DXGI_FORMAT_FROM_FILE     ((DXGI_FORMAT) -3)

//---------------------------------------------------------------------------------------
// Convenience macro for releasing COM objects.
//---------------------------------------------------------------------------------------

#define ReleaseCOM(x) { if(x){ x->Release(); x = 0; } }

//---------------------------------------------------------------------------------------
// Convenience macro for deleting objects.
//---------------------------------------------------------------------------------------

#define SafeDelete(x) { delete x; x = 0; }

//---------------------------------------------------------------------------------------
// Utility classes.
//---------------------------------------------------------------------------------------

class d3dHelper
{
public:
	///<summary>
	/// 
	/// Does not work with compressed formats.
	///</summary>
	static ID3D11ShaderResourceView* CreateTexture2DArraySRV(
		ID3D11Device* device, ID3D11DeviceContext* context,
		std::vector<std::wstring>& filenames);

	static ID3D11ShaderResourceView* CreateRandomTexture1DSRV(ID3D11Device* device);
};

class TextHelper
{
public:

	template<typename T>
	static std::wstring ToString(const T& s)
	{
		std::wostringstream oss;
		oss << s;

		return oss.str();
	}

	template<typename T>
	static T FromString(const std::wstring& s)
	{
		T x;
		std::wistringstream iss(s);
		iss >> x;

		return x;
	}
};

// Order: left, right, bottom, top, near, far.
void ExtractFrustumPlanes(DirectX::XMFLOAT4 planes[6], DirectX::CXMMATRIX M);


// #define XMGLOBALCONST extern CONST __declspec(selectany)
//   1. extern so there is only one copy of the variable, and not a separate
//      private copy in each .obj.
//   2. __declspec(selectany) so that the compiler does not complain about
//      multiple definitions in a .cpp file (it can pick anyone and discard 
//      the rest because they are constant--all the same).

namespace Colors
{
	XMGLOBALCONST DirectX::XMFLOAT4 White     = {1.0f, 1.0f, 1.0f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 Black     = {0.0f, 0.0f, 0.0f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 Red       = {1.0f, 0.0f, 0.0f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 Green     = {0.0f, 1.0f, 0.0f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 Blue      = {0.0f, 0.0f, 1.0f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 Yellow    = {1.0f, 1.0f, 0.0f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 Cyan      = {0.0f, 1.0f, 1.0f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 Magenta   = {1.0f, 0.0f, 1.0f, 1.0f};

	XMGLOBALCONST DirectX::XMFLOAT4 Silver    = {0.75f, 0.75f, 0.75f, 1.0f};
	XMGLOBALCONST DirectX::XMFLOAT4 LightSteelBlue = {0.69f, 0.77f, 0.87f, 1.0f};
}

///<summary>
/// Utility class for converting between types and formats.
///</summary>
class Convert
{
public:
	///<summary>
	/// Converts DirectX::XMVECTOR to XMCOLOR, where DirectX::XMVECTOR represents a color.
	///</summary>
	static DirectX::PackedVector::XMCOLOR ToXmColor(DirectX::FXMVECTOR v)
	{
		DirectX::PackedVector::XMCOLOR dest;
		XMStoreColor(&dest, v);
		return dest;
	}

	///<summary>
	/// Converts DirectX::XMVECTOR to DirectX::XMFLOAT4, where DirectX::XMVECTOR represents a color.
	///</summary>
	static DirectX::XMFLOAT4 ToXMFLOAT4(DirectX::FXMVECTOR v)
	{
		DirectX::XMFLOAT4 dest;
		XMStoreFloat4(&dest, v);
		return dest;
	}

	static UINT ArgbToAbgr(UINT argb)
	{
		BYTE A = (argb >> 24) & 0xff;
		BYTE R = (argb >> 16) & 0xff;
		BYTE G = (argb >>  8) & 0xff;
		BYTE B = (argb >>  0) & 0xff;

		return (A << 24) | (B << 16) | (G << 8) | (R << 0);
	}

};

#endif // D3DUTIL_H