#include <QCoreApplication>
#include <QTest>

#include "test_problemcasevalidator.h"

int main(int argc, char *argv[])
{
    int status = 0;
    QTest::setMainSourcePath(__FILE__, QT_TESTCASE_BUILDDIR);

    QCoreApplication app(argc, argv);
    {
        TestProblemCaseValidator tc;
        status |= QTest::qExec(&tc, argc, argv);
    }

    return status;
}
