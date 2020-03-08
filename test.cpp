#include "log.h"
#include <string>


#define DEBUG LOG_TYPE::LOG_DEBUG
#define INFO LOG_TYPE::LOG_INFO
#define WARN LOG_TYPE::LOG_WARN
#define ERROR LOG_TYPE::LOG_ERROR

using std::string;

void test_log() {
    string str{"test log function"};
    int n = 100;
    float f = 3.14;

    LOG(ERROR, "%s %d %f", str.c_str(), n, f);
}



int main() {
    test_log();
}