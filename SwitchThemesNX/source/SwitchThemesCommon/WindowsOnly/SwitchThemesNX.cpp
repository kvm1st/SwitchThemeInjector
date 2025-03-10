#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <stdexcept>
#include <filesystem>
#include "../SwitchThemesCommon.hpp"
#include "../NXTheme.hpp"

using namespace std;

vector<u8> OpenFile(const string &name)
{
	basic_ifstream<u8> fin{ name, ios::binary };
	if (fin.fail())
		throw "File open failed";

	fin.seekg(0, fin.end);
	size_t len = fin.tellg();
	fin.seekg(0, fin.beg);

	vector<u8> coll(len);
	fin.read(coll.data(), len);
	fin.close();
	return coll;
}

string OpenTextFile(const string &name) 
{
	ifstream t(name);
	if (t.fail())
		throw "File open failed";

	std::string str;

	t.seekg(0, std::ios::end);
	str.reserve(t.tellg());
	t.seekg(0, std::ios::beg);

	str.assign((std::istreambuf_iterator<char>(t)),
		std::istreambuf_iterator<char>());
	
	return str;
}

void WriteFile(const string &name,const vector<u8> &data)
{
	basic_ofstream<u8> fout{ name, ios::binary };
	if (fout.fail())
		throw "File open failed";
	fout.write(data.data(), data.size());
	fout.close();
}

void println(string s)
{
	cout << s << endl;
}

#include "../Layouts/Bflyt.hpp"

int main(int argc, char** argv)
{
	if (argc != 3)
	{
		cout << "Usage:\nSwitchThemesNX <Theme pack> <SZS File>" << endl;
		return 1;
	}

	string TargetPath(argv[2]);
	vector<u8> ThemePack = OpenFile(TargetPath);
	ThemePack = Yaz0::Decompress(ThemePack);
	auto SData = SARC::Unpack(ThemePack);

	auto themeInfo = ParseNXThemeFile(SData);
	printf("Installing...");
	auto BaseFile = OpenFile(argv[1]);
	BaseFile = Yaz0::Decompress(BaseFile);
	auto ToPatch = SARC::Unpack(BaseFile);
	auto patch = SwitchThemesCommon::DetectSarc(ToPatch);
	if (patch.FirmName == "")
	{
		println("Couldn't find any patch for " + string(argv[2]) + "\nThe theme was not installed");
		return 0;
	}
	if (SData.files.count("image.dds")) {
		auto pResult = SwitchThemesCommon::PatchBgLayouts(ToPatch, patch);
		if (pResult != BflytFile::PatchResult::OK)
		{
			println("PatchBgLayouts failed for " + string(argv[2]) + "\nThe theme was not installed");
			return 0;
		}
		pResult = SwitchThemesCommon::PatchBntx(ToPatch, SData.files["image.dds"], patch);
		if (pResult != BflytFile::PatchResult::OK)
		{
			println("PatchBntx failed for " + string(argv[2]) + "\nThe theme was not installed");
			return 0;
		}
	}
	if (SData.files.count("layout.json"))
	{
		auto JsonBinary = SData.files["layout.json"];
		string JSON(reinterpret_cast<char*>(JsonBinary.data()), JsonBinary.size());
		auto patch = Patches::LoadLayout(JSON);
		if (!patch.IsCompatible(ToPatch))
		{
			println("The provided layout is not compatible with " + string(argv[2]) + "\nThe theme was not installed");
			return 0;
		}
		auto res = SwitchThemesCommon::PatchLayouts(ToPatch, patch, true);
		if (res != BflytFile::PatchResult::OK)
		{
			println("PatchLayouts failed for " + string(argv[2]) + "\nThe theme was not installed");
			return 0;
		}
		res = SwitchThemesCommon::PatchAnimations(ToPatch, patch.Anims);
		if (res != BflytFile::PatchResult::OK)
		{
			println("Patch anims failed for " + string(argv[2]) + "\nThe theme was not installed");
			return 0;
		}
	}
	auto packed = SARC::Pack(ToPatch);
	BaseFile = Yaz0::Compress(packed.data, 3, packed.align);
	WriteFile("PATCHED"+ patch.szsName, BaseFile);

	cout << "DONE !" << endl;

	getchar();
    return 0;
}

