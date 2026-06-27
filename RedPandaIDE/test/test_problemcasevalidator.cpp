#include "test_problemcasevalidator.h"

#include "../src/problems/problemcasevalidator.h"

#include <QTest>

namespace {

POJProblemCase makeCase(const QString& input, const QString& output, const QString& expected)
{
    POJProblemCase problemCase = std::make_shared<OJProblemCase>();
    problemCase->setInput(input);
    problemCase->output = output;
    problemCase->setExpected(expected);
    return problemCase;
}

QString checkerPath()
{
    return QString::fromLocal8Bit(TEST_SPJ_CHECKER_PATH);
}

}

void TestProblemCaseValidator::exactComparisonKeepsExistingBehavior()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("", "hello\nworld\n", "hello\nworld\n");
    QVERIFY(validator.validate(problemCase, ProblemCaseValidateType::Exact));
    QCOMPARE(problemCase->firstDiffLine, -1);

    problemCase = makeCase("", "hello\nworld\n", "hello\nWorld\n");
    QVERIFY(!validator.validate(problemCase, ProblemCaseValidateType::Exact));
    QCOMPARE(problemCase->firstDiffLine, 1);
}

void TestProblemCaseValidator::ignoreSpacesComparisonKeepsExistingBehavior()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("", "1   2\n3\t4\n", "1 2\n3 4\n");
    QVERIFY(validator.validate(problemCase, ProblemCaseValidateType::IgnoreSpaces));
}

void TestProblemCaseValidator::customSpjAcceptsCheckerSuccess()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("unused\n", "42\n", "40 45\n");
    QVERIFY(validator.validate(problemCase, ProblemCaseValidateType::CustomSPJ, checkerPath()));
}

void TestProblemCaseValidator::customSpjRejectsCheckerFailure()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("unused\n", "50\n", "40 45\n");
    QVERIFY(!validator.validate(problemCase, ProblemCaseValidateType::CustomSPJ, checkerPath()));
}

void TestProblemCaseValidator::customSpjRejectsMissingChecker()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("unused\n", "42\n", "40 45\n");
    QVERIFY(!validator.validate(problemCase, ProblemCaseValidateType::CustomSPJ, QString()));
}
