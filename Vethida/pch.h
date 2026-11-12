#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <DbgHelp.h>
#include <dwmapi.h>

#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <functional>
#include <filesystem>
#include <chrono>

#include <QApplication>
#include <QMainWindow>
#include <QDialog>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTranslator>
#include <QLibraryInfo>
#include <QLoggingCategory>
