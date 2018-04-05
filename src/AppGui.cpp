#include "stdafx.h"

#include "AppGui.hxx"

#include "IReprojectionsRenderer.h"
#include "IImmersionScene.h"
#include <SimpleImage.h>

#include "DCModel.h"

#ifdef Max
  #undef Max
#endif

#ifdef Min
  #undef Min
#endif

#include <algorithm>

#include "tinyfiledialogs.h"

#include "imgui.h"
#include "imgui_user.h"

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

static ImVec4 waveLengthToRGB (int theWavelength);

//! 
class SpectralDataContainer
{
public:

  //=======================================================================
  // function : SpectralDataContainer
  // purpose  :
  //=======================================================================
  SpectralDataContainer()
  {
    int aPointCount = 20;

    for (int anIdx = 0; anIdx < aPointCount; ++anIdx)
    {
      int w = WAVE_MIN + (WAVE_MAX - WAVE_MIN) * anIdx / (aPointCount - 1);
      ImVec4 aColor = waveLengthToRGB (w);
      myRefractPoints.push_back (ImTFPoint (ImVec2 (0.f, 2.4f), aColor));
      myAbsorptPoints.push_back (ImTFPoint (ImVec2 (0.f, 0.f), aColor));
    }

    myRefractSelectedPoint = &myRefractPoints[0];
  }

  //=======================================================================
  // function : SetConstantIor
  // purpose  :
  //=======================================================================
  void SetConstantIor (float theIor)
  {
    for (int i = 0; i < myRefractPoints.size(); ++i)
    {
      myRefractPoints[i].pos.y = theIor;
    }
  }

  //=======================================================================
  // function : FetchSpectrumFromScene
  // purpose  :
  //=======================================================================
  void FetchSpectrumFromScene (IImmersionScene* theScene, int theLayer)
  {
    for (int anIdx = 0; anIdx < myRefractPoints.size(); ++anIdx)
    {
      int w = WAVE_MIN + (WAVE_MAX - WAVE_MIN) * anIdx / (myRefractPoints.size() - 1);
      double anIor = 1.0;
      double anAbsorpt = 0.0;
      theScene->GetLayerMaterial (theLayer, w, anIor, anAbsorpt);
      myRefractPoints[anIdx].pos.y = (float) anIor;
      myAbsorptPoints[anIdx].pos.y = (float) anAbsorpt;
    }
  }

  //=======================================================================
  // function : UploadSpectrumToScene
  // purpose  :
  //=======================================================================
  void UploadSpectrumToScene (IImmersionScene* theScene, int theLayer)
  {
    for (int anIdx = 0; anIdx < myRefractPoints.size(); ++anIdx)
    {
      int w = WAVE_MIN + (WAVE_MAX - WAVE_MIN) * anIdx / (myRefractPoints.size() - 1);
      theScene->SetLayerMaterial (theLayer, w,
                                  myRefractPoints[anIdx].pos.y,
                                  myAbsorptPoints[anIdx].pos.y);
    }
  }

  std::vector<ImTFPoint> myRefractPoints;
  ImTFPoint* myRefractSelectedPoint;

  std::vector<ImTFPoint> myAbsorptPoints;
  ImTFPoint* myAbsorptSelectedPoint;
};

//=======================================================================
//function : AppGui
//purpose  :
//=======================================================================
AppGui::AppGui()
  : mySpectralData (new SpectralDataContainer),
    m_pSetManager (NULL)
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

  delete mySpectralData;
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
  if (m_pSetManager->m_sets.empty()
   || !m_pSetManager->m_sets[m_currentSet].m_pScene.IsSet())
  {
    return;
  }

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
    window->DrawList->PushClipRect (window->DrawList->GetClipRectMin() - ImVec2 (10.f, 0.f), window->DrawList->GetClipRectMax());
    isDraggingPanel = Splitter (ImVec2 (2.f, theScreenHeight - 4.f));
    window->DrawList->PopClipRect();

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

    // User panels

    IImmersionScene* aScene = m_pSetManager->m_sets[m_currentSet].m_pScene.Get();
    ISpectralRenderer* aRenderer = m_pSetManager->m_pSpectralRenderer;

    if (ImGui::Checkbox ("Open default", &AlwaysOpenLastFile))
    {
      mySettings->SetBoolean ("file", "open_default", AlwaysOpenLastFile);
    }

    ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
    if (CollapsingHeader ("Rendering", true))
    {
      static bool aProgressiveMode = false;
      if (ImGui::Checkbox ("Continuous update", &aProgressiveMode))
      {
        m_toggleRepaintCallback (m_view, aProgressiveMode);
      }

      ImGui::Spacing();
      int aDepth = aRenderer->GetDepth();
      if (ImGui::SliderInt ("Depth", &aDepth, 1, 30))
      {
        aRenderer->SetDepth (aDepth);
      }

      float aMinContrib = aRenderer->GetMinContrib();
      if (ImGui::SliderFloat ("Min contrib", &aMinContrib, 1e-5f, 1e-1f, "%.5f", 2.f))
      {
        aRenderer->SetMinContrib (aMinContrib);
      }

      ImGui::Spacing();
      int aRenderMode = aRenderer->GetRenderMode();
      if (ImGui::RadioButton ("Progressive rendering", &aRenderMode, IRM_Progressive))
      {
        aRenderer->SetRenderMode ((IGOptixSpectralRenderingMode) aRenderMode);
      }

      if (ImGui::RadioButton ("Single-frame rendering", &aRenderMode, IRM_SingleFrame))
      {
        aRenderer->SetRenderMode ((IGOptixSpectralRenderingMode) aRenderMode);
      }

      float aPanoramaRadius = aRenderer->GetPanoramaRadius();
      float aPanoramaScale = aRenderer->GetPanoramaScale();

      ImGui::Spacing();
      static float aRotation[4] = {0.f, 0.f, 0.f, 0.f};
      ImGui::InputFloat4 ("Rot", aRotation);

      bool aLightRotChanged = false;

      static float aLightRotation[4] = {0.f, 0.f, 0.f, 0.f};
      aLightRotChanged |= ImGui::InputFloat4 ("Light Rot", aLightRotation);

      static glm::vec3 anEulerRot (0.f, 0.f, 0.f);
      aLightRotChanged |= ImGui::SliderAngle ("X##angle", &anEulerRot.x);
      aLightRotChanged |= ImGui::SliderAngle ("Y##angle", &anEulerRot.y);
      aLightRotChanged |= ImGui::SliderAngle ("Z##angle", &anEulerRot.z);

      SCamera& aCamera = m_pSetManager->m_sets[m_currentSet].m_loader->m_cameras[m_currentPhoto];

      static float aZoom = 9.0f;
      if (ImGui::DragFloat ("Zoom", &aZoom, 0.1f) || ImGui::Button ("Default zoom"))
      {
#define ASSIGN3(v, glmv) { v[0] = glmv.x; v[1] = glmv.y; v[2] = glmv.z; }

        //aCamera.m_xResolution = 708;
        //aCamera.m_yResolution = 627;

        aCamera.m_xResolution = 800;
        aCamera.m_yResolution = 700;

        float anAspect = aCamera.m_yResolution / (float) aCamera.m_xResolution;

        glm::vec3 anEye = glm::vec3 (0.f, 0.f, aZoom);

        glm::vec3 aForward (0.f, 0.f, -1.f);
        glm::vec3 anUp (0.f, aZoom * anAspect, 0.f);
        glm::vec3 aRight (aZoom, 0.f, 0.f);

        // apply quaternion

        glm::quat aRot (aRotation[0], aRotation[1], aRotation[2], aRotation[3]);

        aRot = glm::inverse (aRot);

        aForward = aRot * aForward;
        anUp = aRot * anUp;
        aRight = aRot * aRight;

        anEye = aRot * anEye;

        ASSIGN3 (aCamera.m_eye, anEye);

        ASSIGN3 (aCamera.m_forward, aForward);
        ASSIGN3 (aCamera.m_up, anUp);
        ASSIGN3 (aCamera.m_right, aRight);
      }

      ImGui::Spacing();
      ImGui::InputFloat ("Proj dist", &aCamera.m_projectionDistance);
      const float aFovy = glm::atan (glm::length (glm::make_vec3(aCamera.m_up)), aCamera.m_projectionDistance) / 3.141592f * 180.f;
      ImGui::Text("FOVy: %f", aFovy);

      ImGui::Spacing();
      ImGui::InputFloat3 ("Eye", aCamera.m_eye);

      ImGui::InputFloat3 ("Forward", aCamera.m_forward);
      ImGui::InputFloat3 ("Up", aCamera.m_up);
      ImGui::InputFloat3 ("Right", aCamera.m_right);

      ImGui::Spacing();
      if (ImGui::Button ("Load Camera from DC"))
      {
        std::string aFileName = OpenFileDialog ("last_loaded_dcmodel", "*.dmc\0\0");

        if (!aFileName.empty())
        {
          DCModel aModel (aFileName.c_str());

          SCamera& aCamera = m_pSetManager->m_sets[m_currentSet].m_loader->m_cameras[m_currentPhoto];
          for (int i = 0; i < 4; ++i)
          {
            aRotation[i] = aModel.Rotation[i];
            aLightRotation[i] = aModel.LightRotation[i];
          }

          aLightRotChanged = true;
        }
      }

      ImGui::Spacing ();
      if (ImGui::Button ("Load Camera from TXT"))
      {
        std::string aFileName = OpenFileDialog ("last_loaded_camera", "*.txt\0\0");

        if (!aFileName.empty())
        {
          std::ifstream file (aFileName);

          SCamera camera;

          if (file.is_open())
          {
            std::string tmp;

            file >> tmp >> camera.m_right[0] >> camera.m_right[1] >> camera.m_right[2];
            file >> tmp >> camera.m_up[0] >> camera.m_up[1] >> camera.m_up[2];
            file >> tmp >> camera.m_forward[0] >> camera.m_forward[1] >> camera.m_forward[2];
            file >> tmp >> camera.m_eye[0] >> camera.m_eye[1] >> camera.m_eye[2];

            file >> tmp >> camera.m_xResolution >> camera.m_yResolution;

            file >> tmp >> camera.m_projectionDistance;
            file >> tmp >> camera.m_aperture;

            file.close();
          }

          m_pSetManager->m_sets[m_currentSet].m_loader->m_cameras[m_currentPhoto] = camera;

          aLightRotChanged = true;
        }
      }

      ImGui::Spacing ();
      if (ImGui::Button ("Save Camera to TXT"))
      {
        std::string aDefaultPath = mySettings->Get ("files", "last_saved_camera", "");

        const char* aFilters[] = { "*.txt" };

        const char* aFileName = tinyfd_saveFileDialog ("Select file to save", aDefaultPath.c_str(), 1, aFilters,
                                                       "All supported formats (*.txt)");

        if (aFileName != NULL)
        {
          mySettings->Set ("files", "last_saved_camera", aFileName);

          std::ofstream file (aFileName);

          if (file.is_open())
          {
            SCamera& camera = m_pSetManager->m_sets[m_currentSet].m_loader->m_cameras[m_currentPhoto];

            file << "right "   << camera.m_right[0] << " " << camera.m_right[1] << " " << camera.m_right[2] << "\n";
            file << "up "      << camera.m_up[0] << " " << camera.m_up[1] << " " << camera.m_up[2] << "\n";
            file << "forward " << camera.m_forward[0] << " " << camera.m_forward[1] << " " << camera.m_forward[2] << "\n";
            file << "eye "     << camera.m_eye[0] << " " << camera.m_eye[1] << " " << camera.m_eye[2] << "\n";

            file << "image "     << camera.m_xResolution << " " << camera.m_yResolution << "\n";

            file << "projection_distance " << camera.m_projectionDistance << "\n";
            file << "aperture " << camera.m_aperture << "\n";

            file.close();
          }
        }
      }

      static float aNewRotation[4];

      if (aLightRotChanged)
      {
        glm::quat aRot (aRotation[0], aRotation[1], aRotation[2], aRotation[3]);
        glm::quat aLightRot (aLightRotation[0], aLightRotation[1], aLightRotation[2], aLightRotation[3]);

        glm::quat aEulerRot (anEulerRot);
        aLightRot = aLightRot * aEulerRot;

        aNewRotation[0] = aLightRot.w;
        aNewRotation[1] = aLightRot.x;
        aNewRotation[2] = aLightRot.y;
        aNewRotation[3] = aLightRot.z;

        aRenderer->SetLightRotation (aNewRotation);
      }

      ImGui::Text ("%f %f %f %f", aNewRotation[0], aNewRotation[1], aNewRotation[2], aNewRotation[3]);

      ImGui::Spacing();
      if (ImGui::Button ("Save HDR image"))
      {
        std::string aDefaultPath = mySettings->Get ("files", "last_saved_image", "");

        const char* aFilters[] = { "*.hdr", "*.exr" };

        const char* aFileName = tinyfd_saveFileDialog ("Select file to save", aDefaultPath.c_str(), 2, aFilters,
                                                       "All supported formats (*.hdr, *.exr)");

        if (aFileName != NULL)
        {
          mySettings->Set ("files", "last_saved_image", aFileName);

          IImageWrapper* anImagePtr = NULL;
          SCamera aCamera = m_pSetManager->m_sets[m_currentSet].m_loader->m_cameras[m_currentPhoto];

          aRenderer->GetHdrImage (&anImagePtr, ICS_RGB, aCamera);
          if (anImagePtr != NULL)
          {
            CSimpleImage* anImage = static_cast<CSimpleImage*> (anImagePtr);

            anImage->SaveFloatToHdrImage (aFileName);
          }
        }
      }

      ImGui::SameLine();
      if (ImGui::Button ("Save LDR image"))
      {
        std::string aDefaultPath = mySettings->Get ("files", "last_saved_image", "");

        const char* aFilters[] = { "*.png" };

        const char* aFileName = tinyfd_saveFileDialog ("Select file to save", aDefaultPath.c_str(), 1, aFilters,
                                                       "All supported formats (*.png)");

        if (aFileName != NULL)
        {
          mySettings->Set ("files", "last_saved_image", aFileName);

          IImageWrapper* anImagePtr = NULL;
          SCamera aCamera = m_pSetManager->m_sets[m_currentSet].m_loader->m_cameras[m_currentPhoto];

          aRenderer->Render (&anImagePtr, aCamera);
          if (anImagePtr != NULL)
          {
            CSimpleImage* anImage = static_cast<CSimpleImage*> (anImagePtr);

            anImage->SaveToImage (aFileName);
          }
        }
      }

      ImGui::Spacing();
      if (ImGui::Button ("Load RGB panorama"))
      {
        std::string aFileName = OpenFileDialog ("last_panorama", "*.hdr\0*.exr\0\0");

        if (!aFileName.empty ())
        {
          CTexture m_pPanorama;
          bool isLoaded = S_OK == LoadFloatTextureFromImage (m_pPanorama, aFileName);

          if (isLoaded)
          {
            CSimpleImage panorama;
            FloatTextureToSimpleImage (&m_pPanorama, &panorama);

            aRenderer->SetPanorama (&panorama);
          }
        }
      }

      ImGui::Spacing();
      if (ImGui::Button ("Load Mono panorama"))
      {
        std::string aFileName = OpenFileDialog ("last_panorama", "*.hdr\0*.exr\0\0");

        if (!aFileName.empty())
        {
          CTexture m_pPanorama;
          bool isLoaded = S_OK == LoadFloatTextureFromImage (m_pPanorama, aFileName);

          if (isLoaded)
          {
            CSimpleImage panorama;
            FloatTextureToSimpleImage (&m_pPanorama, &panorama);

            aRenderer->SetMonoPanorama (&panorama);
          }
        }
      }
    }

    ImGui::Spacing();
    if (CollapsingHeader ("Scene", true))
    {
      ImGui::Text ("Layers:");

      static int aCurrentLayer = 0;
      ImGui::PushItemWidth (ImGui::GetContentRegionAvailWidth());
      ImGui::ListBox ("##SceneLayerList", &aCurrentLayer, LayerItemGetter, NULL, aScene->GetLayerCount());
      ImGui::PopItemWidth();

      if (aCurrentLayer < aScene->GetLayerCount())
      {
        if (ImGui::Button ("Remove"))
        {
          aScene->DeleteLayer (aCurrentLayer);
        }

        ImGui::SameLine();
        if (ImGui::Button ("Save"))
        {
          std::string aDefaultPath = mySettings->Get ("files", "last_saved_layer", "");

          const char* aFilters[] = { "*.obj" };

          const char* aFileName = tinyfd_saveFileDialog ("Select file to save", aDefaultPath.c_str(), 1, aFilters,
                                                         "All supported formats (*.obj)");

          if (aFileName != NULL)
          {
            mySettings->Set ("files", "last_saved_layer", aFileName);

            int aNumVertices = 0;
            int aNumFaces = 0;

            // Retrieve aNumVertices and aNumIndices
            aScene->GetLayer (aCurrentLayer, NULL, aNumVertices, NULL, aNumFaces);

            std::vector<double> aVertices (aNumVertices * 3);
            std::vector<int> anIndices (aNumFaces * 3);

            // Retrieve data
            aScene->GetLayer (aCurrentLayer, aVertices.data(), aNumVertices, anIndices.data(), aNumFaces);

            std::ofstream aFile (aFileName);
            if (aFile.is_open())
            {
              for (int i = 0; i < aVertices.size(); i += 3)
              {
                aFile << "v " << aVertices[i + 0] << " "
                              << aVertices[i + 1] << " "
                              << aVertices[i + 2] << std::endl;
              }

              for (int i = 0; i < anIndices.size(); i += 3)
              {
                aFile << "f " << (anIndices[i + 0] + 1) << " "
                              << (anIndices[i + 1] + 1) << " "
                              << (anIndices[i + 2] + 1) << std::endl;
              }

              aFile.close();
            }
          }
        }

        ImGui::SameLine();
      }

      if (ImGui::Button ("Add"))
      {
        std::string aFileName = OpenFileDialog ("last_added_layer", "*.obj\0\0");

        if (!aFileName.empty())
        {
          tinyobj::attrib_t attrib;
          std::vector<tinyobj::shape_t> shapes;
          std::vector<tinyobj::material_t> materials;

          std::string err;
          bool ret = tinyobj::LoadObj (&attrib, &shapes, &materials, &err, aFileName.c_str());

          if (ret)
          {
            std::vector<int> anIndices (shapes[0].mesh.indices.size());
            for (int i = 0; i < anIndices.size(); ++i)
            {
              anIndices[i] = shapes[0].mesh.indices[i].vertex_index;
            }

            aScene->AddLayer (aScene->GetLayerCount(), attrib.vertices.data(), attrib.vertices.size() / 3, anIndices.data(), anIndices.size() / 3);
          }
        }
      }

      ImGui::Spacing();
      if (aCurrentLayer < aScene->GetLayerCount()
          && ImGui::CollapsingHeader ("Material", NULL, true, true))
      {
        double anIor, anAbsorpt;
        aScene->GetLayerMaterial (aCurrentLayer, anIor, anAbsorpt);
        float r = anIor, a = anAbsorpt;

        ImGui::Spacing ();
        const char* listbox_items[] = { "Standard", "Accurate", "Highest" };
        static int listbox_item_current = IRA_Standard;

        if (ImGui::ListBox ("Spectrum accuracy", &listbox_item_current, listbox_items, IM_ARRAYSIZE (listbox_items), 3))
        {
          aRenderer->SetAccuracy (IGOptixRenderAccuracy (listbox_item_current));
        }

        ImGui::Spacing ();
        const char* listbox_items1[] = { "Diamond", "Mirror ball" };
        static int listbox_item_current1 = 0;

        if (ImGui::ListBox ("Object to render", &listbox_item_current1, listbox_items1, IM_ARRAYSIZE (listbox_items1), 2))
        {
          aRenderer->SetMirrorBallMode (listbox_item_current1 == 0 ? false : true);
        }

        ImGui::Spacing ();
        static bool aLinearMode = false;

        if (ImGui::Checkbox ("Linear sensor response", &aLinearMode))
        {
          aRenderer->SetLinearResponse (aLinearMode);
        }

        ImGui::Spacing();
        if (ImGui::Button ("Load Refraction"))
        {
          std::string aFileName = OpenFileDialog ("last_loaded_refract", "*.txt\0\0");

          if (!aFileName.empty())
          {
            std::ifstream aFile (aFileName);
            if (aFile.is_open())
            {
              int aSampleCounter = 0;
              while (!aFile.eof())
              {
                float aWavelength = 0.f;
                aFile >> aWavelength;

                float aValue;
                aFile >> aValue;
                aScene->SetLayerMaterial (aCurrentLayer, (int) aWavelength,
                                          aValue,
                                         -1.0);
              }
              aFile.close();
            }
          }
        }

        ImGui::SameLine();
        if (ImGui::Button ("Load Absorption"))
        {
          std::string aFileName = OpenFileDialog ("last_loaded_absorpt", "*.txt\0\0");

          if (!aFileName.empty())
          {
            std::ifstream aFile (aFileName);
            if (aFile.is_open())
            {
              int aSampleCounter = 0;
              while (!aFile.eof())
              {
                float aWavelength = 0.f;
                aFile >> aWavelength;

                if (aWavelength < FLT_EPSILON)
                {
                  break;
                }

                float aValue;
                aFile >> aValue;
                aScene->SetLayerMaterial (aCurrentLayer, (int) aWavelength,
                                         -1.0,
                                          aValue);
              }
              aFile.close();
            }
          }
        }

        ImGui::Spacing();

        float aLensRadius = aRenderer->GetApertureRadius();
        float aFocalDist = aRenderer->GetFocalDistance();

        if (ImGui::SliderFloat ("Radius", &aLensRadius, 0.f, 10.f))
        {
          aRenderer->SetApertureRadius (aLensRadius);
        }
        if (ImGui::SliderFloat ("Distance", &aFocalDist, 0.f, 200.f))
        {
          aRenderer->SetFocalDistance (aFocalDist);
        }

        float aPanoramaRadius = aRenderer->GetPanoramaRadius ();
        float aPanoramaFactor = aRenderer->GetPanoramaScale ();

        if (ImGui::SliderFloat ("Panorama radius", &aPanoramaRadius, 1.f, 5000.f))
        {
          aRenderer->SetPanoramaRadius (aPanoramaRadius);
        }
        if (ImGui::SliderFloat ("Panorama scaling", &aPanoramaFactor, 0.01f, 5.f))
        {
          aRenderer->SetPanoramaScale (aPanoramaFactor);
        }
      }
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
  style.Colors[ImGuiCol_ComboBg]               = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
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

//=======================================================================
// function : waveLengthToRGB
// purpose  :
//=======================================================================
static ImVec4 waveLengthToRGB (int theWavelength)
{
  float factor;
  float Red, Green, Blue;

  if ( (theWavelength >= 380) && (theWavelength < 440))
  {
    Red = - (theWavelength - 440) / (440 - 380);
    Green = 0.0f;
    Blue = 1.0f;
  }
  else if ( (theWavelength >= 440) && (theWavelength < 490))
  {
    Red = 0.0f;
    Green = (theWavelength - 440) / (490 - 440);
    Blue = 1.0f;
  }
  else if ( (theWavelength >= 490) && (theWavelength < 510))
  {
    Red = 0.0f;
    Green = 1.0;
    Blue = - (theWavelength - 510) / (510 - 490);
  }
  else if ( (theWavelength >= 510) && (theWavelength < 580))
  {
    Red = (theWavelength - 510) / (580 - 510);
    Green = 1.0f;
    Blue = 0.0f;
  }
  else if ( (theWavelength >= 580) && (theWavelength < 645))
  {
    Red = 1.0f;
    Green = - (theWavelength - 645) / (645 - 580);
    Blue = 0.0f;
  }
  else if ( (theWavelength >= 645) && (theWavelength < 781))
  {
    Red = 1.0f;
    Green = 0.0f;
    Blue = 0.0f;
  }
  else
  {
    Red = 0.0f;
    Green = 0.0f;
    Blue = 0.0f;
  };

  // Let the intensity fall off near the vision limits

  if ( (theWavelength >= 380) && (theWavelength < 420))
  {
    factor = 0.3f + 0.7f * (theWavelength - 380) / (420 - 380);
  }
  else if ( (theWavelength >= 420) && (theWavelength < 701))
  {
    factor = 1.0f;
  }
  else if ( (theWavelength >= 701) && (theWavelength < 781))
  {
    factor = 0.3f + 0.7f * (780 - theWavelength) / (780 - 700);
  }
  else
  {
    factor = 0.0f;
  };

  const float Gamma = 0.80f;

  ImVec4 anRgb;

  // Don't want 0^x = 1 for x <> 0
  anRgb.x = Red == 0.0f ? 0.0f : powf (Red * factor, Gamma);

  anRgb.y = Green == 0.0f ? 0.0f : powf (Green * factor, Gamma);

  anRgb.z = Blue == 0.0f ? 0.0f : powf (Blue * factor, Gamma);

  anRgb.w = 1.0f;

  return anRgb;
}