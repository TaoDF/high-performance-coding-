#pragma once

#include <cassert>
#include <deque>

template<typename T>
class Container
{
    std::deque<T> dq;
    int len = 0;

    void deepCopy(const Container &other) {
        len = other.len;
        dq.clear();
        for (int i = 0; i < len; i++)
        {
            dq.push_back(other.dq.at(i));
        }
    }

public:
    Container() {
        // nop
    }

    ~Container() {
        clear();
    }

    Container(const Container &other) {
        deepCopy(other);
    }

    Container& operator=(const Container &other) {
        if (this != &other) {
            deepCopy(other);
        }

        return *this;
    }

    int size() const {
        return len;
    }

    T operator[](int idx) const {
        if (!(0 <= idx && idx < len)) {
            assert(false && "Accessing index out of range");
        }

        return dq.at(idx);
    }

    void clear() {
        dq.clear();
        len = 0;
    }

    T front() const {
        return dq.front();
    }

    void pushFront(T item) {
        len += 1;
        dq.push_front(item);
    }

    void pushBack(T item) {
        len += 1;
        dq.push_back(item);
    }

    T popFront() {
        if (len < 1) {
            assert(false && "Trying to pop an empty Container");
        }
        len -= 1;

        T front = dq.front();
        dq.pop_front();

        return front;
    }

    void push(T item) {
        pushBack(item);
    }

    T pop() {
        return popFront();
    }
};
