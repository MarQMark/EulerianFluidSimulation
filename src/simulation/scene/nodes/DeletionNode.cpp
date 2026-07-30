#include "simulation/scene/nodes/DeletionNode.h"

#include "simulation/Properties.h"
#include "simulation/data/IGrid.h"
#include "simulation/data/ScalField3D.h"
#include "simulation/display/SimulationDisplay3D.h"

DeletionNode::DeletionNode() : SimSpaceNode("DeletionNode", glm::vec3(0.7, 0.0, 0.0)) {
}

DeletionNode::DeletionNode(const nlohmann::json json) : SimSpaceNode(json){
}

void DeletionNode::update(const float deltaTime) {
    SimSpaceNode::update(deltaTime);

    const auto* data = static_cast<SimulationDisplay3D::RenderData3D*>(_sim_data);
    auto* grid = data->grid;
    if (!grid)
        return;

    if (PROPERTY_B(PAUSED))
        return;

    const int w = PROPERTY_I(WIDTH_3D);
    const int h = PROPERTY_I(HEIGHT_3D);
    const int d = PROPERTY_I(DEPTH_3D);

    for (int z = std::max(0, (int)position.z); z < std::min(d - 1, (int)(position.z + scale.z)); z++)
    for (int y = std::max(0, (int)position.y); y < std::min(h - 1, (int)(position.y + scale.y)); y++)
    for (int x = std::max(0, (int)position.x); x < std::min(w - 1, (int)(position.x + scale.x)); x++) {
         grid->rho()(x, y, z) = 0.f;
    }
}

std::string DeletionNode::getTypeName() const {
    return "DeletionNode";
}
