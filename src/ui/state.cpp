#include "ui/state.hpp"

#include <memory>
#include <utility>
#include <fstream>
#include <spdlog/spdlog.h>

using namespace UI;
using namespace Img;

State::State(const std::filesystem::path& sequenceFilePath, const std::shared_ptr<Sequence>& sequence, Fits&& image)
  : m_sequenceFilePath(sequenceFilePath)
  , m_sequence(sequence)
  , m_imageFile(std::move(image)) {

}

std::shared_ptr<State> State::fromSequenceFile(const std::filesystem::path& sequence_path) {
  auto sequence = Sequence::readSequence(sequence_path);
  if(!sequence)
    return nullptr;

  std::filesystem::path fits_path(sequence_path);
  fits_path.replace_extension("fit");
  Fits fits(fits_path.string());

  return std::make_shared<State>(sequence_path, sequence, std::move(fits));
}

void State::saveSequence() {
  std::ofstream stream(m_sequenceFilePath, std::ios::out | std::ios::trunc);
  if(!stream.is_open()) {
    spdlog::error("Failed to open stream for sequence save");
    return;
  }
  m_sequence->writeStream(stream);
}

