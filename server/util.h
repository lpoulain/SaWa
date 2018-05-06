#ifndef UTIL_H
#define UTIL_H

#include <string>

class Util {
    static int debug_int;
    static std::string debug_str;

public:
    static void setDebugInfo(int info);
    static void setDebugInfo(std::string info);
    static void displayError(int failure_code);
    static void dumpMem(uint8_t *addr, int size);
    static int strnCaseStr(const char *s, const char *find, const int max);
};

#endif /* UTIL_H */
