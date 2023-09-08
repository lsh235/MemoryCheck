#include <iostream>
#include <vector>
#include <thread>
#include <windows.h>
#include <psapi.h>
#include <fstream>
#include <string>
#include <ctime>

#include <Shlwapi.h> // PathFindFileName 함수를 사용하기 위한 헤더
#pragma comment(lib, "Shlwapi.lib") // Shlwapi 라이브러리 링크

using namespace std;

string NowTime() {
    std::time_t current_time = std::time(nullptr);
    std::tm time_info;
    localtime_s(&time_info, &current_time);
    char time_str[20]; // 현재 시간을 문자열로 저장하기 위한 버퍼
    std::strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &time_info);
    return time_str;
}

string GetThisProcessDir() {
    char buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);

    // 파일 이름 부분을 제거하여 디렉토리만 남깁니다.
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
        ++pFileName; // '\\' 문자 다음부터 파일 이름이 시작됩니다.
    }
    else {
        pFileName = szProcessName; // '\\' 문자가 없으면 전체 경로가 이미 파일 이름입니다.
    }
    return pFileName;
}

bool MonitorMemory(const DWORD targetProcessPID) {
    HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, targetProcessPID);

    if (processHandle == NULL) {
        std::cerr << "프로세스 핸들을 열 수 없습니다. error code : " << GetLastError() << std::endl;
        return false;
    }

    wchar_t szProcessName[MAX_PATH];
    DWORD dwSize = sizeof(szProcessName) / sizeof(wchar_t);
    if (QueryFullProcessImageNameW(processHandle, 0, szProcessName, &dwSize)) {
        std::wcout << L"현재 프로세스의 이미지 경로: " << szProcessName << std::endl;
    }
    else {
        std::cerr << "현재 프로세스의 이미지 경로를 가져올 수 없습니다." << std::endl;
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
            std::cout << "프로세스 메모리 사용량: " << working_set_size << " KB" << std::endl;
            std::cout << "프로세스 개인 사용량: " << private_usage << " KB" << std::endl;
#endif

            std::string log_file_path = GetThisProcessDir() + "\\" + process_name + std::string("_memory_log.txt");
            std::ofstream outfile(log_file_path, std::ios::app);
            if (outfile) {
                outfile << NowTime() << " 프로세스 메모리 사용량: " << working_set_size << " KB" << std::endl;
                outfile << NowTime() << " 프로세스 개인 사용량: " << private_usage << " KB" << std::endl;
                outfile << std::endl;
                outfile.close();
            }
        }
        else {
            std::cerr << "메모리 정보를 가져올 수 없습니다." << std::endl;
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

    // 스레드의 결과를 저장하는 벡터
    std::vector<bool> threadResults(threads.size(), false);

    // 모든 스레드를 시작
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].detach(); // 스레드를 독립적으로 실행
    }

    while (true) {

    }

    return 0;
}