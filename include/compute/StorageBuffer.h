#ifndef EULERIANFLUIDSIMULATION_STORAGEBUFFER_H
#define EULERIANFLUIDSIMULATION_STORAGEBUFFER_H

#include "simulation/ScalField2D.h"
#include "simulation/VecField2D.h"

class StorageBuffer {
public:
    StorageBuffer(unsigned int size);
    ~StorageBuffer();

    void bind(const int point) const;

    void upload (const void* data, unsigned int bytes = 0) const;
    void download(void* data, unsigned int bytes = 0) const;

    void upload (ScalField2D* field, unsigned int bytes = 0) const;
    void download(ScalField2D* field, unsigned int bytes = 0) const;
    void upload (VecField2D* field, unsigned int bytes = 0) const;
    void download(VecField2D* field, unsigned int bytes = 0) const;

    unsigned int id() const { return _id; }

private:
    unsigned int _id;
    const unsigned int _size;
};


#endif //EULERIANFLUIDSIMULATION_STORAGEBUFFER_H