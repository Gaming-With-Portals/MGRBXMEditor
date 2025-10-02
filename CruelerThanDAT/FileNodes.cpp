#include "FileNodes.h"
#include <queue>
#include "CRC32.h"


int IntLength(int value) {
	int length = 0;
	while (value > 0) {
		value >>= 1;
		length++;
	}
	return length;
}

int HelperFunction::Align(int value, int alignment) {
	return (value + (alignment - 1)) & ~(alignment - 1);
}

FileNode::FileNode(std::string fName) {
	fileExtension = fileName.substr(fileName.find_last_of(".") + 1);
}

FileNode::~FileNode() {

}

void FileNode::PopupOptionsEx() {

}

void FileNode::AppendFile()
{

}

void FileNode::ExportFile() {


}

void FileNode::ReplaceFile() {


}

void FileNode::PopupOptions() {

}


void FileNode::Render() {


}

void FileNode::SetFileData(const std::vector<char>& data) {
	fileData = data;
}

const std::vector<char>& FileNode::GetFileData() const {
	return fileData;
}


std::string BxmFileNode::ConvertToXML(BXMInternal::XMLNode* node, int indentLevel) {
	if (!node) return "";

	std::string indent(indentLevel * 2, ' '); // Indentation for readability
	std::string xml = indent + "<" + node->name;

	// Process attributes
	for (const auto& attr : node->childAttributes) {
		if (!attr) continue;
		xml += " " + attr->name + "=\"" + attr->value + "\"";
	}

	if (node->childNodes.empty() && node->value.empty()) {
		xml += "/>\n"; // Self-closing tag
		return xml;
	}

	xml += ">";

	// Add value if present
	if (!node->value.empty()) {
		xml += node->value;

	}

	// Process child nodes recursively
	if (!node->childNodes.empty()) {
		xml += "\n";
		for (const auto& child : node->childNodes) {
			xml += ConvertToXML(child, indentLevel + 1);
		}
		xml += indent;
	}

	xml += "</" + node->name + ">\n";

	return xml;
}


BxmFileNode::BxmFileNode(std::string fName) : FileNode(fName) {
	fileIcon = "";
	fileIsBigEndian = false;
	nodeType = BXM;
	fileFilter = L"Binary XML Files(*.bxm)\0*.bxm;\0";
}

void BxmFileNode::RenderGUI() {

	ImEditor.Render("TextEditor");



}


BXMInternal::XMLNode* BxmFileNode::ReadXMLNode(BinaryReader reader, BXMInternal::XMLNode* parent) {

	BXMInternal::XMLNode* node = new BXMInternal::XMLNode();
	node->parent = parent;
	int childCount = reader.ReadUINT16();
	int firstChildIndex = reader.ReadUINT16();
	int attributeNumber = reader.ReadUINT16();
	int dataIndex = reader.ReadUINT16();

	reader.Seek(dataOffset + (dataIndex * 4)); // Seek to data position

	int nameOffset = reader.ReadUINT16();
	int valueOffset = reader.ReadUINT16();

	if (nameOffset != 0xFFFF) {
		reader.Seek(stringOffset + nameOffset);
		node->name = reader.ReadNullTerminatedString();
	}
	if (valueOffset != 0xFFFF) {
		reader.Seek(stringOffset + valueOffset);
		node->value = reader.ReadNullTerminatedString();
	}

	for (int i = 0; i < attributeNumber; i++) {
		reader.Seek(dataOffset + ((dataIndex + i + 1) * 4));
		nameOffset = reader.ReadUINT16();
		valueOffset = reader.ReadUINT16();
		BXMInternal::XMLAttribute* attrib = new BXMInternal::XMLAttribute();

		if (nameOffset != 0xFFFF) {
			reader.Seek(stringOffset + nameOffset);
			attrib->name = reader.ReadNullTerminatedString();
		}
		if (valueOffset != 0xFFFF) {
			reader.Seek(stringOffset + valueOffset);
			attrib->value = reader.ReadNullTerminatedString();
		}




		node->childAttributes.push_back(attrib);
	}



	for (int i = 0; i < childCount; i++) {
		reader.Seek(infoOffset + ((firstChildIndex + i) * 8));
		node->childNodes.push_back(ReadXMLNode(reader, node));

	}

	return node;
}

void BxmFileNode::LoadFile() {
	BinaryReader reader(fileData, true);
	reader.Seek(0x8);
	int nodeCount = reader.ReadUINT16();
	int dataCount = reader.ReadUINT16();
	reader.Skip(sizeof(int)); // TODO: dataSize

	infoOffset = 16;
	dataOffset = 16 + 8 * nodeCount;
	stringOffset = 16 + 8 * nodeCount + dataCount * 4;


	baseNode = ReadXMLNode(reader, nullptr);
	xmlData = ConvertToXML(baseNode);

	ImEditor.SetText(xmlData);

	return;
}

std::vector<BXMInternal::XMLNode*> BxmFileNode::FlattenXMLTree(BXMInternal::XMLNode* root) {
	std::vector<BXMInternal::XMLNode*> output;
	std::queue<BXMInternal::XMLNode*> q;
	q.push(root);

	while (!q.empty()) {
		BXMInternal::XMLNode* node = q.front();
		q.pop();
		output.push_back(node);

		for (BXMInternal::XMLNode* child : node->childNodes) {
			q.push(child);
		}
	}

	return output;
}

BXMInternal::XMLNode* BxmFileNode::ConvertXML(tinyxml2::XMLElement* element, BXMInternal::XMLNode* parent) {
	BXMInternal::XMLNode* node = new BXMInternal::XMLNode();
	node->name = element->Name();
	node->value = element->GetText() ? element->GetText() : "";
	node->parent = parent;

	const tinyxml2::XMLAttribute* attr = element->FirstAttribute();
	while (attr) {
		auto* attrPtr = new BXMInternal::XMLAttribute();
		attrPtr->name = attr->Name();
		attrPtr->value = attr->Value();
		node->childAttributes.push_back(attrPtr);
		attr = attr->Next();
	}

	for (tinyxml2::XMLElement* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
		BXMInternal::XMLNode* childNode = ConvertXML(child, node);
		node->childNodes.push_back(childNode);
	}

	return node;
}

int BxmFileNode::TinyXMLToGwpBXM() {
	tinyxml2::XMLDocument doc;
	if (doc.Parse(ImEditor.GetText().c_str()) != tinyxml2::XML_SUCCESS) {
		//CTDLog::Log::getInstance().LogError("Failed to repack BXM! Check your XML file for errors.");
		MessageBoxA(nullptr, "Failed to repack BXM", "GWP BXM Editor", MB_ICONERROR | MB_OK);
		return -1;
	}

	tinyxml2::XMLElement* root = doc.RootElement();
	if (!root) {
		//CTDLog::Log::getInstance().LogError("Failed to repack BXM! Your XML requires a root node to be valid.");
		MessageBoxA(nullptr, "Failed to repack BXM", "GWP BXM Editor", MB_ICONERROR | MB_OK);
		return -1;
	}

	baseNode = ConvertXML(root);

	return 0;
}

void BxmFileNode::SaveFile() {
	if (!isEdited) {
		return;
	}

	if (TinyXMLToGwpBXM() != 0) {
		//CTDLog::Log::getInstance().LogError(fileName + " failed to repack!");

	}

	BinaryWriter* writer = new BinaryWriter(true);
	writer->WriteString("BXM");
	writer->WriteByteZero();
	writer->WriteUINT32(0);
	std::vector<BXMInternal::XMLNode*> nodes = FlattenXMLTree(baseNode);
	std::unordered_set<std::string> uniqueStrings;
	for (BXMInternal::XMLNode* node : nodes) {
		uniqueStrings.insert(node->name);
		uniqueStrings.insert(node->value);
		for (BXMInternal::XMLAttribute* attrib : node->childAttributes) {
			uniqueStrings.insert(attrib->name);
			uniqueStrings.insert(attrib->value);
		}
	}

	std::unordered_map<std::string, int> stringOffsets;
	int offsetTicker = 0;
	for (std::string nodeString : uniqueStrings) {
		if (nodeString.empty()) {
			stringOffsets[nodeString] = 0xFFFF;
			offsetTicker += 1; // I have no idea why this happens but it works 
		}
		else {
			stringOffsets[nodeString] = offsetTicker;
			offsetTicker += (nodeString.size() + 1);
		}


	}

	struct DataInfo {
		int name;
		int value;
	};


	std::vector<DataInfo> dataInfos;
	std::unordered_map <BXMInternal::XMLNode*, int> nodeInfoToDataIndice;
	int i = 0;
	for (BXMInternal::XMLNode* node : nodes) {
		dataInfos.push_back({ stringOffsets[node->name], stringOffsets[node->value] });
		nodeInfoToDataIndice[node] = i;
		for (BXMInternal::XMLAttribute* attrib : node->childAttributes) {
			dataInfos.push_back({ stringOffsets[attrib->name], stringOffsets[attrib->value] });
			i++;
		}

		i++;
	}

	writer->WriteUINT16(nodes.size());
	writer->WriteUINT16(dataInfos.size());
	writer->WriteUINT32(offsetTicker);

	struct BXMNodeInfo {
		int childSize;
		int firstChildIdx;
		int attributeSize;
		int dataIdx;
	};

	std::unordered_map<BXMInternal::XMLNode*, BXMNodeInfo> nodeInfos;
	std::unordered_map<BXMInternal::XMLNode*, int> nodeToIndex;
	for (int i = 0; i < nodes.size(); ++i) {
		nodeToIndex[nodes[i]] = i;
		nodeInfos[nodes[i]] = { (int)nodes[i]->childNodes.size(), -1, (int)nodes[i]->childAttributes.size(), nodeInfoToDataIndice[nodes[i]] };
	}


	for (BXMInternal::XMLNode* node : nodes) {
		BXMNodeInfo& info = nodeInfos[node];
		int nextIndex = -1;

		if (!node->childNodes.empty()) {
			BXMInternal::XMLNode* firstChild = node->childNodes.front();
			nextIndex = nodeToIndex[firstChild];
		}
		else {
			BXMInternal::XMLNode* parent = node->parent;

			if (parent != nullptr) {
				BXMInternal::XMLNode* lastChild = parent->childNodes.back();
				int lastChildIndex = nodeToIndex[lastChild];
				nextIndex = lastChildIndex + 1;
			}
			else {
				nextIndex = static_cast<int>(nodes.size());
			}
		}

		info.firstChildIdx = nextIndex;
	}


	for (BXMInternal::XMLNode* node : nodes) {
		writer->WriteUINT16(nodeInfos[node].childSize);
		writer->WriteUINT16(nodeInfos[node].firstChildIdx);
		writer->WriteUINT16(nodeInfos[node].attributeSize);
		writer->WriteUINT16(nodeInfos[node].dataIdx);
	}

	for (DataInfo info : dataInfos) {
		writer->WriteUINT16(info.name);
		writer->WriteUINT16(info.value);
	}

	for (std::string str : uniqueStrings) {
		writer->WriteString(str);
		writer->WriteByteZero();
	}

	fileData = writer->GetData();
}


DatFileNode::DatFileNode(std::string fName) : FileNode(fName) {
	fileIcon = "";
	nodeType = DAT;
	fileFilter = L"Platinum File Container(*.dat, *.dtt, *.eff, *.evn, *.eft)\0*.dat;*.dtt;*.eff;*.evn;*.eft;\0";
	canHaveChildren = true;
}

void DatFileNode::LoadFile() {
	BinaryReader reader(fileData, false);
	reader.Seek(0x4);
	unsigned int FileCount = reader.ReadUINT32();
	unsigned int PositionsOffset = reader.ReadUINT32();
	reader.Skip(sizeof(unsigned int)); // TODO: ExtensionsOffset
	unsigned int NamesOffset = reader.ReadUINT32();
	unsigned int SizesOffset = reader.ReadUINT32();
	reader.Skip(sizeof(unsigned int)); // TODO: HashMapOffset

	reader.Seek(PositionsOffset);
	std::vector<int> offsets;
	for (unsigned int f = 0; f < FileCount; f++) {
		offsets.push_back(reader.ReadUINT32());
	}

	reader.Seek(NamesOffset);
	int nameLength = reader.ReadUINT32();
	std::vector<std::string> names;
	for (unsigned int f = 0; f < FileCount; f++) {
		std::string temp_name = reader.ReadString(nameLength);
		temp_name.erase(std::remove(temp_name.begin(), temp_name.end(), '\0'), temp_name.end());
		names.push_back(temp_name);
	}

	reader.Seek(SizesOffset);
	std::vector<int> sizes;
	for (unsigned int f = 0; f < FileCount; f++) {
		sizes.push_back(reader.ReadUINT32());
	}

	for (unsigned int f = 0; f < FileCount; f++) {
		reader.Seek(offsets[f]);
		

		//FileNode* childNode = HelperFunction::LoadNode(names[f], reader.ReadBytes(sizes[f]), fileIsBigEndian, fileIsBigEndian);
		FileNode* childNode = new UnkFileNode(names[f]);
		childNode->SetFileData(reader.ReadBytes(sizes[f]));

		childNode->parent = this;
		if (childNode) {
			children.push_back(childNode);
		}

	}


}

UnkFileNode::UnkFileNode(std::string fName) : FileNode(fName) {
}

void UnkFileNode::LoadFile() {
}

void UnkFileNode::SaveFile() {
}

void DatFileNode::SaveFile() {



	int longestName = 0;

	for (FileNode* child : children) {
		child->SaveFile();
		if (child->fileName.length() > longestName) {
			longestName = static_cast<int>(child->fileName.length() + 1);
		}
	}

	CRC32 crc32;

	std::vector<std::string> fileNames;
	for (FileNode* node : children) {
		fileNames.push_back(node->fileName);
	}

	int shift = std::min(31, 32 - IntLength(static_cast<int>(fileNames.size())));
	int bucketSize = 1 << (31 - shift);

	std::vector<short> bucketTable(bucketSize, -1);

	std::vector<std::pair<int, short>> hashTuple;
	for (int i = 0; i < fileNames.size(); ++i) {
		//int hashValue = crc32.HashToUInt32(fileNames[i]) & 0x7FFFFFFF;
		int hashValue = ComputeHash(fileNames[i], crc32);
		hashTuple.push_back({ hashValue, static_cast<short>(i) });
	}

	// Sort the hash tuples based on shifted hash values
	std::sort(hashTuple.begin(), hashTuple.end(), [shift](const std::pair<int, short>& a, const std::pair<int, short>& b) {
		return (a.first >> shift) < (b.first >> shift);
		});

	// Populate bucket table with the first unique index for each bucket
	for (int i = 0; i < fileNames.size(); ++i) {
		int bucketIndex = hashTuple[i].first >> shift;
		if (bucketTable[bucketIndex] == -1) {
			bucketTable[bucketIndex] = static_cast<short>(i);
		}
	}

	// Create the result object with the hash data
	HashDataContainer hashData;
	hashData.Shift = shift;
	hashData.Offsets = bucketTable;
	hashData.Hashes.reserve(hashTuple.size());
	hashData.Indices.reserve(hashTuple.size());

	for (const auto& tuple : hashTuple) {
		hashData.Hashes.push_back(tuple.first);
		hashData.Indices.push_back(tuple.second);
	}

	hashData.StructSize = static_cast<int>(4 + 2 * bucketTable.size() + 4 * hashTuple.size() + 2 * hashTuple.size());

	BinaryWriter* writer = new BinaryWriter();
	writer->SetEndianess(fileIsBigEndian);
	writer->WriteString("DAT");
	writer->WriteByteZero();
	int fileCount = static_cast<int>(children.size());

	int positionsOffset = 0x20;
	int extensionsOffset = positionsOffset + 4 * fileCount;
	int namesOffset = extensionsOffset + 4 * fileCount;
	int sizesOffset = namesOffset + (fileCount * longestName) + 6;
	int hashMapOffset = sizesOffset + 4 * fileCount;

	writer->WriteUINT32(fileCount);
	writer->WriteUINT32(positionsOffset);
	writer->WriteUINT32(extensionsOffset);
	writer->WriteUINT32(namesOffset);
	writer->WriteUINT32(sizesOffset);
	writer->WriteUINT32(hashMapOffset);
	writer->WriteUINT32(0);

	writer->Seek(positionsOffset);
	for (FileNode* child : children) {
		(void)child; // TODO: If child isn't gonna be used in a future patch, remove it entirely instead of discarding.
		writer->WriteUINT32(0);
	}


	writer->Seek(extensionsOffset);
	for (FileNode* child : children) {
		writer->WriteString(child->fileExtension);
		writer->WriteByteZero();
	}

	writer->Seek(namesOffset);
	writer->WriteUINT32(longestName);
	for (FileNode* child : children) {
		writer->WriteString(child->fileName);
		for (int i = 0; i < longestName - child->fileName.length(); ++i) {
			writer->WriteByteZero();
		}
	}
	// Pad
	writer->WriteINT16(0);

	writer->Seek(sizesOffset);
	for (FileNode* child : children) {
		writer->WriteUINT32(static_cast<uint32_t>(child->fileData.size()));
	}

	writer->Seek(hashMapOffset);

	// Prepare for hash writing
	writer->WriteUINT32(hashData.Shift);
	writer->WriteUINT32(16);
	writer->WriteUINT32(16 + static_cast<uint32_t>(hashData.Offsets.size()) * 2);
	writer->WriteUINT32(16 + static_cast<uint32_t>(hashData.Offsets.size()) * 2 + static_cast<uint32_t>(hashData.Hashes.size()) * 4);

	for (int i = 0; i < hashData.Offsets.size(); i++)
		writer->WriteINT16(hashData.Offsets[i]);

	for (int i = 0; i < fileCount; i++)
		writer->WriteINT32(hashData.Hashes[i]);

	for (int i = 0; i < fileCount; i++)
		writer->WriteINT16(hashData.Indices[i]);

	std::vector<int> offsets;
	for (FileNode* child : children) {
		(void)child; // TODO: If child isn't gonna be used in a future patch, remove it entirely instead of discarding.

		int targetPosition = HelperFunction::Align(static_cast<int>(writer->Tell()), 1024);
		int padding = targetPosition - static_cast<int>(writer->GetData().size());
		if (padding > 0) { // TODO: Replace this with an skip function
			std::vector<char> zeroPadding(padding, 0);
			writer->WriteBytes(zeroPadding);
		}


		offsets.push_back(static_cast<int>(writer->Tell()));
		writer->WriteBytes(child->fileData);

	}

	writer->Seek(positionsOffset);
	for (int i = 0; i < fileCount; i++) {
		writer->WriteUINT32(offsets[i]);
	}

	fileData = writer->GetData();
}