#include <jni.h>
#include <string>
#include <vector>

#include <aramid/library_main.hpp>

#include <gtest/gtest.h>

namespace {

class JUnit4Listener : public testing::EmptyTestEventListener {
public:
    JUnit4Listener(JNIEnv *env_, jobject listener_)
        : env(env_), listener(listener_) {
        klass = env->GetObjectClass(listener);
    }

    ~JUnit4Listener() { env->DeleteLocalRef(klass); }

    void OnTestSuiteStart(const testing::TestSuite &test_suite) override {
        current_test_suite = test_suite.name();
        auto test_suite_name = env->NewStringUTF(current_test_suite.c_str());
        jmethodID method_id = env->GetMethodID(klass, "onTestSuiteStart",
                                               "(Ljava/lang/String;)V");
        env->CallVoidMethod(listener, method_id, test_suite_name);
        env->DeleteLocalRef(test_suite_name);
    }

    void OnTestStart(const testing::TestInfo &test_info) override {
        current_test_case = test_info.name();
        auto test_suite_name = env->NewStringUTF(current_test_suite.c_str());
        auto test_case_name = env->NewStringUTF(current_test_case.c_str());
        jmethodID method_id =
            env->GetMethodID(klass, "onTestCaseStart",
                             "(Ljava/lang/String;Ljava/lang/String;)V");
        env->CallVoidMethod(listener, method_id, test_suite_name,
                            test_case_name);
        env->DeleteLocalRef(test_suite_name);
        env->DeleteLocalRef(test_case_name);
    }

    void OnTestPartResult(
        const ::testing::TestPartResult &test_part_result) override {
        if (test_part_result.passed()) {
            auto test_suite_name =
                env->NewStringUTF(current_test_suite.c_str());
            auto test_case_name = env->NewStringUTF(current_test_case.c_str());
            jmethodID method_id =
                env->GetMethodID(klass, "onTestPartPassed",
                                 "(Ljava/lang/String;Ljava/lang/String;)V");
            env->CallVoidMethod(listener, method_id, test_suite_name,
                                test_case_name);
            env->DeleteLocalRef(test_suite_name);
            env->DeleteLocalRef(test_case_name);
            return;
        }

        // failed
        {
            auto test_suite_name =
                env->NewStringUTF(current_test_suite.c_str());
            auto test_case_name = env->NewStringUTF(current_test_case.c_str());
            auto file_name = env->NewStringUTF(test_part_result.file_name());
            int line_number = test_part_result.line_number();
            auto message = env->NewStringUTF(test_part_result.message());
            jmethodID method_id =
                env->GetMethodID(klass, "onTestPartFailed",
                                 "(Ljava/lang/String;Ljava/lang/String;Ljava/"
                                 "lang/String;ILjava/lang/String;)V");
            env->CallVoidMethod(listener, method_id, test_suite_name,
                                test_case_name, file_name, line_number,
                                message);
            env->DeleteLocalRef(test_suite_name);
            env->DeleteLocalRef(test_case_name);
            env->DeleteLocalRef(file_name);
            env->DeleteLocalRef(message);
        }
    }

    void OnTestEnd(const testing::TestInfo &test_info) override {
        (void)test_info;

        auto test_suite_name = env->NewStringUTF(current_test_suite.c_str());
        auto test_case_name = env->NewStringUTF(current_test_case.c_str());
        jmethodID method_id = env->GetMethodID(
            klass, "onTestPartEnd", "(Ljava/lang/String;Ljava/lang/String;)V");
        env->CallVoidMethod(listener, method_id, test_suite_name,
                            test_case_name);
        env->DeleteLocalRef(test_suite_name);
        env->DeleteLocalRef(test_case_name);

        current_test_case = "";
    }

    void OnTestSuiteEnd(const testing::TestSuite &test_suite) override {
        (void)test_suite;

        auto test_suite_name = env->NewStringUTF(current_test_suite.c_str());
        jmethodID method_id =
            env->GetMethodID(klass, "onTestSuiteEnd", "(Ljava/lang/String;)V");
        env->CallVoidMethod(listener, method_id, test_suite_name);
        env->DeleteLocalRef(test_suite_name);

        current_test_suite = "";
    }

private:
    JNIEnv *env;
    jobject listener;
    jclass klass;
    std::string current_test_suite;
    std::string current_test_case;
};

} // namespace

extern "C" JNIEXPORT jint JNICALL
Java_dev_nontan_libaramid_android_testproject_GoogleTestRunner_initialize(
    JNIEnv *env, jclass klass, jobjectArray args) {
    (void)klass;

    auto stringClass = env->FindClass("java/lang/String");
    auto arrayLength = env->GetArrayLength(args);
    for (jsize i = 0; i < arrayLength; i++) {
        auto arg = env->GetObjectArrayElement(args, i);
        if (arg == nullptr) {
            env->DeleteLocalRef(stringClass);
            return -1;
        }

        if (!env->IsInstanceOf(arg, stringClass)) {
            env->DeleteLocalRef(stringClass);
            env->DeleteLocalRef(arg);
            return -1;
        }

        env->DeleteLocalRef(arg);
    }
    env->DeleteLocalRef(stringClass);

    std::vector<char *> cpp_args;
    for (jsize i = 0; i < arrayLength; i++) {
        auto arg =
            reinterpret_cast<jstring>(env->GetObjectArrayElement(args, i));
        if (arg == nullptr) {
            env->DeleteLocalRef(stringClass);
            return -1;
        }

        auto length = env->GetStringUTFLength(arg);
        auto utf_chars = env->GetStringUTFChars(arg, nullptr);

        auto buf = new char[length + 1];
        memcpy(buf, utf_chars, static_cast<size_t>(length));
        cpp_args.push_back(buf);

        env->ReleaseStringUTFChars(arg, utf_chars);
        env->DeleteLocalRef(arg);
    }

    aramid::test::initialize(arrayLength, cpp_args.data());

    for (auto cpp_arg : cpp_args) {
        delete[] cpp_arg;
    }
    cpp_args.clear();

    return 0;
}

extern "C" JNIEXPORT jint JNICALL
Java_dev_nontan_libaramid_android_testproject_GoogleTestRunner_getNumTestSuites(
    JNIEnv *env, jclass klass) {
    (void)env;
    (void)klass;

    return aramid::test::get_num_test_suites();
}

extern "C" JNIEXPORT jstring JNICALL
Java_dev_nontan_libaramid_android_testproject_GoogleTestRunner_getTestSuiteName(
    JNIEnv *env, jclass klass, jint testSuiteIndex) {
    (void)klass;

    auto cpp_name = aramid::test::get_test_suite_name(testSuiteIndex);
    return env->NewStringUTF(cpp_name.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_dev_nontan_libaramid_android_testproject_GoogleTestRunner_getNumTestCases(
    JNIEnv *env, jclass klass, jint testSuiteIndex) {
    (void)env;
    (void)klass;

    return aramid::test::get_num_test_cases(testSuiteIndex);
}

extern "C" JNIEXPORT jstring JNICALL
Java_dev_nontan_libaramid_android_testproject_GoogleTestRunner_getTestCaseName(
    JNIEnv *env, jclass klass, jint testSuiteIndex, jint testCaseIndex) {
    (void)klass;

    auto cpp_name =
        aramid::test::get_test_case_name(testSuiteIndex, testCaseIndex);
    return env->NewStringUTF(cpp_name.c_str());
}

extern "C" JNIEXPORT jint JNICALL
Java_dev_nontan_libaramid_android_testproject_GoogleTestRunner_runAllTests(
    JNIEnv *env, jclass klass, jobject listener) {
    (void)klass;

    JUnit4Listener junit4_listener(env, listener);
    return aramid::test::run_all_tests(junit4_listener);
}
