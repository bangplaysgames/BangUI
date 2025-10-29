#pragma once
#include <string>

class Mesh {
public:
    std::string id;
    void* resource; // pointer/reference to mesh resource
};
