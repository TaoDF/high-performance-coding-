#include "IdeaGenerator.h"
#include "Container.h"
#include "utils.h"
#include <cassert>



IdeaGenerator::IdeaGenerator(EventQueue* eq, int ideaStartIdx, int ideaEndIdx, int numPackages, int numStudents)
    : ChecksumTracker()
    , eq(eq)
    , numIdeas(ideaEndIdx - ideaStartIdx + 1)
    , ideaStartIdx(ideaStartIdx)
    , ideaEndIdx(ideaEndIdx)
    , numPackages(numPackages)
    , numStudents(numStudents)
{
    Container<std::string> products = readFile("data/ideas-products.txt");
    Container<std::string> customers = readFile("data/ideas-customers.txt");
    for (int i = 0; i < products.size(); i++) {
        for (int j = 0; j < customers.size(); j++) {
            ideas.push(StrPair(products[i], customers[j]));
        }
    }
    #ifdef DEBUG
    std::unique_lock<std::mutex> stdoutLock(eq->stdoutMutex);
    printf("IdeaGenerator n:%d s:%d e:%d p:%d s:%d\n", numIdeas, ideaStartIdx, ideaEndIdx, numPackages, numStudents);
    #endif
}

IdeaGenerator::~IdeaGenerator() {
    // nop
}

std::string IdeaGenerator::getNextIdea(int i) {

    assert(ideas.size() > 0);
    auto ideaPair = ideas[i % ideas.size()];

    return ideaPair.a + " for " + ideaPair.b;
}

void IdeaGenerator::run() {

    int packagesPerIdea = numPackages / numIdeas;
    int ideasWithExtraPackage = numPackages % numIdeas;

    for (int i = ideaStartIdx; i <= ideaEndIdx; i++) {
        std::string ideaName = getNextIdea(i);
        int packagesReq = packagesPerIdea + (((i - ideaStartIdx) < ideasWithExtraPackage) ? 1 : 0);

        eq->enqueueEvent(Event(Event::NEW_IDEA, new Idea(ideaName, packagesReq)));
        updateGlobalChecksum(sha256(ideaName));
    }

    for (int i = 0; i < numStudents; i++) {
        eq->enqueueEvent(Event(Event::OUT_OF_IDEAS, nullptr));
    }
}
