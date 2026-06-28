#include <QCoreApplication>

#include "test_problemcasevalidator.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    TestProblemCaseValidator tc;
    return tc.run() ? 0 : 1;
}
