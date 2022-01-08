#include "func.h"
#include "namespace.h"
#include "raw_driver.h"

unsigned long long local_player_address;
unsigned long long player_array_address;
unsigned long long render_view_address;

void close_driver_handle(HANDLE h_driver)
{
	CloseHandle(h_driver);

}

HANDLE get_driver_handle()
{
	HANDLE h_driver = CreateFileA("\\\\.\\*****", GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);

	if (h_driver == INVALID_HANDLE_VALUE)
	{
		printf("[-] failed to locate driver handle!\n");
		close_driver_handle(h_driver);
		return 0;

	}
	else
	{
		printf("[+] retrieved driver handle! driver handle: 0x%x\n", h_driver);
		return h_driver;

	}

}

bool read_physical(unsigned long long address, uintptr_t size, unsigned char* buffer)
{
	ZeroMemory(buffer, size);

	DWORD64 map_address = XALIGN_DOWN(address, PAGE_SIZE);
	DWORD64 map_size = XALIGN_UP(size, PAGE_SIZE);

	PUCHAR dataa = (PUCHAR)M_ALLOC(map_size);
	if (dataa)
	{
		UCHAR request[0x100];
		ZeroMemory(&request, sizeof(request));

		*(PDWORD64)(request + 0x00) = map_address;
		*(PDWORD64)(request + 0x10) = (DWORD64)dataa;
		*(PDWORD)(request + 0x08) = map_size;
		*(PDWORD)(request + 0x0c) = 2;

		DWORD returned = 0;

		if (!DeviceIoControl(data::handles::h_driver, 0x222808, &request, sizeof(request), &request, sizeof(request), &returned, NULL))
		{
			printf("[-] failed to read physical memory! error code: %d\n", GetLastError());
			M_FREE(dataa);
			return false;

		}
		else
		{
			//printf("[+] read physical memory succeeded!\n");
			CopyMemory(buffer, RVATOVA(dataa, address - map_address), size);
			M_FREE(dataa);
			return true;

		}

	}
	else
	{
		printf("[-] data allocation failed!\n");
		return false;

	}

}

bool write_physical(unsigned long long address, int size, unsigned char* buffer)
{
	ZeroMemory(buffer, size);

	DWORD64 map_address = XALIGN_DOWN(address, PAGE_SIZE);
	DWORD64 map_size = XALIGN_UP(size, PAGE_SIZE);

	PUCHAR dataa = (PUCHAR)M_ALLOC(map_size);
	if (dataa)
	{
		if (read_physical(map_address, map_size, dataa))
		{
			UCHAR request[0x100];
			ZeroMemory(&request, sizeof(request));

			CopyMemory(RVATOVA(dataa, address - map_address), buffer, size);

			*(PDWORD64)(request + 0x00) = map_address;
			*(PDWORD64)(request + 0x10) = (DWORD64)dataa;
			*(PDWORD)(request + 0x08) = map_size;
			*(PDWORD)(request + 0x0c) = 2;

			DWORD returned = 0;

			if (!DeviceIoControl(data::handles::h_driver, 0x22280C, &request, sizeof(request), &request, sizeof(request), &returned, NULL))
			{
				printf("[-] failed to write physical!\n");
				M_FREE(dataa);
				return false;

			}
			else
			{
				printf("[+] write successful!\n");
				M_FREE(dataa);
				return true;

			}

		}
		else
		{
			printf("[-] failed to read physical before write!\n");
			M_FREE(dataa);
			return false;

		}

	}
	else
	{
		printf("[-] data allocation failed!\n");
		return false;

	}

}

//CR0 = 0; CR1 = 1; CR2 = 2; CR3 = 3, CR4 = 4, IRQL = 8
bool control_registry(unsigned long long* cr0, unsigned long long* cr3, unsigned long long* cr4)
{
	UCHAR request[0x100];
	ZeroMemory(&request, sizeof(request));

	if (cr0)
	{
		DWORD returned = 0;
		*(PDWORD)(request + 0x00) = 0;

		if (!DeviceIoControl(data::handles::h_driver, 0x22286C, &request, sizeof(request), &request, sizeof(request), &returned, 0))
		{
			printf("[-] failed to find register! error code: 0x%x\n", GetLastError());
			return false;

		}
		else
		{
			printf("[+] found cr0!\n");
			*cr0 = *(PDWORD)(request + 0x08);

		}
	}

	if (cr3)
	{
		DWORD returned = 0;
		*(PDWORD)(request + 0x00) = 3;

		if (!DeviceIoControl(data::handles::h_driver, 0x22286C, &request, sizeof(request), &request, sizeof(request), &returned, 0))
		{
			printf("[-] failed to find register! error code: 0x%x\n", GetLastError());
			return false;

		}
		else
		{
			printf("[+] found cr3!\n");
			*cr3 = *(PDWORD)(request + 0x08);

		}
	}

	if (cr4)
	{
		DWORD returned = 0;
		*(PDWORD)(request + 0x00) = 4;

		if (!DeviceIoControl(data::handles::h_driver, 0x22286C, &request, sizeof(request), &request, sizeof(request), &returned, 0))
		{
			printf("[-] failed to find register! error code: 0x%x\n", GetLastError());
			return false;

		}
		else
		{
			printf("[+] found cr4!\n");
			*cr4 = *(PDWORD)(request + 0x08);

		}

	}
	return true;

}

bool msr(unsigned int registry, unsigned long long* value)
{
	UCHAR request[0x100];
	ZeroMemory(&request, sizeof(request));

	DWORD returned = 0;

	*(PDWORD)(request + 0x08) = registry;

	if (!DeviceIoControl(data::handles::h_driver, 0x222848, &request, sizeof(request), &request, sizeof(request), &returned, NULL))
	{
		printf("[-] failed to retrieve MSR! error code: %d\n", GetLastError());
		return false;

	}
	else
	{
		printf("[+] retrieved MSR!\n");

		LARGE_INTEGER large_value;

		large_value.HighPart = *(PDWORD)(request + 0x0C);
		large_value.LowPart = *(PDWORD)(request + 0x00);

		*value = large_value.QuadPart;

		return true;

	}

}

bool vtop(unsigned long long va_address, unsigned long long* returned_physical_address)
{
	DWORD64 physical_address = 0;
	int16_t pml4 = PML4_INDEX(va_address);
	int16_t directory_pointer = PDPT_INDEX(va_address);
	int16_t directory = PDE_INDEX(va_address);
	int16_t table = PTE_INDEX(va_address);

	DWORD64 pml4e = 0;
	if (!read_physical(data::registry::cr3 + pml4 * sizeof(DWORD64), sizeof(DWORD64), (unsigned char*)&pml4e))
	{
		printf("[-] failed to read physical memory!\n");
		*returned_physical_address = 0;
		return false;

	}
	if (!pml4e)
	{
		printf("[-] failed to return pml4e!\n");
		*returned_physical_address = 0;
		return false;

	}

	DWORD64 pdpte = 0;
	if (!read_physical((pml4e & 0xFFFFFFFFFF000) + directory_pointer * sizeof(DWORD64), sizeof(DWORD64), (unsigned char*)&pdpte))
	{
		printf("[-] failed to read physical memory!\n");
		*returned_physical_address = 0;
		return false;

	}
	if (!pdpte)
	{
		printf("[-] failed to return pdpte!\n");
		*returned_physical_address = 0;
		return false;

	}
	if ((pdpte & (1 << 7)) != 0)
	{
		printf("[+] returned an address!\n");
		*returned_physical_address = (pdpte & 0xFFFFFC0000000) + (va_address & 0x3FFFFFFF);
		return true;
	}

	DWORD64 pde = 0;
	if (!read_physical((pdpte & 0xFFFFFFFFFF000) + directory * sizeof(DWORD64), sizeof(DWORD64), (unsigned char*)&pde))
	{
		printf("[-] failed to read physical memory!\n");
		*returned_physical_address = 0;
		return false;

	}
	if (!pde)
	{
		printf("[-] failed to return pde!\n");
		*returned_physical_address = 0;
		return false;

	}
	if ((pde & (1 << 7)) != 0)
	{
		printf("[+] returned an address!\n");
		*returned_physical_address = (pde & 0xFFFFFFFE00000) + (va_address & 0x1FFFFF);
		return true;

	}

	DWORD64 pte = 0;
	if (!read_physical((pde & 0xFFFFFFFFFF000) + table * sizeof(DWORD64), sizeof(DWORD64), (unsigned char*)&pte))
	{
		printf("[-] failed to read physical memory!\n");
		*returned_physical_address = 0;
		return false;

	}
	if (!pte)
	{
		printf("[-] failed to return pte!\n");
		return false;

	}
	*returned_physical_address = (pte & 0xFFFFFFFFFF000) + (va_address & 0xFFF);
	return true;

}

bool rvpm(unsigned long long address, uintptr_t size, unsigned char* buffer)
{
	unsigned long long physical_address = 0;

	if (vtop(address, &physical_address))
	{
		return read_physical(physical_address, size, buffer);

	}
	return false;

}

bool wvpm(unsigned long long address, int size, unsigned char* buffer)
{
	unsigned long long physical_address = 0;

	if (vtop(address, &physical_address))
	{
		return write_physical(physical_address, size, buffer);

	}
	return false;

}

bool crvpm(unsigned long long address, std::vector<unsigned long long>offsets)
{
	unsigned long long sum_address = address;
	for (int i = 0; i < offsets.size(); i++)
	{
		unsigned long long temp;
		rvpm(sum_address + offsets[i], sizeof(unsigned long long), (unsigned char*)&temp);
		sum_address = temp;

	}
	return sum_address;

}

bool nop(unsigned long long address, int size)
{
	unsigned char* nop_array = new unsigned char[size];
	memset(nop_array, 0x90, size);
	wvpm(address, size, (unsigned char*)&nop_array);
	return true;

}

PIMAGE_SECTION_HEADER get_code_section()
{
	size_t readSize = 0x1000;
	LPVOID lpHeader = malloc(readSize);
	rvpm(data::process::base_address, readSize, (unsigned char*)lpHeader);
	PIMAGE_FILE_HEADER pfh = (PIMAGE_FILE_HEADER)PEFHDROFFSET(lpHeader);
	if (pfh->NumberOfSections < 1)
	{
		free(lpHeader);
		return NULL;

	}
	PIMAGE_SECTION_HEADER psh = (PIMAGE_SECTION_HEADER)SECHDROFFSET(lpHeader);
	free(lpHeader);
	return psh;

}

uintptr_t find_pattern(uint64_t start, uint64_t size, const char* pattern, const char* mask, int offset)
{
	size_t pos = 0;
	auto maskLength = strlen(mask);

	auto startAddress = start;

	char* block = (char*)calloc(1, size);
	rvpm(startAddress, size, reinterpret_cast<unsigned char*>(&block[0]));

	for (int j = 0; j < size; j++)
	{
		if (block[j] == pattern[pos] || mask[pos] == '?')
		{
			if (mask[pos + 1] == '\0')
			{
				for (int i = j - 100; i < j; i++)
					printf("0x%x\n", block[i]);

				free(block);
				return startAddress + j + offset - maskLength;

			}
			pos++;
			if (pos >= 16)
				printf("pattern [%i]\n", pos);

		}
		else pos = 0;

	}
	free(block);
	printf("[-] pattern scan failed!\n");

}


bool leak_kernel_pointers(std::vector<uintptr_t>& kernel_pointers)
{
	const unsigned long system_handle_information = 0x40;

	unsigned long buffer_length = 0;
	unsigned char probe_buffer[1024] = { };

	NTSTATUS status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(system_handle_information), &probe_buffer, sizeof(probe_buffer), &buffer_length);

	if (!buffer_length)
	{
		printf("[-] failed to acquire buffer length!\n");
		return false;

	}

	buffer_length += 50 * (sizeof(SYSTEM_HANDLE_INFORMATION_EX) + sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX));
	PVOID buffer = VirtualAlloc(nullptr, buffer_length, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (!buffer)
	{
		printf("[-] failed to acquire buffer!\n");
		return false;

	}

	RtlSecureZeroMemory(buffer, buffer_length);

	unsigned long new_buffer_length = 0;
	status = NtQuerySystemInformation(static_cast<SYSTEM_INFORMATION_CLASS>(system_handle_information), buffer, buffer_length, &new_buffer_length);

	if (!NT_SUCCESS(status))
	{
		printf("[-] failed to call NtQuerySystemInformation!\n");
		return false;

	}
	SYSTEM_HANDLE_INFORMATION_EX* handle_information = reinterpret_cast<SYSTEM_HANDLE_INFORMATION_EX*>(buffer);

	for (unsigned int i = 0; i < handle_information->NumberOfHandles; i++)
	{
		const unsigned int system_unique_reserved = 4;
		const unsigned int system_kprocess_handle_attributes = 0x102A;

		if (handle_information->Handles[i].UniqueProcessId == system_unique_reserved && handle_information->Handles[i].HandleAttributes == system_kprocess_handle_attributes)
		{
			kernel_pointers.push_back(reinterpret_cast<uintptr_t>(handle_information->Handles[i].Object));

		}

	}
	VirtualFree(buffer, 0, MEM_RELEASE);
	return true;

}

bool leak_kprocess(uintptr_t* returned_pointer)
{
	std::vector<uintptr_t>kernel_pointers;

	if (!leak_kernel_pointers(kernel_pointers))
	{
		printf("[-] failed to leak kernel pointers!\n");
		return false;

	}

	const unsigned int sanity_check = 0xB60003;

	for (uintptr_t pointer : kernel_pointers)
	{
		unsigned int check = 0;
		if (!rvpm(pointer, sizeof(uintptr_t), (unsigned char*)&check))
		{
			printf("[-] rvpm failed!\n");
			*returned_pointer = 0;
			return false;

		}

		if (check == sanity_check)
		{
			printf("[+] returned pointer!\n");
			*returned_pointer = pointer;

		}

	}
	if (!*returned_pointer) return false;
	else return true;

}


bool find_kprocess(unsigned long long cr3, const char* image_name, unsigned long long* returned_base_address, int* returned_process_id)
{
	data::registry::cr3 = cr3;
	uintptr_t kprocess_intial;

	if (!leak_kprocess(&kprocess_intial))
	{
		printf("[-] failed to leak kprocess!\n");
		return false;

	}

	const unsigned long long limit = 400;
	unsigned unsigned long count = 0;

	uintptr_t link_start = kprocess_intial + EPROCESS_ACTIVEPROCESSLINK_OFFSET;
	uintptr_t flink = link_start;
	uintptr_t image_base_out = 0;

	do
	{
		rvpm(flink, sizeof(PVOID), (unsigned char*)&flink);

		uintptr_t kprocess = flink - EPROCESS_ACTIVEPROCESSLINK_OFFSET;
		uintptr_t virtual_size;
		rvpm(kprocess + EPROCESS_VIRTUALSIZE, sizeof(uintptr_t), (unsigned char*)&virtual_size);

		if (!virtual_size) continue;

		int process_id;
		rvpm(kprocess + EPROCESS_UNIQUEPROCESSID_OFFSET, sizeof(int), (unsigned char*)&process_id);

		uintptr_t directory_base;
		rvpm(kprocess + EPROCESS_DIRECTORYTABLE_OFFSET, sizeof(uintptr_t), (unsigned char*)&directory_base);

		uintptr_t base_address;
		rvpm(kprocess + EPROCESS_SECTIONBASE, sizeof(uintptr_t), (unsigned char*)&base_address);

		char name[16] = { };
		rvpm(kprocess + EPROCESS_IMAGEFILENAME, sizeof(name), (unsigned char*)&name);

		rvpm(kprocess + EPROCESS_VIRTUALSIZE, sizeof(uintptr_t), (unsigned char*)&(data::process::size));

		printf("[+] image name: %s\n", name);

		if (strcmp(name, image_name) == 0)
		{
			printf("[+] process: %s\n", name);
			printf("[+] base: 0x%x\n", base_address);
			*returned_base_address = base_address;
			printf("[+] process_id: %i\n", process_id);
			*returned_process_id = process_id;
			data::registry::process_cr3 = directory_base;
			return true;

		}
		if (count >= limit) break;

		count++;

	} while (flink != link_start);
	return false;

}

void move_window()
{
	while (true)
		SetWindowPos(data::window::hwnd, HWND_TOPMOST, 0, 0, 1920, 1080, SWP_SHOWWINDOW);

}

BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
	OverlayFinderParams& params = *(OverlayFinderParams*)lParam;

	unsigned char satisfiedCriteria = 0, unSatisfiedCriteria = 0;

	// If looking for window at a specific position
	RECT pos;
	GetWindowRect(hwnd, &pos);
	if (params.pos.left || params.pos.top || params.pos.right || params.pos.bottom)
		if (params.pos.left == pos.left && params.pos.top == pos.top && params.pos.right == pos.right && params.pos.bottom == pos.bottom)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// If looking for window of a specific size
	POINT res = { pos.right - pos.left, pos.bottom - pos.top };
	if (params.res.x || params.res.y)
		if (res.x == params.res.x && res.y == params.res.y)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// If looking for windows taking more than a specific percentage of all the screens
	float ratioAllScreensX = res.x / GetSystemMetrics(SM_CXSCREEN);
	float ratioAllScreensY = res.y / GetSystemMetrics(SM_CYSCREEN);
	float percentAllScreens = ratioAllScreensX * ratioAllScreensY * 100;
	if (params.percentAllScreens != 0.0f)
		if (percentAllScreens >= params.percentAllScreens)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// If looking for windows taking more than a specific percentage or the main screen
	RECT desktopRect;
	GetWindowRect(GetDesktopWindow(), &desktopRect);
	POINT desktopRes = { desktopRect.right - desktopRect.left, desktopRect.bottom - desktopRect.top };
	float ratioMainScreenX = res.x / desktopRes.x;
	float ratioMainScreenY = res.y / desktopRes.y;
	float percentMainScreen = ratioMainScreenX * ratioMainScreenY * 100;
	if (params.percentMainScreen != 0.0f)
		if (percentAllScreens >= params.percentMainScreen)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// Looking for windows with specific styles
	LONG_PTR style = GetWindowLongPtr(hwnd, GWL_STYLE);
	if (params.style)
		if (params.style & style)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	// Looking for windows with specific extended styles
	LONG_PTR styleEx = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
	if (params.styleEx)
		if (params.styleEx & styleEx)
			++satisfiedCriteria;
		else
			++unSatisfiedCriteria;

	if (!satisfiedCriteria)
		return TRUE;

	if (params.satisfyAllCriteria && unSatisfiedCriteria)
		return TRUE;

	// If looking for multiple windows
	params.hwnds.push_back(hwnd);
	return TRUE;

}

std::vector<HWND> OverlayFinder(OverlayFinderParams params) 
{
	EnumWindows(EnumWindowsCallback, (LPARAM)&params);
	return params.hwnds;

}

void find_window()
{
	OverlayFinderParams parameters;
	parameters.style = WS_VISIBLE;
	parameters.styleEx = WS_EX_LAYERED | WS_EX_TRANSPARENT;
	parameters.percentMainScreen = 90.0f;
	parameters.satisfyAllCriteria = true;
	std::vector<HWND> hwnds = OverlayFinder(parameters);

	printf("[+] found hwnd!\n");
	data::window::hwnd = hwnds[data::window::pick_hwnd];

}

bool w2s(vec3 position, OUT vec2& screen)
{
	unsigned long long render_view_address = crvpm(data::process::base_address, { offsets::view::GAME_RENDERER, offsets::view::RENDER_VIEW });

	if (!render_view_address)
		return false;

	view_matrix view;
	rvpm(render_view_address + offsets::view::VIEW_MATRIX, sizeof(view_matrix), (unsigned char*)&view);

	vec4 clip_coordinates;
	clip_coordinates.x = position.x * view.matrix[0][0] + position.y * view.matrix[1][0] + position.z * view.matrix[2][0] + view.matrix[3][0];
	clip_coordinates.y = position.x * view.matrix[0][1] + position.y * view.matrix[1][1] + position.z * view.matrix[2][1] + view.matrix[3][1];
	clip_coordinates.w = position.x * view.matrix[0][3] + position.y * view.matrix[1][3] + position.z * view.matrix[2][3] + view.matrix[3][3];

	if (clip_coordinates.w < 0.1f)
		return false;

	int mX = 1920 / 2;
	int mY = 1080 / 2;

	float x = (mX + mX * clip_coordinates.x / clip_coordinates.w);
	float y = (mY - mY * clip_coordinates.y / clip_coordinates.w);

	screen.x = x;
	screen.y = y;

	return true;

}

void drawer(ImDrawList* draw, vec2 start, vec2 end, ImColor color, float thickness)
{
	ImVec2 buffer[2];
	buffer[0].x = start.x;
	buffer[0].y = start.y;
	buffer[1].x = end.x;
	buffer[1].y = end.y;

	draw->AddLine(buffer[0], buffer[1], color, thickness);

}


void cache()
{
	while (true)
	{
		local_player_address = crvpm(data::process::base_address, { offsets::game::GAME_MANAGER, offsets::game::PLAYER_MANAGER, offsets::game::LOCAL_PLAYER });
		player_array_address = crvpm(data::process::base_address, { offsets::game::GAME_MANAGER, offsets::game::PLAYER_MANAGER, offsets::game::PLAYER_ARRAY });
		render_view_address = crvpm(data::process::base_address, { offsets::view::GAME_RENDERER, offsets::view::RENDER_VIEW });
	
	}

}

void misc()
{
	while (true)
	{
		unsigned long long local_player_health_address = crvpm(local_player_address, { offsets::entity::SOLDIER, offsets::entity::HEALTH });
		float local_player_health;
		rvpm(local_player_health_address, sizeof(float), (unsigned char*)&local_player_health);

		if (local_player_health)
		{
			nop(data::process::base_address + offsets::misc::SPREAD, 5);
			nop(data::process::base_address + offsets::misc::RECOIL, 5);

			unsigned char* shellcode = new unsigned char[6]{ 0xB9, 0x00, 0x00, 0x00, 0x00, 0x90 };
			int new_damage = 15;
			memcpy(shellcode + 1, &new_damage, 4);
			wvpm(data::process::base_address + offsets::misc::DAMAGE, sizeof(shellcode) / shellcode[0], (unsigned char*)&shellcode);
			delete[] shellcode;

		}

	}

}

void render()
{
	ImDrawList* draw = ImGui::GetWindowDrawList();

	if (local_player_address && player_array_address)
	{
		for (int i = 0; i < 100; i++)
		{
			int l_team_id;
			rvpm(local_player_address + offsets::entity::TEAM_ID, sizeof(int), (unsigned char*)&l_team_id);
			if (l_team_id > 3 || l_team_id < 0) continue;

			unsigned long long enemy;
			rvpm(player_array_address + i * 0x8, sizeof(unsigned long long), (unsigned char*)&enemy);
			if (!enemy) continue;

			int enemy_id;
			rvpm(enemy + offsets::entity::TEAM_ID, sizeof(int), (unsigned char*)&enemy_id);
			if (enemy_id == l_team_id) continue;

			unsigned long long enemy_player_health_address = crvpm(enemy, { offsets::entity::SOLDIER, offsets::entity::HEALTH });
			float enemy_health;
			rvpm(enemy_player_health_address + offsets::entity::HEALTH2, sizeof(float), (unsigned char*)&enemy_health);
			if (enemy_health <= 0.0f) continue;

			unsigned long long enemy_position_address = crvpm(enemy, { offsets::entity::SOLDIER, offsets::entity::POSITION });
			vec3 enemy_position;
			rvpm(enemy_position_address + offsets::entity::POSITION2, sizeof(vec3), (unsigned char*)&enemy_position);

			unsigned long long local_player_position_address = crvpm(local_player_address, { offsets::entity::SOLDIER, offsets::entity::POSITION });
			vec3 local_player_position;
			rvpm(local_player_position_address + offsets::entity::POSITION2, sizeof(vec3), (unsigned char*)&local_player_position);

			float distance = sqrt(pow(enemy_position.x - local_player_position.x, 2) + pow(enemy_position.y - local_player_position.y, 2) + pow(enemy_position.z - local_player_position.z, 2));
			if (distance > 5000.0f) continue;

			vec3 enemy_head_position;
			enemy_head_position.x = enemy_position.x;
			enemy_head_position.y = enemy_position.y + 1.5;
			enemy_head_position.z = enemy_position.z;

			vec2 head_2d;
			vec2 feet_2d; 

			if (!w2s(enemy_head_position, head_2d)) continue;
			if(!w2s(enemy_position, feet_2d)) continue;

			float height = head_2d.y - feet_2d.y;
			float width = height / 2.4;

			vec2 top_left;
			vec2 top_right;
			vec2 bottom_right;
			vec2 bottom_left;

			top_left.x = feet_2d.x - (width / 1.5);
			top_left.y = head_2d.y;

			top_right.x = feet_2d.x + (width / 1.5);
			top_right.y = head_2d.y;

			bottom_right.x = feet_2d.x + (width / 1.5);
			bottom_right.y = feet_2d.y;

			bottom_left.x = feet_2d.x - (width / 1.5);
			bottom_left.y = feet_2d.y;

			drawer(draw, top_left, top_right, ImColor(0, 0, 0, 255), 2);
			drawer(draw, top_left, top_right, ImColor(255, 0, 0, 255), 1);
				
			drawer(draw, top_right, bottom_right, ImColor(0, 0, 0, 255), 2);
			drawer(draw, top_right, bottom_right, ImColor(255, 0, 0, 255), 1);
				
			drawer(draw, bottom_right, bottom_left, ImColor(0, 0, 0, 255), 2);
			drawer(draw, bottom_right, bottom_left, ImColor(255, 0, 0, 255), 1);
				
			drawer(draw, bottom_left, top_right, ImColor(0, 0, 0, 255), 2);
			drawer(draw, bottom_left, top_right, ImColor(255, 0, 0, 255), 1);

			vec2 bottom_screen;
			bottom_screen.x = 960;
			bottom_screen.y = 1080;

			drawer(draw, bottom_screen, head_2d, ImColor(0, 0, 0, 255), 2);
			drawer(draw, bottom_screen, head_2d, ImColor(255, 0, 0, 255), 1);

		}

	}

}

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void ResetDevice();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

PVOID(__stdcall* VirtualProtectModified)(LPVOID, SIZE_T, DWORD, PDWORD);

using namespace std;

int main(int, char**)
{
	data::handles::h_driver = get_driver_handle();

	printf("[?] Pick an HWND: ");
	std::cin >> data::window::pick_hwnd;

	std::thread move_window_thread(move_window);

	system("pause");

	data::registry::cr3 = 0x1AB000;

	printf("[?] patch? (N = 0 | Y = 1) : ");
	int patch_driver;
	std::cin >> patch_driver;

	if (patch_driver == 1)
	{
		LPVOID drivers_array[1024];
		DWORD size_needed;

		if (EnumDeviceDrivers(drivers_array, sizeof(drivers_array), &size_needed) && size_needed < sizeof(drivers_array))
		{
			TCHAR to_find[] = "RwDrv.sys";
			TCHAR driver_name[1024];
			int driver_count = size_needed / sizeof(drivers_array[0]);

			for (int i = 0; i < driver_count; i++)
			{
				GetDeviceDriverBaseName(drivers_array[i], driver_name, sizeof(driver_name) / sizeof(driver_name[0]));
				if (strcmp(driver_name, to_find) == 0)
				{
					data::registry::rwdrv_address = drivers_array[i];
					printf("[+] RwDrv.sys address: 0x%x\n", data::registry::rwdrv_address);

				}


			}

		}

		if (wvpm(reinterpret_cast<uintptr_t>(data::registry::rwdrv_address), sizeof(raw_driver) / sizeof(raw_driver[0]), (unsigned char*)&raw_driver))
			printf("[+] RwDrv.sys was successfully patched!\n");
		else
		{
			printf("[-] RwDrv.sys could not be patched!\n");
			system("pause");
			return 0;

		}

	}

	system("pause");

	if (find_kprocess(data::registry::cr3, "VALORANT-Win64", &(data::process::base_address), &(data::process::process_id)))
	{
		system("cls");
		printf("[+] SWBF2 found!\n");
		printf("[+] base_address: 0x%x\n", data::process::base_address);
		printf("[+] process_id: %i\n", data::process::process_id);

	}
	else
	{
		system("cls");
		printf("[-] failed to find SWBF2!\n");
		system("pause");
		return 0;

	}

	//change cr3 to active process
	data::registry::cr3 = PML4_ADDRESS(data::registry::process_cr3);

	find_window();

	MARGINS margins = { -1 };
	SetWindowLongPtrA(data::window::hwnd, GWL_EXSTYLE, WS_EX_LAYERED | WS_POPUP | WS_EX_TRANSPARENT);
	SetLayeredWindowAttributes(data::window::hwnd, D3DCOLOR_ARGB(0, 0, 0, 0), 255, LWA_ALPHA);
	DwmExtendFrameIntoClientArea(data::window::hwnd, &margins);

	// Initialize Direct3D
	CreateDeviceD3D(data::window::hwnd);

	ShowWindow(data::window::hwnd, SW_SHOWDEFAULT);
	UpdateWindow(data::window::hwnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(data::window::hwnd);
	ImGui_ImplDX9_Init(g_pd3dDevice);

	std::thread cache_thread(cache);
	std::thread misc_thread(misc);

	// Main loop
	bool done = false;
	while (!done)
	{
		MSG msg;
		while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
		{
			::TranslateMessage(&msg);
			::DispatchMessage(&msg);
			if (msg.message == WM_QUIT)
				done = true;
		}
		if (done)
			break;

		ImGui_ImplDX9_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		ImGui::SetNextWindowBgAlpha(-1.0f);
		ImGui::Begin("mega_kek", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBackground);

		ImGui::SetWindowPos(ImVec2(0, 0));
		ImGui::SetWindowSize(ImVec2(1920, 1080));

		if (GetAsyncKeyState(VK_F1)) break;

		render();

		// Rendering
		ImGui::EndFrame();

		g_pd3dDevice->Clear(0, NULL, D3DCLEAR_TARGET, IM_COL32(0, 0, 0, 0), 1.0f, 0);
		g_pd3dDevice->BeginScene();
		ImGui::Render();
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		g_pd3dDevice->EndScene();

		g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

	}
	move_window_thread.join();
	cache_thread.join();
	misc_thread.join();

	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();
	DestroyWindow(data::window::hwnd);

	return 0;

}

bool CreateDeviceD3D(HWND hWnd)
{
	if ((g_pD3D = Direct3DCreate9(D3D_SDK_VERSION)) == NULL)
		return false;

	// Create the D3DDevice
	ZeroMemory(&g_d3dpp, sizeof(g_d3dpp));
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;; // Need to use an explicit format with alpha if needing per-pixel alpha composition.
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;           // Present with vsync
	//g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;   // Present without vsync, maximum unthrottled framerate
	if (g_pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, hWnd, D3DCREATE_HARDWARE_VERTEXPROCESSING, &g_d3dpp, &g_pd3dDevice) < 0)
		return false;

	return true;
}

void CleanupDeviceD3D()
{
	if (g_pd3dDevice) { g_pd3dDevice->Release(); g_pd3dDevice = NULL; }
	if (g_pD3D) { g_pD3D->Release(); g_pD3D = NULL; }
}

void ResetDevice()
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	HRESULT hr = g_pd3dDevice->Reset(&g_d3dpp);
	if (hr == D3DERR_INVALIDCALL)
		IM_ASSERT(0);
	ImGui_ImplDX9_CreateDeviceObjects();
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
		return true;

	switch (msg)
	{
	case WM_SIZE:
		if (g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED)
		{
			g_d3dpp.BackBufferWidth = LOWORD(lParam);
			g_d3dpp.BackBufferHeight = HIWORD(lParam);
			ResetDevice();
		}
		return 0;
	case WM_SYSCOMMAND:
		if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage(0);
		return 0;
	}
	return ::DefWindowProc(hWnd, msg, wParam, lParam);
}
