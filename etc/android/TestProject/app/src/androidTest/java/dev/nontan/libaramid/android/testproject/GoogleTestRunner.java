package dev.nontan.libaramid.android.testproject;

import org.junit.runner.Description;
import org.junit.runner.Runner;
import org.junit.runner.notification.RunNotifier;

public final class GoogleTestRunner extends Runner {
    static {
        System.loadLibrary("aramid_test_jni");
        initialize(new String[0]);
    }

    private Class testClass;

    public GoogleTestRunner(Class testClass) {
        this.testClass = testClass;
    }

    @Override
    public Description getDescription() {
        Description rootDescription = Description.createSuiteDescription(testClass.getName());

        int numTestSuites = getNumTestSuites();
        for (int testSuiteIndex = 0; testSuiteIndex < numTestSuites; testSuiteIndex++) {
            String testSuiteName = getTestSuiteName(testSuiteIndex);

            int numTestCases = getNumTestCases(testSuiteIndex);
            for (int testCaseIndex = 0; testCaseIndex < numTestCases; testCaseIndex++) {
                String testCaseName = getTestCaseName(testSuiteIndex, testCaseIndex);

                Description caseDescription = Description.createTestDescription(testSuiteName, testCaseName);
                rootDescription.addChild(caseDescription);
            }
        }

        return rootDescription;
    }

    @Override
    public void run(RunNotifier notifier) {
        Description description = getDescription();

        runAllTests(new GoogleTestEventListener(notifier, description));
    }

    private native static int initialize(String[] args);
    private native static int getNumTestSuites();
    private native static String getTestSuiteName(int testSuiteIndex);
    private native static int getNumTestCases(int testSuiteIndex);
    private native static String getTestCaseName(int testSuiteIndex, int testCaseIndex);
    private native static int runAllTests(GoogleTestEventListener listener);
}
