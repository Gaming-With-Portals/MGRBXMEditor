#pragma once
#define NOMINMAX
#include <string>
#include <vector>
#include <Windows.h>
#include "../TextEditor.h"
#include "BinaryHandler.h"
#include "tinyxml2.h"
#include <math.h>
#include <algorithm>
enum FileNodeTypes {
	DEFAULT,
	UNKNOWN,
	MOT,
	BXM,
	WEM,
	WMB,
	BNK,
	DAT,
	WTB,
	LY2,
	UID,
	UVD,
	TRG,
	EST,
	B1EFF,
	B1PHYS,
	CT2,
	ACB,
	SDX
};

namespace HelperFunction {
	int Align(int value, int alignment);
}

class HashDataContainer {
public:
	std::vector<int> Hashes;
	std::vector<short> Indices;
	std::vector<short> Offsets;
	int Shift;
	int StructSize;
};

namespace BXMInternal {
	extern const std::vector<std::string> possibleParams;

	std::vector<std::string> SplitString(const std::string& str, char delimiter);

	struct XMLAttribute {
		std::string value = "";
		std::string name = "";
	};

	struct XMLNode {
		std::string name = "";
		std::string value = "";
		XMLNode* parent;
		std::vector<XMLNode*> childNodes;
		std::vector<XMLAttribute*> childAttributes;

	};

}

class FileNode {
public:
	std::string fileName;
	std::string fileExtension;
	std::vector<FileNode*> children;
	std::vector<char> fileData;
	FileNodeTypes nodeType;
	FileNode* parent = nullptr;
	LPCWSTR fileFilter = L"All Files(*.*)\0*.*;\0";
	bool loadFailed = false;
	bool isEdited = false;
	bool canHaveChildren = false;
	static FileNode* selectedNode;


	std::string fileIcon = "";
	bool fileIsBigEndian = false;
	//ImVec4 TextColor = { 1.0f, 1.0f, 1.0f, 1.0f };

	FileNode(std::string fName);

	virtual ~FileNode();

	virtual void PopupOptionsEx();

	virtual void AppendFile();

	virtual void ExportFile();

	virtual void ReplaceFile();

	virtual void PopupOptions();


	virtual void Render();

	virtual void LoadFile() = 0;
	virtual void SaveFile() = 0;

	void SetFileData(const std::vector<char>& data);

	const std::vector<char>& GetFileData() const;
};

class UnkFileNode : public FileNode {
public:
	UnkFileNode(std::string fName);
	void LoadFile() override;
	void SaveFile() override;
};

class DatFileNode : public FileNode {
public:
	std::unordered_map<unsigned int, unsigned int> textureInfo;

	DatFileNode(std::string fName);

	void LoadFile() override;



	void SaveFile() override;
};

class BxmFileNode : public FileNode {
public:
	std::string ConvertToXML(BXMInternal::XMLNode* node, int indentLevel = 0);

	TextEditor ImEditor;
	BXMInternal::XMLNode* baseNode;
	int infoOffset;
	int dataOffset;
	int stringOffset;

	std::string xmlData = "";
	std::vector<std::array<char, 32>> roomBuffers;
	std::vector<char> xmlBuffer;

	BxmFileNode(std::string fName);

	void RenderGUI();


	BXMInternal::XMLNode* ReadXMLNode(BinaryReader reader, BXMInternal::XMLNode* parent);

	void LoadFile() override;

	std::vector<BXMInternal::XMLNode*> FlattenXMLTree(BXMInternal::XMLNode* root);

	BXMInternal::XMLNode* ConvertXML(tinyxml2::XMLElement* element, BXMInternal::XMLNode* parent = nullptr);

	int TinyXMLToGwpBXM();

	void SaveFile() override;
};

