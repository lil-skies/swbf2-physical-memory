#pragma once

#include <Windows.h>

namespace data
{
	namespace handles
	{
		HANDLE h_driver;

	};

	namespace window
	{
		int pick_hwnd;
		HWND hwnd;

	}

	namespace registry
	{
		unsigned long long cr0;
		unsigned long long cr3;
		unsigned long long cr4;

		uintptr_t process_cr3;

		LPVOID rwdrv_address;

	}

	namespace process
	{
		uintptr_t base_address;
		int process_id;

		uintptr_t size;

	}

};

namespace offsets
{
	namespace game
	{
		unsigned long long GAME_MANAGER = 0x3DD7948;
		//48 89 15 ? ? ? ? 48 89 CB

		unsigned long long PLAYER_MANAGER = 0x58;
		unsigned long long PLAYER_ARRAY = 0x768;
		unsigned long long LOCAL_PLAYER = 0x568;

	}

	namespace entity
	{
		unsigned long long SOLDIER = 0x210;
		unsigned long long TEAM_ID = 0x58;
		unsigned long long HEALTH = 0x2C8;
		unsigned long long HEALTH2 = 0x20;
		unsigned long long POSITION = 0x758;
		unsigned long long POSITION2 = 0x20;
		unsigned long long NAME = 0x68;

	}

	namespace view
	{
		unsigned long long GAME_RENDERER = 0x3FFBE10;
		//48 89 05 ? ? ? ? 4C 8B 00 

		unsigned long long RENDER_VIEW = 0x538;
		unsigned long long VIEW_MATRIX = 0x430;
		unsigned long long YAW_ROLL_PITCH = 0x560;
		unsigned long long PITCH = 0x55E;
		unsigned long long YAW = 0x566;
	}

	namespace misc
	{
		unsigned long long DAMAGE = 0x8367B0B;
		//8B 8A 50 01 00 00 49 8B 50 20 E9
		unsigned long long SPREAD = 0x79C39F0;
		//43 0F 10 04 D9 0F 11 2 F2 43 0F 10 4C D9 10 F2 0F
		unsigned long long GAME_TIME_SETTINGS = 0x3AEBBE8;
		//48 89 05 ? ? ? ? ? ? ? ? C7 40 ? ? ? ? ? ? ? ? ? ? 8B 43 18
		unsigned long long FIRST_TYPE_INFO = 0x3D05F08;
		//48 8B 05 ? ? ? ? ? ? ? ? 48 89 41 08 48 89 0D
		unsigned long long RECOIL = 0x7EEE11E;
		//E8 ? ? ? ? F2 0F 10 86 A8 01 00 00 8B86 7C 01 00 00

	}

};