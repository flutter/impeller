// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "impeller/compiler/types.h"

#include <filesystem>
#include <sstream>

#include "flutter/fml/logging.h"

namespace impeller {
namespace compiler {

static bool StringEndWith(const std::string& string,
                          const std::string& suffix) {
  if (suffix.size() > string.size()) {
    return false;
  }

  if (suffix.empty() || suffix.empty()) {
    return false;
  }

  return string.rfind(suffix) == (string.size() - suffix.size());
}

SourceType SourceTypeFromFileName(const std::string& file_name) {
  if (StringEndWith(file_name, ".vert")) {
    return SourceType::kVertexShader;
  }

  if (StringEndWith(file_name, ".frag")) {
    return SourceType::kFragmentShader;
  }

  return SourceType::kUnknown;
}

std::string TargetPlatformToString(TargetPlatform platform) {
  switch (platform) {
    case TargetPlatform::kUnknown:
      return "Unknown";
    case TargetPlatform::kMetalDesktop:
      return "MetalDesktop";
    case TargetPlatform::kMetalIOS:
      return "MetaliOS";
    case TargetPlatform::kFlutterSPIRV:
      return "FlutterSPIRV";
    case TargetPlatform::kOpenGLES:
      return "OpenGLES";
    case TargetPlatform::kOpenGLDesktop:
      return "OpenGLDesktop";
  }
  FML_UNREACHABLE();
}

static std::string UniqueEntryPointFunctionNameFromSourceName(
    const std::string& file_name,
    SourceType type) {
  std::stringstream stream;
  std::filesystem::path file_path(file_name);
  stream << ToUtf8(file_path.stem().native()) << "_";
  switch (type) {
    case SourceType::kUnknown:
      stream << "unknown";
      break;
    case SourceType::kVertexShader:
      stream << "vertex";
      break;
    case SourceType::kFragmentShader:
      stream << "fragment";
      break;
  }
  stream << "_main";
  return stream.str();
}

std::string EntryPointFunctionNameFromSourceName(const std::string& file_name,
                                                 SourceType type,
                                                 TargetPlatform platform) {
  switch (platform) {
    case TargetPlatform::kMetalDesktop:
    case TargetPlatform::kMetalIOS:
      return UniqueEntryPointFunctionNameFromSourceName(file_name, type);
    case TargetPlatform::kUnknown:
    case TargetPlatform::kFlutterSPIRV:
    case TargetPlatform::kOpenGLES:
    case TargetPlatform::kOpenGLDesktop:
      return "main";
  }
  FML_UNREACHABLE();
}

bool TargetPlatformNeedsSL(TargetPlatform platform) {
  switch (platform) {
    case TargetPlatform::kMetalIOS:
    case TargetPlatform::kMetalDesktop:
    case TargetPlatform::kOpenGLES:
    case TargetPlatform::kOpenGLDesktop:
      return true;
    case TargetPlatform::kUnknown:
    case TargetPlatform::kFlutterSPIRV:
      return false;
  }
  FML_UNREACHABLE();
}

bool TargetPlatformNeedsReflection(TargetPlatform platform) {
  switch (platform) {
    case TargetPlatform::kMetalIOS:
    case TargetPlatform::kMetalDesktop:
    case TargetPlatform::kOpenGLES:
    case TargetPlatform::kOpenGLDesktop:
      return true;
    case TargetPlatform::kUnknown:
    case TargetPlatform::kFlutterSPIRV:
      return false;
  }
  FML_UNREACHABLE();
}

std::string ShaderCErrorToString(shaderc_compilation_status status) {
  using Status = shaderc_compilation_status;
  switch (status) {
    case Status::shaderc_compilation_status_success:
      return "Success";
    case Status::shaderc_compilation_status_invalid_stage:
      return "Invalid shader stage specified";
    case Status::shaderc_compilation_status_compilation_error:
      return "Compilation error";
    case Status::shaderc_compilation_status_internal_error:
      return "Internal error";
    case Status::shaderc_compilation_status_null_result_object:
      return "Internal error. Null result object";
    case Status::shaderc_compilation_status_invalid_assembly:
      return "Invalid assembly";
    case Status::shaderc_compilation_status_validation_error:
      return "Validation error";
    case Status::shaderc_compilation_status_transformation_error:
      return "Transformation error";
    case Status::shaderc_compilation_status_configuration_error:
      return "Configuration error";
  }
  return "Unknown internal error";
}

shaderc_shader_kind ToShaderCShaderKind(SourceType type) {
  switch (type) {
    case SourceType::kVertexShader:
      return shaderc_shader_kind::shaderc_vertex_shader;
    case SourceType::kFragmentShader:
      return shaderc_shader_kind::shaderc_fragment_shader;
    case SourceType::kUnknown:
      break;
  }
  return shaderc_shader_kind::shaderc_glsl_infer_from_source;
}

spv::ExecutionModel ToExecutionModel(SourceType type) {
  switch (type) {
    case SourceType::kVertexShader:
      return spv::ExecutionModel::ExecutionModelVertex;
    case SourceType::kFragmentShader:
      return spv::ExecutionModel::ExecutionModelFragment;
    case SourceType::kUnknown:
      break;
  }
  return spv::ExecutionModel::ExecutionModelMax;
}

spirv_cross::CompilerMSL::Options::Platform TargetPlatformToMSLPlatform(
    TargetPlatform platform) {
  switch (platform) {
    case TargetPlatform::kMetalIOS:
      return spirv_cross::CompilerMSL::Options::Platform::iOS;
    case TargetPlatform::kMetalDesktop:
      return spirv_cross::CompilerMSL::Options::Platform::macOS;
    case TargetPlatform::kFlutterSPIRV:
    case TargetPlatform::kOpenGLES:
    case TargetPlatform::kOpenGLDesktop:
    case TargetPlatform::kUnknown:
      return spirv_cross::CompilerMSL::Options::Platform::macOS;
  }
  FML_UNREACHABLE();
}

std::string SourceTypeToString(SourceType type) {
  switch (type) {
    case SourceType::kUnknown:
      return "unknown";
    case SourceType::kVertexShader:
      return "vert";
    case SourceType::kFragmentShader:
      return "frag";
  }
  FML_UNREACHABLE();
}

std::string TargetPlatformSLExtension(TargetPlatform platform) {
  switch (platform) {
    case TargetPlatform::kUnknown:
      return "unknown";
    case TargetPlatform::kMetalDesktop:
    case TargetPlatform::kMetalIOS:
      return "metal";
    case TargetPlatform::kFlutterSPIRV:
    case TargetPlatform::kOpenGLES:
    case TargetPlatform::kOpenGLDesktop:
      return "glsl";
  }
  FML_UNREACHABLE();
}

std::string ToUtf8(const std::wstring& wstring) {
  std::wstring_convert<std::codecvt_utf8<wchar_t>> myconv;
  return myconv.to_bytes(wstring);
}

std::string ToUtf8(const std::string& string) {
  return string;
}

}  // namespace compiler
}  // namespace impeller
