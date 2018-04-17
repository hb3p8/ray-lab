// ImGui - standalone example application for GLFW + OpenGL 3, using programmable pipeline
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
// (GLFW is a cross-platform general purpose library for handling windows, inputs, OpenGL/Vulkan graphics context creation, etc.)
// (GL3W is a helper library to access OpenGL functions since there is no standard header to access modern OpenGL functions easily. Alternatives are GLEW, Glad, etc.)

#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"
#include <stdio.h>
#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>

#include "AppGui.hpp"

#include "TestCube.h"

#include "Camera.h"

//Create the Camera
Camera camera;

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}


// Callback function to handle keypresses
void handleKeypress (GLFWwindow* window, int theKey, int theSancode, int theAction, int theBitField)
{
  std::cout << theKey << ", " << theSancode<< ", " << theAction << ", " << theBitField << std::endl;

  switch (theKey) {
  case 87:
    camera.Move (FORWARD);
    break;
  case 65:
    camera.Move (LEFT);
    break;
  case 83:
    camera.Move (BACK);
    break;
  case 68:
    camera.Move (RIGHT);
    break;
  case 81:
    camera.Move (DOWN);
    break;
  case 69:
    camera.Move (UP);
    break;
  case 'x':
  case 27:
    exit (0);
    return;
  default:
    break;
  }

}

// Callback function to handle mouse movements
void handleMouseMove (GLFWwindow* window, double theX, double theY)
{
  //std::cout << theX << ", " << theY << std::endl;
  camera.Move2D (theX, theY);
}

// Callback function to handle mouse keypress
void handleMouseKeypress (GLFWwindow* window, int theButton, int theAction, int theBitfield)
{
  std::cout << theButton << ", " << theAction << ", " << theBitfield << std::endl;

  if (theButton == 0 && theAction == 1)
  {
    camera.move_camera = true;

  }
  else if (theButton == 0 && theAction == 0)
  {
    camera.move_camera = false;
  }
}

int main(int, char**)
{
    // Setup window
    glfwSetErrorCallback(error_callback);
    if (!glfwInit())
        return 1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(1280, 720, "ImGui OpenGL3 example", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    gl3wInit();

    // Setup ImGui binding
    ImGui_ImplGlfwGL3_Init(window, true);

    // Setup style
    ImGui::StyleColorsClassic();
    //ImGui::StyleColorsDark();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'extra_fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //ImGuiIO& io = ImGui::GetIO();
    //io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../extra_fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    bool show_demo_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    AppGui gui;

    // Create test cube

    TestCube* cube = new TestCube();

    cube->CreateProgram ("Cube_Vert.glsl", "Cube_Frag.glsl");
    cube->Create ();

    // Specify the function which should execute when a key is pressed or released
    glfwSetKeyCallback (window, handleKeypress);

    glfwSetMouseButtonCallback (window, handleMouseKeypress);
    
    glfwSetCursorPosCallback (window, handleMouseMove);

    //Setup camera
    camera.SetMode (FREE);
    camera.SetPosition (glm::vec3 (0, 0, -1));
    camera.SetLookAt (glm::vec3 (0, 0, 0));
    camera.SetClipping (.1, 1000);
    camera.SetFOV (45);


    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();
        ImGui_ImplGlfwGL3_NewFrame();

        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
        {
            ImGui::Begin("Stats", nullptr, ImVec2(200, 100), 0.2f, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings);
            if (ImGui::Button("Demo Window"))                       // Use buttons to toggle our bools. We could use Checkbox() as well.
                show_demo_window ^= 1;

            ImGui::Spacing();
            ImGui::Text("%.3f ms/frame\n%.1f FPS", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
            ImGui::End();
        }

        if (show_demo_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);

        glm::mat4 model, view, projection;
        camera.SetViewport (0, 0, display_w, display_h);
        camera.Update();
        camera.GetMatricies (projection, view, model);

        cube->Draw (projection, view);

        //

        gui.Draw(display_w, display_h);

        ImGui::Render();
        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplGlfwGL3_Shutdown();
    glfwTerminate();

    delete cube;

    return 0;
}
