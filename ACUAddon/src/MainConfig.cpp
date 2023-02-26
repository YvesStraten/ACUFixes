#include "pch.h"

#include "MainConfig.h"
#include "SimpleJSON/json.hpp"
#include "Serialization/Serialization.h"
#include "Serialization/BooleanAdapter.h"
#include "Serialization/File/JSONFileBridge.h"

#include "base.h"


namespace MainConfig {

bool showSuccessfulInjectionIndicator = false;
bool showDebugOutputConsole = false;

} // namespace MainConfig


using json::JSON;
fs::path& GetThisDLLAbsolutePath()
{
    static fs::path dllPath = []()
    {
        char path[MAX_PATH];
        GetModuleFileName(Base::Data::thisDLLModule, path, (DWORD)std::size(path));
        return fs::path(path);
    }();
    return dllPath;
}
fs::path AbsolutePathInMyDirectory(const fs::path& filenameRel)
{
    fs::path& dll = GetThisDLLAbsolutePath();
    fs::path fullPath = dll.parent_path() / filenameRel;
    return fullPath;
}

void DumpConfig(JSON& cfg)
{
    using namespace MainConfig;
    WRITE_JSON_VARIABLE(cfg, showSuccessfulInjectionIndicator, BooleanAdapter);
    WRITE_JSON_VARIABLE(cfg, showDebugOutputConsole, BooleanAdapter);
}
void dprintf(const std::string& s);
void MainConfig::FindAndLoadConfigFileOrCreateDefault()
{
    std::string configFileName = "acufixes-config.json";
    JSONFileBridge configFile(AbsolutePathInMyDirectory(configFileName), true);
    JSON& cfg = configFile.m_jsonObject;
    if (cfg.IsObject())
    {
        READ_JSON_VARIABLE(cfg, showSuccessfulInjectionIndicator, BooleanAdapter);
        READ_JSON_VARIABLE(cfg, showDebugOutputConsole, BooleanAdapter);
    }
    DumpConfig(cfg);
}
