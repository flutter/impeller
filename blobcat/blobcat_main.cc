// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <filesystem>
#include <iostream>

#include "flutter/fml/command_line.h"
#include "impeller/blobcat/blob_writer.h"

namespace impeller {

bool Main(const fml::CommandLine& command_line) {
  BlobWriter writer;

  std::string output;
  if (!command_line.GetOptionValue("output", &output)) {
    std::cerr << "Output path not specified." << std::endl;
    return false;
  }

  for (const auto& input : command_line.GetOptionValues("input")) {
    if (!writer.AddBlobAtPath(std::string{input})) {
      std::cerr << "Could not add blob at path: " << input << std::endl;
      return false;
    }
  }

  auto blob = writer.CreateMapping();
  if (!blob) {
    std::cerr << "Could not create combined shader blob." << std::endl;
    return false;
  }

  auto current_directory =
      fml::OpenDirectory(std::filesystem::current_path().native().c_str(),
                         false, fml::FilePermission::kReadWrite);
  if (!fml::WriteAtomically(current_directory, output.c_str(), *blob)) {
    std::cerr << "Could not write shader blob to path " << output << std::endl;
    return false;
  }

  return true;
}

}  // namespace impeller

int main(int argc, char const* argv[]) {
  return impeller::Main(fml::CommandLineFromArgcArgv(argc, argv))
             ? EXIT_SUCCESS
             : EXIT_FAILURE;
}
