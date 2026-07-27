//
// Created by ruut on 7/27/26.
//

#ifndef SPHLASH_QUERYDATAPANEL_H
#define SPHLASH_QUERYDATAPANEL_H

#include "Viewable.h"
#include "glm/vec3.hpp"

struct Metrics;
class Editor;

using FloatMetricGetter = float(*)(const Metrics&);
using VectorMetricGetter = glm::vec3(*)(const Metrics&);

class QueryDataView : public Viewable {
public:
    explicit QueryDataView(Editor* editor);

    void render(void *renderData) override;

private:
    struct MetricData {
        const std::vector<Metrics>* history;
        union {
            FloatMetricGetter getter;
            VectorMetricGetter getterVector;
        };
    };

    void plotFloatMetric(
        const char* id,
        const char* label,
        const std::vector<Metrics>& history,
        FloatMetricGetter getter
    );

    void plotVectorMetric(
        const char* id,
        const char* label,
        const std::vector<Metrics>& history,
        VectorMetricGetter getter
    );

    Editor* _editor{};
    int _selectedNode = 0;
};

#endif //SPHLASH_QUERYDATAPANEL_H
