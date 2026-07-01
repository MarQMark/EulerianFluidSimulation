#ifndef EULERIANFLUIDSIMULATION_STORAGEBUFFER_H
#define EULERIANFLUIDSIMULATION_STORAGEBUFFER_H

class VecField3D;
class VecField2D;
class ScalField3D;
class ScalField2D;

class StorageBuffer {
public:
    StorageBuffer(unsigned int size);
    ~StorageBuffer();

    void bind(const int point) const;
    void clear() const;

    void resize(unsigned int size);

    void upload(const void* data, unsigned int bytes = 0) const;
    void download(void* data, unsigned int bytes = 0) const;

    void upload(ScalField2D* field, unsigned int bytes = 0) const;
    void download(ScalField2D* field, unsigned int bytes = 0) const;

    void upload(ScalField3D* field, unsigned int bytes = 0) const;
    void download(ScalField3D* field, unsigned int bytes = 0) const;

    void upload(VecField2D* field, unsigned int bytes = 0) const;
    void download(VecField2D* field, unsigned int bytes = 0) const;

    void upload(VecField3D* field, unsigned int bytes = 0) const;
    void download(VecField3D* field, unsigned int bytes = 0) const;

    unsigned int id() const { return _id; }

private:
    unsigned int _id;
    unsigned int _size;
};


#endif //EULERIANFLUIDSIMULATION_STORAGEBUFFER_H