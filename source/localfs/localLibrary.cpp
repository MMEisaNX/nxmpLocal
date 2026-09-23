#include "localLibrary.h"
#include "logger.h"
#include "utils.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <sstream>
#include <iomanip>

LocalLib::LocalLibraryManager *localLibManager = nullptr;

namespace LocalLib {

LocalLibraryManager::LocalLibraryManager() {
}

LocalLibraryManager::~LocalLibraryManager() {
}

void LocalLibraryManager::setRootPath(const std::string &path) {
    rootPath = path;
    if (!rootPath.empty() && rootPath.back() == '/' && rootPath.length() > 1) {
        rootPath.pop_back();
    }
}

static bool fileExists(const std::string &filepath) {
    struct stat buffer;
    return (stat(filepath.c_str(), &buffer) == 0 && !S_ISDIR(buffer.st_mode));
}

bool LocalLibraryManager::isVideoFile(const std::string &filename) {
    const std::vector<std::string> videoExtensions = {
        ".mp4", ".mkv", ".avi", ".mov", ".flv", ".wmv", ".webm",
        ".m4v", ".3gp", ".ts", ".m2ts", ".vob", ".mpg", ".mpeg",
        ".MP4", ".MKV", ".AVI", ".MOV", ".FLV", ".WMV", ".WEBM"
    };
    for (const auto &ext : videoExtensions) {
        if (Utility::endsWith(filename, ext, false)) {
            return true;
        }
    }
    return false;
}

bool LocalLibraryManager::isImageFile(const std::string &filename) {
    const std::vector<std::string> imageExtensions = {
        ".jpg", ".jpeg", ".png", ".bmp", ".webp",
        ".JPG", ".JPEG", ".PNG", ".BMP", ".WEBP"
    };
    for (const auto &ext : imageExtensions) {
        if (Utility::endsWith(filename, ext, false)) {
            return true;
        }
    }
    return false;
}

std::string LocalLibraryManager::getFilenameWithoutExt(const std::string &filename) {
    size_t lastDot = filename.find_last_of(".");
    if (lastDot == std::string::npos) return filename;
    return filename.substr(0, lastDot);
}

std::string LocalLibraryManager::formatFileSize(size_t bytes) {
    const char *sizes[] = {"B", "KB", "MB", "GB", "TB"};
    int order = 0;
    double dblBytes = static_cast<double>(bytes);
    while (dblBytes >= 1024.0 && order < 4) {
        order++;
        dblBytes /= 1024.0;
    }
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.1f %s", dblBytes, sizes[order]);
    return std::string(buf);
}

std::string LocalLibraryManager::findPosterForFolder(const std::string &folderPath) {
    static const std::vector<std::string> posterCandidates = {
        "poster.jpg", "cover.jpg", "folder.jpg", "default.jpg",
        "poster.png", "cover.png", "folder.png", "default.png",
        "poster.jpeg", "cover.jpeg", "folder.jpeg",
        "poster.webp", "cover.webp",
        "Poster.jpg", "Cover.jpg", "Folder.jpg",
        "POSTER.JPG", "COVER.JPG", "FOLDER.JPG"
    };

    std::string base = folderPath;
    if (!base.empty() && base.back() == '/') base.pop_back();

    for (const auto &name : posterCandidates) {
        std::string testPath = base + "/" + name;
        if (fileExists(testPath)) {
            return testPath;
        }
    }

    // Also check folderName.jpg / folderName.png in parent directory
    size_t lastSlash = base.find_last_of('/');
    if (lastSlash != std::string::npos) {
        std::string parent = base.substr(0, lastSlash);
        std::string folderName = base.substr(lastSlash + 1);
        for (const auto &ext : {".jpg", ".png", ".jpeg", ".webp"}) {
            std::string testPath = parent + "/" + folderName + ext;
            if (fileExists(testPath)) {
                return testPath;
            }
        }
    }

    return "";
}

std::string LocalLibraryManager::findPosterForVideo(const std::string &videoPath) {
    size_t lastSlash = videoPath.find_last_of('/');
    std::string dir = (lastSlash != std::string::npos) ? videoPath.substr(0, lastSlash) : "";
    std::string filename = (lastSlash != std::string::npos) ? videoPath.substr(lastSlash + 1) : videoPath;
    std::string noExt = getFilenameWithoutExt(filename);

    // 1. Check videoName.jpg / videoName.png in same folder
    const std::vector<std::string> exts = {".jpg", ".png", ".jpeg", ".webp", ".JPG", ".PNG"};
    for (const auto &ext : exts) {
        std::string testPath = dir.empty() ? (noExt + ext) : (dir + "/" + noExt + ext);
        if (fileExists(testPath)) {
            return testPath;
        }
    }

    // 2. Check general poster names in same folder
    if (!dir.empty()) {
        std::string folderPoster = findPosterForFolder(dir);
        if (!folderPoster.empty()) {
            return folderPoster;
        }
    }

    return "";
}

bool LocalLibraryManager::scanLibrary() {
    categories.clear();
    if (rootPath.empty()) return false;

    DIR *dir = opendir(rootPath.c_str());
    if (!dir) {
        NXLOG::DEBUGLOG("LocalLibraryManager: Failed to open root path: %s\n", rootPath.c_str());
        return false;
    }

    struct dirent *ent;
    std::vector<std::string> subdirs;
    std::vector<std::string> rootVideos;

    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue; // skip hidden and ./..

        std::string fullPath = rootPath + "/" + ent->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                subdirs.push_back(ent->d_name);
            } else if (isVideoFile(ent->d_name)) {
                rootVideos.push_back(ent->d_name);
            }
        }
    }
    closedir(dir);

    // Sort subdirectories alphabetically
    std::sort(subdirs.begin(), subdirs.end());

    // Create a category for each subfolder
    for (const auto &subdirName : subdirs) {
        MediaCategory cat;
        cat.name = subdirName;
        cat.path = rootPath + "/" + subdirName;
        cat.posterPath = findPosterForFolder(cat.path);
        scanCategoryItems(cat);
        categories.push_back(cat);
    }

    // If there are videos directly in the root path, group them in a "Videos" / "General" category
    if (!rootVideos.empty()) {
        MediaCategory rootCat;
        rootCat.name = "All Videos";
        rootCat.path = rootPath;
        rootCat.posterPath = findPosterForFolder(rootPath);
        std::sort(rootVideos.begin(), rootVideos.end());
        for (const auto &vidName : rootVideos) {
            MediaItem item;
            item.title = getFilenameWithoutExt(vidName);
            item.videoPath = rootPath + "/" + vidName;
            item.posterPath = findPosterForVideo(item.videoPath);
            item.isDirectory = false;
            struct stat st;
            if (stat(item.videoPath.c_str(), &st) == 0) {
                item.fileSize = st.st_size;
            }
            rootCat.items.push_back(item);
        }
        categories.insert(categories.begin(), rootCat);
    }

    NXLOG::DEBUGLOG("LocalLibraryManager: Scanned %zu categories in %s\n", categories.size(), rootPath.c_str());
    return !categories.empty();
}

void LocalLibraryManager::scanCategoryItems(MediaCategory &cat) {
    cat.items.clear();
    DIR *dir = opendir(cat.path.c_str());
    if (!dir) return;

    struct dirent *ent;
    std::vector<std::string> subdirs;
    std::vector<std::string> files;

    while ((ent = readdir(dir)) != nullptr) {
        if (ent->d_name[0] == '.') continue;
        std::string fullPath = cat.path + "/" + ent->d_name;
        struct stat st;
        if (stat(fullPath.c_str(), &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                subdirs.push_back(ent->d_name);
            } else if (isVideoFile(ent->d_name)) {
                files.push_back(ent->d_name);
            }
        }
    }
    closedir(dir);

    std::sort(subdirs.begin(), subdirs.end());
    std::sort(files.begin(), files.end());

    // Process subfolders inside category (e.g. Movies/Inception/)
    for (const auto &sub : subdirs) {
        std::string subPath = cat.path + "/" + sub;
        MediaItem item;
        item.title = sub;
        item.folderPath = subPath;
        item.isDirectory = true;
        item.posterPath = findPosterForFolder(subPath);

        // Find the first playable video inside this subfolder
        DIR *subDir = opendir(subPath.c_str());
        if (subDir) {
            struct dirent *subEnt;
            while ((subEnt = readdir(subDir)) != nullptr) {
                if (subEnt->d_name[0] == '.') continue;
                if (isVideoFile(subEnt->d_name)) {
                    item.videoPath = subPath + "/" + subEnt->d_name;
                    struct stat st;
                    if (stat(item.videoPath.c_str(), &st) == 0) {
                        item.fileSize = st.st_size;
                    }
                    break;
                }
            }
            closedir(subDir);
        }

        // If poster wasn't found in folder, try findPosterForVideo
        if (item.posterPath.empty() && !item.videoPath.empty()) {
            item.posterPath = findPosterForVideo(item.videoPath);
        }

        cat.items.push_back(item);
    }

    // Process direct video files inside category
    for (const auto &vid : files) {
        MediaItem item;
        item.title = getFilenameWithoutExt(vid);
        item.videoPath = cat.path + "/" + vid;
        item.isDirectory = false;
        item.posterPath = findPosterForVideo(item.videoPath);
        struct stat st;
        if (stat(item.videoPath.c_str(), &st) == 0) {
            item.fileSize = st.st_size;
        }
        cat.items.push_back(item);
    }
}

void LocalLibraryManager::loadItemTexture(MediaItem &item, CImgLoader *imgLoader) {
    if (item.textureAttempted || !imgLoader) return;
    item.textureAttempted = true;

    if (!item.posterPath.empty() && fileExists(item.posterPath)) {
        item.posterTexture = imgLoader->OpenImageFile(item.posterPath);
        if (item.posterTexture.id != static_cast<DkResHandle>(-1)) {
            item.textureLoaded = true;
        }
    }
}

void LocalLibraryManager::loadCategoryTexture(MediaCategory &cat, CImgLoader *imgLoader) {
    if (cat.textureAttempted || !imgLoader) return;
    cat.textureAttempted = true;

    if (!cat.posterPath.empty() && fileExists(cat.posterPath)) {
        cat.categoryTexture = imgLoader->OpenImageFile(cat.posterPath);
        if (cat.categoryTexture.id != static_cast<DkResHandle>(-1)) {
            cat.textureLoaded = true;
        }
    }
}

void LocalLibraryManager::releaseCategoryTextures(MediaCategory &cat, NXMPRenderer *renderer) {
    if (!renderer) return;

    for (auto &item : cat.items) {
        if (item.textureLoaded && item.posterTexture.id != static_cast<DkResHandle>(-1)) {
            renderer->unregister_texture(item.posterTexture);
            item.posterTexture = {};
            item.textureLoaded = false;
            item.textureAttempted = false;
        }
    }

    if (cat.textureLoaded && cat.categoryTexture.id != static_cast<DkResHandle>(-1)) {
        renderer->unregister_texture(cat.categoryTexture);
        cat.categoryTexture = {};
        cat.textureLoaded = false;
        cat.textureAttempted = false;
    }
}

void LocalLibraryManager::releaseAllTextures(NXMPRenderer *renderer) {
    if (!renderer) return;
    for (auto &cat : categories) {
        releaseCategoryTextures(cat, renderer);
    }
}

} // namespace LocalLib
