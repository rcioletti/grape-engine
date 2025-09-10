#pragma once

#include <string>
#include <vector>

namespace grape {

	class MapManager {

	public:

		MapManager(std::string mapName, std::string filePath);
		~MapManager();

		std::string getMapName() { return mapName; }
		std::string getFilePath() { return fileName; }

		void loadMap(std::string fileName);
		void saveMap(std::string fileName);

	private:

		std::string mapName;
		std::string fileName;
	};
}