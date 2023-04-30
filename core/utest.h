#define UTEST_ENABLE
#pragma once

#define TEST_SUCCESS EXIT_SUCCESS
#define TEST_FAILURE EXIT_FAILURE
#define PASS() return TEST_SUCCESS
#define FAIL() return TEST_FAILURE
#define TEST_FUNC(n) int test_##n(void)

#ifdef UTEST_ENABLE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

typedef int (*test_func_t)(void);

struct utest {
	const char *name;
	test_func_t func;
};

#define TEST(n)	TEST_FUNC(n); TEST_FUNC(n)

#define ASSERT(e, ...)							\
	do if (!(e)) {							\
		fprintf(stderr, "FAIL %s: %s:%u `%s`\n",		\
			__func__ +5, __FILE__, __LINE__, #e);		\
		FAIL();							\
	} while (0);

static inline int
utest_run(const struct utest *t)
{
	pid_t pid;
	int wstatus;
	int ret = TEST_FAILURE;

	pid = fork();
	if (pid == -1) {
		fprintf(stderr, "FAIL %s: fork %s\n", t->name, strerror(errno));
		return ret;
	}
	if (pid == 0) {
		ret = t->func();
		exit(ret);
	}
	/* pid > 0 */
	if (waitpid(pid, &wstatus, 0) == -1) {
		fprintf(stderr, "FAIL %s: wait %s\n", t->name, strerror(errno));
	} else if (WIFEXITED(wstatus)) {
		ret = WEXITSTATUS(wstatus);
		fprintf(stderr, "%s %s\n", ret == TEST_SUCCESS ? "PASS" : "FAIL", t->name);
	} else if (WIFSIGNALED(wstatus)) {
		fprintf(stderr, "FAIL %s: killed by signal %d\n", t->name, WTERMSIG(wstatus));
	}
	fflush(stderr);

	return ret;
}

static inline int
utest_match(const struct utest *t, size_t count, char **names)
{
	size_t i;
	for (i = 0; i < count; i++) {
		if (strstr(t->name, names[i]) != NULL)
			return 1;
	}
	return count == 0 ? 1 : 0;
}

static inline int
utest_main(int argc, char **argv)
{
	extern const struct utest tests[];
	extern const size_t tests_count;
	const struct utest *t;
	size_t count = 0, fails = 0;

	for (t = tests; t < &tests[tests_count]; t++) {
		if (!utest_match(t, argc, argv))
			continue;
		count++;
		if (utest_run(t) != TEST_SUCCESS)
			fails++;
	}

	fprintf(stderr, "TEST %zu / %zu\n", count - fails, count);
	return fails != 0;
}

#else
#define TEST(n) static TEST_FUNC(n)
#define utest_run() (TEST_FAILURE)
#define utest_main(...) (1)
#define ASSERT(...)
#endif
