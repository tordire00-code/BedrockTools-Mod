#include "ModuleRegistry.hpp"
#include "visual/zoom.hpp"
#include "zaphkiel/zaphkiel.hpp"
void RegisterModules(){
    ModuleRegistry::get().registerModule<ZoomModule>();
    ModuleRegistry::get().registerModule<ZaphkielModule>();
}
