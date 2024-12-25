#include <cstring>
#include <format>
#include <nfd.h>
#include <stdexcept>

#include "src/app/nfd.h"

Nfd::Nfd() : init_(false) {}

Nfd::~Nfd() {
  if (init_) {
    NFD_Quit();
  }
}

void Nfd::init() {
  if (init_) {
    return;
  }
  auto result = NFD_Init();
  if (result == NFD_OKAY) {
    init_ = true;
  } else {
    throw std::runtime_error(
        std::format("failed to initialize NFD: {}", NFD_GetError())
    );
  }
}

std::optional<std::string>
Nfd::open_dialog(nfdu8filteritem_t *filters, int count) {
  init();

  nfdu8char_t          *out_path;
  nfdopendialogu8args_t args;
  std::memset(&args, 0, sizeof(args));
  args.filterList    = filters;
  args.filterCount   = count;
  nfdresult_t result = NFD_OpenDialogU8_With(&out_path, &args);
  if (result == NFD_OKAY) {
    std::optional<std::string> result = out_path;
    NFD_FreePathU8(out_path);
    return result;
  } else if (result == NFD_CANCEL) {
    return std::nullopt;
  } else {
    throw std::runtime_error(std::format("NFD error: {}", NFD_GetError()));
  }
}
