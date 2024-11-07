#include "ui/widgets/util.hpp"
#include "ui/widgets/sequence_list.hpp"

void UI::initCustomWidgets() {
  // Make sure GTK knows about our custom widgets by calling their dummy constructors
  static_cast<void>(UI::SequenceView());
}

