#ifndef TEST_PROBLEMCASEVALIDATOR_H
#define TEST_PROBLEMCASEVALIDATOR_H

class TestProblemCaseValidator
{
public:
    bool run();

private:
    bool exactComparisonKeepsExistingBehavior();
    bool ignoreSpacesComparisonKeepsExistingBehavior();
    bool customSpjAcceptsCheckerSuccess();
    bool customSpjRejectsCheckerFailure();
    bool customSpjRejectsMissingChecker();
    bool problemSpjCreatesPerProblemSourceAndTestlib();
};

#endif // TEST_PROBLEMCASEVALIDATOR_H
