#include "core/util/thread.h"

#include <algorithm>
#include <future>
#include <thread>

#include <rte_lcore.h>
#include <rte_eal.h>
#include <pthread.h>

#include "core/util/logger.h"

#define PQOS_MAX_SOCKETS	8
#define PQOS_MAX_SOCKET_CORES	64
#define PQOS_MAX_CORES		(PQOS_MAX_SOCKET_CORES * PQOS_MAX_SOCKETS)

static int dump_affinity(rte_cpuset_t *cpuset, char *str, unsigned int size)
{
    unsigned cpu;
    int ret;
    unsigned int out = 0;

    for (cpu = 0; cpu < CPU_SETSIZE; cpu++) {
        if (!CPU_ISSET(cpu, cpuset))
            continue;

        ret = snprintf(str + out,
                   size - out, "%u,", cpu);
        if (ret < 0 || (unsigned)ret >= size - out) {
            /* string will be truncated */
            ret = -1;
            goto exit;
        }

        out += ret;
    }

    ret = 0;
exit:
    /* remove the last separator */
    if (out > 0)
        str[out - 1] = '\0';

    return ret;
}

// dpdk examples/l2fwd-cat/cat.c
/*
 * Parse elem, the elem could be single number/range or '(' ')' group
 * 1) A single number elem, it's just a simple digit. e.g. 9
 * 2) A single range elem, two digits with a '-' between. e.g. 2-6
 * 3) A group elem, combines multiple 1) or 2) with '( )'. e.g (0,2-4,6)
 *    Within group elem, '-' used for a range separator;
 *                       ',' used for a single number.
 */
static int
parse_set(const char *input, rte_cpuset_t *cpusetp)
{
	unsigned idx;
	const char *str = input;
	char *end = NULL;
	unsigned min, max;
	const unsigned num = PQOS_MAX_CORES;

	CPU_ZERO(cpusetp);

	while (isblank(*str))
		str++;

	/* only digit or left bracket is qualify for start point */
	if ((!isdigit(*str) && *str != '(') || *str == '\0')
		return -1;

	/* process single number or single range of number */
	if (*str != '(') {
		errno = 0;
		idx = strtoul(str, &end, 10);

		if (errno || end == NULL || idx >= num)
			return -1;

		while (isblank(*end))
			end++;

		min = idx;
		max = idx;
		if (*end == '-') {
			/* process single <number>-<number> */
			end++;
			while (isblank(*end))
				end++;
			if (!isdigit(*end))
				return -1;

			errno = 0;
			idx = strtoul(end, &end, 10);
			if (errno || end == NULL || idx >= num)
				return -1;
			max = idx;
			while (isblank(*end))
				end++;
			if (*end != ',' && *end != '\0')
				return -1;
		}

		if (*end != ',' && *end != '\0' && *end != '@')
			return -1;

		for (idx = std::min(min, max); idx <= std::max(min, max);
				idx++)
			CPU_SET(idx, cpusetp);

		return end - input;
	}

	/* process set within bracket */
	str++;
	while (isblank(*str))
		str++;
	if (*str == '\0')
		return -1;

	min = PQOS_MAX_CORES;
	do {

		/* go ahead to the first digit */
		while (isblank(*str))
			str++;
		if (!isdigit(*str))
			return -1;

		/* get the digit value */
		errno = 0;
		idx = strtoul(str, &end, 10);
		if (errno || end == NULL || idx >= num)
			return -1;

		/* go ahead to separator '-',',' and ')' */
		while (isblank(*end))
			end++;
		if (*end == '-') {
			if (min == PQOS_MAX_CORES)
				min = idx;
			else /* avoid continuous '-' */
				return -1;
		} else if ((*end == ',') || (*end == ')')) {
			max = idx;
			if (min == PQOS_MAX_CORES)
				min = idx;
			for (idx = std::min(min, max); idx <= std::max(min, max);
					idx++)
				CPU_SET(idx, cpusetp);

			min = PQOS_MAX_CORES;
		} else
			return -1;

		str = end + 1;
	} while (*end != '\0' && *end != ')');

	return str - input;
}

namespace seeder::core::util {

void unbind_lcore() {
    using seeder::core::logger;

    auto thread_name = get_thread_name();
    char old_cpuset_str[32] = "";
    char new_cpuset_str[32] = "";
    rte_cpuset_t cpuset;
    rte_thread_get_affinity(&cpuset);
    dump_affinity(&cpuset, old_cpuset_str, sizeof(old_cpuset_str));
    RTE_CPU_FILL(&cpuset);
    int ret = rte_thread_set_affinity(&cpuset);
    dump_affinity(&cpuset, new_cpuset_str, sizeof(new_cpuset_str));
    if (ret) {
        logger->error("unbind_lcore(): rte_thread_set_affinity() failed, thread={} old_cpuset={} new_cpuset={}", thread_name, old_cpuset_str, new_cpuset_str);
    } else {
        logger->debug("unbind_lcore(): rte_thread_set_affinity() success, thread={} old_cpuset={} new_cpuset={}", thread_name, old_cpuset_str, new_cpuset_str);
    }
}

void thread_set_affinity(const std::string &str) {
    rte_cpuset_t cpuset;
    char new_cpuset_str[32] = "";
    if (parse_set(str.c_str(), &cpuset) < 0 || CPU_COUNT(&cpuset) == 0) {
        logger->error("thread_set_affinity error, invalid argument: {}", str);
        throw std::runtime_error("thread_set_affinity");
    }
    dump_affinity(&cpuset, new_cpuset_str, sizeof(new_cpuset_str));
    auto thread_name = get_thread_name();
    int ret = rte_thread_set_affinity(&cpuset);
    if (ret) {
        logger->error("thread_set_affinity() failed, thread={} new_cpuset={}", thread_name, new_cpuset_str);
        throw std::runtime_error("rte_thread_set_affinity");
    }
    logger->debug("thread_set_affinity() success, thread={} new_cpuset={}", thread_name, new_cpuset_str);
}

void run_with_lcore(const std::string &str, std::function<void()> f) {
    std::packaged_task<void()> task([&] {
        thread_set_affinity(str);
        logger->debug("run_with_lcore {}", str);
        f();
    });
    auto future = task.get_future();
    std::thread tmp_thread(std::move(task));
    tmp_thread.join();
    future.get();
}

void run_with_unbind_lcore(std::function<void()> f) {
   
    std::packaged_task<void()> task([&] {
        rte_cpuset_t cpuset;
        RTE_CPU_FILL(&cpuset);
        int ret = rte_thread_set_affinity(&cpuset);
        if (ret) {
            logger->error("run_with_unbind_lcore(): rte_thread_set_affinity() failed");
        }
        f();
    });
    auto future = task.get_future();
    std::thread tmp_thread(std::move(task));
    tmp_thread.join();
    future.get();
}

std::string get_thread_name() {
    char thread_name[32] = "";
    pthread_t current_thread = pthread_self();
    pthread_getname_np(current_thread, thread_name, sizeof(thread_name));
    return thread_name;
}

void set_thread_name(const std::string &_name) {
    const int NAME_MAX_LEN = 15;
    std::string name = _name;
    if (name.size() > NAME_MAX_LEN) {
        name.erase(name.begin() + NAME_MAX_LEN, name.end());
    }
    pthread_t current_thread = pthread_self();
    int ret = 0;
    if ((ret = pthread_setname_np(current_thread, name.c_str()))) {
        logger->warn("pthread_setname_np() failed: ret={} name={}", ret, name);
    }
}

} // namespace seeder::core::util