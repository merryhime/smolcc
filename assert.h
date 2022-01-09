// This file is part of the smolcc project
// SPDX-License-Identifier: MIT

#pragma once

#include <exception>

#include <fmt/core.h>

#define ASSERT(x) [&] { if (!(x)) { fmt::print("failed assert {}\n", #x); std::terminate(); } }()
