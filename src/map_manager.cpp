#pragma warning(disable: 4996)

#include "map_manager.hpp"

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/istreamwrapper.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/ostreamwrapper.h>
#include <rapidjson/prettywriter.h>

#include <filesystem>
#include <sstream> 
#include <fstream>
#include <string>
#include <iostream>

grape::MapManager::MapManager(std::string mapName, std::string filePath)
{
}

grape::MapManager::~MapManager()
{
}

void grape::MapManager::loadMap(std::string fileName)
{

	FILE* filepoint;
	errno_t err;
	std::string filePath = "maps/" + fileName;
	rapidjson::Document document;

	if ((err = fopen_s(&filepoint, filePath.c_str(), "rb")) != 0) {
		std::cout << "Failed to open " << fileName << "\n";
		return;
	}

	char buffer[65536];
	rapidjson::FileReadStream is(filepoint, buffer, sizeof(buffer));
	document.ParseStream<0, rapidjson::UTF8<>, rapidjson::FileReadStream>(is);
	if (document.HasParseError()) {
		//std::cout << "Error : " << document.GetParseError() << '\n' << "Offset : " << document.GetErrorOffset() < '\n';
	}
	fclose(filepoint);

	if (document.HasMember("mapObjects")) {	
		std::cout << document["mapObjects"][1]["position"].GetString();
	}
}
