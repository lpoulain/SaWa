#ifndef UTIL_H
#define UTIL_H

class Util {
    static int debug_int;

public:
    static void setDebugInfo(int info);
    static void displayError(int failure_code);
    static void dumpMem(unsigned char *addr, int size);
    static int strnCaseStr(const char *s, const char *find, const int max);
};

#endif /* UTIL_H */
