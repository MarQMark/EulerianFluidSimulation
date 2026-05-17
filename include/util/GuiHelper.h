#ifndef EULERIANFLUIDSIMULATION_GUIHELPER_H
#define EULERIANFLUIDSIMULATION_GUIHELPER_H
#include <string>

class Util {
public:
    static void AddTextField(const std::string& name, int& val);
    static void AddTextField(const std::string& name, float& val);
    static void AddTextField(const std::string& name, unsigned int& val);

private:
    static void parse_val(float& dest, std::string& src, bool reset = true);
    static void parse_val(int& dest, std::string& src, bool reset = true);
    static void parse_val(unsigned int& dest, std::string& src, bool reset = true);
};

#endif //EULERIANFLUIDSIMULATION_GUIHELPER_H