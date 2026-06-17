#include "compute/StorageBuffer.h"

#include <GL/glew.h>

StorageBuffer::StorageBuffer(const unsigned int size) : _size(size){
    glGenBuffers(1, &_id);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, _id);
    glBufferData(GL_SHADER_STORAGE_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
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

