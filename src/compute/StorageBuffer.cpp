#include "compute/StorageBuffer.h"
#include "glad/gl.h"

StorageBuffer::StorageBuffer(const unsigned int size) : _size(size){
    glGenBuffers(1, &_id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    constexpr float zero = 0.0f;
    glClearBufferData(GL_SHADER_STORAGE_BUFFER, GL_R32F, GL_RED, GL_FLOAT, &zero);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

StorageBuffer::~StorageBuffer() {
    glDeleteBuffers(1, &_id);
}

void StorageBuffer::bind(const int point) const {
    //glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, point, _id);
}

void StorageBuffer::upload(const void *data, unsigned int bytes) const {
    if (bytes == 0) bytes = _size;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bytes, data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void StorageBuffer::download(void *data, unsigned int bytes) const {
    if (bytes == 0) bytes = _size;
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, bytes, data);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void StorageBuffer::upload(ScalField2D *field, const unsigned int bytes) const {
    upload(field->data(), bytes);
}

void StorageBuffer::download(ScalField2D *field, const unsigned int bytes) const {
    download(field->data(), bytes);
}

void StorageBuffer::upload(VecField2D *field, const unsigned int bytes) const {
    upload(field->data(), bytes);
}

void StorageBuffer::download(VecField2D *field, const unsigned int bytes) const {
    download(field->data(), bytes);
}

