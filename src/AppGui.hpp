#pragma once

#include <memory>

#include "Settings.hpp"

//! Application GUI wrapper
class AppGui
{
public:

  AppGui();

  ~AppGui();

  void Draw (int theScreenWidth, int theScreenHeight);

  void SetupImGuiStyle();

  bool IsViewBlocked ();

  void HandleFileDrop (const char* thePath);

private:

  std::unique_ptr<Settings> mySettings;

};
