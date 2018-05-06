#include <iostream>
#include <string>
#include "test_helper.h"

using namespace std;

When::When(int &nb) {
    lvalue_int = &nb;
}

When::When(string &str) {
    lvalue_str = str;
}

When &When::IsSetTo(int rvalue) {
    *lvalue_int = rvalue;
    return *this;
}

When &When::IsSetTo(string rvalue) {
    lvalue_str = rvalue;
    return *this;
}

Then::Then(int nb) {
    op = true;
    op_err_msg = " != ";
    lvalue_int = nb;
}
    
Then &Then::Should() {
    return *this;
}
    
Then &Then::Not() {
    op = false;
    op_err_msg = " == ";
    return *this;
}
    
Then &Then::BeEqualTo(int rvalue) {
    if (((this->lvalue_int != rvalue) && op) ||
        ((this->lvalue_int == rvalue) && !op)) {
        cout << "Error: " << this->lvalue_int << " (found) " << op_err_msg << rvalue << " (expected)" << endl;
    }
    else {
        cout << "PASS" << endl;
    }
    return *this;
}

Then &Then::BeEqualTo(string rvalue) {
    if (((this->lvalue_str.compare(rvalue) != 0) && op) ||
        ((this->lvalue_str.compare(rvalue) == 0) && !op)) {
        cout << "Error: " << this->lvalue_str << op_err_msg << rvalue << endl;
    }
    else {
        cout << "PASS" << endl;
    }
    return *this;
}
