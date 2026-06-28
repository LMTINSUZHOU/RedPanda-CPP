#include "test_problemcasevalidator.h"

#include "../src/problems/problemcasevalidator.h"

#include <QDebug>

namespace {

bool reportFailure(const char *file, int line, const QString &message)
{
    qCritical().noquote() << QString("%1:%2: %3").arg(file).arg(line).arg(message);
    return false;
}

#define CHECK_TRUE(expr) \
    do { \
        if (!(expr)) \
            return reportFailure(__FILE__, __LINE__, QStringLiteral("Expected true: %1").arg(QStringLiteral(#expr))); \
    } while (false)

#define CHECK_FALSE(expr) \
    do { \
        if (expr) \
            return reportFailure(__FILE__, __LINE__, QStringLiteral("Expected false: %1").arg(QStringLiteral(#expr))); \
    } while (false)

#define CHECK_EQ(actual, expected) \
    do { \
        const auto actualValue = (actual); \
        const auto expectedValue = (expected); \
        if (!(actualValue == expectedValue)) \
            return reportFailure(__FILE__, __LINE__, \
                QStringLiteral("Expected %1 == %2, got %3 and %4") \
                    .arg(QStringLiteral(#actual), QStringLiteral(#expected)) \
                    .arg(actualValue).arg(expectedValue)); \
    } while (false)

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

bool TestProblemCaseValidator::run()
{
    return exactComparisonKeepsExistingBehavior()
        && ignoreSpacesComparisonKeepsExistingBehavior()
        && customSpjAcceptsCheckerSuccess()
        && customSpjRejectsCheckerFailure()
        && customSpjRejectsMissingChecker();
}

bool TestProblemCaseValidator::exactComparisonKeepsExistingBehavior()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("", "hello\nworld\n", "hello\nworld\n");
    CHECK_TRUE(validator.validate(problemCase, ProblemCaseValidateType::Exact));
    CHECK_EQ(problemCase->firstDiffLine, -1);

    problemCase = makeCase("", "hello\nworld\n", "hello\nWorld\n");
    CHECK_FALSE(validator.validate(problemCase, ProblemCaseValidateType::Exact));
    CHECK_EQ(problemCase->firstDiffLine, 1);

    return true;
}

bool TestProblemCaseValidator::ignoreSpacesComparisonKeepsExistingBehavior()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("", "1   2\n3\t4\n", "1 2\n3 4\n");
    CHECK_TRUE(validator.validate(problemCase, ProblemCaseValidateType::IgnoreSpaces));

    return true;
}

bool TestProblemCaseValidator::customSpjAcceptsCheckerSuccess()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("unused\n", "42\n", "40 45\n");
    CHECK_TRUE(validator.validate(problemCase, ProblemCaseValidateType::CustomSPJ, checkerPath()));

    return true;
}

bool TestProblemCaseValidator::customSpjRejectsCheckerFailure()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("unused\n", "50\n", "40 45\n");
    CHECK_FALSE(validator.validate(problemCase, ProblemCaseValidateType::CustomSPJ, checkerPath()));

    return true;
}

bool TestProblemCaseValidator::customSpjRejectsMissingChecker()
{
    ProblemCaseValidator validator;

    POJProblemCase problemCase = makeCase("unused\n", "42\n", "40 45\n");
    CHECK_FALSE(validator.validate(problemCase, ProblemCaseValidateType::CustomSPJ, QString()));

    return true;
}
