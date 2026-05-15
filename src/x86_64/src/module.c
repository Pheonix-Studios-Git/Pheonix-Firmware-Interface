#include <stdint.h>

#include <core.h>
#include <builtins.h>
#include <serial.h>
#include <avmf.h>
#include <pager.h>

#include <module.h>

#define AOS_MODULE_SIGN(id) ((0x414F53ULL << 32) | ((uint64_t)(id) & 0xFFFFFFFFULL))
#define AOS_MODULE_PACK_VERSION(major, minor, patch) ((((major) & 0xFF) << 24) | (((minor) & 0xFF) << 16) | ((patch) & 0xFFFF))

static struct AOS_Module* modules = NULL;
static size_t module_count = 0;
static size_t module_cap = 0;

static struct AOS_Module** registered_modules = NULL;
static size_t registered_module_count = 0;
static size_t registered_module_cap = 0;

uint8_t modules_init(void) {
    // Loads core modules and more
    modules = (struct AOS_Module*)avmf_alloc(sizeof(struct AOS_Module)*16, MALLOC_TYPE_KERNEL, PAGE_RW | PAGE_PRESENT, NULL);
    if (!modules) {
        serial_print("[Module-System] Allocation failed!\n");
        return 0;
    }
    module_cap = 16;

    serial_print("[Module-System] Core Modules Loaded!\n");
    return 1;
}

uint8_t module_register(struct AOS_Module* module) {
    if (!registered_modules || registered_module_count >= registered_module_cap) {
        struct AOS_Module** nptr = (struct AOS_Module**)avmf_alloc(sizeof(struct AOS_Module*)*(registered_module_cap + 16), MALLOC_TYPE_KERNEL, PAGE_RW | PAGE_PRESENT, NULL);
        if (!nptr) {
            serial_print("[Module-System] Reallocation of registered modules failed!\n");
            return 0;
        }
        registered_module_cap += 16;
        if (registered_modules) {
            memcpy(nptr, registered_modules, sizeof(struct AOS_Module*)*registered_module_count);
            avmf_free((uint64_t)registered_modules);
        }
        registered_modules = nptr;
    }
    registered_modules[registered_module_count++] = module;
    module->hdr.registered = 1;
    return 1;
}

struct AOS_Module* module_get_first_applicable_driver(uint8_t class, uint8_t subclass, uint8_t use_subclass, uint8_t progif, uint8_t use_progif, uint8_t revision, uint8_t use_revision, uint16_t vendor, uint8_t use_vendor) {
    for (size_t i = 0; i < module_count; i++) {
        struct AOS_Module* mod = &modules[i];
        if (mod->hdr.type != MODULE_TYPE_DRIVER) continue;
        
        if (mod->Modules.driver_module.target_class == class) {
            if (use_subclass == 1 && mod->Modules.driver_module.target_subclass != subclass) continue;
            if (use_progif == 1 && mod->Modules.driver_module.target_use_progifclass == 1) {
                if (mod->Modules.driver_module.target_progifclass != progif) continue;
            }
            if (use_revision == 1 && mod->Modules.driver_module.target_use_revision == 1) {
                if (mod->Modules.driver_module.target_revision != revision) continue;
            }
            if (use_vendor == 1 && mod->Modules.driver_module.target_use_vendor == 1) {
                if (mod->Modules.driver_module.target_vendor != vendor) continue;
            }
            
            return mod;
        }
    }
    return NULL;
}

struct AOS_Module* module_get_first_applicable_registered_driver(uint8_t class, uint8_t subclass, uint8_t use_subclass, uint8_t progif, uint8_t use_progif, uint8_t revision, uint8_t use_revision, uint16_t vendor, uint8_t use_vendor) {
    for (size_t i = 0; i < registered_module_count; i++) {
        struct AOS_Module* mod = registered_modules[i];
        if (mod->hdr.type != MODULE_TYPE_DRIVER) continue;
        
        if (mod->Modules.driver_module.target_class == class) {
            if (use_subclass == 1 && mod->Modules.driver_module.target_subclass != subclass) continue;
            if (use_progif == 1 && mod->Modules.driver_module.target_use_progifclass == 1) {
                if (mod->Modules.driver_module.target_progifclass != progif) continue;
            }
            if (use_revision == 1 && mod->Modules.driver_module.target_use_revision == 1) {
                if (mod->Modules.driver_module.target_revision != revision) continue;
            }
            if (use_vendor == 1 && mod->Modules.driver_module.target_use_vendor == 1) {
                if (mod->Modules.driver_module.target_vendor != vendor) continue;
            }
            return mod;
        }
    }
    return NULL;
}

uint8_t module_already_initialized(struct AOS_Module* module) {
    for (size_t i = 0; i < registered_module_count; i++) {
        struct AOS_Module* mod = registered_modules[i];
        if (mod == module) return 1;
    }
    return 0;
}
