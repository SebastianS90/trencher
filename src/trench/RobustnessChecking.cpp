#include "RobustnessChecking.h"

#include <atomic>

#include <boost/threadpool.hpp>

#include "Foreach.h"
#include "Program.h"
#include "Reduction.h"
#include "SpinModelChecker.h"

namespace trench {

bool checkIsRobust(Program &program) {
	trench::Program augmentedProgram;
	trench::reduce(program, augmentedProgram);

	trench::SpinModelChecker checker;
	return !checker.check(augmentedProgram);
}

namespace {

class AttackChecker {
	Program &program_;
	Thread *attacker_;
	std::atomic<bool> &attackFound_;

	public:

	AttackChecker(Program &program, Thread *attacker, std::atomic<bool> &attackFound):
		program_(program), attacker_(attacker), attackFound_(attackFound)
	{}

	void operator()() {
		trench::Program augmentedProgram;
		trench::reduce(program_, augmentedProgram, attacker_);

		trench::SpinModelChecker checker;
		if (checker.check(augmentedProgram)) {
			attackFound_ = true;
		}
	}
};

} // anonymous namespace

bool checkIsRobustParallel(Program &program) {
	boost::threadpool::pool pool(boost::thread::hardware_concurrency());

	std::atomic<bool> attackFound(false);
	foreach (Thread *attacker, program.threads()) {
		pool.schedule(AttackChecker(program, attacker, attackFound));
	}

	std::size_t tasksLeft;
	while (((tasksLeft = pool.active() + pool.pending()) > 0) && !attackFound) {
		pool.wait(tasksLeft - 1);
	}
	if (attackFound) {
		pool.clear();
	}
	pool.wait();

	return !attackFound;
}

} // namespace trench
