#include "Settings.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>

namespace SoulSettings {
	////takes an .ini filename
	//void Parse(std::string file) {
	//	detail::fileName = file;
	//	std::ifstream infile(detail::fileName);
	//	if (!infile.is_open()) {
	//		std::ofstream outfile(detail::fileName);
	//		outfile.close();
	//		infile.open(detail::fileName);
	//	}

	//	std::string line;
	//	while (std::getline(infile, line))
	//	{
	//		std::istringstream iss(line);
	//		std::string a;
	//		int b;
	//		if (!(iss >> a >> b)) {
	//			std::cerr << "Couldn't read line in " + detail::fileName << std::endl;
	//		}

	//		detail::values.insert(std::make_pair(a, b));
	//	}
	//	infile.close();
	//}
	////Returns a string with the value at 'Setting'. If one does not exist, this 
	////returns an empty string. Remember to have a default case for this instance!
	//int Retrieve(std::string setting) {

	//	std::map<std::string, int>::const_iterator itr = values.find(setting);
	//	if (itr != values.end()) {
	//		return itr->second;
	//	}
	//	else {
	//		return -1;
	//	}
	//}
	//int Retrieve(std::string setting, int defaultSet) {

	//	std::map<std::string, int>::const_iterator itr = values.find(setting);
	//	if (itr != values.end()) {
	//		return itr->second;
	//	}
	//	else {
	//		Set(setting, defaultSet);
	//		return defaultSet;
	//	}
	//}
	////Inserts the value at setting. If a setting already exists, value replaces it
	//void Settings::Set(std::string setting, int value) {

	//	if (values.insert(std::make_pair(setting, value)).second == true) {
	//	}
	//	else {
	//		values.find(setting)->second = value;
	//	}
	//	Update();
	//}
	//void Settings::Update() {

	//	remove(fileName.c_str());

	//	std::ofstream outfile(fileName);

	//	for (std::map<std::string, int>::iterator itr = values.begin();
	//		itr != values.end(); itr++) {
	//		outfile << itr->first << " " << itr->second << std::endl;
	//	}

	//	outfile.close();
	//}

}