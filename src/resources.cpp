#include "resources.h"

std::string getResourcePath()
{
	const char* dataPath = getenv("SARTRE_DATA_PATH");
	if (dataPath != nullptr) {
		printf("Reading data from path: %s\n", dataPath);
		return std::string(dataPath) + "/";
	}
#ifdef __APPLE__
	CFBundleRef mainBundle = CFBundleGetMainBundle();
	if (mainBundle) {
		CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(mainBundle);
		char path[PATH_MAX];
		if (CFURLGetFileSystemRepresentation(resourcesURL, TRUE, (UInt8 *)path, PATH_MAX)) {
			CFRelease(resourcesURL);
			return std::string(path) + "/data/";
		}
		CFRelease(resourcesURL);
	}
	return "./data/";
#else
	return "./data/";
#endif
}

