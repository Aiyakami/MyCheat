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


///�Ҽ������߳̿�ʼ
#include<windows.h>
int globalFlag = 0;
HANDLE g_event;
LRESULT CALLBACK MouseHookCallback(int nCode, WPARAM wParam, LPARAM lParam)//hook����ж��Ҽ��Ƿ���
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
///�Ҽ������߳̽���



//����������
struct player_list
{
	bool effective;//�Ƿ���Ч
	int aimbot_len;//���鳤��
	float location[3];//����λ��
	int camp;//��Ӫ
	int blood;//Ѫ��
	int dormant;//���߻���ַ
};
player_list players[10];

void read_memory(HANDLE process, int address, void* recv, int size)
{
	DWORD read_size;
	ReadProcessMemory(process, (LPCVOID)address, recv, size, nullptr);
	return;
}

//��ʼ������������Ϣ    �������ֱ���������ȡ�����ַ���������
void get_player_list(struct player_list* players)
{

	while (true) {
		for (int i = 0; i < 10; i++)
		{
			int player_base_address;
			int g_players_address = globals.MoudleBase + 0x4D3858C;
			read_memory(globals.GameHandle, (g_players_address + i * 0x10), &player_base_address, sizeof(int));//��������Ļ���ַ��ʼ��
			if (player_base_address == 0) continue;//�������ȡʶ��ʱ

			read_memory(globals.GameHandle, player_base_address + 0x100, &players[i].blood, sizeof(int));//���������Ѫ����ʼ��

			players[i].effective = true;

			players[i].aimbot_len = 9999;

			read_memory(globals.GameHandle, player_base_address + 0xF4, &players[i].camp, sizeof(int));
			

			//��д���߻���ַ
			read_memory(globals.GameHandle, player_base_address + 0xED, &players[i].dormant, sizeof(BYTE));
			
		}
	}

}

//���Ʋ˵�
void DrawMenu()
{


	ImGui::Begin(u8"CSGO-CHEAT-OUT Bilibili Idadc", &uiStop, ImGuiWindowFlags_NoSavedSettings + ImGuiWindowFlags_AlwaysAutoResize);

	switch (autoaim) {
	case false:
		if (ImGui::Button(u8"��������")) {
			autoaim = true;
		}
		break;
	case true:
		if (ImGui::Button(u8"�ر�����")) {
			autoaim = false;
		}
		break;
	}
	ImGui::SameLine();
	switch (perspective) {
	case false:
		if (ImGui::Button(u8"��������͸��")) {
			perspective = true;
		}
		break;
	case true:
		if (ImGui::Button(u8"�ر�����͸��")) {
			perspective = false;
		}
		break;
	}


	ImGui::SliderInt(u8"���鷶Χ", &dist, 1, 640);
	ImGui::SliderFloat(u8"����λ��", &pos, 0.01f, 1.0f);


	ImGui::End();
}

//͸�ӻ��Ʒ���������д
void ESP()
{
	//if (!globals.StartESP) return;

	char show[100] = { 0 };

	for (int i = 0; i < 10; i++) {

		if (GetKeyState(VK_END) & 0x8000)//������ʾ���߹ر�
		{
			IsRedCamp = !IsRedCamp;
			Sleep(200);
		}

		
		DWORD heath = 0;
		BYTE RedCamp = 0;
		
		
		if (players[i].dormant == 1) { //���߻���ַ
			continue;
		}

		if (players[i].blood == 0) {  //���Ѫ��Ϊ0������
			continue;
		}
		if (!perspective) {  //Ĭ��false
			if (players[i].camp == players[0].camp) {
				continue;
			}
		}
		Box2D box = { 0 };
		if (!draw.WorldToScreen(NULL, &box, 75.0f,players,i)) continue;//�����ٽ��о�������ת��
		draw.DrawBox(box.x - box.w, box.y, box.w * 2, box.h * 2, draw.BoxColor, 2);//���Ʒ�����  
	}
}


//��ѭ����ȡ����
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
//����
void AutoAimThread()
{
	
	while (globals.Running)
	{

		if (globalFlag != 1) continue;
		Box2D box = { 0 };
		int distence = 999;
		int x = 0; int y = 0;
		for (int i = 0; i < 10; i++) {
			
			//��������
			if (players[i].blood == 0) {
				continue;
			}
			if (players[i].camp == players[0].camp) {
				continue;
			}
			if (!draw.WorldToScreen(NULL, &box, 75.0f, players, i)) continue;
			int dis = sqrt(pow((box.x- globals.WinXMid),2) + pow((box.y - globals.WinYMid),2));//box.x�����Ͻ�����
			if (dis < distence) {
				distence = dis;
				x = box.x - globals.WinXMid;
				y = box.y + box.h/8  - globals.WinYMid;
			}
		}
		//seninputִ��
		if (distence < 200) {
			Move(x,y);//��������Ƕ������ϽǶ��Ե�
		}
		
		Sleep(10);
		
		
	}
}

//������
int main()
{

	//�������ֲ���pid
	DWORD pid = Util::FindProcessId(L"csgo.exe");
	cout << "PID:" << pid << endl;

	HWND GameHwnd = FindWindow(NULL, L"Counter-Strike: Global Offensive");
	cout << "��Ϸ����:" << GameHwnd << endl;

	globals.InitGlobals(GameHwnd, pid);//��ȡģ�����ַ �򿪽���  �ܳɹ���ȡģ�����ַ

	//���������߳�
	HANDLE hookThreadHandle = CreateThread(NULL, 0, HookThread, NULL, 0, NULL);

	CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(UpDate), nullptr, 0, nullptr);//�������߳�
	CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(AutoAimThread), nullptr, 0, nullptr);//���λ���ж�

	draw.CreateTransparenceWindows();//�ȴ���͸������ 
	draw.DrawMenu = DrawMenu;//���Ʋ˵�  �����Ǻ���ָ�����ʽ�����Բ��ü�()
	draw.ESP = ESP;//����͸�ӷ���   ������˶���
	draw.RunImGuiWindow();//ѭ�����ƣ������˴�����Զ�ڶ��㣬���ҿ��԰�home������������  
	return 0;
}