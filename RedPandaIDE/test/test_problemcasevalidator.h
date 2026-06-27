#ifndef TEST_PROBLEMCASEVALIDATOR_H
#define TEST_PROBLEMCASEVALIDATOR_H

#include <QObject>

class TestProblemCaseValidator : public QObject
{
    Q_OBJECT

private slots:
    void exactComparisonKeepsExistingBehavior();
    void ignoreSpacesComparisonKeepsExistingBehavior();
    void customSpjAcceptsCheckerSuccess();
    void customSpjRejectsCheckerFailure();
    void customSpjRejectsMissingChecker();
};

#endif // TEST_PROBLEMCASEVALIDATOR_H
