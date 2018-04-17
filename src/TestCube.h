#pragma once

#include <GL/gl3w.h>    // This example is using gl3w to access OpenGL functions (because it is small). You may use glew/glad/glLoadGen/etc. whatever already works for you.
#include <GLFW/glfw3.h>

#include "glm/glm.hpp"
#include <iostream>;
#include <vector>
#include <fstream>

class TestCube
{
public:
  TestCube ();
  ~TestCube ();

  void Create ();
  void Draw (const glm::mat4& projection_matrix, const glm::mat4& view_matrix);
  void Update ();

  std::string ReadShader (const std::string& filename);
  GLuint CreateShader (GLenum shaderType, const std::string& source, const std::string& shaderName);
  void CreateProgram (const std::string& vertexShaderFilename, const std::string& fragmentShaderFilename);

private:
  
  glm::vec3 rotation, rotation_speed;
  time_t timer;

  GLuint vao;
  GLuint program;
  std::vector<GLuint> vbos;
};

