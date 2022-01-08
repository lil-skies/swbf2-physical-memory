#pragma once
#include <Windows.h>
#include <winternl.h>
#include <vector>
#pragma comment( lib, "ntdll.lib" )
#include <d3d9.h>
#pragma comment( lib, "d3d9.lib")
#include <d3dx9.h>
#pragma comment ( lib, "d3dx9.lib")
#include <d3dx9core.h>
#include <stdlib.h>

#include <dwmapi.h>
#pragma comment (lib, "dwmapi.lib")
#pragma comment (lib, "ntdll.lib")

#include <Psapi.h>
#include <iostream>

#include <thread>

#include "alignment.h"
#include "structs.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_dx9.h"
#include "imgui/imgui_impl_win32.h"

#define M_PI 3.14159265358979323846264338327950288

#define SIZE_OF_NT_SIGNATURE sizeof(IMAGE_NT_SIGNATURE)
#define PEFHDROFFSET(a) ((LPVOID)((BYTE *)a + ((PIMAGE_DOS_HEADER)a)->e_lfanew + SIZE_OF_NT_SIGNATURE))
#define SECHDROFFSET(a) ((LPVOID)((BYTE *)a + ((PIMAGE_DOS_HEADER)a)->e_lfanew + SIZE_OF_NT_SIGNATURE + sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER)))

#define EPROCESS_VIRTUALSIZE                0x338 
#define EPROCESS_DIRECTORYTABLE_OFFSET		0x028	// [+0x028] _EPROCESS.DirectoryTableBase
#define EPROCESS_UNIQUEPROCESSID_OFFSET		0x2E0	// [+0x2E0]	_EPROCESS.UniqueProcessId
#define EPROCESS_ACTIVEPROCESSLINK_OFFSET	0x2E8	// [+0x2E8] _EPROCESS.ActiveProcessLinks	
#define EPROCESS_VIRTUALSIZE				0x338	// [+0x338] _EPROCESS.VirtualSize
#define EPROCESS_SECTIONBASE				0x3C0	// [+0x3C0] _EPROCESS.SectionBaseAddress
#define EPROCESS_IMAGEFILENAME				0x450	// [+0x450] _EPROCESS.ImageFileName [15]

#define MAX_CLASSNAME 255
#define MAX_WNDNAME MAX_CLASSNAME

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

void close_driver_handle(HANDLE h_driver);
HANDLE get_driver_handle(); 
bool read_physical(unsigned long long address, uintptr_t size, unsigned char* buffer);
bool write_physical(unsigned long long address, int size, unsigned char* buffer);
bool control_registry(unsigned long long* cr0, unsigned long long* cr3, unsigned long long* cr4);
bool msr(unsigned int registry, unsigned long long* value);
bool vtop(unsigned long long va_address, unsigned long long* returned_physical_address);
bool rvpm(unsigned long long address, uintptr_t size, unsigned char* buffer);
bool wvpm(unsigned long long address, int size, unsigned char* buffer);
PIMAGE_SECTION_HEADER get_code_section();
uintptr_t find_pattern(uint64_t start, uint64_t size, const char* pattern, const char* mask, int offset);
bool leak_kernel_pointers(std::vector<uintptr_t>& kernel_pointers);
bool leak_kprocess(uintptr_t* returned_pointer);
bool find_kprocess(unsigned long long cr3, const char* image_name, unsigned long long* returned_base_address, int* returned_process_id);
bool crvpm(unsigned long long address, std::vector<unsigned long long>offsets);
bool nop(unsigned long long address, int size);
void move_window();
void find_window();
bool w2s(vec3 position, OUT vec2& screen);
void drawer(ImDrawList* draw, vec2 start, vec2 end, ImColor color, float thickness);
void cache();
void misc();
void render();








