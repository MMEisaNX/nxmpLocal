#ifndef NXMP_LOCAL_LIBRARY_H
#define NXMP_LOCAL_LIBRARY_H

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <algorithm>
#include "nxmp-render.h"
#include "imgloader.h"
#include "localfiles.h"

namespace LocalLib {

struct MediaItem {
    std::string title;
    std::string videoPath;
    std::string posterPath;
    std::string folderPath;
    bool isDirectory = false;
    size_t fileSize = 0;
    int resumePercent = 0;
    
    // Texture information
    Texture posterTexture = {};
    bool textureLoaded = false;
    bool textureAttempted = false;
};

struct MediaCategory {
    std::string name;
    std::string path;
    std::string posterPath;
    std::vector<MediaItem> items;
    
    Texture categoryTexture = {};
    bool textureLoaded = false;
    bool textureAttempted = false;
};

class LocalLibraryManager {
public:
    LocalLibraryManager();
    ~LocalLibraryManager();

    void setRootPath(const std::string &path);
    std::string getRootPath() const { return rootPath; }
    
    bool scanLibrary();
    void scanCategoryItems(MediaCategory &cat);
    
    const std::vector<MediaCategory>& getCategories() const { return categories; }
    std::vector<MediaCategory>& getCategories() { return categories; }
    
    bool hasCategories() const { return !categories.empty(); }
    
    void loadItemTexture(MediaItem &item, CImgLoader *imgLoader);
    void loadCategoryTexture(MediaCategory &cat, CImgLoader *imgLoader);
    void releaseCategoryTextures(MediaCategory &cat, NXMPRenderer *renderer);
    void releaseAllTextures(NXMPRenderer *renderer);
    
    static std::string findPosterForVideo(const std::string &videoPath);
    static std::string findPosterForFolder(const std::string &folderPath);
    static bool isVideoFile(const std::string &filename);
    static bool isImageFile(const std::string &filename);
    static std::string getFilenameWithoutExt(const std::string &filename);
    static std::string formatFileSize(size_t bytes);

private:
    std::string rootPath;
    std::vector<MediaCategory> categories;
};

} // namespace LocalLib

extern LocalLib::LocalLibraryManager *localLibManager;

#endif // NXMP_LOCAL_LIBRARY_H
