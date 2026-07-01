//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_PROPERTIES_H
#define SPHLASH_PROPERTIES_H

#include <functional>
#include <math.h>
#include <memory>
#include <string>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "assets/Texture2D.h"

struct NodeProperty {
    virtual ~NodeProperty() = default;
    virtual void draw() = 0;
};

struct FloatProperty : NodeProperty {
    FloatProperty(
        const std::string& name,
        std::function<float()> get,
        std::function<void (float)> set,
        float min = -INFINITY, float max = INFINITY, float speed = 1.0f
    )
        : name(name), get(std::move(get)), set(std::move(set)), min(min), max(max), speed(speed)
    {}

    std::string                     name;
    std::function<float()>          get;
    std::function<void (float)>     set;

    float min = -INFINITY;
    float max = INFINITY;
    float speed = 0.1f;

    void draw() override;
};

struct BoolProperty : NodeProperty {
    BoolProperty(const std::string& name, std::function<bool()> get, std::function<void (bool)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<bool()>          get;
    std::function<void (bool)>     set;

    void draw() override;
};

struct Vec3Property : NodeProperty {
    Vec3Property(const std::string& name, std::function<glm::vec3()> get, std::function<void (glm::vec3)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<glm::vec3()>      get;
    std::function<void (glm::vec3)> set;

    void draw() override;
};

struct IVec3Property : NodeProperty {
    IVec3Property(const std::string& name, std::function<glm::ivec3()> get, std::function<void (glm::ivec3)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<glm::ivec3()>      get;
    std::function<void (glm::ivec3)> set;

    void draw() override;
};

struct ColorProperty : NodeProperty {
    ColorProperty(const std::string& name, std::function<glm::vec3()> get, std::function<void (glm::vec3)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                     name;
    std::function<glm::vec3()>      get;
    std::function<void (glm::vec3)> set;

    void draw() override;
};

struct Texture2DProperty : NodeProperty {
    Texture2DProperty(const std::string& name, std::function<std::shared_ptr<Texture2D>()> get, std::function<void (std::shared_ptr<Texture2D>)> set)
        : name(name), get(std::move(get)), set(std::move(set))
    {}

    std::string                                    name;
    std::function<std::shared_ptr<Texture2D>()>      get;
    std::function<void (std::shared_ptr<Texture2D>)> set;

    void draw() override;
};

struct Section {
    std::string name;
    int index;
};

class NodeProperties {
public:
    void addSection(const std::string& section);
    Section getSection(int index);

    void addProperty(std::unique_ptr<NodeProperty> property);
    NodeProperty* getProperty(int index) const;

    int getPropertyCount() const;
    int getSectionCount() const;

private:
    std::vector<Section>                        sections;
    std::vector<std::unique_ptr<NodeProperty>>  properties;
};

#endif //SPHLASH_PROPERTIES_H
