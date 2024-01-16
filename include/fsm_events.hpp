#pragma once

#include "tinyfsm.hpp"

struct EvInputExit   : public tinyfsm::Event { };
struct EvInputUp     : public tinyfsm::Event { };
struct EvInputDown   : public tinyfsm::Event { };
struct EvInputLeft   : public tinyfsm::Event { };
struct EvInputRight  : public tinyfsm::Event { };
struct EvInputAction : public tinyfsm::Event { };
struct EvInputOption : public tinyfsm::Event { };
struct EvInputBack   : public tinyfsm::Event { };
