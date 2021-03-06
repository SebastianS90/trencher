/*
 * ----------------------------------------------------------------------------
 * "THE JUICE-WARE LICENSE" (Revision 42):
 * <derevenetc@cs.uni-kl.de> wrote this file. As long as you retain this notice
 * you can do whatever you want with this stuff. If we meet some day, and you
 * think this stuff is worth it, you can buy me a glass of juice in return.
 * ----------------------------------------------------------------------------
 */

#include "Census.h"

#include <algorithm>
#include <cassert>

#include "Program.h"
#include "SortAndUnique.h"
#include "Thread.h"
#include "Transition.h"

namespace trench {

void Census::visit(const Program &program) {
	for (Thread *thread : program.threads()) {
		visit(thread);
	}
}

void Census::visit(const Thread *thread) {
	for (Transition *transition : thread->transitions()) {
		visit(transition->instruction());
	}
}

void Census::visit(const std::shared_ptr<Instruction> &instruction) {
	instructions_.push_back(instruction.get());
	unique_ = false;

	switch (instruction->mnemonic()) {
		case Instruction::READ: {
			Read *read = instruction->as<Read>();
			visit(read->reg());
			visit(read->address());
			spaces_.push_back(read->space());
			break;
		}
		case Instruction::WRITE: {
			Write *write = instruction->as<Write>();
			visit(write->value());
			visit(write->address());
			spaces_.push_back(write->space());
			break;
		}
		case Instruction::MFENCE: {
			break;
		}
		case Instruction::LOCAL: {
			Local *local = instruction->as<Local>();
			visit(local->reg());
			visit(local->value());
			break;
		}
		case Instruction::CONDITION: {
			Condition *condition = instruction->as<Condition>();
			visit(condition->expression());
			break;
		}
		case Instruction::ATOMIC: {
			Atomic *atomic = instruction->as<Atomic>();
			for (const auto &instr : atomic->instructions()) {
				visit(instr);
			}
			break;
		}
		case Instruction::NOOP: /* FALLTHROUGH */
		case Instruction::LOCK: /* FALLTHROUGH */
		case Instruction::UNLOCK:
			break;
		default: {
			assert(!"NEVER REACHED");
		}
	}
}

void Census::visit(const std::shared_ptr<Expression> &expression) {
	expressions_.push_back(expression.get());
	unique_ = false;

	if (UnaryOperator *unary = expression->as<UnaryOperator>()) {
		visit(unary->operand());
	} else if (BinaryOperator *binary = expression->as<BinaryOperator>()) {
		visit(binary->left());
		visit(binary->right());
	}
}

void Census::unique() const {
	if (unique_) {
		return;
	}

	sortAndUnique(instructions_);
	sortAndUnique(expressions_);
	sortAndUnique(spaces_);

	unique_ = true;
}

} // namespace trench
