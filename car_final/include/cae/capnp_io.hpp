#pragma once
#include "cae/db.hpp"

#include <string>

namespace cae {

bool save_capnp(const BoardDb& db, const std::string& path);
bool load_capnp(BoardDb& db, const std::string& path);

}  // namespace cae
