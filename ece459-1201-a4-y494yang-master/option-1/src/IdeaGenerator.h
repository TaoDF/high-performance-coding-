#pragma once

#include "ChecksumTracker.h"
#include "EventQueue.h"
#include <string>

struct StrPair
{
    std::string a;
    std::string b;

    StrPair() { }

    StrPair(std::string a, std::string b) : a(a), b(b) { }

    StrPair(const StrPair& other) {
        a = other.a;
        b = other.b;
    }

    StrPair& operator=(const StrPair& other) {
        if (this != &other) {
            a = other.a;
            b = other.b;
        }

        return *this;
    }
};

class IdeaGenerator : public ChecksumTracker<IdeaGenerator, ChecksumType::IDEA>
{
    EventQueue* eq;
    const int numIdeas;
    const int ideaStartIdx;
    const int ideaEndIdx;
    const int numPackages;
    const int numStudents;
    Container<StrPair> ideas;

    std::string getNextIdea(int i);

public:
    IdeaGenerator(EventQueue* eq, int ideaStartIdx, int ideaEndIdx, int numPackages, int numStudents);
    ~IdeaGenerator();
    void run();
};
