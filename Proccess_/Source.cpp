#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>
#include <psapi.h>
#include <fstream>
#include <string>
#include <ctime>

#include <Shlwapi.h> // PathFindFileName �Լ��� ����ϱ� ���� ���
#pragma comment(lib, "Shlwapi.lib") // Shlwapi ���̺귯�� ��ũ

using namespace std;

string NowTime() {
    std::time_t current_time = std::time(nullptr);
    std::tm time_info;
    localtime_s(&time_info, &current_time);
    char time_str[20]; // ���� �ð��� ���ڿ��� �����ϱ� ���� ����
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &time_info);
    return time_str;
}

string GetThisProcessDir() {
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);

    // ���� �̸� �κ��� �����Ͽ� ���丮�� ����ϴ�.
    PathRemoveFileSpec(buffer);

    return buffer;
}

string ConvertWchartToString(const wchar_t* w_char) {
    std::wstring wstrProcessName(w_char);
    std::string szAnsiProcessName(wstrProcessName.begin(), wstrProcessName.end());
    return szAnsiProcessName;
}

wchar_t* GetProcessName(wchar_t* szProcessName) {
    wchar_t* pFileName = wcsrchr(szProcessName, L'\\');
    if (pFileName) {
        ++pFileName; // '\\' ���� �������� ���� �̸��� ���۵˴ϴ�.
    }
    else {
        pFileName = szProcessName; // '\\' ���ڰ� ������ ��ü ��ΰ� �̹� ���� �̸��Դϴ�.
    }
    return pFileName;
}

bool MonitorMemory(const DWORD targetProcessPID) {
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetProcessPID);

    if (processHandle == NULL) {
        std::cerr << "���μ��� �ڵ��� �� �� �����ϴ�. error code : " << GetLastError() << std::endl;
        return false;
    }

    wchar_t szProcessName[MAX_PATH];
    DWORD dwSize = sizeof(szProcessName) / sizeof(wchar_t);
    if (QueryFullProcessImageNameW(processHandle, 0, szProcessName, &dwSize)) {
        std::wcout << L"���� ���μ����� �̹��� ���: " << szProcessName << std::endl;
    }
    else {
        std::cerr << "���� ���μ����� �̹��� ��θ� ������ �� �����ϴ�." << std::endl;
    }

    const auto process_name_ext = ConvertWchartToString(GetProcessName(szProcessName));

    //std::wstring wstrProcessName(pFileName);
    //std::string szAnsiProcessName(wstrProcessName.begin(), wstrProcessName.end());

    size_t lastDotPos = process_name_ext.find_last_of('.');
    std::string process_name;
    if (lastDotPos != std::string::npos) {
        process_name = process_name_ext.substr(0, lastDotPos);
    }
    else {
        process_name = process_name_ext;
    }

    try {
    while (true) {
        PROCESS_MEMORY_COUNTERS_EX pmc;
        pmc.cb = sizeof(PROCESS_MEMORY_COUNTERS_EX);

        if (GetProcessMemoryInfo(processHandle, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(PROCESS_MEMORY_COUNTERS_EX))) {
            const auto& working_set_size = pmc.WorkingSetSize / (1024);
            const auto& private_usage = pmc.PrivateUsage / (1024);
#ifdef DEBUG
            std::cout << "���μ��� �޸� ��뷮: " << working_set_size << " KB" << std::endl;
            std::cout << "���μ��� ���� ��뷮: " << private_usage << " KB" << std::endl;
#endif

            std::string log_file_path = GetThisProcessDir() + "\\" + process_name + std::string("_memory_log.txt");
            std::ofstream outfile(log_file_path, std::ios::app);
            if (outfile) {
                outfile << NowTime() << " ���μ��� �޸� ��뷮: " << working_set_size << " KB" << std::endl;
                outfile << NowTime() << " ���μ��� ���� ��뷮: " << private_usage << " KB" << std::endl;
                outfile << std::endl;
                outfile.close();
            }
        }
        else {
            std::cerr << "�޸� ������ ������ �� �����ϴ�." << std::endl;
        }

        Sleep(1000);
    }
    }
    catch(...){
        CloseHandle(processHandle);
        return false;
    }

    CloseHandle(processHandle);
    return true;
}

int main(int argc, char* argv[]) {

    std::vector<std::thread> threads;
    
    for (int i = 1; i < argc; ++i) {
        DWORD targetProcessPID = std::atoi(argv[i]);
        threads.emplace_back(MonitorMemory, targetProcessPID);
    }

    // �������� ����� �����ϴ� ����
    std::vector<bool> threadResults(threads.size(), false);

    // ��� �����带 ����
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].detach(); // �����带 ���������� ����
    }

    while (true) {

    }

    return 0;
}