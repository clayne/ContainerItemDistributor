#include "Settings.h"

#include "Parser.h"

void Settings::LoadSettings() noexcept
{
    logger::info("Loading settings");

    CSimpleIniA ini;

    ini.SetUnicode();
    ini.SetMultiKey();
    ini.LoadFile(R"(.\Data\SKSE\Plugins\ContainerItemDistributor.ini)");

    debug_logging = ini.GetBoolValue("Log", "Debug");

    if (debug_logging) {
        spdlog::set_level(spdlog::level::debug);
        logger::debug("Debug logging enabled");
    }

    logger::info("Loaded settings");
    logger::info("");

    Parser::ParseINIs(ini);
}
