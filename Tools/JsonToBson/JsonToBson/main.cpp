#include <iostream>
#include <fstream>
#include <Windows.h>
#include <nlohmann_json/json.hpp>
#include <filesystem>

using namespace std::filesystem;
using namespace nlohmann;

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2) {
		std::wcout << L"Drag-Drop this .exe to image file";
		return -1;
	}

	std::wstring wstrIn = argv[1];

	std::ifstream inJsonFile{wstrIn};
	json jOpened = json::parse(inJsonFile);


}