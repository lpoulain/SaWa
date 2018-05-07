#include <iostream>
#include <string>
#include "test_helper.h"
#include "util.h"
#include "sawa_admin.h"

using namespace std;

void ctrl_c_handler(int s) {}
int debug_flag;

int main() {
    int result;
    
    When(result).IsSetTo(Util::strnCaseStr("This is a test KEEP-ALIVE", "Keep-Alive", 30));
    Then(result).Should().BeEqualTo(1);

    When(result).IsSetTo(Util::strnCaseStr("This is a test KEEP-ALVIE", "Keep-Alive", 30));
    Then(result).Should().Not().BeEqualTo(1);
    
    return 0;
}
