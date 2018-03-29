/***********************************************************************************
 * Copyright (c) 2017, UT-Battelle
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of the xacc nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 *   Initial API and implementation - Alex McCaskey
 *
 **********************************************************************************/
#ifndef QUANTUM_GATE_ACCELERATORS_RIGETTI_QUILVISITOR_HPP_
#define QUANTUM_GATE_ACCELERATORS_RIGETTI_QUILVISITOR_HPP_

#include <memory>
#include "AllGateVisitor.hpp"

namespace xacc {
namespace quantum {

/**
 * The QuilVisitor is an InstructionVisitor that visits
 * quantum gate instructions and creates an equivalent
 * Quil string that can be executed by the Rigetti
 * superconducting quantum computer.
 *
 */
class QuilVisitor: public AllGateVisitor {
protected:

	/**
	 * Reference to the Quil string
	 * this visitor is trying to construct
	 */
	std::string quilStr;

	/**
	 * Reference to the classical memory address indices
	 * where measurements are recorded.
	 */
	std::string classicalAddresses;

	std::map<int, int> qubitToClassicalBitIndex;

	std::vector<int> measuredQubits;

	bool includeMeasures = true;

	int numAddresses = 0;

public:

	QuilVisitor() {}
	QuilVisitor(bool measures) : includeMeasures(measures) {}

	/**
	 * Visit hadamard gates
	 */
	void visit(Hadamard& h) {
		quilStr += "H " + std::to_string(h.bits()[0]) + "\n";
	}

	void visit(Identity& i) {
		quilStr += "I " + std::to_string(i.bits()[0]) + "\n";
	}

	void visit(CZ& cz) {
		quilStr += "CZ " + std::to_string(cz.bits()[0]) + " " + std::to_string(cz.bits()[1]) + "\n";
	}

	/**
	 * Visit CNOT gates
	 */
	void visit(CNOT& cn) {
		quilStr += "CNOT " + std::to_string(cn.bits()[0]) + " " + std::to_string(cn.bits()[1]) + "\n";
	}
	/**
	 * Visit X gates
	 */
	void visit(X& x) {
		quilStr += "X " + std::to_string(x.bits()[0]) + "\n";
	}

	/**
	 *
	 */
	void visit(Y& y) {
		quilStr += "Y " + std::to_string(y.bits()[0]) + "\n";
	}

	/**
	 * Visit Z gates
	 */
	void visit(Z& z) {
		quilStr += "Z " + std::to_string(z.bits()[0]) + "\n";
	}

	/**
	 * Visit Measurement gates
	 */
	void visit(Measure& m) {
		if (includeMeasures) {
			int classicalBitIdx = m.getClassicalBitIndex();
			quilStr += "MEASURE " + std::to_string(m.bits()[0]) + " ["
					+ std::to_string(classicalBitIdx) + "]\n";
			classicalAddresses += std::to_string(classicalBitIdx) + ", ";
			numAddresses++;
			qubitToClassicalBitIndex.insert(
					std::make_pair(m.bits()[0], classicalBitIdx));
		} else {
			measuredQubits.push_back(m.bits()[0]);
		}
	}

	/**
	 * Visit Conditional functions
	 */
	void visit(ConditionalFunction& c) {
		auto visitor = std::make_shared<QuilVisitor>();
		auto classicalBitIdx = qubitToClassicalBitIndex[c.getConditionalQubit()];
		quilStr += "JUMP-UNLESS @" + c.name() + " [" + std::to_string(classicalBitIdx) + "]\n";
		for (auto inst : c.getInstructions()) {
			inst->accept(visitor);
		}
		quilStr += visitor->getQuilString();
		quilStr += "LABEL @" + c.name() + "\n";
	}

	void visit(Rx& rx) {
		auto angleStr = boost::lexical_cast<std::string>(rx.getParameter(0));
		quilStr += "RX("
				+ angleStr
				+ ") " + std::to_string(rx.bits()[0]) + "\n";
	}

	void visit(Ry& ry) {
		auto angleStr = boost::lexical_cast<std::string>(ry.getParameter(0));
		quilStr += "RY("
				+ angleStr
				+ ") " + std::to_string(ry.bits()[0]) + "\n";
	}

	void visit(Rz& rz) {
		auto angleStr = boost::lexical_cast<std::string>(rz.getParameter(0));
		quilStr += "RZ("
				+ angleStr
				+ ") " + std::to_string(rz.bits()[0]) + "\n";
	}

	void visit(CPhase& cp) {
		auto angleStr = boost::lexical_cast<std::string>(cp.getParameter(0));
		quilStr += "CPHASE("
				+ angleStr
				+ ") " + std::to_string(cp.bits()[0]) + " " + std::to_string(cp.bits()[1]) + "\n";
	}

	void visit(Swap& s) {
		quilStr += "SWAP " + std::to_string(s.bits()[0]) + " " + std::to_string(s.bits()[1]) + "\n";
	}

	void visit(GateFunction& f) {
		return;
	}
	/**
	 * Return the quil string
	 */
	std::string getQuilString() {
		return quilStr;
	}

	/**
	 * Return the classical measurement indices
	 * as a json int array represented as a string.
	 */
	std::string getClassicalAddresses() {
		auto retStr = classicalAddresses.substr(0, classicalAddresses.size() - 2);
		return "[" + retStr + "]";
	}

	int getNumberOfAddresses() {
		return numAddresses;
	}

	std::vector<int> getMeasuredQubits() {
		return measuredQubits;
	}

	/**
	 * The destructor
	 */
	virtual ~QuilVisitor() {}
};


}
}

#endif
