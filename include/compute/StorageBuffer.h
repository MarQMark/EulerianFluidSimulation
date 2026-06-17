#ifndef EULERIANFLUIDSIMULATION_STORAGEBUFFER_H
#define EULERIANFLUIDSIMULATION_STORAGEBUFFER_H


class StorageBuffer {
public:
    StorageBuffer(unsigned int size);
    ~StorageBuffer();

    void bind(const int point) const;

    void upload (const void* data, unsigned int bytes = 0) const;
    void download(void* data, unsigned int bytes = 0) const;

    unsigned int id() const { return _id; }

private:
    unsigned int _id;
    const unsigned int _size;
};


#endif //EULERIANFLUIDSIMULATION_STORAGEBUFFER_H