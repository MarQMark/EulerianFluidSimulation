#ifndef EULERIANFLUIDSIMULATION_COMPUTESHADER_H
#define EULERIANFLUIDSIMULATION_COMPUTESHADER_H
#include <string>


class ComputeShader {
public:
    ComputeShader(const std::string& code);

    void bind() const;

    void dispatch (int gx, int gy = 1, int gz = 1) const;

    void setInt   (const std::string& name, int   v) const;
    void setFloat (const std::string& name, float v) const;

    static std::string LoadTextFile(const std::string& path);

private:
    unsigned int _id;
};


#endif //EULERIANFLUIDSIMULATION_COMPUTESHADER_H