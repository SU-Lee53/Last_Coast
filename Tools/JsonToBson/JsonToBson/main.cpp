#include <iostream>
#include <conio.h>
#include <fstream>
#include <nlohmann_json/json.hpp>
#include <filesystem>
#include <vector>

using namespace std::filesystem;
using namespace nlohmann;

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2) {
		std::wcout << L"Drag-Drop this .exe to json file" << std::endl;
		std::wcout << L"Press any key to continue" << std::endl;
		_getch();

		return -1;
	}

	for (int i = 1; i < argc; ++i) {
		std::wstring wstrIn = argv[i];

		std::ifstream inJsonFile{ wstrIn };
		if (!inJsonFile) {
			std::wcout << L"Failed to open" << wstrIn << std::endl;
			std::wcout << L"Press any key to continue" << std::endl;
			_getch();

			continue;
		}

		json jOpened = json::parse(inJsonFile);
		std::vector<uint8_t> bson = json::to_bson(jOpened);

		path jsonPath{ wstrIn };
		std::wstring wstrBsonPath = jsonPath.stem().wstring() + L".bin";

		std::ofstream outBson{ wstrBsonPath, std::ios::binary };
		outBson.write(reinterpret_cast<const char*>(bson.data()), bson.size());

		std::wcout << wstrIn << "Successfully converted to " << wstrBsonPath << std::endl;
	}

	std::wcout << L"Convert Successfully done" << std::endl;
	std::wcout << L"Press any key to continue" << std::endl;
	_getch();

	return 0;

}