#include "PackageDownloader.h"
#include "utils.h"
#include <fstream>
#include <cassert>

PackageDownloader::PackageDownloader(EventQueue* eq, int packageStartIdx, int packageEndIdx)
    : ChecksumTracker()
    , eq(eq)
    , numPackages(packageEndIdx - packageStartIdx + 1)
    , packageStartIdx(packageStartIdx)
    , packageEndIdx(packageEndIdx)
{
    packageName = readFile("data/packages.txt");
    #ifdef DEBUG
    std::unique_lock<std::mutex> stdoutLock(eq->stdoutMutex);
    printf("PackageDownloader n:%d s:%d e:%d\n", numPackages, packageStartIdx, packageEndIdx);
    #endif
}


PackageDownloader::~PackageDownloader() {
    // nop
}

void PackageDownloader::run() {
    for (int i = packageStartIdx; i <= packageEndIdx; i++) {
        eq->enqueueEvent(Event(Event::DOWNLOAD_COMPLETE, new Package(packageName[i % packageName.size()])));
        updateGlobalChecksum(sha256(packageName[i % packageName.size()]));
    }
}
