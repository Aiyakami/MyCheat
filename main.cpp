#include"Draw.h"
#include"Util.h"
Global globals;
Draw draw;
BOOL IsRedCamp = true;
DWORD AutoAimKey = VK_RBUTTON;
UINT64 AimObject = 0;
float MinDistance = 0;
bool autoaim = false;
bool perspective = false;
float pos = 0.75;
int dist = 360;
bool uiStop = TRUE;
#pragma warning(disable:4244)


///右键自瞄线程开始
#include<windows.h>
int globalFlag = 0;
HANDLE g_event;
LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam)//hook鼠标判断右键是否按下
{
	if (nCode >= 0 && wParam == WM_RBUTTONDOWN)
	{
		globalFlag = 1;
	}
	else if (nCode >= 0 && wParam == WM_RBUTTONUP)
	{
		globalFlag = 0;

	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}
DWORD WINAPI HookThread(LPVOID lpParameter)
{
	g_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	HHOOK mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseHookCallback, NULL, 0);
	if (mouseHook == NULL)
	{
		return 1;
	}

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	UnhookWindowsHookEx(mouseHook);

	return 0;
}
///右键自瞄线程结束



//人物链表定义
struct player_list
{
	bool effective;//是否有效
	int aimbot_len;//自瞄长度
	float location[3];//身体位置
	int camp;//阵营
	int blood;//血量
	int dormant;//休眠基地址
};
player_list players[10];

void read_memory(HANDLE process, int address, void* recv, int size)
{
	DWORD read_size;
	ReadProcessMemory(process, (LPCVOID)address, recv, size, nullptr);
	return;
}

//初始化人物链表信息    但是如果直接在这里获取坐标地址后续会出错
void get_player_list(struct player_list* players)
{

	while (true) {
		for (int i = 0; i < 10; i++)
		{
			int player_base_address;
			int g_players_address = globals.MoudleBase + 0x4D3858C;
			read_memory(globals.GameHandle, (g_players_address + i * 0x10), &player_base_address, sizeof(int));//所有人物的基地址初始化
			if (player_base_address == 0) continue;//当坐标获取识别时

			read_memory(globals.GameHandle, player_base_address + 0x100, &players[i].blood, sizeof(int));//所有人物的血量初始化

			players[i].effective = true;

			players[i].aimbot_len = 9999;

			read_memory(globals.GameHandle, player_base_address + 0xF4, &players[i].camp, sizeof(int));
			

			//改写休眠基地址
			read_memory(globals.GameHandle, player_base_address + 0xED, &players[i].dormant, sizeof(BYTE));
			
		}
	}

}

//绘制菜单
void DrawMenu()
{


	ImGui::Begin(u8"CSGO-CHEAT-OUT Bilibili Idadc", &uiStop, ImGuiWindowFlags_NoSavedSettings + ImGuiWindowFlags_AlwaysAutoResize);

	switch (autoaim) {
	case false:
		if (ImGui::Button(u8"开启自瞄")) {
			autoaim = true;
		}
		break;
	case true:
		if (ImGui::Button(u8"关闭自瞄")) {
			autoaim = false;
		}
		break;
	}
	ImGui::SameLine();
	switch (perspective) {
	case false:
		if (ImGui::Button(u8"开启人物透视")) {
			perspective = true;
		}
		break;
	case true:
		if (ImGui::Button(u8"关闭人物透视")) {
			perspective = false;
		}
		break;
	}


	ImGui::SliderInt(u8"自瞄范围", &dist, 1, 640);
	ImGui::SliderFloat(u8"自瞄位置", &pos, 0.01f, 1.0f);


	ImGui::End();
}

//透视绘制方框在这里写
void ESP()
{
	//if (!globals.StartESP) return;

	char show[100] = { 0 };

	for (int i = 0; i < 10; i++) {

		if (GetKeyState(VK_END) & 0x8000)//开启显示或者关闭
		{
			IsRedCamp = !IsRedCamp;
			Sleep(200);
		}

		
		DWORD heath = 0;
		BYTE RedCamp = 0;
		
		
		if (players[i].dormant == 1) { //休眠基地址
			continue;
		}

		if (players[i].blood == 0) {  //如果血量为0不绘制
			continue;
		}
		if (!perspective) {  //默认false
			if (players[i].camp == players[0].camp) {
				continue;
			}
		}
		Box2D box = { 0 };
		if (!draw.WorldToScreen(NULL, &box, 75.0f,players,i)) continue;//这里再进行矩阵坐标转换
		draw.DrawBox(box.x - box.w, box.y, box.w * 2, box.h * 2, draw.BoxColor, 2);//绘制方框函数  
	}
}


//死循环获取数据
void UpDate()
{
	get_player_list(players);
	
}

void Move(int dx, int dy)
{
	INPUT input;
	input.type = INPUT_MOUSE;
	input.mi.dx = dx;
	input.mi.dy = dy;
	input.mi.mouseData = 0;
	input.mi.dwFlags = MOUSEEVENTF_MOVE;
	input.mi.time = 0;
	SendInput(1, &input, sizeof(INPUT));
}
//自瞄
void AutoAimThread()
{
	
	while (globals.Running)
	{

		if (globalFlag != 1) continue;
		Box2D box = { 0 };
		int distence = 999;
		int x = 0; int y = 0;
		for (int i = 0; i < 10; i++) {
			
			//自瞄条件
			if (players[i].blood == 0) {
				continue;
			}
			if (players[i].camp == players[0].camp) {
				continue;
			}
			if (!draw.WorldToScreen(NULL, &box, 75.0f, players, i)) continue;
			int dis = sqrt(pow((box.x- globals.WinXMid),2) + pow((box.y - globals.WinYMid),2));//box.x是左上角坐标
			if (dis < distence) {
				distence = dis;
				x = box.x - globals.WinXMid;
				y = box.y + box.h/8  - globals.WinYMid;
			}
		}
		//seninput执行
		if (distence < 200) {
			Move(x,y);//这个坐标是对于左上角而言的
		}
		
		Sleep(10);
		
		
	}
}

//主函数
int main()
{

	//根据名字查找pid
	DWORD pid = Util::FindProcessId(L"csgo.exe");
	cout << "PID:" << pid << endl;

	HWND GameHwnd = FindWindow(NULL, L"Counter-Strike: Global Offensive");
	cout << "游戏窗口:" << GameHwnd << endl;

	globals.InitGlobals(GameHwnd, pid);//获取模块基地址 打开进程  能成功获取模块基地址

	//创建自瞄线程
	HANDLE hookThreadHandle = CreateThread(NULL, 0, HookThread, NULL, 0, NULL);

	CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(UpDate), nullptr, 0, nullptr);//创建多线程
	CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(AutoAimThread), nullptr, 0, nullptr);//最佳位置判断

	draw.CreateTransparenceWindows();//先创建透明窗口 
	draw.DrawMenu = DrawMenu;//绘制菜单  这里是函数指针的形式，所以不用加()
	draw.ESP = ESP;//绘制透视方框   这里给了定义
	draw.RunImGuiWindow();//循环绘制，设置了窗口永远在顶层，并且可以按home键开启和隐藏  
	return 0;
}