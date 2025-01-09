#pragma once

#include <filesystem>
#include <nfd.h>
#include <optional>

class Nfd {
public:
  struct Filter {
    const char *name;
    const char *spec;
  };

  Nfd();
  ~Nfd();

  std::optional<std::filesystem::path>
  open_dialog(nfdu8filteritem_t *filters, int count);

private:
  void init();

  bool init_;
};