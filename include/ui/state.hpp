#pragma once

#include <filesystem>

#include "io/sequence.hpp"
#include "io/fits.hpp"

namespace UI {
class Window;

class State {
  std::filesystem::path m_sequenceFilePath;

public:
  std::shared_ptr<IO::Sequence> m_sequence;
  IO::Fits m_imageFile;

  State(const std::filesystem::path& sequenceFilePath, const std::shared_ptr<IO::Sequence>& sequence, IO::Fits&& image);
  ~State() = default;

  void saveSequence();

  static std::shared_ptr<State> fromSequenceFile(const std::filesystem::path& sequence_path);
};

} // namespace UI

