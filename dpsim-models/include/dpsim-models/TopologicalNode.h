/* Copyright 2017-2021 Institute for Automation of Complex Power Systems,
 *                     EONERC, RWTH Aachen University
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *********************************************************************************/

#pragma once

#include <dpsim-models/PtrFactory.h>
#include <dpsim-models/IdentifiedObject.h>
#include <dpsim-models/MathUtils.h>

namespace CPS {

	class TopologicalNode : public IdentifiedObject {
	protected:
		PhaseType mPhaseType = PhaseType::Single;
		Bool mIsGround = false;
	public:
		typedef std::shared_ptr<TopologicalNode> Ptr;
		typedef std::vector<Ptr> List;

		const Attribute<MatrixComp>::Ptr mInitialVoltage;

		TopologicalNode() { }
		/// This very general constructor is used by other constructors.
		TopologicalNode(String uid, String name,
			PhaseType phaseType, const std::vector<Complex> &initialVoltage);
		///
		virtual ~TopologicalNode() { }

		///
		Bool isGround();
		///
		MatrixComp initialVoltage();
		///
		void setInitialVoltage(MatrixComp voltage);
		///
		void setInitialVoltage(Complex voltage);
		///
		void setInitialVoltage(Complex voltage, Int phaseIndex);
		///
		Complex initialSingleVoltage(PhaseType phaseType = PhaseType::Single);
		///
		PhaseType phaseType();
		///
		virtual UInt matrixNodeIndex(PhaseType phaseType = PhaseType::Single) = 0;
		///
		virtual std::vector<UInt> matrixNodeIndices() = 0;
		///
		virtual void setMatrixNodeIndex(UInt phase, UInt matrixNodeIndex) = 0;
	};
}
