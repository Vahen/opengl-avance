//
// Created by ValasNaerth on 23/02/2018.
//

#ifndef OPENGL_OBJECTINFO_HPP
#define OPENGL_OBJECTINFO_HPP


#include <glm/vec3.hpp>
#include <vector>

struct Trajectoire{
    std::vector<glm::vec3> points; // Differents points sur la trajectoire
};
class ObjectInfo{
// todo -> voir quels informations sont utile
// position : vec3

public:

    void update();

private:
};

#endif //OPENGL_OBJECTINFO_HPP
