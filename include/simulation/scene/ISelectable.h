//
// Created by ruut on 5/31/26.
//

#ifndef SPHLASH_ISELECTABLE_H
#define SPHLASH_ISELECTABLE_H

#include "NodeProperties.h"

class ISelectable {
public:
    virtual ~ISelectable() = default;
    virtual NodeProperties buildProperties() = 0;
};

#endif //SPHLASH_ISELECTABLE_H
