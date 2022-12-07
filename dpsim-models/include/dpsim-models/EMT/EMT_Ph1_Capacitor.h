/* Copyright 2017-2021 Institute for Automation of Complex Power Systems,
 *                     EONERC, RWTH Aachen University
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 *********************************************************************************/

#pragma once

#include <dpsim-models/MNASimPowerComp.h>
#include <dpsim-models/Solver/MNAInterface.h>
#include <dpsim-models/Base/Base_Ph1_Capacitor.h>

namespace CPS {
namespace EMT {
namespace Ph1 {
	/// \brief Capacitor model
	///
	/// The capacitor is represented by a DC equivalent circuit which corresponds to one
	/// iteration of the trapezoidal integration method.
	/// The equivalent DC circuit is a resistance in paralel with a current source.
	/// The resistance is constant for a defined time step and system
	///frequency and the current source changes for each iteration.
	class Capacitor :
		public MNASimPowerComp<Real>,
		public Base::Ph1::Capacitor,
		public SharedFactory<Capacitor> {
	protected:
		/// DC equivalent current source [A]
		Real mEquivCurrent;
		/// Equivalent conductance [S]
		Real mEquivCond;
	public:
		/// Defines UID, name and logging level
		Capacitor(String uid, String name, Logger::Level logLevel = Logger::Level::off);
		/// Defines name and logging level
		Capacitor(String name, Logger::Level logLevel = Logger::Level::off)
			: Capacitor(name, name, logLevel) { }

		SimPowerComp<Real>::Ptr clone(String name);

		// #### General ####
		/// Initializes component from power flow data
		void initializeFromNodesAndTerminals(Real frequency);

		// #### MNA section ####
		/// Initializes internal variables of the component
		void mnaInitialize(Real omega, Real timeStep, Attribute<Matrix>::Ptr leftVector);
		/// Stamps system matrix
		void mnaApplySystemMatrixStamp(Matrix& systemMatrix);
		/// Stamps right side (source) vector
		void mnaApplyRightSideVectorStamp(Matrix& rightVector);
		/// Update interface voltage from MNA system result
		void mnaUpdateVoltage(const Matrix& leftVector);
		/// Update interface current from MNA system result
		void mnaUpdateCurrent(const Matrix& leftVector);

		void mnaPreStep(Real time, Int timeStepCount) override;
		void mnaPostStep(Real time, Int timeStepCount, Attribute<Matrix>::Ptr &leftVector) override;

		/// Add MNA pre step dependencies
		void mnaAddPreStepDependencies(AttributeBase::List &prevStepDependencies, AttributeBase::List &attributeDependencies, AttributeBase::List &modifiedAttributes) override;
		/// Add MNA post step dependencies
		void mnaAddPostStepDependencies(AttributeBase::List &prevStepDependencies, AttributeBase::List &attributeDependencies, AttributeBase::List &modifiedAttributes, Attribute<Matrix>::Ptr &leftVector) override;
	};
}
}
}
