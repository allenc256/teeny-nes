#pragma once

#include <nfd.h>
#include <optional>
#include <string>

class Nfd {
public:
  struct Filter {
    const char *name;
    const char *spec;
  };

  Nfd();
  ~Nfd();

  std::optional<std::string> open_dialog(nfdu8filteritem_t *filters, int count);

private:
  void init();

  bool init_;
};