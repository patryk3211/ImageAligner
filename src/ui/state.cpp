#include "ui/state.hpp"
#include <iostream>
#include <memory>
#include <utility>

using namespace UI;
using namespace Img;

State::State(const std::shared_ptr<Sequence>& sequence, Fits&& image)
  : m_sequence(sequence)
  , m_imageFile(std::move(image)) {

}

std::shared_ptr<State> State::fromSequenceFile(const std::filesystem::path& sequence_path) {
  auto sequence = Sequence::readSequence(sequence_path);
  if(!sequence)
    return nullptr;

  std::filesystem::path fits_path(sequence_path);
  fits_path.replace_extension("fit");
  Fits fits(fits_path.string());

  return std::make_shared<State>(sequence, std::move(fits));
}
