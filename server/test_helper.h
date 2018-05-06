#ifndef TEST_HELPER_H
#define TEST_HELPER_H

#include <string>

using namespace std;

class When {
    int *lvalue_int;
    string lvalue_str;
    
public:
    When(int &nb);
    When(string &str);
    When &IsSetTo(int rvalue);
    When &IsSetTo(string rvalue);
};

class Then {
    int lvalue_int;
    string lvalue_str;
    bool op;
    string op_err_msg;
    
public:
    Then(int nb);
    Then(string str);
    Then &Should();
    Then &Not();
    Then &BeEqualTo(int rvalue);
    Then &BeEqualTo(string rvalue);
};

#endif /* TEST_HELPER_H */
