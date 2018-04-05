#include "AppGui.hpp"

#include <algorithm>

#include "tinyfiledialogs.h"

#include "imgui.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>

#define TINYOBJLOADER_USE_DOUBLE
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#define SETTINGS_FILE "viewer_settings.ini"

#define WAVE_MIN 380
#define WAVE_MAX 760

#include "glm/gtc/quaternion.hpp"
#include "glm/glm.hpp"
#include "glm/common.hpp"
#include "glm/gtc/type_ptr.hpp"

//=======================================================================
//function : AppGui
//purpose  :
//=======================================================================
AppGui::AppGui()
{
  char aPath[256];
  std::sprintf (aPath, "%s\\%s", getenv ("USERPROFILE"), SETTINGS_FILE);
  mySettings.reset (new Settings (aPath));

  SetupImGuiStyle();
}

//=======================================================================
//function : ~AppGui
//purpose  :
//=======================================================================
AppGui::~AppGui()
{
  char aPath[256];
  std::sprintf (aPath, "%s\\%s", getenv ("USERPROFILE"), SETTINGS_FILE);
  mySettings->Dump (aPath);
}

//=======================================================================
// function : Splitter
// purpose  :
//=======================================================================
bool Splitter (const ImVec2& size_arg)
{
  ImGuiWindow* window = ImGui::GetCurrentWindow();
  if (window->SkipItems)
      return false;

  ImGuiContext& g = *GImGui;
  const ImGuiStyle& style = g.Style;
  const ImGuiID id = window->GetID ("Splitter");

  ImVec2 pos = window->DC.CursorPos;
  ImVec2 size = size_arg;

  const ImRect bb (pos, pos + size);

  ImGui::ItemAdd (bb, id);

  bool hovered, held;
  ImGui::ButtonBehavior (bb, id, &hovered, &held, 0);

  // Render
  const ImU32 col = ImGui::GetColorU32 ((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_WindowBg);
  ImGui::RenderFrame (bb.Min, bb.Max, col, true, style.FrameRounding);

  return held;
}

//=======================================================================
// function : LayerItemGetter
// purpose  :
//=======================================================================
static bool LayerItemGetter (void* theData, int theItem, const char** theName)
{
  static char aName[256];
  sprintf_s (aName, 256, "Layer %d", theItem);
  *theName = aName;

  return true;
};

//=======================================================================
//function : Draw
//purpose  :
//=======================================================================
void AppGui::Draw (int theScreenWidth, int theScreenHeight)
{
  // Draw panel
  static float aPanelWidth = (float) mySettings->GetReal ("main-panel", "width",
    std::max (400.0, theScreenWidth * 0.2));

  static bool isDraggingPanel = false;

  if (isDraggingPanel && ImGui::GetIO().MouseDown[0])
  {
    aPanelWidth -= ImGui::GetIO().MouseDelta.x;
    mySettings->SetReal ("main-panel", "width", aPanelWidth);
  }

  ImGui::SetNextWindowPos (ImVec2 (theScreenWidth - aPanelWidth, 0.f), ImGuiCond_Always);
  ImGui::SetNextWindowSize (ImVec2 (aPanelWidth, (float) theScreenHeight), ImGuiCond_Always);
  if (ImGui::Begin ("MainPanel", NULL, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
  {
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    ImVec2 aCursorPos = window->DC.CursorPos;
    window->DC.CursorPos = window->Pos + ImVec2 (2.f, 2.f);
    ImGui::PushClipRect (window->DrawList->GetClipRectMin() - ImVec2 (10.f, 0.f), window->DrawList->GetClipRectMax(), false);
    isDraggingPanel = Splitter (ImVec2 (2.f, theScreenHeight - 4.f));
    ImGui::PopClipRect();

    window->DC.CursorPos = aCursorPos;
    ImGui::Indent (2.f);

    auto CollapsingHeader = [&](const char* theName, bool theDefaultOpen) -> bool
    {
      bool isOpened = false;
      if (ImGui::CollapsingHeader (theName, ImGuiTreeNodeFlags_DefaultOpen * mySettings->GetBoolean ("headers", theName, theDefaultOpen)))
      {
        isOpened = true;
        mySettings->SetBoolean ("headers", theName, true);
      }
      else
      {
        mySettings->SetBoolean ("headers", theName, false);
      }

      return isOpened;
    };

    static bool AlwaysOpenLastFile = mySettings->GetBoolean ("file", "open_default", false);

    auto OpenFileDialog = [&](const char* theFileId, const char* theFiltersSeparatedByZeros) -> std::string
    {
      std::string aDefaultPath = mySettings->Get ("files", theFileId, "");

      const char* aFileName = NULL;

      if (!AlwaysOpenLastFile)
      {
        std::vector<const char*> aFilters;
        std::string aCombinedFilters ("All supported formats (");
        const char* p = theFiltersSeparatedByZeros;
        while (*p)
        {
          aFilters.push_back (p);
          aCombinedFilters += p + std::string (", ");
          p += strlen(p) + 1;
        }
        aCombinedFilters.resize (aCombinedFilters.size() - 2);
        aCombinedFilters += ")";

        aFileName = tinyfd_openFileDialog ("Select file to open", aDefaultPath.c_str(), aFilters.size(), aFilters.data(),
                                           aCombinedFilters.c_str(), 0);
      }
      else
      {
        return aDefaultPath;
      }

      if (aFileName != NULL)
      {
        mySettings->Set ("files", theFileId, aFileName);
      }

      return aFileName ? aFileName : "";
    };

    // ---------------- User panels ---------------------

    if (CollapsingHeader("Test", true))
    {
        ImGui::Button("Test button");
    }

    ImGui::End();
  }
}

//=======================================================================
//function : HandleFileDrop
//purpose  :
//=======================================================================
void AppGui::HandleFileDrop (const char * thePath)
{
  // myShowImportDialog = true;
  // static_cast<ImportSettingsEditor*> (getPanel ("ImportSettingsEditor"))->SetFileName (thePath);
}

//=======================================================================
//function : SetupImGuiStyle
//purpose  :
//=======================================================================
void AppGui::SetupImGuiStyle()
{
  ImGuiStyle& style = ImGui::GetStyle();
  style.Colors[ImGuiCol_Text]                  = ImVec4(0.93f, 0.94f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.93f, 0.94f, 0.95f, 0.58f);
  style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.24f, 0.24f, 0.24f, 1.00f);
  style.Colors[ImGuiCol_ChildWindowBg]         = ImVec4(0.13f, 0.13f, 0.13f, 0.00f);
  style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.13f, 0.13f, 0.13f, 0.82f);
  style.Colors[ImGuiCol_Border]                = ImVec4(0.39f, 0.39f, 0.39f, 0.31f);
  style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.13f, 0.13f, 0.13f, 0.00f);
  style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.47f, 0.47f, 0.47f, 1.00f);
  style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
  style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.31f, 0.31f, 0.31f, 0.75f);
  style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.39f, 0.39f, 0.39f, 0.63f);
  style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.22f, 0.54f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.20f, 0.60f, 1.00f, 0.98f);
  style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.22f, 0.54f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_Button]                = ImVec4(0.22f, 0.54f, 0.86f, 1.00f);
  style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_Header]                = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
  style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.27f, 0.60f, 0.93f, 1.00f);
  style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.35f, 0.67f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_Column]                = ImVec4(0.20f, 0.60f, 1.00f, 0.32f);
  style.Colors[ImGuiCol_ColumnHovered]         = ImVec4(0.20f, 0.60f, 1.00f, 0.78f);
  style.Colors[ImGuiCol_ColumnActive]          = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.20f, 0.60f, 1.00f, 0.00f);
  style.Colors[ImGuiCol_ResizeGripHovered]     = ImVec4(0.20f, 0.60f, 1.00f, 0.78f);
  style.Colors[ImGuiCol_ResizeGripActive]      = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_CloseButton]           = ImVec4(0.93f, 0.94f, 0.95f, 0.16f);
  style.Colors[ImGuiCol_CloseButtonHovered]    = ImVec4(0.93f, 0.94f, 0.95f, 0.39f);
  style.Colors[ImGuiCol_CloseButtonActive]     = ImVec4(0.93f, 0.94f, 0.95f, 1.00f);
  style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.93f, 0.94f, 0.95f, 0.63f);
  style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.93f, 0.94f, 0.95f, 0.63f);
  style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(0.20f, 0.60f, 1.00f, 1.00f);
  style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.20f, 0.60f, 1.00f, 0.43f);
  style.Colors[ImGuiCol_ModalWindowDarkening]  = ImVec4(0.31f, 0.31f, 0.31f, 0.73f);

  style.WindowRounding    = 0.f;
  style.GrabRounding      = std::floor (2.f * ImGui::GetIO().FontGlobalScale);
  style.FrameRounding     = std::floor (2.f * ImGui::GetIO().FontGlobalScale);
  style.ScrollbarRounding = std::floor (2.f * ImGui::GetIO().FontGlobalScale);

  style.FramePadding.x = std::floor (6.0f * ImGui::GetIO().FontGlobalScale);
  style.FramePadding.y = std::floor (3.0f * ImGui::GetIO().FontGlobalScale);
  style.ItemSpacing.x  = std::floor (8.0f * ImGui::GetIO().FontGlobalScale);
  style.ItemSpacing.y  = std::floor (4.0f * ImGui::GetIO().FontGlobalScale);
  style.ScrollbarSize  = std::floor (16.f * ImGui::GetIO().FontGlobalScale);
}

//=======================================================================
//function : IsViewBlocked
//purpose  :
//=======================================================================
bool AppGui::IsViewBlocked()
{
  return false; //ImGui::IsAnyPopupOpen();
}
