//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_TRANSFORM_H
#define SPHLASH_TRANSFORM_H

#include "glm/glm.hpp"
#include "glm/detail/type_quat.hpp"

class Transform {
public:
    virtual ~Transform() = default;

    glm::mat4 getTransform() const;
    virtual void setTransform(glm::mat4 transform);

    glm::vec3 getPosition();
    virtual void setPosition(glm::vec3 position);

    glm::quat getRotation() const;
    virtual void setRotation(glm::quat rotation);

    glm::vec3 getEulerRotation() const;
    virtual void setEulerRotation(glm::vec3 rotation);

    glm::vec3 getScale() const;
    virtual void setScale(glm::vec3 scale);

    virtual void updateTransform();

protected:
    glm::vec3 position = { 0.0, 0.0, 0.0 };
    glm::quat rotation = { 0.0, 0.0, 0.0, 0.0 };
    glm::vec3 scale = { 1.0, 1.0, 1.0 };

    glm::mat4 transform = glm::mat4(1.0);
};

#endif //SPHLASH_TRANSFORM_H
