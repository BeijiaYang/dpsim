/** Reference Circuits
 *
 * @author Markus Mirz <mmirz@eonerc.rwth-aachen.de>
 * @copyright 2017, Institute for Automation of Complex Power Systems, EONERC
 * @license GNU General Public License (version 3)
 *
 * DPsim
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *********************************************************************************/

#include "Simulation.h"
#include "Utilities.h"

using namespace DPsim;

int main() {
	// Define simulation scenario
	Real timeStep = 0.001;
	Real omega = 2.0*M_PI*50.0;
	Real finalTime = 0.3;
	std::ostringstream fileName;
	fileName << "SimulationExampleIdealVS3_" << timeStep;
	BaseComponent::List circElements;
	circElements.push_back(std::make_shared<IdealVoltageSource>("v_1", 1, 0, Complex(10, 0)));
	circElements.push_back(std::make_shared<ResistorDP>("r_1", 1, 2, 1));
	circElements.push_back(std::make_shared<ResistorDP>("r_2", 2, 0, 1));
	circElements.push_back(std::make_shared<ResistorDP>("r_3", 2, 3, 1));
	circElements.push_back(std::make_shared<ResistorDP>("r_4", 3, 0, 1));
	circElements.push_back(std::make_shared<ResistorDP>("r_5", 3, 4, 1));
	circElements.push_back(std::make_shared<IdealVoltageSource>("v_2", 4, 0, Complex(20, 0)));

	// Define log names
	Logger log("Logs/" + fileName.str() + ".log");
	Logger leftVectorLog("Logs/LeftVector_" + fileName.str() + ".csv");
	Logger rightVectorLog("Logs/RightVector_" + fileName.str() + ".csv");

	// Set up simulation and start main simulation loop
	Simulation newSim(circElements, omega, timeStep, finalTime, log);

	std::cout << "Start simulation." << std::endl;

	while (newSim.step(leftVectorLog, rightVectorLog)) {
		newSim.increaseByTimeStep();
		updateProgressBar(newSim.getTime(), newSim.getFinalTime());
	}

	std::cout << "Simulation finished." << std::endl;

	return 0;
}
