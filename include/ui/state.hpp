#pragma once

#include <filesystem>

#include "img/sequence.hpp"
#include "img/fits.hpp"

namespace UI {
class Window;

class State {
  std::filesystem::path m_sequenceFilePath;

public:
  std::shared_ptr<Img::Sequence> m_sequence;
  Img::Fits m_imageFile;

  State(const std::filesystem::path& sequenceFilePath, const std::shared_ptr<Img::Sequence>& sequence, Img::Fits&& image);
  ~State() = default;

  void saveSequence();

  static std::shared_ptr<State> fromSequenceFile(const std::filesystem::path& sequence_path);
};

} // namespace UI

