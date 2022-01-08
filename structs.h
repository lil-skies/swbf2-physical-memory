#pragma once
#include <Windows.h>
#include <stdexcept>
#include <vector>
#include <locale>
#include <set>
#include <iostream>

struct SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
	PVOID Object;
	ULONG UniqueProcessId;
	ULONG HandleValue;
	ULONG GrantedAccess;
	USHORT CreatorBackTraceIndex;
	USHORT ObjectTypeIndex;
	ULONG HandleAttributes;
	ULONG Reserved;

};

struct SYSTEM_HANDLE_INFORMATION_EX
{
	ULONG NumberOfHandles;
	ULONG Reserved;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];

};

struct OverlayFinderParams 
{
	DWORD pidOwner = NULL;
	std::wstring wndClassName = L"";
	std::wstring wndName = L"";
	RECT pos = { 0, 0, 0, 0 };
	POINT res = { 0, 0 };
	float percentAllScreens = 0.0f;
	float percentMainScreen = 0.0f;
	DWORD style = NULL;
	DWORD styleEx = NULL;
	bool satisfyAllCriteria = false;
	std::vector<HWND> hwnds;

};

struct view_matrix
{
	float matrix[4][4];

};

struct vec4
{
	float x;
	float y;
	float z;
	float w;

	vec4 operator + (vec4 L)
	{
		return { x + L.x, y + L.y, z + L.z, w + L.z };

	}

	vec4 operator - (vec4 L)
	{
		return { x - L.x, y - L.y, z - L.z, w - L.z };

	}

	vec4 operator * (int L)
	{
		return { x * L, y * L, z * L, w * L };

	}

};

struct vec3
{
	float x;
	float y;
	float z;

	vec3 operator + (vec3 w)
	{
		return { x + w.x, y + w.y, z + w.z };

	}

	vec3 operator - (vec3 w)
	{
		return { x - w.x, y - w.y, z - w.z };

	}

	vec3 operator * (int w)
	{
		return { x * w, y * w, z * w };

	}

	void normalize()
	{
		while (y < -180)
		{
			y += 360;

		}

		while (y > 180)
		{
			y -= 360;

		}

		if (x > 89)
		{
			x = 89;

		}

		if (x < -89)
		{
			x = -89;

		}

	}

};

struct vec2
{
	float x;
	float y;

	vec2 operator + (vec2 w)
	{
		return { x + w.x, y + w.y };

	}

	vec2 operator - (vec2 w)
	{
		return { x - w.x, y - w.y };

	}

	vec2 operator * (int w)
	{
		return { x * w, y * w };

	}

	void normalize()
	{
		while (y < -180)
		{
			y += 360;

		}

		while (y > 180)
		{
			y -= 360;

		}

		if (x > 89)
		{
			x = 89;

		}

		if (x < -89)
		{
			x = -89;

		}

	}

	void normalizePitch()
	{
		if (x > 89)
		{
			x = 89;

		}

		if (x < -89)
		{
			x = -89;

		}

	}

};
