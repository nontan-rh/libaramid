package dev.nontan.libaramid.android.testproject;

import org.junit.runner.Description;
import org.junit.runner.notification.Failure;
import org.junit.runner.notification.RunNotifier;

import java.util.HashMap;

final class GoogleTestEventListener {
    private RunNotifier runNotifier;
    private HashMap<String, Description> caseDescriptions;

    GoogleTestEventListener(RunNotifier runNotifier, Description description) {
        this.runNotifier = runNotifier;
        this.caseDescriptions = new HashMap<>();

        for (Description caseDescription: description.getChildren()) {
            String suiteName = caseDescription.getClassName();
            String caseName = caseDescription.getMethodName();
            addCaseDescription(suiteName, caseName, caseDescription);
        }
    }

    public void onTestSuiteStart(String testSuiteName) {
    }

    public void onTestCaseStart(String testSuiteName, String testCaseName) {
        Description suiteDescription = getCaseDescription(testSuiteName, testCaseName);
        runNotifier.fireTestStarted(suiteDescription);
    }

    public void onTestPartSuccess(String testSuiteName, String testCaseName) {
    }

    public void onTestPartFailed(String testSuiteName, String testCaseName, String fileName, int lineNo, String message) {
        Description caseDescription = getCaseDescription(testSuiteName, testCaseName);
        Throwable throwable = new Throwable(String.format("Assertion failed at %s:%d\n%s\n", fileName, lineNo, message));
        throwable.setStackTrace(new StackTraceElement[0]);
        Failure failure = new Failure(caseDescription, throwable);
        runNotifier.fireTestFailure(failure);
    }

    public void onTestPartEnd(String testSuiteName, String testCaseName) {
        Description caseDescription = getCaseDescription(testSuiteName, testCaseName);
        runNotifier.fireTestFinished(caseDescription);
    }

    public void onTestSuiteEnd(String testSuiteName) {
    }

    public void onTestCaseEnd(String testSuiteName, String testCaseName) {
        Description caseDescription = getCaseDescription(testSuiteName, testCaseName);
        runNotifier.fireTestFinished(caseDescription);
    }

    private void addCaseDescription(String testSuiteName, String testCaseName, Description description) {
        caseDescriptions.put(combineSuiteAndCase(testSuiteName, testCaseName), description);
    }

    private Description getCaseDescription(String testSuiteName, String testCaseName) {
        return caseDescriptions.get(combineSuiteAndCase(testSuiteName, testCaseName));
    }

    private static String combineSuiteAndCase(String testSuiteName, String testCaseName) {
        return testSuiteName + "." + testCaseName;
    }
}
