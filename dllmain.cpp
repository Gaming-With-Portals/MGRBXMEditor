#include "pch.h"
#include <assert.h>
#include "gui.h"
#include <Events.h>

#include "imgui/imgui.h"
#include <Hw.h>
#include <EntitySystem.h>
#include <Entity.h>
#include <sstream>
#include <map>
#include <BehaviorEmBase.h>
#include "cXmlBinary.h"
#include "CruelerThanDAT/FileNodes.h"
#include "TextEditor.h"

bool bxm_viewer_visible = false;
BxmFileNode* bxmnode = new BxmFileNode("");
TextEditor editor;
std::string current_file = "NONE";
class Plugin
{
public:
	static inline void InitGUI()
	{
		Events::OnDeviceReset.before += gui::OnReset::Before;
		Events::OnDeviceReset.after += gui::OnReset::After;
		Events::OnEndScene += gui::OnEndScene; 
		/* // Or if you want to switch it to Present
		Events::OnPresent += gui::OnEndScene;
		*/
	}

	Plugin()
	{
		InitGUI();

		// and here's your code
	}
} plugin;

std::string formatHex(int hexNumber) { // this function fucking sucks lmao
	std::stringstream ss;
	ss << std::hex << hexNumber;
	std::string hexString = ss.str();

	char firstDigit = hexString[0];

	std::map<char, std::string> replacements = {
		{'1', "pl"}, {'2', "em"}, {'3', "wp"},
		{'4', "et"}, {'5', "ef"}, {'6', "es"},
		{'7', "it"}, {'8', "?"}, {'9', "st"},
		{'a', "ui"}, {'b', "?"}, {'c', "?"},
		{'d', "bm"}, {'e', "bh"}, {'f', "ba"}
	};

	std::string formattedString = replacements[firstDigit] + hexString.substr(1);

	return formattedString;
}

std::string formatHexNoPrefix(int hexNumber) {

	std::stringstream ss;
	ss << std::hex << hexNumber;
	std::string hexString = ss.str();



	return hexString;
}

void gui::RenderWindow()
{



	ImGui::Begin("GWP BXM Editor");

	cVec4 last_position = cVec4(0, 0, 0, 0);
	for (auto i = (Entity**)EntitySystem::ms_Instance.m_EntityList.m_pFirst; i != (Entity**)EntitySystem::ms_Instance.m_EntityList.m_pLast; i = (Entity**)i[2])
	{
		Entity* entity = *i;

		if (!entity) {
			continue;
		}
		BehaviorEmBase* enemy = (BehaviorEmBase*)entity->m_pInstance;
		
		if (enemy->isAlive()) {
			if (ImGui::TreeNode(formatHex(entity->m_EntityIndex).c_str())) {

				for (uint32_t i = 0; i < entity->m_EntityData.getFileAmount(); i++) {
					char ext[4] = {};
					entity->m_EntityData.getExtension(ext, i);
					if (strcmp(ext, "bxm") == 0) {
						if (ImGui::Button(("Edit###" + std::to_string(i)).c_str())) {
							bxm_viewer_visible = true;
							const char* fdata = (char*)entity->m_EntityData.getFiledata(false, entity->m_EntityData.getNameByFileIndex(i));
							std::vector<char> dataVector(fdata, fdata + entity->m_EntityData.getSizeByFileIndex(i));
							bxmnode->SetFileData(dataVector);
							bxmnode->LoadFile();
							current_file = entity->m_EntityData.getNameByFileIndex(i);
						}
						ImGui::SameLine();
						ImGui::Text(entity->m_EntityData.getNameByFileIndex(i));
					}

				}


				ImGui::TreePop();
			}
		}


	}


	ImGui::End();

	if (bxm_viewer_visible) {
		ImGui::Begin("BXM Editor");
		if (ImGui::Button("Save")) {
			bxmnode->TinyXMLToGwpBXM();




		}
		if (ImGui::Button("Close")) {
			bxm_viewer_visible = false;
		}
		ImGui::SameLine();

		ImGui::Text(current_file.c_str());

		ImGui::SameLine();

		//editor.Render("TextEditor");
		bxmnode->RenderGUI();

		ImGui::End();
	}
}