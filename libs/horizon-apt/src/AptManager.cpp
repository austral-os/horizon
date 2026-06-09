#include <horizon/apt/AptManager.hpp>
#include <apt-pkg/init.h>
#include <apt-pkg/configuration.h>
#include <apt-pkg/pkgsystem.h>
#include <apt-pkg/cachefile.h>
#include <apt-pkg/pkgcache.h>
#include <apt-pkg/depcache.h>
#include <apt-pkg/pkgrecords.h>
#include <iostream>
#include <algorithm>
#include <cstdlib>

namespace horizon::apt {

struct AptManager::Private {
    pkgCacheFile cache_file;
    bool initialized = false;
};

AptManager::AptManager() : d(new Private()) {
}

AptManager::~AptManager() {
    delete d;
}

void AptManager::reload_cache() {
    delete d;
    d = new Private();
    initialize();
}

bool AptManager::initialize() {
    if (d->initialized) return true;

    if (!pkgInitConfig(*_config)) {
        std::cerr << "Failed to init config" << std::endl;
        return false;
    }

    if (!pkgInitSystem(*_config, _system)) {
        std::cerr << "Failed to init system" << std::endl;
        return false;
    }

    if (!d->cache_file.BuildCaches(nullptr, false)) {
        std::cerr << "Failed to build caches" << std::endl;
        return false;
    }

    if (!d->cache_file.Open(nullptr, false)) {
        std::cerr << "Failed to open caches" << std::endl;
        return false;
    }

    d->initialized = true;
    return true;
}

std::vector<PackageInfo> AptManager::search_packages(const std::string& keyword) {
    std::vector<PackageInfo> results;
    if (!initialize()) return results;

    pkgCache* cache = d->cache_file.GetPkgCache();
    if (!cache) return results;

    pkgRecords records(d->cache_file);

    for (pkgCache::PkgIterator pkg = cache->PkgBegin(); !pkg.end(); ++pkg) {
        if (!pkg.Name()) continue;

        std::string name = pkg.Name();
        // Simple substring search for now
        if (name.find(keyword) == std::string::npos) {
            // Also search description if possible
            pkgCache::VerIterator ver = pkg.VersionList();
            if (!ver.end()) {
                pkgCache::DescIterator desc = ver.DescriptionList();
                if (!desc.end()) {
                    pkgRecords::Parser& parser = records.Lookup(desc.FileList());
                    std::string description = parser.LongDesc();
                    if (description.find(keyword) == std::string::npos) {
                        continue;
                    }
                } else {
                    continue;
                }
            } else {
                continue;
            }
        }

        pkgCache::VerIterator ver = pkg.VersionList();
        if (ver.end()) continue;

        PackageInfo info;
        info.name = name;
        info.version = ver.VerStr() ? ver.VerStr() : "";
        
        pkgCache::DescIterator desc = ver.DescriptionList();
        if (!desc.end()) {
            pkgRecords::Parser& parser = records.Lookup(desc.FileList());
            info.description = parser.ShortDesc();
        }

        pkgCache::State::VerPriority prio;
        // checking installation status
        pkgDepCache* depCache = d->cache_file.GetDepCache();
        if (depCache) {
            pkgDepCache::StateCache &state = (*depCache)[pkg];
            info.is_installed = (state.Status == 2); // 2 usually indicates installed, but there's a better way:
        }
        
        if (pkg->CurrentVer != 0) {
            info.is_installed = true;
        } else {
            info.is_installed = false;
        }

        results.push_back(info);
    }

    return results;
}

std::vector<PackageInfo> AptManager::list_packages_by_sections(const std::vector<std::string>& sections, int max_results) {
    std::vector<PackageInfo> results;
    if (!initialize()) return results;

    pkgCache* cache = d->cache_file.GetPkgCache();
    if (!cache) return results;

    pkgRecords records(d->cache_file);

    for (pkgCache::PkgIterator pkg = cache->PkgBegin(); !pkg.end(); ++pkg) {
        if (!pkg.Name()) continue;

        pkgCache::VerIterator ver = pkg.VersionList();
        if (ver.end()) continue;

        const char* section_cstr = ver.Section();
        std::string section = section_cstr ? section_cstr : "";

        if (!sections.empty()) {
            bool matches = false;
            for (const auto& s : sections) {
                if (section.find(s) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
            if (!matches) continue;
        }

        PackageInfo info;
        info.name = pkg.Name();
        info.version = ver.VerStr() ? ver.VerStr() : "";
        
        pkgCache::DescIterator desc = ver.DescriptionList();
        if (!desc.end()) {
            pkgRecords::Parser& parser = records.Lookup(desc.FileList());
            info.description = parser.ShortDesc();
        }

        info.is_installed = (pkg->CurrentVer != 0);
        results.push_back(info);
    }

    std::sort(results.begin(), results.end(), [](const PackageInfo& a, const PackageInfo& b) {
        return a.name < b.name;
    });

    if (results.size() > static_cast<size_t>(max_results)) {
        results.resize(max_results);
    }

    return results;
}

std::optional<PackageInfo> AptManager::get_package_info(const std::string& pkg_name) {
    if (!initialize()) return std::nullopt;

    pkgCache* cache = d->cache_file.GetPkgCache();
    if (!cache) return std::nullopt;

    pkgCache::PkgIterator pkg = cache->FindPkg(pkg_name);
    if (pkg.end()) return std::nullopt;

    pkgRecords records(d->cache_file);
    pkgCache::VerIterator ver = pkg.VersionList();
    if (ver.end()) return std::nullopt;

    PackageInfo info;
    info.name = pkg.Name();
    info.version = ver.VerStr() ? ver.VerStr() : "";
    
    pkgCache::DescIterator desc = ver.DescriptionList();
    if (!desc.end()) {
        pkgRecords::Parser& parser = records.Lookup(desc.FileList());
        info.description = parser.LongDesc();
    }

    if (pkg->CurrentVer != 0) {
        info.is_installed = true;
    } else {
        info.is_installed = false;
    }

    return info;
}

bool AptManager::install_package(const std::string& pkg_name) {
    if (pkg_name.empty()) return false;
    std::string cmd = "pkexec env DEBIAN_FRONTEND=noninteractive apt-get install -y \"" + pkg_name + "\"";
    int result = std::system(cmd.c_str());
    return result == 0;
}

bool AptManager::remove_package(const std::string& pkg_name) {
    if (pkg_name.empty()) return false;
    std::string cmd = "pkexec env DEBIAN_FRONTEND=noninteractive apt-get remove -y \"" + pkg_name + "\"";
    int result = std::system(cmd.c_str());
    return result == 0;
}

} // namespace horizon::apt
