/* *****************************************************************************
Copyright (c) 2015-2017, The Regents of the University of California (Regents).
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

The views and conclusions contained in the software and documentation are those
of the authors and should not be interpreted as representing official policies,
either expressed or implied, of the FreeBSD Project.

REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED HEREUNDER IS
PROVIDED "AS IS". REGENTS HAS NO OBLIGATION TO PROVIDE MAINTENANCE, SUPPORT,
UPDATES, ENHANCEMENTS, OR MODIFICATIONS.

*************************************************************************** */


// Written: Minjie

// Description: all opensees APIs are defined or declared here
//

#include "OpenSeesCommands.h"
#include <OPS_Globals.h>
#include <elementAPI.h>
#include <UniaxialMaterial.h>
#include <NDMaterial.h>
#include <SectionForceDeformation.h>
#include <SectionRepres.h>
#include <TimeSeries.h>
#include <CrdTransf.h>
#include <Damping.h>
#include <BeamIntegration.h>
#include <NodalLoad.h>
#include <AnalysisModel.h>
#include <PlainHandler.h>
#include <RCM.h>
#include <AMDNumberer.h>
#include <LimitCurve.h>
#include <DamageModel.h>
#include <FrictionModel.h>
#include <HystereticBackbone.h>
#include <StiffnessDegradation.h>
#include <StrengthDegradation.h>
#include <UnloadingRule.h>
#include <YieldSurface_BC.h>
#include <CyclicModel.h>
#include <FileStream.h>
#include <CTestNormUnbalance.h>
#include <NewtonRaphson.h>
#include <TransformationConstraintHandler.h>
#include <Newmark.h>
#include <GimmeMCK.h>
#include <HarmonicSteadyState.h>
#include <ProfileSPDLinSolver.h>
#include <ProfileSPDLinDirectSolver.h>
#include <ProfileSPDLinSOE.h>
#include <SymBandEigenSolver.h>
#include <SymBandEigenSOE.h>
#include <FullGenEigenSolver.h>
#include <FullGenEigenSOE.h>
#include <ArpackSOE.h>
#include <LoadControl.h>
#include <CTestPFEM.h>
#include <PFEMIntegrator.h>
#include <TransientIntegrator.h>
#include <PFEMSolver.h>
#include <PFEMLinSOE.h>
#include <Accelerator.h>
#include <KrylovAccelerator.h>
#include <AcceleratedNewton.h>
#include <RaphsonAccelerator.h>
#include <SecantAccelerator1.h>
#include <SecantAccelerator2.h>
#include <SecantAccelerator3.h>
#include <PeriodicAccelerator.h>
#include <LineSearch.h>
#include <InitialInterpolatedLineSearch.h>
#include <BisectionLineSearch.h>
#include <SecantLineSearch.h>
#include <RegulaFalsiLineSearch.h>
#include <NewtonLineSearch.h>
#include <FileDatastore.h>
#include <Mesh.h>
#include <BackgroundMesh.h>
#ifdef _MUMPS
#include <MumpsSolver.h>
#include <MumpsSOE.h>
#endif

#ifdef _PARALLEL_INTERPRETERS
bool setMPIDSOEFlag = false;

#include <mpi.h>
#include <MPI_MachineBroker.h>
#include <ParallelNumberer.h>
#include <DistributedDisplacementControl.h>
#include <DistributedBandSPDLinSOE.h>
#include <DistributedSparseGenColLinSOE.h>
#include <DistributedSparseGenRowLinSOE.h>
#include <DistributedBandGenLinSOE.h>
#include <DistributedDiagonalSOE.h>
#include <DistributedDiagonalSolver.h>
#include <MPIDiagonalSOE.h>
#include <MPIDiagonalSolver.h>
#define MPIPP_H
#include <DistributedSuperLU.h>
#include <DistributedProfileSPDLinSOE.h>
#ifdef _MUMPS
#include <MumpsParallelSOE.h>
#include <MumpsParallelSolver.h>
#endif
#elif _PARALLEL_PROCESSING
#include <mpi.h>
#include <PartitionedDomain.h>
#ifdef _MUMPS
#include <MumpsParallelSOE.h>
#include <MumpsParallelSolver.h>
#endif
#endif

typedef struct materialFunction {
    char* funcName;
    matFunct theFunct;
    struct materialFunction* next;
} MaterialFunction;

typedef struct limitCurveFunction {
    char* funcName;
    limCrvFunct theFunct;
    struct limitCurveFunction* next;
} LimitCurveFunction;

typedef struct elementFunction {
    char* funcName;
    eleFunct theFunct;
    struct elementFunction* next;
} ElementFunction;

// active object
static OpenSeesCommands* cmds = 0;
static MaterialFunction* theMaterialFunctions = 0;
static LimitCurveFunction* theLimitCurveFunctions = 0;
static ElementFunction* theElementFunctions = 0;


OpenSeesCommands::OpenSeesCommands(DL_Interpreter* interp)
    :interpreter(interp), theDomain(0), 
     ndf(0), ndm(0),
     theSOE(0), theEigenSOE(0), theNumberer(0), theHandler(0),
     theStaticIntegrator(0), theTransientIntegrator(0),
     theAlgorithm(0), theStaticAnalysis(0), theTransientAnalysis(0),
     theVariableTimeStepTransientAnalysis(0),
     thePFEMAnalysis(0), theAnalysisModel(0), theTest(0),
     numEigen(0), builtModel(false), theDatabase(0),
     theBroker(), theTimer(), theSimulationInfo(), theMachineBroker(0),
     theChannels(0), numChannels(0), reliability(0)
{
#ifdef _PARALLEL_INTERPRETERS
    theMachineBroker = new MPI_MachineBroker(&theBroker, 0, 0);
    int rank = theMachineBroker->getPID();
    int np = theMachineBroker->getNP();
    if (rank == 0) {
        theChannels = new Channel *[np-1];
        numChannels = np-1;

        for (int j=0; j<np-1; j++) {
            Channel *otherChannel = theMachineBroker->getRemoteProcess();
            theChannels[j] = otherChannel;
        }
    } else {
        theChannels = new Channel *[1];
        numChannels = 1;
        theChannels[0] = theMachineBroker->getMyChannel();
    }
#endif

    cmds = this;

    theDomain = new Domain;

    reliability = new OpenSeesReliabilityCommands(theDomain);
}

OpenSeesCommands::~OpenSeesCommands()
{
    if (reliability != 0) delete reliability;
    if (theDomain != 0) delete theDomain;
    if (theDatabase != 0) delete theDatabase;
    cmds = 0;

#ifdef _PARALLEL_INTERPRETERS
    if (theChannels != 0) {
        delete [] theChannels;
    }
    if (theMachineBroker != 0) {
        // called in cleanup function, opserr not working
	std::cerr << "Process "<<theMachineBroker->getPID() << " Terminating\n";
	theMachineBroker->shutdown();
	delete theMachineBroker;
    }
#endif

}

DL_Interpreter*
OpenSeesCommands::getInterpreter()
{
    return interpreter;
}

Domain*
OpenSeesCommands::getDomain()
{
    return theDomain;
}

ReliabilityDomain*
OpenSeesCommands::getReliabilityDomain()
{
    if (reliability == 0) {
        return 0;
    }
    return reliability->getDomain();
}

AnalysisModel**
OpenSeesCommands::getAnalysisModel()
{
    return &theAnalysisModel;
}

void
OpenSeesCommands::setSOE(LinearSOE* soe)
{
    // if not in analysis object, delete old one
    if (theStaticAnalysis==0 && theTransientAnalysis==0) {
	if (theSOE != 0) {
	    delete theSOE;
	    theSOE = 0;
	}
    }

    // set new one
    theSOE = soe;
    if (soe == 0) return;

    // set in analysis object
    if (theStaticAnalysis != 0) {
	theStaticAnalysis->setLinearSOE(*soe);
    }
    if (theTransientAnalysis != 0) {
	theTransientAnalysis->setLinearSOE(*soe);
    }
}

int
OpenSeesCommands::eigen(int typeSolver, double shift,
    bool generalizedAlgo, bool findSmallest)
{
    //
    // create a transient analysis if no analysis exists
    //
    bool newanalysis = false;
    if (theStaticAnalysis == 0 && theTransientAnalysis == 0) {
	if (theAnalysisModel == 0)
	    theAnalysisModel = new AnalysisModel();
	if (theTest == 0)
	    theTest = new CTestNormUnbalance(1.0e-6,25,0);
	if (theAlgorithm == 0) {
	    theAlgorithm = new NewtonRaphson(*theTest);
	}
	if (theHandler == 0) {
	    theHandler = new TransformationConstraintHandler();
	}
	if (theNumberer == 0) {
	    RCM *theRCM = new RCM(false);
	    theNumberer = new DOF_Numberer(*theRCM);
	}
	if (theTransientIntegrator == 0) {
        setIntegrator(new Newmark(0.5,0.25), true);
	}
	if (theSOE == 0) {
	    ProfileSPDLinSolver *theSolver;
	    theSolver = new ProfileSPDLinDirectSolver();
	    theSOE = new ProfileSPDLinSOE(*theSolver);
	}

	theTransientAnalysis = new DirectIntegrationAnalysis(*theDomain,
							     *theHandler,
							     *theNumberer,
							     *theAnalysisModel,
							     *theAlgorithm,
							     *theSOE,
							     *theTransientIntegrator,
							     theTest);
	newanalysis = true;
    }

    //
    // create a new eigen system and solver
    //
    if (theEigenSOE != 0) {
	if (theEigenSOE->getClassTag() != typeSolver) {
	    //	delete theEigenSOE;
	    theEigenSOE = 0;
	}
    }

    if (theEigenSOE == 0) {

	if (typeSolver == EigenSOE_TAGS_SymBandEigenSOE) {
	    SymBandEigenSolver *theEigenSolver = new SymBandEigenSolver();
	    theEigenSOE = new SymBandEigenSOE(*theEigenSolver, *theAnalysisModel);

	} else if (typeSolver == EigenSOE_TAGS_FullGenEigenSOE) {

	    FullGenEigenSolver *theEigenSolver = new FullGenEigenSolver();
	    theEigenSOE = new FullGenEigenSOE(*theEigenSolver, *theAnalysisModel);

	} else {

	    theEigenSOE = new ArpackSOE(shift);

	}

	//
	// set the eigen soe in the system
	//

	if (theStaticAnalysis != 0) {
	    theStaticAnalysis->setEigenSOE(*theEigenSOE);
	} else if (theTransientAnalysis != 0) {
	    theTransientAnalysis->setEigenSOE(*theEigenSOE);
	}

    } // theEigenSOE != 0


    // run analysis
    int result = 0;
    if (theStaticAnalysis != 0) {
	result = theStaticAnalysis->eigen(numEigen, generalizedAlgo, findSmallest);
    } else if (theTransientAnalysis != 0) {
	result = theTransientAnalysis->eigen(numEigen, generalizedAlgo, findSmallest);
    }
    if (newanalysis) {
	delete theTransientAnalysis;
	theTransientAnalysis = 0;
    }

    if (result == 0) {
	const Vector &eigenvalues = theDomain->getEigenvalues();
	double* data = new double[numEigen];
	for (int i=0; i<numEigen; i++) {
	    data[i] = eigenvalues(i);
	}
	OPS_SetDoubleOutput(&numEigen, data, false);
	delete [] data;
    }

    return result;
}

void
OpenSeesCommands::setNumberer(DOF_Numberer* numberer)
{
    // if not in analysis object, delete old one
    if (theStaticAnalysis==0 && theTransientAnalysis==0) {
	if (theNumberer != 0) {
	    delete theNumberer;
	    theNumberer = 0;
	}
    }

    // set new one
    theNumberer = numberer;
    if (numberer == 0) return;

    // set in analysis object
    if (theStaticAnalysis != 0) {
	theStaticAnalysis->setNumberer(*numberer);
    }
    if (theTransientAnalysis != 0) {
	theTransientAnalysis->setNumberer(*numberer);
    }
}

void
OpenSeesCommands::setHandler(ConstraintHandler* handler)
{
    // if not in analysis object, delete old one and set new one
    if (theStaticAnalysis==0 && theTransientAnalysis==0) {
	if (theHandler != 0) {
	    delete theHandler;
	    theHandler = 0;
	}
	theHandler = handler;
	return;
    }

    // if analysis object is created, not set new one
    if (handler != 0) {
	opserr<<"WARNING can't set handler after analysis is created\n";
	delete handler;
    }

}

void
OpenSeesCommands::setCTest(ConvergenceTest* test)
{
    // if not in analysis object, delete old one and set new one
    if (theStaticAnalysis==0 && theTransientAnalysis==0) {
	if (theTest != 0) {
	    delete theTest;
	    theTest = 0;
	}
	theTest = test;
	return;
    }

    // set new one
    theTest = test;
    if (test == 0) return;

    // set in analysis object
    if (theStaticAnalysis != 0) {
	theStaticAnalysis->setConvergenceTest(*test);
    }
    if (theTransientAnalysis != 0) {
	theTransientAnalysis->setConvergenceTest(*test);
    }
}

void
OpenSeesCommands::setStaticIntegrator(StaticIntegrator* integrator)
{
    // error in transient analysis
    if (theTransientAnalysis != 0) {
	opserr<<"WARNING can't set static integrator in transient analysis\n";
	if (integrator != 0) {
	    delete integrator;
	}
	return;
    }

    // if not in static analysis object, delete old one
    if (theStaticAnalysis==0) {
	if (theStaticIntegrator != 0) {
	    delete theStaticIntegrator;
	    theStaticIntegrator = 0;
	}
    }

    // set new one
    setIntegrator(integrator, false);
    if (integrator == 0) return;

    // set in analysis object
    if (theStaticAnalysis != 0) {
	theStaticAnalysis->setIntegrator(*integrator);
    }
}

void
OpenSeesCommands::setTransientIntegrator(TransientIntegrator* integrator)
{
    // error in static analysis
    if (theStaticAnalysis != 0) {
	opserr<<"WARNING can't set transient integrator in static analysis\n";
	if (integrator != 0) {
	    delete integrator;
	}
	return;
    }

    // if not in transient analysis object, delete old one
    if (theTransientAnalysis==0) {
	if (theTransientIntegrator != 0) {
	    delete theTransientIntegrator;
	    theTransientIntegrator = 0;
	}
    }

    // set new one
    setIntegrator(integrator, true);
    if (integrator == 0) return;

    // set in analysis object
    if (theTransientAnalysis != 0) {
	theTransientAnalysis->setIntegrator(*integrator);
    }
}

void
OpenSeesCommands::setIntegrator(Integrator* inte,
                                     bool transient) {
  if (inte == 0) {
    return;
  }
  if (transient) {
    theTransientIntegrator = (TransientIntegrator*)inte;
  } else {
    theStaticIntegrator = (StaticIntegrator*)inte;
  }
  if (this->reliability != 0) {
    this->reliability->setSensitivityAlgorithm(inte);
  }
}

void
OpenSeesCommands::setAlgorithm(EquiSolnAlgo* algorithm)
{
    // if not in analysis object, delete old one
    if (theStaticAnalysis==0 && theTransientAnalysis==0) {
	if (theAlgorithm != 0) {
	    delete theAlgorithm;
	    theAlgorithm = 0;
	}
    }

    // set new one
    theAlgorithm = algorithm;
    if (algorithm == 0) return;

    // set in analysis object
    if (theStaticAnalysis != 0) {
	theStaticAnalysis->setAlgorithm(*algorithm);
	if (theTest != 0) {
	    algorithm->setConvergenceTest(theTest);
	}
    }
    if (theTransientAnalysis != 0) {
	theTransientAnalysis->setAlgorithm(*algorithm);
	if (theTest != 0) {
	    algorithm->setConvergenceTest(theTest);
	}
    }
}

void
OpenSeesCommands::setStaticAnalysis(bool suppress)
{
    // delete the old analysis
    if (theStaticAnalysis != 0) {
	delete theStaticAnalysis;
	theStaticAnalysis = 0;
    }
    if (theTransientAnalysis != 0) {
	delete theTransientAnalysis;
	theTransientAnalysis = 0;
    }

    // create static analysis
    if (theAnalysisModel == 0) {
	theAnalysisModel = new AnalysisModel();
    }
    if (theTest == 0) {
	theTest = new CTestNormUnbalance(1.0e-6,25,0);
    }
    if (theAlgorithm == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Static - no Algorithm yet specified, \n";
	opserr << " NewtonRaphson default will be used\n";
      }
	theAlgorithm = new NewtonRaphson(*theTest);
    }
    if (theHandler == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Static - no ConstraintHandler yet specified, \n";
	opserr << " PlainHandler default will be used\n";
      }
	theHandler = new PlainHandler();
    }
    if (theNumberer == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Static - no Numberer specified, \n";
	opserr << " RCM default will be used\n";
      }
	RCM* theRCM = new RCM(false);
	theNumberer = new DOF_Numberer(*theRCM);
    }
    if (theStaticIntegrator == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Static - no Integrator specified, \n";
	opserr << " StaticIntegrator default will be used\n";
      }
    setIntegrator(new LoadControl(1, 1, 1, 1), false);
    }
    if (theSOE == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Static - no LinearSOE specified, \n";
	opserr << " ProfileSPDLinSOE default will be used\n";
      }
	ProfileSPDLinSolver *theSolver;
	theSolver = new ProfileSPDLinDirectSolver();
	theSOE = new ProfileSPDLinSOE(*theSolver);
    }

    theStaticAnalysis = new StaticAnalysis(*theDomain,
    					   *theHandler,
    					   *theNumberer,
    					   *theAnalysisModel,
    					   *theAlgorithm,
    					   *theSOE,
    					   *theStaticIntegrator,
    					   theTest);

    if (theEigenSOE != 0) {
	theStaticAnalysis->setEigenSOE(*theEigenSOE);
    }

#ifdef _PARALLEL_INTERPRETERS
    if (setMPIDSOEFlag) {
        ((MPIDiagonalSOE*)theSOE)->setAnalysisModel(*theAnalysisModel);
    }
#endif
}

int
OpenSeesCommands::setPFEMAnalysis(bool suppress)
{
    // delete the old analysis
    if (theStaticAnalysis != 0) {
	delete theStaticAnalysis;
	theStaticAnalysis = 0;
    }
    if (theTransientAnalysis != 0) {
	delete theTransientAnalysis;
	theTransientAnalysis = 0;
    }

    // create PFEM analysis
    if(OPS_GetNumRemainingInputArgs() < 3) {
	opserr<<"WARNING: wrong no of args -- analysis PFEM dtmax dtmin gravity <ratio>\n";
	return -1;
    }

    int numdata = 1;
    double dtmax, dtmin, gravity, ratio=0.5;
    if (OPS_GetDoubleInput(&numdata, &dtmax) < 0) {
	opserr<<"WARNING: invalid dtmax \n";
	return -1;
    }
    if (OPS_GetDoubleInput(&numdata, &dtmin) < 0) {
	opserr<<"WARNING: invalid dtmin \n";
	return -1;
    }
    if (OPS_GetDoubleInput(&numdata, &gravity) < 0) {
	opserr<<"WARNING: invalid gravity \n";
	return -1;
    }
    if(OPS_GetNumRemainingInputArgs() > 0) {
	if (OPS_GetDoubleInput(&numdata, &ratio) < 0) {
	    opserr<<"WARNING: invalid ratio \n";
	    return -1;
	}
    }

    if(theAnalysisModel == 0) {
	theAnalysisModel = new AnalysisModel();
    }
    if(theTest == 0) {
	//theTest = new CTestNormUnbalance(1e-2,10000,1,2,3);
	theTest = new CTestPFEM(1e-2,1e-2,1e-2,1e-2,1e-4,1e-3,10000,100,1,2);
    }
    if(theAlgorithm == 0) {
	theAlgorithm = new NewtonRaphson(*theTest);
    }
    if(theHandler == 0) {
	theHandler = new TransformationConstraintHandler();
    }
    if(theNumberer == 0) {
	RCM* theRCM = new RCM(false);
	theNumberer = new DOF_Numberer(*theRCM);
    }
    if(theTransientIntegrator == 0) {
        setIntegrator(new PFEMIntegrator(), true);
    }
    if(theSOE == 0) {
	PFEMSolver* theSolver = new PFEMSolver();
	theSOE = new PFEMLinSOE(*theSolver);
    }
    thePFEMAnalysis = new PFEMAnalysis(*theDomain,
				       *theHandler,
				       *theNumberer,
				       *theAnalysisModel,
				       *theAlgorithm,
				       *theSOE,
				       *theTransientIntegrator,
				       theTest,dtmax,dtmin,gravity,ratio);

    theTransientAnalysis = thePFEMAnalysis;

    if (theEigenSOE != 0) {
	theTransientAnalysis->setEigenSOE(*theEigenSOE);
    }

#ifdef _PARALLEL_INTERPRETERS
    if (setMPIDSOEFlag) {
        ((MPIDiagonalSOE*)theSOE)->setAnalysisModel(*theAnalysisModel);
    }
#endif

    return 0;
}

void
OpenSeesCommands::setVariableAnalysis(bool suppress)
{
    // delete the old analysis
    if (theStaticAnalysis != 0) {
	delete theStaticAnalysis;
	theStaticAnalysis = 0;
    }
    if (theTransientAnalysis != 0) {
	delete theTransientAnalysis;
	theTransientAnalysis = 0;
    }

    // make sure all the components have been built,
    // otherwise print a warning and use some defaults
    if (theAnalysisModel == 0) {
	theAnalysisModel = new AnalysisModel();
    }

    if (theTest == 0) {
	theTest = new CTestNormUnbalance(1.0e-6,25,0);
    }

    if (theAlgorithm == 0) {
      if (!suppress) {
	opserr << "WARNING analysis VariableTransient - no Algorithm yet specified, \n";
	opserr << " NewtonRaphson default will be used\n";
      }
	theAlgorithm = new NewtonRaphson(*theTest);
    }

    if (theHandler == 0) {
      if (!suppress) {
	opserr << "WARNING analysis VariableTransient dt tFinal - no ConstraintHandler\n";
	opserr << " yet specified, PlainHandler default will be used\n";
      }
	theHandler = new PlainHandler();
    }

    if (theNumberer == 0) {
      if (!suppress) {
	opserr << "WARNING analysis VariableTransient dt tFinal - no Numberer specified, \n";
	opserr << " RCM default will be used\n";
      }
	RCM *theRCM = new RCM(false);
	theNumberer = new DOF_Numberer(*theRCM);
    }

    if (theTransientIntegrator == 0) {
      if (!suppress) {
	opserr << "WARNING analysis VariableTransient dt tFinal - no Integrator specified, \n";
	opserr << " Newmark(.5,.25) default will be used\n";
      }
        setIntegrator(new Newmark(0.5, 0.25), true);
    }

    if (theSOE == 0) {
      if (!suppress) {
	opserr << "WARNING analysis VariableTransient dt tFinal - no LinearSOE specified, \n";
	opserr << " ProfileSPDLinSOE default will be used\n";
      }
	ProfileSPDLinSolver *theSolver;
	theSolver = new ProfileSPDLinDirectSolver();
	theSOE = new ProfileSPDLinSOE(*theSolver);
    }

    theVariableTimeStepTransientAnalysis = new VariableTimeStepDirectIntegrationAnalysis
	(*theDomain,
	 *theHandler,
	 *theNumberer,
	 *theAnalysisModel,
	 *theAlgorithm,
	 *theSOE,
	 *theTransientIntegrator,
	 theTest);

    // set the pointer for variable time step analysis
    theTransientAnalysis = theVariableTimeStepTransientAnalysis;

    if (theEigenSOE != 0) {
	theTransientAnalysis->setEigenSOE(*theEigenSOE);
    }
}

void
OpenSeesCommands::setTransientAnalysis(bool suppress)
{
    // delete the old analysis
    if (theStaticAnalysis != 0) {
	delete theStaticAnalysis;
	theStaticAnalysis = 0;
    }
    if (theTransientAnalysis != 0) {
	delete theTransientAnalysis;
	theTransientAnalysis = 0;
    }

    // create transient analysis
    if (theAnalysisModel == 0) {
	theAnalysisModel = new AnalysisModel();
    }
    if (theTest == 0) {
	theTest = new CTestNormUnbalance(1.0e-6,25,0);
    }
    if (theAlgorithm == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Transient - no Algorithm yet specified, \n";
	opserr << " NewtonRaphson default will be used\n";
      }
	theAlgorithm = new NewtonRaphson(*theTest);
    }
    if (theHandler == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Transient - no ConstraintHandler yet specified, \n";
	opserr << " PlainHandler default will be used\n";
      }
	theHandler = new PlainHandler();
    }
    if (theNumberer == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Transient - no Numberer specified, \n";
	opserr << " RCM default will be used\n";
      }
	RCM* theRCM = new RCM(false);
	theNumberer = new DOF_Numberer(*theRCM);
    }
    if (theTransientIntegrator == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Transient - no Integrator specified, \n";
	opserr << " TransientIntegrator default will be used\n";
      }
    setIntegrator(new Newmark(0.5,0.25), true);
    }
    if (theSOE == 0) {
      if (!suppress) {
	opserr << "WARNING analysis Transient - no LinearSOE specified, \n";
	opserr << " ProfileSPDLinSOE default will be used\n";
      }
	ProfileSPDLinSolver *theSolver;
	theSolver = new ProfileSPDLinDirectSolver();
	theSOE = new ProfileSPDLinSOE(*theSolver);
    }

    theTransientAnalysis = new DirectIntegrationAnalysis(*theDomain,
							 *theHandler,
							 *theNumberer,
							 *theAnalysisModel,
							 *theAlgorithm,
							 *theSOE,
							 *theTransientIntegrator,
							 theTest);
    if (theEigenSOE != 0) {
	theTransientAnalysis->setEigenSOE(*theEigenSOE);
    }
#ifdef _PARALLEL_INTERPRETERS
	if (setMPIDSOEFlag) {
	  ((MPIDiagonalSOE*) theSOE)->setAnalysisModel(*theAnalysisModel);
	}
#endif
}

void
OpenSeesCommands::wipeAnalysis()
{
    if (theStaticAnalysis==0 && theTransientAnalysis==0) {
	if (theSOE != 0) delete theSOE;
	if (theEigenSOE != 0) delete theEigenSOE;
	if (theNumberer != 0) delete theNumberer;
	if (theHandler != 0) delete theHandler;
	if (theStaticIntegrator != 0) delete theStaticIntegrator;
	if (theTransientIntegrator != 0) delete theTransientIntegrator;
	if (theAlgorithm != 0) delete theAlgorithm;
	if (theTest != 0) delete theTest;
    }

    if (theStaticAnalysis != 0) {
    	theStaticAnalysis->clearAll();
    	delete theStaticAnalysis;
    }
    if (theTransientAnalysis != 0) {
    	theTransientAnalysis->clearAll();
    	delete theTransientAnalysis;
    }

    theAlgorithm = 0;
    theHandler = 0;
    theNumberer = 0;
    theAnalysisModel = 0;
    theSOE = 0;
    theEigenSOE = 0;
    theStaticIntegrator = 0;
    theTransientIntegrator = 0;
    theStaticAnalysis = 0;
    theTransientAnalysis = 0;
    theVariableTimeStepTransientAnalysis = 0;
    thePFEMAnalysis = 0;
    theTest = 0;

}

void
OpenSeesCommands::wipe()
{
    this->wipeAnalysis();

    // data base
    if (theDatabase != 0) {
	delete theDatabase;
	theDatabase = 0;
    }

    // wipe domain
    if (theDomain != 0) {
	theDomain->clearAll();
    }

    // wipe all meshes
    OPS_clearAllMesh();
    OPS_getBgMesh().clearAll();

    // time set to zero
    ops_Dt = 0.0;

    // wipe uniaxial material
    OPS_clearAllUniaxialMaterial();
    OPS_clearAllNDMaterial();

    // wipe sections
    OPS_clearAllSectionForceDeformation();
    OPS_clearAllSectionRepres();

    // wipe time series
    OPS_clearAllTimeSeries();

    // wipe GeomTransf
    OPS_clearAllCrdTransf();

    // wipe damping
    OPS_clearAllDamping();

    // wipe BeamIntegration
    OPS_clearAllBeamIntegrationRule();

    // wipe limit state curve
    OPS_clearAllLimitCurve();

    // wipe damages model
    OPS_clearAllDamageModel();

    // wipe friction model
    OPS_clearAllFrictionModel();

    // wipe HystereticBackbone
    OPS_clearAllHystereticBackbone();
    OPS_clearAllStiffnessDegradation();
    OPS_clearAllStrengthDegradation();
    OPS_clearAllUnloadingRule();

    // wipe YieldSurface_BC
    OPS_clearAllYieldSurface_BC();

    // wipe CyclicModel
    OPS_clearAllCyclicModel();
}

void
OpenSeesCommands::setFileDatabase(const char* filename)
{
    if (theDatabase != 0) delete theDatabase;
    theDatabase = new FileDatastore(filename, *theDomain, theBroker);
    if (theDatabase == 0) {
	opserr << "WARNING ran out of memory - database File " << filename << endln;
    }
}

/////////////////////////////
//// OpenSees APIs  /// /////
/////////////////////////////
static
void OPS_InvokeMaterialObject(struct matObject* theMat, modelState* theModel, double* strain, double* tang, double* stress, int* isw, int* result)
{
    int matType = (int)theMat->theParam[0];

    if (matType == 1) {
        //  UniaxialMaterial *theMaterial = theUniaxialMaterials[matCount];
        UniaxialMaterial* theMaterial = (UniaxialMaterial*)theMat->matObjectPtr;
        if (theMaterial == 0) {
            *result = -1;
            return;
        }

        if (*isw == ISW_COMMIT) {
            *result = theMaterial->commitState();
            return;
        }
        else if (*isw == ISW_REVERT) {
            *result = theMaterial->revertToLastCommit();
            return;
        }
        else if (*isw == ISW_REVERT_TO_START) {
            *result = theMaterial->revertToStart();
            return;
        }
        else if (*isw == ISW_FORM_TANG_AND_RESID) {
            double matStress = 0.0;
            double matTangent = 0.0;
            int res = theMaterial->setTrial(strain[0], matStress, matTangent);
            stress[0] = matStress;
            tang[0] = matTangent;
            *result = res;
            return;
        }
    }

    return;
}

int OPS_GetNumRemainingInputArgs()
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->getNumRemainingInputArgs();
}

int OPS_ResetCommandLine(int nArgs, int cArg, const char** argv)
{
    if (cArg == 0) {
        opserr << "WARNING can't reset to argv[0]\n";
        return -1;
    }
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    interp->resetInput(nArgs, cArg, argv);
    return 0;
}

int OPS_ResetCurrentInputArg(int cArg)
{
    if (cArg == 0) {
        opserr << "WARNING can't reset to argv[0]\n";
        return -1;
    }
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    interp->resetInput(cArg);
    return 0;
}

int OPS_Error(char* errorMessage, int length)
{
    opserr << errorMessage;
    opserr << endln;
    return 0;
}

int OPS_GetIntInput(int *numData, int*data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    if (numData == 0 || data == 0) return -1;
    return interp->getInt(data, *numData);
}

int OPS_SetIntOutput(int *numData, int*data, bool scalar)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setInt(data, *numData, scalar);
}

int OPS_SetIntListsOutput(std::vector<std::vector<int>>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setInt(data);
}

int OPS_SetIntDictOutput(std::map<const char*, int>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setInt(data);
}

int OPS_SetIntDictListOutput(std::map<const char*, std::vector<int>>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setInt(data);
}

int OPS_GetDoubleInput(int *numData, double *data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    if (numData == 0 || data == 0) return -1;
    return interp->getDouble(data, *numData);
}

int OPS_GetDoubleListInput(int* size, Vector* data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->getDoubleList(size, data);
}

int OPS_EvalDoubleStringExpression(const char* theExpression, double& current_val) {
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->evalDoubleStringExpression(theExpression, current_val);
}

int OPS_SetDoubleOutput(int *numData, double *data, bool scalar)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setDouble(data, *numData, scalar);
}

int OPS_SetDoubleListsOutput(std::vector<std::vector<double>>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setDouble(data);
}

int OPS_SetDoubleDictOutput(std::map<const char* ,double>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setDouble(data);
}

int OPS_SetDoubleDictListOutput(std::map<const char*, std::vector<double>>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setDouble(data);
}

const char* OPS_GetString(void)
{
    const char* res = 0;
    if (cmds == 0) return res;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->getString();
}

const char* OPS_GetStringFromAll(char* buffer, int len)
{
    const char* res = 0;
    if (cmds == 0) return res;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->getStringFromAll(buffer, len);
}

int OPS_SetString(const char* str)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setString(str);
}

int OPS_SetStringList(std::vector<const char*>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setString(data);
}

int OPS_SetStringLists(std::vector<std::vector<const char*>>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setString(data);
}

int OPS_SetStringDict(std::map<const char*, const char*>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setString(data);
}

int OPS_SetStringDictList(std::map<const char*, std::vector<const char*>>& data)
{
    if (cmds == 0) return 0;
    DL_Interpreter* interp = cmds->getInterpreter();
    return interp->setString(data);
}

int OPS_GetNDF()
{
    if (cmds == 0) return 0;
    return cmds->getNDF();
}

int OPS_GetNDM()
{
    if (cmds == 0) return 0;
    return cmds->getNDM();
}

extern "C" int OPS_GetNodeCrd(int* nodeTag, int* sizeCrd, double* data)
{
    Domain* theDomain = cmds->getDomain();
    Node* theNode = theDomain->getNode(*nodeTag);
    if (theNode == 0) {
        opserr << "OPS_GetNodeCrd - no node with tag " << *nodeTag << endln;
        return -1;
    }
    int size = *sizeCrd;
    const Vector& crd = theNode->getCrds();
    if (crd.Size() != size) {
        opserr << "OPS_GetNodeCrd - crd size mismatch\n";
        opserr << "Actual crd size is: " << crd.Size() << endln;
        return -1;
    }
    for (int i = 0; i < size; i++)
        data[i] = crd(i);
    
    return 0;
}

extern "C" int OPS_GetNodeDisp(int* nodeTag, int* sizeData, double* data)
{
    Domain* theDomain = cmds->getDomain();
    Node* theNode = theDomain->getNode(*nodeTag);
    if (theNode == 0) {
        opserr << "OPS_GetNodeDisp - no node with tag " << *nodeTag << endln;
        return -1;
    }
    int size = *sizeData;
    const Vector& disp = theNode->getTrialDisp();
    if (disp.Size() != size) {
        opserr << "OPS_GetNodeDisp - crd size mismatch\n";
        return -1;
    }
    for (int i = 0; i < size; i++)
        data[i] = disp(i);
    
    return 0;
}

extern "C" int OPS_GetNodeVel(int* nodeTag, int* sizeData, double* data)
{
    Domain* theDomain = cmds->getDomain();
    Node* theNode = theDomain->getNode(*nodeTag);
    if (theNode == 0) {
        opserr << "OPS_GetNodeVel - no node with tag " << *nodeTag << endln;
        return -1;
    }
    int size = *sizeData;
    const Vector& vel = theNode->getTrialVel();
    if (vel.Size() != size) {
        opserr << "OPS_GetNodeVel - crd size mismatch\n";
        return -1;
    }
    for (int i = 0; i < size; i++)
        data[i] = vel(i);
    
    return 0;
}

extern "C" int OPS_GetNodeAccel(int* nodeTag, int* sizeData, double* data)
{
    Domain* theDomain = cmds->getDomain();
    Node* theNode = theDomain->getNode(*nodeTag);
    if (theNode == 0) {
        opserr << "OPS_GetNodeAccel - no node with tag " << *nodeTag << endln;
        return -1;
    }
    int size = *sizeData;
    const Vector& accel = theNode->getTrialAccel();
    if (accel.Size() != size) {
        opserr << "OPS_GetNodeAccel - accel size mismatch\n";
        return -1;
    }
    for (int i = 0; i < size; i++)
        data[i] = accel(i);
    
    return 0;
}

extern "C" int OPS_GetNodeIncrDisp(int* nodeTag, int* sizeData, double* data)
{
    Domain* theDomain = cmds->getDomain();
    Node* theNode = theDomain->getNode(*nodeTag);
    if (theNode == 0) {
        opserr << "OPS_GetNodeIncrDisp - no node with tag " << *nodeTag << endln;
        return -1;
    }
    int size = *sizeData;
    const Vector& disp = theNode->getIncrDisp();
    if (disp.Size() != size) {
        opserr << "OPS_GetNodeIncrDis - crd size mismatch\n";
        return -1;
    }
    for (int i = 0; i < size; i++)
        data[i] = disp(i);
    
    return 0;
}

extern "C" int OPS_GetNodeIncrDeltaDisp(int* nodeTag, int* sizeData, double* data)
{
    Domain* theDomain = cmds->getDomain();
    Node* theNode = theDomain->getNode(*nodeTag);
    if (theNode == 0) {
        opserr << "OPS_GetNodeIncrDeltaDisp - no node with tag " << *nodeTag << endln;
        return -1;
    }
    int size = *sizeData;
    const Vector& disp = theNode->getIncrDeltaDisp();
    if (disp.Size() != size) {
        opserr << "OPS_GetNodeIncrDis - crd size mismatch\n";
        return -1;
    }
    for (int i = 0; i < size; i++)
        data[i] = disp(i);
    
    return 0;
}

extern "C"
matObj* OPS_GetMaterial(int* matTag, int* matType)
{
    if (*matType == OPS_UNIAXIAL_MATERIAL_TYPE) {
        UniaxialMaterial* theUniaxialMaterial = OPS_getUniaxialMaterial(*matTag);

        if (theUniaxialMaterial != 0) {

            UniaxialMaterial* theCopy = theUniaxialMaterial->getCopy();
            //uniaxialMaterialObjectCount++;
            //theUniaxialMaterials[uniaxialMaterialObjectCount] = theCopy;

            matObject* theMatObject = new matObject;
            theMatObject->tag = *matTag;
            theMatObject->nParam = 1;
            theMatObject->nState = 0;

            theMatObject->theParam = new double[1];
            //theMatObject->theParam[0] = uniaxialMaterialObjectCount;
            theMatObject->theParam[0] = 1; // code for uniaxial material

            theMatObject->tState = 0;
            theMatObject->cState = 0;
            theMatObject->matFunctPtr = OPS_InvokeMaterialObject;

            theMatObject->matObjectPtr = theCopy;

            return theMatObject;
        }

        fprintf(stderr, "getMaterial - no uniaxial material exists with tag %d\n", *matTag);
        return 0;
    }
    else if (*matType == OPS_SECTION_TYPE) {
        fprintf(stderr, "getMaterial - not yet implemented for Section\n");
        return 0;
    }
    else {

        //    NDMaterial *theNDMaterial = theModelBuilder->getNDMaterial(*matTag);

        //    if (theNDMaterial != 0) 
          //      theNDMaterial = theNDMaterial->getCopy(matType);
          //    else {
          //      fprintf(stderr,"getMaterial - no nd material exists with tag %d\n", *matTag);          
          //      return 0;
          //    }

          //    if (theNDMaterial == 0) {
        //      fprintf(stderr,"getMaterial - material with tag %d cannot deal with %d\n", *matTag, matType);          
        //      return 0;
        //    }

        fprintf(stderr, "getMaterial - not yet implemented for nDMaterial\n");
        return 0;
    }

    fprintf(stderr, "getMaterial - unknown material type\n");
    return 0;
}

extern "C"
matObj* OPS_GetMaterialType(char* type, int sizeType)
{
    // try existing loaded routines
    MaterialFunction* matFunction = theMaterialFunctions;
    bool found = false;
    while (matFunction != NULL && found == false) {
        if (strcmp(type, matFunction->funcName) == 0) {

            // create a new matObject, set the function ptr & return it
            matObj* theMatObject = new matObj;
            theMatObject->matFunctPtr = matFunction->theFunct;
            //opserr << "matObj *OPS_GetMaterialType() - FOUND " << endln;
            return theMatObject;
        }
        else
            matFunction = matFunction->next;
    }

    // try to load new routine from dynamic library in load path
    matFunct matFunctPtr;
    void* libHandle;

    int res = getLibraryFunction(type, type, &libHandle, (void**)&matFunctPtr);

    if (res == 0) {

        // add the routine to the list of possible elements
        char* funcName = new char[strlen(type) + 1];
        strcpy(funcName, type);
        matFunction = new MaterialFunction;
        matFunction->theFunct = matFunctPtr;
        matFunction->funcName = funcName;
        matFunction->next = theMaterialFunctions;
        theMaterialFunctions = matFunction;

        // create a new matObject, set the function ptr & return it
        matObj* theMatObject = new matObj;
        //eleObj *theEleObject = (eleObj *)malloc(sizeof( eleObj));;      

        theMatObject->matFunctPtr = matFunction->theFunct;
        //fprintf(stderr,"getMaterial Address %p\n",theMatObject);

        return theMatObject;
    }

    return 0;
}

extern "C"
limCrvObj* OPS_GetLimitCurveType(char* type, int sizeType)
{
    // try existing loaded routines
    LimitCurveFunction* limCrvFunction = theLimitCurveFunctions;
    bool found = false;
    while (limCrvFunction != NULL && found == false) {
        if (strcmp(type, limCrvFunction->funcName) == 0) {

            // create a new limCrvObject, set the function ptr & return it
            limCrvObj* theLimCrvObject = new limCrvObj;
            theLimCrvObject->limCrvFunctPtr = limCrvFunction->theFunct;
            //opserr << "limCrvObj *OPS_GetLimitCurveType() - FOUND " << endln;
            return theLimCrvObject;
        }
        else
            limCrvFunction = limCrvFunction->next;
    }

    // try to load new routine from dynamic library in load path
    limCrvFunct limCrvFunctPtr;
    void* libHandle;
    int res = getLibraryFunction(type, type, &libHandle, (void**)&limCrvFunctPtr);

    if (res == 0)
    {
        // add the routine to the list of possible elements
        char* funcName = new char[strlen(type) + 1];
        strcpy(funcName, type);
        limCrvFunction = new LimitCurveFunction;
        limCrvFunction->theFunct = limCrvFunctPtr;
        limCrvFunction->funcName = funcName;
        limCrvFunction->next = theLimitCurveFunctions;
        theLimitCurveFunctions = limCrvFunction;

        // create a new limCrvObject, set the function ptr & return it    
        limCrvObj* theLimCrvObject = new limCrvObj;
        theLimCrvObject->limCrvFunctPtr = limCrvFunction->theFunct;
        return theLimCrvObject;
    }

    return 0;
}

extern "C"
eleObj* OPS_GetElement(int* eleTag)
{
    return 0;
}

extern "C"
eleObj* OPS_GetElementType(char* type, int sizeType)
{
    // try existing loaded routines
    ElementFunction* eleFunction = theElementFunctions;
    bool found = false;
    while (eleFunction != NULL && found == false) {
        if (strcmp(type, eleFunction->funcName) == 0) {

            // create a new eleObject, set the function ptr & return it
            eleObj* theEleObject = new eleObj;
            theEleObject->eleFunctPtr = eleFunction->theFunct;
            return theEleObject;
        }
        else
            eleFunction = eleFunction->next;
    }

    // try to load new routine from dynamic library in load path
    eleFunct eleFunctPtr;
    void* libHandle;

    int res = getLibraryFunction(type, type, &libHandle, (void**)&eleFunctPtr);

    if (res == 0) {

        // add the routine to the list of possible elements
        char* funcName = new char[strlen(type) + 1];
        strcpy(funcName, type);
        eleFunction = new ElementFunction;
        eleFunction->theFunct = eleFunctPtr;
        eleFunction->funcName = funcName;
        eleFunction->next = theElementFunctions;
        theElementFunctions = eleFunction;

        // create a new eleObject, set the function ptr & return it
        eleObj* theEleObject = new eleObj;
        //eleObj *theEleObject = (eleObj *)malloc(sizeof( eleObj));;      

        theEleObject->eleFunctPtr = eleFunction->theFunct;

        return theEleObject;
    }

    return 0;
}

extern "C"
int OPS_AllocateMaterial(matObject* theMat)
{
    //fprintf(stderr,"allocateMaterial Address %p\n",theMat);
    if (theMat->nParam > 0)
        theMat->theParam = new double[theMat->nParam];

    int nState = theMat->nState;

    if (nState > 0) {
        theMat->cState = new double[nState];
        theMat->tState = new double[nState];
        for (int i = 0; i < nState; i++) {
            theMat->cState[i] = 0;
            theMat->tState[i] = 0;
        }
    }
    else {
        theMat->cState = 0;
        theMat->tState = 0;
    }

    return 0;
}

extern "C"
int OPS_AllocateLimitCurve(limCrvObject* theLimCrv)
{
    //fprintf(stderr,"allocateLimitCurve Address %p\n",theLimCrv);
    if (theLimCrv->nParam > 0)
        theLimCrv->theParam = new double[theLimCrv->nParam];

    int nState = theLimCrv->nState;

    if (nState > 0) {
        theLimCrv->cState = new double[nState];
        theLimCrv->tState = new double[nState];
        for (int i = 0; i < nState; i++) {
            theLimCrv->cState[i] = 0;
            theLimCrv->tState[i] = 0;
        }
    }
    else {
        theLimCrv->cState = 0;
        theLimCrv->tState = 0;
    }

    return 0;
}

extern "C"
int OPS_AllocateElement(eleObject* theEle, int* matTags, int* matType)
{
    if (theEle->nNode > 0)
        theEle->node = new int[theEle->nNode];

    if (theEle->nParam > 0)
        theEle->param = new double[theEle->nParam];

    if (theEle->nState > 0) {
        theEle->cState = new double[theEle->nState];
        theEle->tState = new double[theEle->nState];
    }

    int numMat = theEle->nMat;
    if (numMat > 0)
        theEle->mats = new matObject * [numMat];


    for (int i = 0; i < numMat; i++) {
        //opserr << "AllocateElement - matTag " << matTags[i] << "\n"; */
        matObject* theMat = OPS_GetMaterial(&(matTags[i]), matType);
        //matObject *theMat = OPS_GetMaterial(&(matTags[i]));

        theEle->mats[i] = theMat;
    }

    return 0;
}

extern "C" int
OPS_InvokeMaterial(eleObject* theEle, int* mat, modelState* model, double* strain, double* stress, double* tang, int* isw)
{
    int error = 0;

    matObject* theMat = theEle->mats[*mat];
    //fprintf(stderr,"invokeMaterial Address %d %d %d\n",*mat, theMat, sizeof(int));

    if (theMat != 0)
        theMat->matFunctPtr(theMat, model, strain, tang, stress, isw, &error);
    else
        error = -1;

    return error;
}

extern "C" int
OPS_InvokeMaterialDirectly(matObject** theMat, modelState* model, double* strain, double* stress, double* tang, int* isw)
{
    int error = 0;
    //fprintf(stderr,"invokeMaterialDirectly Address %d %d %d\n",theMat, sizeof(int), *theMat);
    if (*theMat != 0)
        (*theMat)->matFunctPtr(*theMat, model, strain, tang, stress, isw, &error);
    else
        error = -1;

    return error;
}

extern "C" int
OPS_InvokeMaterialDirectly2(matObject* theMat, modelState* model, double* strain, double* stress, double* tang, int* isw)
{
    int error = 0;
    //fprintf(stderr,"invokeMaterialDirectly Address %d %d\n",theMat, sizeof(int));
    if (theMat != 0)
        theMat->matFunctPtr(theMat, model, strain, tang, stress, isw, &error);
    else
        error = -1;

    return error;
}

UniaxialMaterial* OPS_GetUniaxialMaterial(int matTag)
{
    return OPS_getUniaxialMaterial(matTag);
}

LimitCurve*
OPS_GetLimitCurve(int limCrvTag)
{
    return OPS_getLimitCurve(limCrvTag);
}

NDMaterial*
OPS_GetNDMaterial(int matTag)
{
    return OPS_getNDMaterial(matTag);
}

SectionForceDeformation*
OPS_GetSectionForceDeformation(int secTag)
{
    return OPS_getSectionForceDeformation(secTag);
}

CrdTransf*
OPS_GetCrdTransf(int crdTag)
{
    return OPS_getCrdTransf(crdTag);
}

FrictionModel*
OPS_GetFrictionModel(int frnTag)
{
    return OPS_getFrictionModel(frnTag);
}

Domain* OPS_GetDomain(void)
{
    if (cmds == 0) return 0;
    return cmds->getDomain();
}

ReliabilityDomain* OPS_GetReliabilityDomain(void) {
  if (cmds == 0) return 0;
  return cmds->getReliabilityDomain();
}

FE_Datastore* OPS_GetFEDatastore(void)
{
    if (cmds == 0) return 0;
    return cmds->getDatabase();
}

SimulationInformation* OPS_GetSimulationInfo(void)
{
    if (cmds == 0) return 0;
    return cmds->getSimulationInformation();
}

AnalysisModel** OPS_GetAnalysisModel(void)
{
    if (cmds == 0) return 0;
    return cmds->getAnalysisModel();
}

EquiSolnAlgo** OPS_GetAlgorithm(void) {
    if (cmds == 0) return 0;
    return cmds->getAlgorithmPointer();
}

ConstraintHandler** OPS_GetHandler(void) {
    if (cmds == 0) return 0;
    return cmds->getHandlerPointer();
}

DOF_Numberer** OPS_GetNumberer(void) {
    if (cmds == 0) return 0;
    return cmds->getNumbererPointer();
}

LinearSOE** OPS_GetSOE(void) {
    if (cmds == 0) return 0;
    return cmds->getSOEPointer();
}

EigenSOE** OPS_GetEigenSOE(void) {
    if (cmds == 0) return 0;
    return cmds->getEigenSOEPointer();
}

StaticAnalysis** OPS_GetStaticAnalysis(void) {
    if (cmds == 0) return 0;
    return cmds->getStaticAnalysisPointer();
}

DirectIntegrationAnalysis** OPS_GetTransientAnalysis(void) {
    if (cmds == 0) return 0;
    return cmds->getTransientAnalysisPointer();
}

VariableTimeStepDirectIntegrationAnalysis**
OPS_GetVariableTimeStepTransientAnalysis(void) {
    if (cmds == 0) return 0;
    return cmds->getVariableAnalysisPointer();
}

StaticIntegrator** OPS_GetStaticIntegrator(void) {
    if (cmds == 0) return 0;
    return cmds->getStaticIntegratorPointer();
}

TransientIntegrator** OPS_GetTransientIntegrator(void) {
    if (cmds == 0) return 0;
    return cmds->getTransientIntegratorPointer();
}

ConvergenceTest** OPS_GetTest(void) {
    if (cmds == 0) return 0;
    return cmds->getCTestPointer();
}

bool* OPS_builtModel(void)
{
    static bool bltMdl = 0;
    if (cmds == 0) return 0;
    bltMdl = cmds->getBuiltModel();
    int numdata = 1;
    if (OPS_SetIntOutput(&numdata, (int*) &bltMdl, true) < 0) {
        opserr << "WARNING failed to set output\n";
        return 0;
    }

    return &bltMdl;
}

int* OPS_GetNumEigen(void)
{
    static int numEigen = 0;
    if (cmds == 0) return 0;
    numEigen = cmds->getNumEigen();
    int numdata = 1;
    if (OPS_SetIntOutput(&numdata, &numEigen, true) < 0) {
        opserr << "WARNING failed to set output\n";
        return 0;
    }

    return &numEigen;
}

int OPS_wipe()
{
    // wipe
    if (cmds != 0) {
	    cmds->wipe();
    }

    return 0;
}

int OPS_wipeAnalysis()
{
    // wipe analysis
    if (cmds != 0) {
	    cmds->wipeAnalysis();
    }

    return 0;
}

int OPS_model()
{
    // num args
    if(OPS_GetNumRemainingInputArgs() < 3) {
	opserr<<"WARNING insufficient args: model -ndm ndm <-ndf ndf>\n";
	return -1;
    }

    // model type
    const char* modeltype = OPS_GetString();
    if (strcmp(modeltype,"basic")!=0 && strcmp(modeltype,"Basic")!=0 &&
	strcmp(modeltype,"BasicBuilder")!=0 && strcmp(modeltype,"basicBuilder")!=0) {
	opserr<<"WARNING only basic builder is available at this time\n";
	return -1;
    }

    // ndm
    const char* ndmopt = OPS_GetString();
    if (strcmp(ndmopt,"-ndm") != 0) {
	opserr<<"WARNING first option must be -ndm\n";
	return -1;
    }
    int numdata = 1;
    int ndm = 0;
    if (OPS_GetIntInput(&numdata, &ndm) < 0) {
	opserr<<"WARNING failed to read ndm\n";
	return -1;
    }
    if (ndm!=1 && ndm!=2 && ndm!=3) {
	opserr<<"ERROR ndm msut be 1, 2 or 3\n";
	return -1;
    }

    // ndf
    int ndf = 0;
    if (OPS_GetNumRemainingInputArgs() > 1) {
	const char* ndfopt = OPS_GetString();
	if (strcmp(ndfopt,"-ndf") != 0) {
	    opserr<<"WARNING second option must be -ndf\n";
	    return -1;
	}
	if (OPS_GetIntInput(&numdata, &ndf) < 0) {
	    opserr<<"WARNING failed to read ndf\n";
	    return -1;
	}
    }

    // set ndf
    if(ndf <= 0) {
	if (ndm == 1)
	    ndf = 1;
	else if (ndm == 2)
	    ndf = 3;
	else if (ndm == 3)
	    ndf = 6;
    }

    // set ndm and ndf
    if (cmds != 0) {
	cmds->setNDF(ndf);
	cmds->setNDM(ndm);
    }

    cmds->setBuiltModel(true);

    return 0;
}

int OPS_System()
{
    if (OPS_GetNumRemainingInputArgs() < 1) {
    	opserr << "WARNING insufficient args: system type ...\n";
    	return -1;
    }

    const char* type = OPS_GetString();

    // create soe
    LinearSOE* theSOE = 0;

    if ((strcmp(type,"BandGeneral") == 0) || (strcmp(type,"BandGEN") == 0)
    	|| (strcmp(type,"BandGen") == 0)){
	// BAND GENERAL SOE & SOLVER
    	theSOE = (LinearSOE*)OPS_BandGenLinLapack();

    } else if (strcmp(type,"BandSPD") == 0) {
	// BAND SPD SOE & SOLVER
    	theSOE = (LinearSOE*)OPS_BandSPDLinLapack();

    } else if (strcmp(type,"Diagonal") == 0) {
	// Diagonal SOE & SOLVER
	theSOE = (LinearSOE*)OPS_DiagonalDirectSolver();


    } else if (strcmp(type,"MPIDiagonal") == 0) {
#ifdef _PARALLEL_INTERPRETERS
        MPIDiagonalSolver* theSolver = new MPIDiagonalSolver();
        theSOE = new MPIDiagonalSOE(*theSolver);
        setMPIDSOEFlag = true;
#else
	// Diagonal SOE & SOLVER
	theSOE = (LinearSOE*)OPS_DiagonalDirectSolver();
#endif

    } else if (strcmp(type,"SProfileSPD") == 0) {
	// PROFILE SPD SOE * SOLVER
	// now must determine the type of solver to create from rest of args
	theSOE = (LinearSOE*)OPS_SProfileSPDLinSolver();

    } else if (strcmp(type, "ProfileSPD") == 0) {

	theSOE = (LinearSOE*)OPS_ProfileSPDLinDirectSolver();

#ifdef _PARALLEL_INTERPRETERS
    } else if (strcmp(type, "ParallelProfileSPD") == 0) {
        ProfileSPDLinSolver* theSolver = new ProfileSPDLinDirectSolver();
        DistributedProfileSPDLinSOE* theParallelSOE = new DistributedProfileSPDLinSOE(*theSolver);
        theSOE = theParallelSOE;
        auto theMachineBroker = cmds->getMachineBroker();
        auto rank = theMachineBroker->getPID();
        auto numChannels = cmds->getNumChannels();
        auto theChannels = cmds->getChannels();
        theParallelSOE->setProcessID(rank);
        theParallelSOE->setChannels(numChannels, theChannels);
    
#endif

    } else if (strcmp(type, "PFEM") == 0) {
	// PFEM SOE & SOLVER

	if(OPS_GetNumRemainingInputArgs() < 1) {
	    theSOE = (LinearSOE*)OPS_PFEMSolver_Umfpack();
	} else {

	    const char* type = OPS_GetString();

	    if(strcmp(type, "-compressible") == 0) {

		theSOE = (LinearSOE*)OPS_PFEMCompressibleSolver();
#ifdef _MUMPS
	    } else if(strcmp(type, "-mumps") == 0) {
		
	    	theSOE = (LinearSOE*)OPS_PFEMSolver_Mumps();
#endif
	    } else if (strcmp(type, "-umfpack") == 0) {
	    theSOE = (LinearSOE*)OPS_PFEMSolver_Umfpack();
        }
	}


    } else if ((strcmp(type,"SparseGeneral") == 0) ||
	       (strcmp(type,"SuperLU") == 0) ||
	       (strcmp(type,"SparseGEN") == 0)) {

	// SPARSE GENERAL SOE * SOLVER
	theSOE = (LinearSOE*)OPS_SuperLUSolver();


    } else if ((strcmp(type,"SparseSPD") == 0) || (strcmp(type,"SparseSYM") == 0)) {
	// now must determine the type of solver to create from rest of args
	theSOE = (LinearSOE*)OPS_SymSparseLinSolver();

    } else if (strcmp(type, "UmfPack") == 0 || strcmp(type, "Umfpack") == 0) {

	theSOE = (LinearSOE*)OPS_UmfpackGenLinSolver();

    } else if (strcmp(type,"FullGeneral") == 0) {
	// now must determine the type of solver to create from rest of args
	theSOE = (LinearSOE*)OPS_FullGenLinLapackSolver();

    } else if (strcmp(type,"Petsc") == 0) {
	    
#ifdef _MUMPS
    } else if (strcmp(type,"Mumps") == 0) {
        theSOE = (LinearSOE*)OPS_MumpsSolver();
#endif
    } else {
    	opserr<<"WARNING unknown system type "<<type<<"\n";
    	return -1;
    }

    // set soe
    if (cmds != 0) {
	cmds->setSOE(theSOE);
    }

    return 0;
}

int OPS_Numberer()
{
    if (OPS_GetNumRemainingInputArgs() < 1) {
    	opserr << "WARNING insufficient args: numberer type ...\n";
    	return -1;
    }

    const char* type = OPS_GetString();

    // create numberer
    DOF_Numberer *theNumberer = 0;
    if (strcmp(type,"Plain") == 0) {

    	theNumberer = (DOF_Numberer*)OPS_PlainNumberer();

    } else if (strcmp(type,"RCM") == 0) {

    	RCM *theRCM = new RCM(false);
    	theNumberer = new DOF_Numberer(*theRCM);

    } else if (strcmp(type,"AMD") == 0) {

        AMD *theAMD = new AMD();
        theNumberer = new DOF_Numberer(*theAMD);
    } else if (strcmp(type, "ParallelPlain") == 0) {

        theNumberer = (DOF_Numberer*)OPS_ParallelNumberer();

    } else if (strcmp(type, "ParallelRCM") == 0) {

        theNumberer = (DOF_Numberer*)OPS_ParallelRCM();

    } else {
    	opserr<<"WARNING unknown numberer type "<<type<<"\n";
    	return -1;
    }

    // set numberer
    if (cmds != 0) {
	cmds->setNumberer(theNumberer);
    }

    return 0;
}

int OPS_ConstraintHandler()
{
    if (OPS_GetNumRemainingInputArgs() < 1) {
    	opserr << "WARNING insufficient args: constraints type ...\n";
    	return -1;
    }

    const char* type = OPS_GetString();

    // create handler
    ConstraintHandler* theHandler = 0;
    if (strcmp(type,"Plain") == 0) {

    	theHandler = (ConstraintHandler*)OPS_PlainHandler();

    } else if (strcmp(type,"Penalty") == 0) {

    	theHandler = (ConstraintHandler*)OPS_PenaltyConstraintHandler();

    } else if (strcmp(type,"Lagrange") == 0) {

    	theHandler = (ConstraintHandler*)OPS_LagrangeConstraintHandler();

    } else if (strcmp(type,"Transformation") == 0) {
    	theHandler = (ConstraintHandler*)OPS_TransformationConstraintHandler();

    } else if (strcmp(type,"Auto") == 0) {
    	theHandler = (ConstraintHandler*)OPS_AutoConstraintHandler();

    } else {
    	opserr<<"WARNING unknown ConstraintHandler type "<<type<<"\n";
    	return -1;
    }

    // set handler
    if (cmds != 0) {
	cmds->setHandler(theHandler);
    }

    return 0;
}

int OPS_CTest()
{
    if (OPS_GetNumRemainingInputArgs() < 1) {
    	opserr << "WARNING insufficient args: test type ...\n";
    	return -1;
    }

    const char* type = OPS_GetString();

    // create ctest
    ConvergenceTest* theTest = 0;
    if (strcmp(type,"NormDispAndUnbalance") == 0) {
	theTest = (ConvergenceTest*)OPS_NormDispAndUnbalance();

    } else if (strcmp(type,"NormDispOrUnbalance") == 0) {
	theTest = (ConvergenceTest*)OPS_NormDispOrUnbalance();

    } else if (strcmp(type,"PFEM") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestPFEM();

    } else if (strcmp(type,"FixedNumIter") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestFixedNumIter();

    } else if (strcmp(type,"NormUnbalance") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestNormUnbalance();

    } else if (strcmp(type,"NormDispIncr") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestNormDispIncr();

    } else if (strcmp(type,"EnergyIncr") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestEnergyIncr();

    } else if (strcmp(type,"RelativeNormUnbalance") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestRelativeNormUnbalance();


    } else if (strcmp(type,"RelativeNormDispIncr") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestRelativeNormDispIncr();

    } else if (strcmp(type,"RelativeEnergyIncr") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestRelativeEnergyIncr();

    } else if (strcmp(type,"RelativeTotalNormDispIncr") == 0) {
	theTest = (ConvergenceTest*)OPS_CTestRelativeTotalNormDispIncr();

    } else {

	opserr<<"WARNING unknown CTest type "<<type<<"\n";
    	return -1;
    }

    // set test
    if (cmds != 0) {
	cmds->setCTest(theTest);
    }

    return 0;
}

int OPS_Integrator()
{
    if (OPS_GetNumRemainingInputArgs() < 1) {
    	opserr << "WARNING insufficient args: integrator type ...\n";
    	return -1;
    }

    const char* type = OPS_GetString();

    // create integrator
    StaticIntegrator* si = 0;
    TransientIntegrator* ti = 0;
    if (strcmp(type,"LoadControl") == 0) {

	si = (StaticIntegrator*)OPS_LoadControlIntegrator();

    } else if (strcmp(type,"DisplacementControl") == 0) {

	si = (StaticIntegrator*)OPS_DisplacementControlIntegrator();

    } else if (strcmp(type,"ParallelDisplacementControl") == 0) {

        si = (StaticIntegrator*)OPS_ParallelDisplacementControl();

    } else if (strcmp(type,"ArcLength") == 0) {
	si = (StaticIntegrator*)OPS_ArcLength();

    } else if (strcmp(type,"ArcLength1") == 0) {
	si = (StaticIntegrator*)OPS_ArcLength1();

    } else if (strcmp(type,"HSConstraint") == 0) {
	si = (StaticIntegrator*)OPS_HSConstraint();

    } else if (strcmp(type,"MinUnbalDispNorm") == 0) {
	si = (StaticIntegrator*)OPS_MinUnbalDispNorm();

    } else if (strcmp(type,"HarmonicSteadyState") == 0 || strcmp(type,"HarmonicSS") == 0) {
	si = (StaticIntegrator*)OPS_HarmonicSteadyState();

    } else if (strcmp(type,"Newmark") == 0) {
	ti = (TransientIntegrator*)OPS_Newmark();

    } else if (strcmp(type,"GimmeMCK") == 0 || strcmp(type,"ZZTop") == 0) {
	ti = (TransientIntegrator*)OPS_GimmeMCK();

    } else if (strcmp(type,"TRBDF2") == 0 || strcmp(type,"Bathe") == 0) {
	ti = (TransientIntegrator*)OPS_TRBDF2();

    } else if (strcmp(type,"TRBDF3") == 0 || strcmp(type,"Bathe3") == 0) {
	ti = (TransientIntegrator*)OPS_TRBDF3();

    } else if (strcmp(type,"Houbolt") == 0) {
	ti = (TransientIntegrator*)OPS_Houbolt();

    } else if (strcmp(type,"BackwardEuler") == 0) {
	ti = (TransientIntegrator*)OPS_BackwardEuler();

    } else if (strcmp(type,"PFEM") == 0) {
	ti = (TransientIntegrator*)OPS_PFEMIntegrator();

    } else if (strcmp(type,"NewmarkExplicit") == 0) {
	ti = (TransientIntegrator*)OPS_NewmarkExplicit();

    } else if (strcmp(type,"NewmarkHSIncrLimit") == 0) {
	ti = (TransientIntegrator*)OPS_NewmarkHSIncrLimit();

    } else if (strcmp(type,"NewmarkHSIncrReduct") == 0) {
	ti = (TransientIntegrator*)OPS_NewmarkHSIncrReduct();

    } else if (strcmp(type,"NewmarkHSFixedNumIter") == 0) {
	ti = (TransientIntegrator*)OPS_NewmarkHSFixedNumIter();

    } else if (strcmp(type,"HHT") == 0) {
	ti = (TransientIntegrator*)OPS_HHT();

    } else if (strcmp(type,"HHT_TP") == 0) {
	ti = (TransientIntegrator*)OPS_HHT_TP();

    } else if (strcmp(type,"HHTGeneralized") == 0) {
	ti = (TransientIntegrator*)OPS_HHTGeneralized();

    } else if (strcmp(type,"HHTGeneralized_TP") == 0) {
	ti = (TransientIntegrator*)OPS_HHTGeneralized_TP();

    } else if (strcmp(type,"HHTExplicit") == 0) {
	ti = (TransientIntegrator*)OPS_HHTExplicit();

    } else if (strcmp(type,"HHTExplicit_TP") == 0) {
	ti = (TransientIntegrator*)OPS_HHTExplicit_TP();

    } else if (strcmp(type,"HHTGeneralizedExplicit") == 0) {
	ti = (TransientIntegrator*)OPS_HHTGeneralizedExplicit();

    } else if (strcmp(type,"HHTGeneralizedExplicit_TP") == 0) {
	ti = (TransientIntegrator*)OPS_HHTGeneralizedExplicit_TP();

    } else if (strcmp(type,"HHTHSIncrLimit") == 0) {
	ti = (TransientIntegrator*)OPS_HHTHSIncrLimit();

    } else if (strcmp(type,"HHTHSIncrLimit_TP") == 0) {
	ti = (TransientIntegrator*)OPS_HHTHSIncrLimit_TP();

    } else if (strcmp(type,"HHTHSIncrReduct") == 0) {
	ti = (TransientIntegrator*)OPS_HHTHSIncrReduct();

    } else if (strcmp(type,"HHTHSIncrReduct_TP") == 0) {
	ti = (TransientIntegrator*)OPS_HHTHSIncrReduct_TP();

    } else if (strcmp(type,"HHTHSFixedNumIter") == 0) {
	ti = (TransientIntegrator*)OPS_HHTHSFixedNumIter();

    } else if (strcmp(type,"HHTHSFixedNumIter_TP") == 0) {
	ti = (TransientIntegrator*)OPS_HHTHSFixedNumIter_TP();

    } else if (strcmp(type,"GeneralizedAlpha") == 0) {
	ti = (TransientIntegrator*)OPS_GeneralizedAlpha();

    } else if (strcmp(type,"KRAlphaExplicit") == 0) {
	ti = (TransientIntegrator*)OPS_KRAlphaExplicit();

    } else if (strcmp(type,"KRAlphaExplicit_TP") == 0) {
	ti = (TransientIntegrator*)OPS_KRAlphaExplicit_TP();

    } else if (strcmp(type,"AlphaOS") == 0) {
	ti = (TransientIntegrator*)OPS_AlphaOS();

    } else if (strcmp(type,"AlphaOS_TP") == 0) {
	ti = (TransientIntegrator*)OPS_AlphaOS_TP();

    } else if (strcmp(type,"AlphaOSGeneralized") == 0) {
	ti = (TransientIntegrator*)OPS_AlphaOSGeneralized();

    } else if (strcmp(type,"AlphaOSGeneralized_TP") == 0) {
	ti = (TransientIntegrator*)OPS_AlphaOSGeneralized_TP();

    } else if (strcmp(type,"Collocation") == 0) {
	ti = (TransientIntegrator*)OPS_Collocation();

    } else if (strcmp(type,"CollocationHSIncrReduct") == 0) {
	ti = (TransientIntegrator*)OPS_CollocationHSIncrReduct();

    } else if (strcmp(type,"CollocationHSIncrLimit") == 0) {
	ti = (TransientIntegrator*)OPS_CollocationHSIncrLimit();

    } else if (strcmp(type,"CollocationHSFixedNumIter") == 0) {
	ti = (TransientIntegrator*)OPS_CollocationHSFixedNumIter();

    } else if (strcmp(type,"Newmark1") == 0) {
	ti = (TransientIntegrator*)OPS_Newmark1();

    } else if (strcmp(type,"WilsonTheta") == 0) {
	ti = (TransientIntegrator*)OPS_WilsonTheta();

    } else if (strcmp(type,"CentralDifference") == 0) {
	ti = (TransientIntegrator*)OPS_CentralDifference();

    } else if (strcmp(type,"CentralDifferenceAlternative") == 0) {
	ti = (TransientIntegrator*)OPS_CentralDifferenceAlternative();

    } else if (strcmp(type,"CentralDifferenceNoDamping") == 0) {
	ti = (TransientIntegrator*)OPS_CentralDifferenceNoDamping();

	} else if (strcmp(type, "ExplicitDifference") == 0) {
    ti = (TransientIntegrator*)OPS_ExplicitDifference();

    } else {
	opserr<<"WARNING unknown integrator type "<<type<<"\n";
    }

    // set integrator
    if (si != 0) {
	if (cmds != 0) {
	    cmds->setStaticIntegrator(si);
	}
    } else if (ti != 0) {
	if (cmds != 0) {
	    cmds->setTransientIntegrator(ti);
	}
    }

    return 0;
}

int OPS_Algorithm()
{
    if (OPS_GetNumRemainingInputArgs() < 1) {
    	opserr << "WARNING insufficient args: algorithm type ...\n";
    	return -1;
    }

    const char* type = OPS_GetString();

    // create algorithm
    EquiSolnAlgo* theAlgo = 0;
    if (strcmp(type, "Linear") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_LinearAlgorithm();

    } else if (strcmp(type, "Newton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_NewtonRaphsonAlgorithm();

    } else if (strcmp(type, "ModifiedNewton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_ModifiedNewton();

    } else if (strcmp(type, "KrylovNewton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_KrylovNewton();

    } else if (strcmp(type, "RaphsonNewton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_RaphsonNewton();

    } else if (strcmp(type, "MillerNewton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_MillerNewton();

    } else if (strcmp(type, "SecantNewton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_SecantNewton();

    } else if (strcmp(type, "PeriodicNewton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_PeriodicNewton();

    } else if (strcmp(type, "ExpressNewton") == 0) {
	theAlgo = (EquiSolnAlgo*) OPS_ExpressNewton();	

    } else if (strcmp(type, "Broyden") == 0) {
	theAlgo = (EquiSolnAlgo*)OPS_Broyden();

    } else if (strcmp(type, "BFGS") == 0) {
	theAlgo = (EquiSolnAlgo*)OPS_BFGS();

    } else if (strcmp(type, "NewtonLineSearch") == 0) {
	theAlgo = (EquiSolnAlgo*)OPS_NewtonLineSearch();

    } else {
	opserr<<"WARNING unknown algorithm type "<<type<<"\n";
    }

    // set algorithm
    if (theAlgo != 0) {
	if (cmds != 0) {
	    cmds->setAlgorithm(theAlgo);
	}
    }

    return 0;
}

int OPS_Analysis() {
  if (OPS_GetNumRemainingInputArgs() < 1) {
    opserr << "WARNING insufficient args: analysis type ...\n";
    return -1;
  }

  const char* type = OPS_GetString();
  bool suppressWarnings = false;
  if (OPS_GetNumRemainingInputArgs() > 0) {
    const char* opt = OPS_GetString();
    if (strcmp(opt, "-noWarnings") == 0) {
        suppressWarnings = true;
    } else {
        OPS_ResetCurrentInputArg(-1);
    }
  }

  // create analysis
  if (strcmp(type, "Static") == 0) {
    if (cmds != 0) {
      cmds->setStaticAnalysis(suppressWarnings);
    }
  } else if (strcmp(type, "Transient") == 0) {
    if (cmds != 0) {
      cmds->setTransientAnalysis(suppressWarnings);
    }
  } else if (strcmp(type, "PFEM") == 0) {
    if (cmds != 0) {
      if (cmds->setPFEMAnalysis(suppressWarnings) < 0) {
        return -1;
      }
    }
  } else if (strcmp(type, "VariableTimeStepTransient") == 0 ||
             (strcmp(type, "TransientWithVariableTimeStep") ==
              0) ||
             (strcmp(type, "VariableTransient") == 0)) {
    if (cmds != 0) {
      cmds->setVariableAnalysis(suppressWarnings);
    }

  } else {
    opserr << "WARNING unknown analysis type " << type << "\n";
  }

  return 0;
}

int OPS_analyze() {
  if (cmds == 0) return 0;

  int result = 0;
  StaticAnalysis* theStaticAnalysis = cmds->getStaticAnalysis();
  TransientAnalysis* theTransientAnalysis =
      cmds->getTransientAnalysis();
  PFEMAnalysis* thePFEMAnalysis = cmds->getPFEMAnalysis();

  if (theStaticAnalysis != 0) {
    if (OPS_GetNumRemainingInputArgs() < 1) {
      opserr << "WARNING insufficient args: analyze numIncr "
                "<-noFlush> ...\n";
      return -1;
    }
    int numIncr;
    int numdata = 1;
    if (OPS_GetIntInput(&numdata, &numIncr) < 0) {
      opserr << "WARNING: invalid numIncr\n";
      return -1;
    }

    bool flush = true;
    if (OPS_GetNumRemainingInputArgs() > 0) {
      const char* opt = OPS_GetString();
      if (strcmp(opt, "-noFlush") == 0) {
        flush = false;
      }
    }
    result = theStaticAnalysis->analyze(numIncr, flush);

  } else if (thePFEMAnalysis != 0) {
    bool flush = true;
    if (OPS_GetNumRemainingInputArgs() > 0) {
      const char* opt = OPS_GetString();
      if (strcmp(opt, "-noFlush") == 0) {
        flush = false;
      }
    }
    result = thePFEMAnalysis->analyze(flush);

  } else if (theTransientAnalysis != 0) {
    if (OPS_GetNumRemainingInputArgs() < 2) {
      opserr << "WARNING insufficient args: analyze numIncr deltaT "
                "...\n";
      return -1;
    }
    int numIncr;
    int numdata = 1;
    if (OPS_GetIntInput(&numdata, &numIncr) < 0) {
      opserr << "WARNING: invalid numIncr\n";
      return -1;
    }

    double dt;
    if (OPS_GetDoubleInput(&numdata, &dt) < 0) {
      opserr << "WARNING: invalid dt\n";
      return -1;
    }
    ops_Dt = dt;

    if (OPS_GetNumRemainingInputArgs() == 0) {
      result = theTransientAnalysis->analyze(numIncr, dt, true);
    } else if (OPS_GetNumRemainingInputArgs() == 1) {
      bool flush = true;
      if (OPS_GetNumRemainingInputArgs() > 0) {
        const char* opt = OPS_GetString();
        if (strcmp(opt, "-noFlush") == 0) {
            flush = false;
        }
      }
      result = theTransientAnalysis->analyze(numIncr, dt, flush);

    } else if (OPS_GetNumRemainingInputArgs() < 3) {
      opserr << "WARNING insufficient args for variable transient "
                "need: dtMin dtMax Jd \n";
      opserr << "n_args" << OPS_GetNumRemainingInputArgs() << "\n";
      return -1;

    } else {
      double dtMin;
      if (OPS_GetDoubleInput(&numdata, &dtMin) < 0) {
        opserr << "WARNING: invalid dtMin\n";
        return -1;
      }
      double dtMax;
      if (OPS_GetDoubleInput(&numdata, &dtMax) < 0) {
        opserr << "WARNING: invalid dtMax\n";
        return -1;
      }
      int Jd;
      if (OPS_GetIntInput(&numdata, &Jd) < 0) {
        opserr << "WARNING: invalid Jd\n";
        return -1;
      }
      bool flush = true;
      if (OPS_GetNumRemainingInputArgs() > 0) {
        const char* opt = OPS_GetString();
        if (strcmp(opt, "-noFlush") == 0) {
            flush = false;
        }
      }
      // Included getVariableAnalysis here as dont need it except
      // for here. and analyze() is called a lot.
      VariableTimeStepDirectIntegrationAnalysis*
          theVariableTimeStepTransientAnalysis =
              cmds->getVariableAnalysis();
      result = theVariableTimeStepTransientAnalysis->analyze(
          numIncr, dt, dtMin, dtMax, Jd, flush);
    }
  } else {
    opserr << "WARNING No Analysis type has been specified \n";
    return -1;
  }

  if (result < 0) {
    opserr << "OpenSees > analyze failed, returned: " << result
           << " error flag\n";
  }

  int numdata = 1;
  if (OPS_SetIntOutput(&numdata, &result, true) < 0) {
    opserr << "WARNING failed to set output\n";
    return -1;
  }

  return 0;
}

int OPS_eigenAnalysis()
{
    static bool warning_displayed = false;

    // make sure at least one other argument to contain type of system
    if (OPS_GetNumRemainingInputArgs() < 1) {
	opserr << "WARNING want - eigen <type> numModes?\n";
	return -1;
    }

    // 0 - frequency/generalized (default),1 - standard, 2 - buckling
    bool generalizedAlgo = true;


    int typeSolver = EigenSOE_TAGS_ArpackSOE;
    double shift = 0.0;
    bool findSmallest = true;

    // Check type of eigenvalue analysis
    while (OPS_GetNumRemainingInputArgs() > 1) {

	const char* type = OPS_GetString();

	if ((strcmp(type,"frequency") == 0) ||
	    (strcmp(type,"-frequency") == 0) ||
	    (strcmp(type,"generalized") == 0) ||
	    (strcmp(type,"-generalized") == 0))
	    generalizedAlgo = true;

	else if ((strcmp(type,"standard") == 0) ||
		 (strcmp(type,"-standard") == 0))
	    generalizedAlgo = false;

	else if ((strcmp(type,"-findLargest") == 0))
	    findSmallest = false;

	else if ((strcmp(type,"genBandArpack") == 0) ||
		 (strcmp(type,"-genBandArpack") == 0) ||
		 (strcmp(type,"genBandArpackEigen") == 0) ||
		 (strcmp(type,"-genBandArpackEigen") == 0))
	    typeSolver = EigenSOE_TAGS_ArpackSOE;

	else if ((strcmp(type,"symmBandLapack") == 0) ||
		 (strcmp(type,"-symmBandLapack") == 0) ||
		 (strcmp(type,"symmBandLapackEigen") == 0) ||
		 (strcmp(type,"-symmBandLapackEigen") == 0))
	    typeSolver = EigenSOE_TAGS_SymBandEigenSOE;

    else if ((strcmp(type, "fullGenLapack") == 0) ||
                (strcmp(type, "-fullGenLapack") == 0) ||
                (strcmp(type, "fullGenLapackEigen") == 0) ||
                (strcmp(type, "-fullGenLapackEigen") == 0)) {
	    if (!warning_displayed) {
            opserr << "\nWARNING - the 'fullGenLapack' eigen solver is VERY SLOW. Consider using the default eigen solver.\n";
            warning_displayed = true;
		}
        typeSolver = EigenSOE_TAGS_FullGenEigenSOE;
    }

    else {
        opserr << "eigen - unknown option specified " << type
                << endln;
    }
    }

    // check argv[loc] for number of modes
    int numEigen;
    int numdata = 1;
    if (OPS_GetIntInput(&numdata, &numEigen) < 0) {
	opserr << "WARNING eigen numModes?  - can't read numModes\n";
	return -1;
    }

    if (numEigen < 1) {
	opserr << "WARNING eigen numModes?  - illegal numModes: " << numEigen << "\n";
	return -1;
    }
    cmds->setNumEigen(numEigen);

    // set eigen soe
    if (cmds->eigen(typeSolver,shift,generalizedAlgo,findSmallest) < 0) {
	opserr<<"WANRING failed to do eigen analysis\n";
	return -1;
    }


    return 0;
}

int OPS_resetModel()
{
    if (cmds == 0) return 0;
    Domain* theDomain = OPS_GetDomain();
    if (theDomain != 0) {
	theDomain->revertToStart();
    }
    TransientIntegrator* theTransientIntegrator = cmds->getTransientIntegrator();
    if (theTransientIntegrator != 0) {
	theTransientIntegrator->revertToStart();
    }

    return 0;
}

int OPS_initializeAnalysis()
{
    if (cmds == 0) return 0;
    DirectIntegrationAnalysis* theTransientAnalysis =
	cmds->getTransientAnalysis();
	VariableTimeStepDirectIntegrationAnalysis* theVariableTimeStepTransientAnalysis =
	cmds->getVariableAnalysis();

    StaticAnalysis* theStaticAnalysis =
	cmds->getStaticAnalysis();

    if (theTransientAnalysis != 0) {
	theTransientAnalysis->initialize();
    }else if (theStaticAnalysis != 0) {
	theStaticAnalysis->initialize();
    }

    Domain* theDomain = OPS_GetDomain();
    if (theDomain != 0) {
	theDomain->initialize();
    }

    return 0;
}

int OPS_printA()
{
    if (cmds == 0) return 0;
    FileStream outputFile;
    OPS_Stream *output = &opserr;

    bool ret = false;
    if (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if ((strcmp(flag,"file") == 0) || (strcmp(flag,"-file") == 0)) {

	    const char* filename = OPS_GetString();
	    if (outputFile.setFile(filename) != 0) {
		opserr << "printA <filename> .. - failed to open file: " << filename << endln;
		return -1;
	    }
	    output = &outputFile;
	} else if((strcmp(flag,"ret") == 0) || (strcmp(flag,"-ret") == 0)) {
	    ret = true;
	}
    }

    LinearSOE* theSOE = cmds->getSOE();
    StaticIntegrator* theStaticIntegrator = cmds->getStaticIntegrator();
    TransientIntegrator* theTransientIntegrator = cmds->getTransientIntegrator();

    if (theSOE != 0) {
	if (theStaticIntegrator != 0) {
	    theStaticIntegrator->formTangent();
	} else if (theTransientIntegrator != 0) {
	    theTransientIntegrator->formTangent(0);
	}

    PFEMLinSOE* pfemsoe = dynamic_cast<PFEMLinSOE*>(theSOE);
    if (pfemsoe != 0) {
        pfemsoe->saveK(*output);
        outputFile.close();
        return 0;
    }

	Matrix *A = const_cast<Matrix*>(theSOE->getA());
	if (A != 0) {
	    if (ret) {
		int size = A->noRows() * A->noCols();
		if (size >0) {
		    double& ptr = (*A)(0,0);
		    if (OPS_SetDoubleOutput(&size, &ptr, false) < 0) {
			opserr << "WARNING: printA - failed to set output\n";
			return -1;
		    }
		}
	    } else {
		*output << *A;
	    }
	} else {
        int size = 0;
        double *ptr = 0;
        if (OPS_SetDoubleOutput(&size, ptr, false) < 0) {
            opserr << "WARNING: printA - failed to set output\n";
            return -1;
        }
	}
    } else {
        int size = 0;
        double *ptr = 0;
        if (OPS_SetDoubleOutput(&size, ptr, false) < 0) {
            opserr << "WARNING: printA - failed to set output\n";
            return -1;
        }
    }

    // close the output file
    outputFile.close();

    return 0;
}

int OPS_printB()
{
    if (cmds == 0) return 0;
    FileStream outputFile;
    OPS_Stream *output = &opserr;

    LinearSOE* theSOE = cmds->getSOE();
    StaticIntegrator* theStaticIntegrator = cmds->getStaticIntegrator();
    TransientIntegrator* theTransientIntegrator = cmds->getTransientIntegrator();

    bool ret = false;
    if (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if ((strcmp(flag,"file") == 0) || (strcmp(flag,"-file") == 0)) {
	    const char* filename = OPS_GetString();

	    if (outputFile.setFile(filename) != 0) {
		opserr << "printB <filename> .. - failed to open file: " << filename << endln;
		return -1;
	    }
	    output = &outputFile;
	} else if((strcmp(flag,"ret") == 0) || (strcmp(flag,"-ret") == 0)) {
	    ret = true;
	}
    }
    if (theSOE != 0) {
	if (theStaticIntegrator != 0) {
	    theStaticIntegrator->formUnbalance();
	} else if (theTransientIntegrator != 0) {
	    theTransientIntegrator->formUnbalance();
	}

	Vector &b = const_cast<Vector&>(theSOE->getB());
	if (ret) {
	    int size = b.Size();
	    if (size > 0) {
		double &ptr = b(0);
		if (OPS_SetDoubleOutput(&size, &ptr, false) < 0) {
		    opserr << "WARNING: printB - failed to set output\n";
		    return -1;
		}
	    } else {
            size = 0;
            double *ptr2 = 0;
            if (OPS_SetDoubleOutput(&size, ptr2, false) < 0) {
                opserr << "WARNING: printB - failed to set output\n";
                return -1;
            }
	    }
	} else {
	    *output << b;
	}
    } else {
        int size = 0;
        double *ptr = 0;
        if (OPS_SetDoubleOutput(&size, ptr, false) < 0) {
            opserr << "WARNING: printB - failed to set output\n";
            return -1;
        }
    }

    // close the output file
    outputFile.close();

    return 0;
}

int OPS_printX()
{
    if (cmds == 0) return 0;
    FileStream outputFile;
    OPS_Stream *output = &opserr;

    LinearSOE* theSOE = cmds->getSOE();
    //StaticIntegrator* theStaticIntegrator = cmds->getStaticIntegrator();
    //TransientIntegrator* theTransientIntegrator = cmds->getTransientIntegrator();

    bool ret = false;
    if (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if ((strcmp(flag,"file") == 0) || (strcmp(flag,"-file") == 0)) {
	    const char* filename = OPS_GetString();

	    if (outputFile.setFile(filename) != 0) {
		opserr << "printX <filename> .. - failed to open file: " << filename << endln;
		return -1;
	    }
	    output = &outputFile;
	} else if((strcmp(flag,"ret") == 0) || (strcmp(flag,"-ret") == 0)) {
	    ret = true;
	}
    }
    if (theSOE != 0) {
      /*
	if (theStaticIntegrator != 0) {
	  theStaticIntegrator->formUnbalance();
	} else if (theTransientIntegrator != 0) {
	  theTransientIntegrator->formUnbalance();
	}
      */
	Vector &x = const_cast<Vector&>(theSOE->getX());
	if (ret) {
	    int size = x.Size();
	    if (size > 0) {
		double &ptr = x(0);
		if (OPS_SetDoubleOutput(&size, &ptr, false) < 0) {
		    opserr << "WARNING: printX - failed to set output\n";
		    return -1;
		}
	    } else {
            size = 0;
            double *ptr2 = 0;
            if (OPS_SetDoubleOutput(&size, ptr2, false) < 0) {
                opserr << "WARNING: printX - failed to set output\n";
                return -1;
            }
	    }
	} else {
	    *output << x;
	}
    } else {
        int size = 0;
        double *ptr = 0;
        if (OPS_SetDoubleOutput(&size, ptr, false) < 0) {
            opserr << "WARNING: printX - failed to set output\n";
            return -1;
        }
    }

    // close the output file
    outputFile.close();

    return 0;
}

int printNode(OPS_Stream& output);
int printElement(OPS_Stream& output);
int printAlgorithm(OPS_Stream& output);
int printIntegrator(OPS_Stream& output);

int OPS_printModel()
{
    int res = 0;

    int flag = OPS_PRINT_CURRENTSTATE;

    FileStream outputFile;
    OPS_Stream *output = &opserr;
    bool done = false;

    Domain* theDomain = OPS_GetDomain();
    if (theDomain == 0) return -1;

    // if just 'print' then print out the entire domain
    if (OPS_GetNumRemainingInputArgs() < 1) {
        opserr << *theDomain;
        return 0;
    }

    while (done == false && OPS_GetNumRemainingInputArgs() > 0) {

        const char* arg = OPS_GetString();

        // if 'print ele i j k..' print out some elements
        if ((strcmp(arg, "-ele") == 0) || (strcmp(arg, "ele") == 0)) {
            res = printElement(*output);
            done = true;
        }
        // if 'print node i j k ..' print out some nodes
        else if ((strcmp(arg, "-node") == 0) || (strcmp(arg, "node") == 0)) {
            res = printNode(*output);
            done = true;
        }

        // if 'print integrator flag' print out the integrator
        else if ((strcmp(arg, "integrator") == 0) || (strcmp(arg, "-integrator") == 0)) {
            res = printIntegrator(*output);
            done = true;
        }

        // if 'print algorithm flag' print out the algorithm
        else if ((strcmp(arg, "algorithm") == 0) || (strcmp(arg, "-algorithm") == 0)) {
            res = printAlgorithm(*output);
            done = true;
        }

        // if 'print -JSON' print using JSON format
        else if ((strcmp(arg, "JSON") == 0) || (strcmp(arg, "-JSON") == 0)) {
            flag = OPS_PRINT_PRINTMODEL_JSON;
        }

        else {

            if ((strcmp(arg, "file") == 0) || (strcmp(arg, "-file") == 0)) {}

            if (OPS_GetNumRemainingInputArgs() < 1) break;
            const char* filename = OPS_GetString();

            openMode mode = APPEND;
            if (flag == OPS_PRINT_PRINTMODEL_JSON)
                mode = OVERWRITE;
            if (outputFile.setFile(filename, mode) != 0) {
                opserr << "print <filename> .. - failed to open file: " << filename << endln;
                return -1;
            }

            SimulationInformation* simulationInfo = cmds->getSimulationInformation();
            if (simulationInfo == 0) return -1;

            // if just 'print <filename>' then print out the entire domain to eof
            if (OPS_GetNumRemainingInputArgs() < 1) {
                if (flag == OPS_PRINT_PRINTMODEL_JSON)
                    simulationInfo->Print(outputFile, flag);
                theDomain->Print(outputFile, flag);
                return 0;
            }

            output = &outputFile;

        }
    }

    // close the output file
    outputFile.close();
    return res;
}

void* OPS_KrylovNewton()
{
    if (cmds == 0) return 0;
    int incrementTangent = CURRENT_TANGENT;
    int iterateTangent = CURRENT_TANGENT;
    int maxDim = 3;
    while (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if (strcmp(flag,"-iterate") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		iterateTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		iterateTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		iterateTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-increment") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		incrementTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		incrementTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		incrementTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-maxDim") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    maxDim = atoi(flag);
	    int numdata = 1;
	    if (OPS_GetIntInput(&numdata, &maxDim) < 0) {
		opserr<< "WARNING KrylovNewton failed to read maxDim\n";
		return 0;
	    }
	}
    }

    ConvergenceTest* theTest = cmds->getCTest();
    if (theTest == 0) {
      opserr << "ERROR: No ConvergenceTest yet specified\n";
      return 0;
    }

    Accelerator *theAccel;
    theAccel = new KrylovAccelerator(maxDim, iterateTangent);

    return new AcceleratedNewton(*theTest, theAccel, incrementTangent);
}

void* OPS_RaphsonNewton()
{
    if (cmds == 0) return 0;
    int incrementTangent = CURRENT_TANGENT;
    int iterateTangent = CURRENT_TANGENT;

    while (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if (strcmp(flag,"-iterate") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		iterateTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		iterateTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		iterateTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-increment") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		incrementTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		incrementTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		incrementTangent = NO_TANGENT;
	    }
	}
    }

    ConvergenceTest* theTest = cmds->getCTest();
    if (theTest == 0) {
      opserr << "ERROR: No ConvergenceTest yet specified\n";
      return 0;
    }

    Accelerator *theAccel;
    theAccel = new RaphsonAccelerator(iterateTangent);

    return new AcceleratedNewton(*theTest, theAccel, incrementTangent);
}

void* OPS_MillerNewton()
{
    if (cmds == 0) return 0;
    int incrementTangent = CURRENT_TANGENT;
    int iterateTangent = CURRENT_TANGENT;
    int maxDim = 3;
    while (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if (strcmp(flag,"-iterate") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		iterateTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		iterateTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		iterateTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-increment") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		incrementTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		incrementTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		incrementTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-maxDim") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    maxDim = atoi(flag);
	    int numdata = 1;
	    if (OPS_GetIntInput(&numdata, &maxDim) < 0) {
		opserr<< "WARNING KrylovNewton failed to read maxDim\n";
		return 0;
	    }
	}
    }

    ConvergenceTest* theTest = cmds->getCTest();
    if (theTest == 0) {
      opserr << "ERROR: No ConvergenceTest yet specified\n";
      return 0;
    }

    Accelerator *theAccel = 0;
    return new AcceleratedNewton(*theTest, theAccel, incrementTangent);
}

void* OPS_SecantNewton()
{
    if (cmds == 0) return 0;
    int incrementTangent = CURRENT_TANGENT;
    int iterateTangent = CURRENT_TANGENT;
    int maxDim = 3;
    int numTerms = 2;
    bool cutOut = false;
    double R[2];
    while (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if (strcmp(flag,"-iterate") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		iterateTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		iterateTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		iterateTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-increment") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		incrementTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		incrementTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		incrementTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-maxDim") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    int numdata = 1;
	    if (OPS_GetIntInput(&numdata, &maxDim) < 0) {
		opserr<< "WARNING SecantNewton failed to read maxDim\n";
		return 0;
	    }
	} else if (strcmp(flag,"-numTerms") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    int numdata = 1;
	    if (OPS_GetIntInput(&numdata, &numTerms) < 0) {
		opserr<< "WARNING SecantNewton failed to read maxDim\n";
		return 0;
	    }
	} else if ((strcmp(flag,"-cutOut") == 0 || strcmp(flag,"-cutout") == 0)
		   && OPS_GetNumRemainingInputArgs() > 1) {
	  int numdata = 2;
	  if (OPS_GetDoubleInput(&numdata, R) < 0) {
	    opserr << "WARNING SecantNewton failed to read cutOut values R1 and R2" << endln;
	    return 0;
	  }
	  cutOut = true;
	}
    }

    ConvergenceTest* theTest = cmds->getCTest();
    if (theTest == 0) {
      opserr << "ERROR: No ConvergenceTest yet specified\n";
      return 0;
    }

    Accelerator *theAccel = 0;
    if (numTerms <= 1)
      if (cutOut)
	theAccel = new SecantAccelerator1(maxDim, iterateTangent, R[0], R[1]);
      else
	theAccel = new SecantAccelerator1(maxDim, iterateTangent);
    if (numTerms >= 3)
      if (cutOut)
	theAccel = new SecantAccelerator3(maxDim, iterateTangent, R[0], R[1]);
      else
	theAccel = new SecantAccelerator3(maxDim, iterateTangent);
    if (numTerms == 2)
      if (cutOut)
	theAccel = new SecantAccelerator2(maxDim, iterateTangent, R[0], R[1]);
      else
	theAccel = new SecantAccelerator2(maxDim, iterateTangent);            

    return new AcceleratedNewton(*theTest, theAccel, incrementTangent);
}

void* OPS_PeriodicNewton()
{
    if (cmds == 0) return 0;
    int incrementTangent = CURRENT_TANGENT;
    int iterateTangent = CURRENT_TANGENT;
    int maxDim = 3;
    while (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if (strcmp(flag,"-iterate") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		iterateTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		iterateTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		iterateTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-increment") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2,"current") == 0) {
		incrementTangent = CURRENT_TANGENT;
	    }
	    if (strcmp(flag2,"initial") == 0) {
		incrementTangent = INITIAL_TANGENT;
	    }
	    if (strcmp(flag2,"noTangent") == 0) {
		incrementTangent = NO_TANGENT;
	    }
	} else if (strcmp(flag,"-maxDim") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    maxDim = atoi(flag);
	    int numdata = 1;
	    if (OPS_GetIntInput(&numdata, &maxDim) < 0) {
		opserr<< "WARNING KrylovNewton failed to read maxDim\n";
		return 0;
	    }
	}
    }

    ConvergenceTest* theTest = cmds->getCTest();
    if (theTest == 0) {
      opserr << "ERROR: No ConvergenceTest yet specified\n";
      return 0;
    }

    Accelerator *theAccel;
    theAccel = new PeriodicAccelerator(maxDim, iterateTangent);

    return new AcceleratedNewton(*theTest, theAccel, incrementTangent);
}

void* OPS_NewtonLineSearch()
{
    if (cmds == 0) return 0;
    ConvergenceTest* theTest = cmds->getCTest();

    if (theTest == 0) {
	opserr << "ERROR: No ConvergenceTest yet specified\n";
	return 0;
    }

    // set some default variable
    double tol        = 0.8;
    int    maxIter    = 10;
    double maxEta     = 10.0;
    double minEta     = 0.1;
    int    pFlag      = 1;
    int    typeSearch = 0;

    int numdata = 1;

    while (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if (strcmp(flag, "-tol") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    if (OPS_GetDoubleInput(&numdata, &tol) < 0) {
		opserr << "WARNING NewtonLineSearch failed to read tol\n";
		return 0;
	    }

	} else if (strcmp(flag, "-maxIter") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    if (OPS_GetIntInput(&numdata, &maxIter) < 0) {
		opserr << "WARNING NewtonLineSearch failed to read maxIter\n";
		return 0;
	    }

	} else if (strcmp(flag, "-pFlag") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    if (OPS_GetIntInput(&numdata, &pFlag) < 0) {
		opserr << "WARNING NewtonLineSearch failed to read pFlag\n";
		return 0;
	    }

	} else if (strcmp(flag, "-minEta") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    if (OPS_GetDoubleInput(&numdata, &minEta) < 0) {
		opserr << "WARNING NewtonLineSearch failed to read minEta\n";
		return 0;
	    }

	} else if (strcmp(flag, "-maxEta") == 0 && OPS_GetNumRemainingInputArgs()>0) {

	    if (OPS_GetDoubleInput(&numdata, &maxEta) < 0) {
		opserr << "WARNING NewtonLineSearch failed to read maxEta\n";
		return 0;
	    }

	} else if (strcmp(flag, "-type") == 0 && OPS_GetNumRemainingInputArgs()>0) {
	    const char* flag2 = OPS_GetString();

	    if (strcmp(flag2, "Bisection") == 0)
		typeSearch = 1;
	    else if (strcmp(flag2, "Secant") == 0)
		typeSearch = 2;
	    else if (strcmp(flag2, "RegulaFalsi") == 0)
		typeSearch = 3;
	    else if (strcmp(flag2, "LinearInterpolated") == 0)
		typeSearch = 3;
	    else if (strcmp(flag2, "InitialInterpolated") == 0)
		typeSearch = 0;
	}
    }

    LineSearch *theLineSearch = 0;
    if (typeSearch == 0)
	theLineSearch = new InitialInterpolatedLineSearch(tol, maxIter, minEta, maxEta, pFlag);

    else if (typeSearch == 1)
	theLineSearch = new BisectionLineSearch(tol, maxIter, minEta, maxEta, pFlag);
    else if (typeSearch == 2)
	theLineSearch = new SecantLineSearch(tol, maxIter, minEta, maxEta, pFlag);
    else if (typeSearch == 3)
	theLineSearch = new RegulaFalsiLineSearch(tol, maxIter, minEta, maxEta, pFlag);

    return new NewtonLineSearch(*theTest, theLineSearch);
}

int OPS_getCTestNorms()
{
    if (cmds == 0) return 0;
    ConvergenceTest* theTest = cmds->getCTest();

    if (theTest != 0) {
	const Vector &norms = theTest->getNorms();
	int numdata = norms.Size();
	double* data = new double[numdata];

	for (int i=0; i<numdata; i++) {
	    data[i] = norms(i);
	}

	if (OPS_SetDoubleOutput(&numdata, data, false) < 0) {
	    opserr << "WARNING failed to set test norms\n";
	    delete [] data;
	    return -1;
	}
	delete [] data;
	return 0;
    }

    opserr << "ERROR testNorms - no convergence test!\n";
    return -1;
}

int OPS_getCTestIter()
{
    if (cmds == 0) return 0;
    ConvergenceTest* theTest = cmds->getCTest();

    if (theTest != 0) {
	int res = theTest->getNumTests();
	int numdata = 1;
	if (OPS_SetIntOutput(&numdata, &res, true) < 0) {
	    opserr << "WARNING failed to set test iter\n";
	    return -1;
	}

	return 0;
    }

    opserr << "ERROR testIter - no convergence test!\n";
    return -1;
}

int OPS_Database()
{
    if (cmds == 0) return 0;
    // make sure at least one other argument to contain integrator
    if (OPS_GetNumRemainingInputArgs() < 1) {
	opserr << "WARNING need to specify a Database type; valid type File, MySQL, BerkeleyDB \n";
	return -1;
    }

    //
    // check argv[1] for type of Database, parse in rest of arguments
    // needed for the type of Database, create the object and add to Domain
    //

    // a File Database
    const char* type = OPS_GetString();
    if (strcmp(type,"File") == 0) {
	if (OPS_GetNumRemainingInputArgs() < 1) {
	    opserr << "WARNING database File fileName? ";
	    return -1;
	}

	const char* filename = OPS_GetString();
	cmds->setFileDatabase(filename);

	return 0;
    }
    opserr << "WARNING No database type exists ";
    opserr << "for database of type:" << type << "valid database type File\n";

    return -1;
}

int OPS_save()
{
    if (cmds == 0) return 0;
    // make sure at least one other argument to contain type of system
    if (OPS_GetNumRemainingInputArgs() < 1) {
	opserr << "WARNING save no commit tag - want save commitTag?";
	return -1;
    }

    // check argv[1] for commitTag
    int commitTag;
    int numdata = 1;
    if (OPS_GetIntInput(&numdata, &commitTag) < 0) {
	opserr << "WARNING - save could not read commitTag " << endln;
	return -1;
    }

    FE_Datastore* theDatabase = cmds->getDatabase();
    if (theDatabase == 0) {
	opserr << "WARNING: save - no database has been constructed\n";
	return -1;
    }

    if (theDatabase->commitState(commitTag) < 0) {
	opserr << "WARNING - database failed to commitState \n";
	return -1;
    }

    return 0;
}

int OPS_restore()
{
    if (cmds == 0) return 0;
    // make sure at least one other argument to contain type of system
    if (OPS_GetNumRemainingInputArgs() < 1) {
	opserr << "WARNING restore no commit tag - want restore commitTag?";
	return -1;
    }

    // check argv[1] for commitTag
    int commitTag;
    int numdata = 1;
    if (OPS_GetIntInput(&numdata, &commitTag) < 0) {
	opserr << "WARNING - restore could not read commitTag " << endln;
	return -1;
    }

    FE_Datastore* theDatabase = cmds->getDatabase();
    if (theDatabase == 0) {
	opserr << "WARNING: save - no database has been constructed\n";
	return -1;
    }

    if (theDatabase->restoreState(commitTag) < 0) {
	opserr << "WARNING - database failed to restore state \n";
	return -1;
    }

    return 0;
}

int OPS_startTimer()
{
    if (cmds == 0) return 0;
    Timer* timer = cmds->getTimer();
    if (timer == 0) return -1;
    timer->start();
    return 0;
}

int OPS_stopTimer()
{
    if (cmds == 0) return 0;
    Timer* theTimer = cmds->getTimer();
    if (theTimer == 0) return -1;
    theTimer->pause();
    opserr << *theTimer;
    return 0;
}

int OPS_modalDamping()
{
    if (cmds == 0) return 0;
    if (OPS_GetNumRemainingInputArgs() < 1) {
	opserr << "WARNING modalDamping ?factor - not enough arguments to command\n";
	return -1;
    }

    int numEigen = cmds->getNumEigen();
    EigenSOE* theEigenSOE = cmds->getEigenSOE();

    if (numEigen == 0 || theEigenSOE == 0) {
	opserr << "WARNING modalDamping - eigen command needs to be called first - NO MODAL DAMPING APPLIED\n ";
	return -1;
    }

    int numModes = OPS_GetNumRemainingInputArgs();
    if (numModes != 1 && numModes < numEigen) {
      opserr << "WARNING modalDamping - fewer damping factors than modes were specified\n";
      opserr << "                     - zero damping will be applied to un-specified modes" << endln;
    }
    if (numModes > numEigen) {
      opserr << "WARNING modalDamping - more damping factors than modes were specifed\n";
      opserr << "                     - ignoring additional damping factors" << endln;
    }    

    double factor;
    Vector modalDampingValues(numEigen);
    int numdata = 1;

    //
    // read in values and set factors
    //
    if (numModes == 1) {
      if (OPS_GetDoubleInput(&numdata, &factor) < 0) {
	opserr << "WARNING modalDamping - could not read factor for all modes \n";
	return -1;
      }
      for (int i = 0; i < numEigen; i++)
	modalDampingValues(i) = factor;
    }
    else {
      for (int i = 0; i < numModes; i++) {
	if (OPS_GetDoubleInput(&numdata, &factor) < 0) {
	  opserr << "WARNING modalDamping - could not read factor for mode " << i+1 << endln;
	  return -1;
	}
	modalDampingValues(i) = factor;
      }
      for (int i = numModes; i < numEigen; i++)
	modalDampingValues(i) = 0.0;
    }

    Domain* theDomain = OPS_GetDomain();
    if (theDomain != 0) {
	theDomain->setModalDampingFactors(&modalDampingValues, true);
    }

    return 0;
}

int OPS_modalDampingQ()
{
    if (cmds == 0) return 0;
    if (OPS_GetNumRemainingInputArgs() < 1) {
	opserr << "WARNING modalDampingQ ?factor - not enough arguments to command\n";
	return -1;
    }

    int numEigen = cmds->getNumEigen();
    EigenSOE* theEigenSOE = cmds->getEigenSOE();

    if (numEigen == 0 || theEigenSOE == 0) {
	opserr << "WARNING modalDampingQ - eigen command needs to be called first - NO MODAL DAMPING APPLIED\n ";
	return -1;
    }

    int numModes = OPS_GetNumRemainingInputArgs();
    if (numModes != 1 && numModes < numEigen) {
      opserr << "WARNING modalDampingQ - fewer damping factors than modes were specified\n";
      opserr << "                      - zero damping will be applied to un-specified modes" << endln;
    }
    if (numModes > numEigen) {
      opserr << "WARNING modalDampingQ - more damping factors than modes were specifed\n";
      opserr << "                      - ignoring additional damping factors" << endln;
    }
    
    double factor;
    Vector modalDampingValues(numEigen);
    int numdata = 1;

    //
    // read in values and set factors
    //
    if (numModes == 1) {
      if (OPS_GetDoubleInput(&numdata, &factor) < 0) {
	opserr << "WARNING modalDampingQ - could not read factor for all modes \n";
	return -1;
      }
      for (int i = 0; i < numEigen; i++)
	modalDampingValues(i) = factor;
    }
    else {
      for (int i = 0; i < numModes; i++) {
	if (OPS_GetDoubleInput(&numdata, &factor) < 0) {
	  opserr << "WARNING modalDampingQ - could not read factor for mode " << i+1 << endln;
	  return -1;
	}
	modalDampingValues(i) = factor;
      }
      for (int i = numModes; i < numEigen; i++)
	modalDampingValues(i) = 0.0;
    }

    Domain* theDomain = OPS_GetDomain();
    if (theDomain != 0) {
	theDomain->setModalDampingFactors(&modalDampingValues, false);
    }

    return 0;
}

int OPS_neesMetaData()
{
    if (cmds == 0) return 0;
    if (OPS_GetNumRemainingInputArgs() < 1) {
	opserr << "WARNING missing args \n";
	return -1;
    }

    SimulationInformation* simulationInfo = cmds->getSimulationInformation();
    if (simulationInfo == 0) return -1;

    while (OPS_GetNumRemainingInputArgs() > 0) {
	const char* flag = OPS_GetString();

	if ((strcmp(flag,"-title") == 0) || (strcmp(flag,"-Title") == 0)
	    || (strcmp(flag,"-TITLE") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->setTitle(OPS_GetString());
	    }
	} else if ((strcmp(flag,"-contact") == 0) || (strcmp(flag,"-Contact") == 0)
		   || (strcmp(flag,"-CONTACT") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->setContact(OPS_GetString());
	    }
	} else if ((strcmp(flag,"-description") == 0) || (strcmp(flag,"-Description") == 0)
		   || (strcmp(flag,"-DESCRIPTION") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->setDescription(OPS_GetString());
	    }
	} else if ((strcmp(flag,"-modelType") == 0) || (strcmp(flag,"-ModelType") == 0)
		   || (strcmp(flag,"-MODELTYPE") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->addModelType(OPS_GetString());
	    }
	} else if ((strcmp(flag,"-analysisType") == 0) || (strcmp(flag,"-AnalysisType") == 0)
		   || (strcmp(flag,"-ANALYSISTYPE") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->addAnalysisType(OPS_GetString());
	    }
	} else if ((strcmp(flag,"-elementType") == 0) || (strcmp(flag,"-ElementType") == 0)
		   || (strcmp(flag,"-ELEMENTTYPE") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->addElementType(OPS_GetString());
	    }
	} else if ((strcmp(flag,"-materialType") == 0) || (strcmp(flag,"-MaterialType") == 0)
		   || (strcmp(flag,"-MATERIALTYPE") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->addMaterialType(OPS_GetString());
	    }
	} else if ((strcmp(flag,"-loadingType") == 0) || (strcmp(flag,"-LoadingType") == 0)
		   || (strcmp(flag,"-LOADINGTYPE") == 0)) {
	    if (OPS_GetNumRemainingInputArgs() > 0) {
		simulationInfo->addLoadingType(OPS_GetString());
	    }
	} else {
	    opserr << "WARNING unknown arg type: " << flag << endln;
	    return -1;
	}
    }
    return 0;
}

int OPS_defaultUnits()
{
    if (OPS_GetNumRemainingInputArgs() < 6) {
        opserr << "WARNING defaultUnits - missing a unit type want: defaultUnits -Force type? -Length type? -Time type?\n";
        return -1;
    }

    const char *force = 0;
    const char *length = 0;
    const char *time = 0;
    const char *temperature = "N/A";

    while (OPS_GetNumRemainingInputArgs() > 0) {
        const char* unitType = OPS_GetString();

        if ((strcmp(unitType, "-force") == 0) || (strcmp(unitType, "-Force") == 0)
            || (strcmp(unitType, "-FORCE") == 0)) {
            force = OPS_GetString();
        }
        else if ((strcmp(unitType, "-length") == 0) || (strcmp(unitType, "-Length") == 0)
            || (strcmp(unitType, "-LENGTH") == 0)) {
            length = OPS_GetString();
        }
        else if ((strcmp(unitType, "-time") == 0) || (strcmp(unitType, "-Time") == 0)
            || (strcmp(unitType, "-TIME") == 0)) {
            time = OPS_GetString();
        }
        else if ((strcmp(unitType, "-temperature") == 0) || (strcmp(unitType, "-Temperature") == 0)
            || (strcmp(unitType, "-TEMPERATURE") == 0) || (strcmp(unitType, "-temp") == 0)
            || (strcmp(unitType, "-Temp") == 0) || (strcmp(unitType, "-TEMP") == 0)) {
            temperature = OPS_GetString();
        }
        else {
            opserr << "WARNING defaultUnits - unrecognized unit: " << unitType << " want: defaultUnits -Force type? -Length type? -Time type?\n";
            return -1;
        }
    }

    if (length == 0 || force == 0 || time == 0) {
        opserr << "defaultUnits - missing a unit type want: defaultUnits -Force type? -Length type? -Time type?\n";
        return -1;
    }

    double lb, kip, n, kn, mn, kgf, tonf;
    double in, ft, mm, cm, m;
    double sec, msec;

    if ((strcmp(force, "lb") == 0) || (strcmp(force, "lbs") == 0)) {
        lb = 1.0;
    }
    else if ((strcmp(force, "kip") == 0) || (strcmp(force, "kips") == 0)) {
        lb = 0.001;
    }
    else if ((strcmp(force, "N") == 0)) {
        lb = 4.4482216152605;
    }
    else if ((strcmp(force, "kN") == 0) || (strcmp(force, "KN") == 0) || (strcmp(force, "kn") == 0)) {
        lb = 0.0044482216152605;
    }
    else if ((strcmp(force, "mN") == 0) || (strcmp(force, "MN") == 0) || (strcmp(force, "mn") == 0)) {
        lb = 0.0000044482216152605;
    }
    else if ((strcmp(force, "kgf") == 0)) {
        lb = 4.4482216152605 / 9.80665;
    }
    else if ((strcmp(force, "tonf") == 0)) {
        lb = 4.4482216152605 / 9.80665 / 1000.0;
    }
    else {
        lb = 1.0;
        opserr << "defaultUnits - unknown force type, valid options: lb, kip, N, kN, MN, kgf, tonf\n";
        return -1;
    }

    if ((strcmp(length, "in") == 0) || (strcmp(length, "inch") == 0)) {
        in = 1.0;
    }
    else if ((strcmp(length, "ft") == 0) || (strcmp(length, "feet") == 0)) {
        in = 1.0 / 12.0;
    }
    else if ((strcmp(length, "mm") == 0)) {
        in = 25.4;
    }
    else if ((strcmp(length, "cm") == 0)) {
        in = 2.54;
    }
    else if ((strcmp(length, "m") == 0)) {
        in = 0.0254;
    }
    else {
        in = 1.0;
        opserr << "defaultUnits - unknown length type, valid options: in, ft, mm, cm, m\n";
        return -1;
    }

    if ((strcmp(time, "sec") == 0) || (strcmp(time, "Sec") == 0)) {
        sec = 1.0;
    }
    else if ((strcmp(time, "msec") == 0) || (strcmp(time, "mSec") == 0)) {
        sec = 1000.0;
    }
    else {
        sec = 1.0;
        opserr << "defaultUnits - unknown time type, valid options: sec, msec\n";
        return -1;
    }

    kip = lb / 0.001;
    n = lb / 4.4482216152605;
    kn = lb / 0.0044482216152605;
    mn = lb / 0.0000044482216152605;
    kgf = lb / (4.4482216152605/9.80665);
    tonf = lb / (4.4482216152605/9.80665/1000.0);

    ft = in * 12.0;
    mm = in / 25.4;
    cm = in / 2.54;
    m = in / 0.0254;

    msec = sec * 0.001;

    char string[50];
    DL_Interpreter* theInter = cmds->getInterpreter();
    if (theInter == 0) return -1;

    sprintf(string, "lb = %.18e", lb);   theInter->runCommand(string);
    sprintf(string, "lbf = %.18e", lb);   theInter->runCommand(string);
    sprintf(string, "kip = %.18e", kip);   theInter->runCommand(string);
    sprintf(string, "N = %.18e", n);   theInter->runCommand(string);
    sprintf(string, "kN = %.18e", kn);   theInter->runCommand(string);
    sprintf(string, "Newton = %.18e", n);   theInter->runCommand(string);
    sprintf(string, "kNewton = %.18e", kn);   theInter->runCommand(string);
    sprintf(string, "MN = %.18e", mn);   theInter->runCommand(string);
    sprintf(string, "kgf = %.18e", kgf);   theInter->runCommand(string);
    sprintf(string, "tonf = %.18e", tonf);   theInter->runCommand(string);

    //sprintf(string, "in = %.18e", in);   theInter->runCommand(string);  // "in" is a keyword in Python
    sprintf(string, "inch = %.18e", in);   theInter->runCommand(string);
    sprintf(string, "ft = %.18e", ft);   theInter->runCommand(string);
    sprintf(string, "mm = %.18e", mm);   theInter->runCommand(string);
    sprintf(string, "cm = %.18e", cm);   theInter->runCommand(string);
    sprintf(string, "m = %.18e", m);   theInter->runCommand(string);
    sprintf(string, "meter = %.18e", m);   theInter->runCommand(string);

    sprintf(string, "sec = %.18e", sec);   theInter->runCommand(string);
    sprintf(string, "msec = %.18e", msec);   theInter->runCommand(string);

    double g = 32.174049*ft / (sec*sec);
    sprintf(string, "g = %.18e", g);   theInter->runCommand(string);
    sprintf(string, "kg = %.18e", n*sec*sec / m);   theInter->runCommand(string);
    sprintf(string, "Mg = %.18e", 1e3*n*sec*sec / m);   theInter->runCommand(string);
    sprintf(string, "slug = %.18e", lb*sec*sec / ft);   theInter->runCommand(string);
    sprintf(string, "Pa = %.18e", n / (m*m));   theInter->runCommand(string);
    sprintf(string, "kPa = %.18e", 1e3*n / (m*m));   theInter->runCommand(string);
    sprintf(string, "MPa = %.18e", 1e6*n / (m*m));   theInter->runCommand(string);
    sprintf(string, "psi = %.18e", lb / (in*in));   theInter->runCommand(string);
    sprintf(string, "ksi = %.18e", kip / (in*in));   theInter->runCommand(string);
    sprintf(string, "psf = %.18e", lb / (ft*ft));   theInter->runCommand(string);
    sprintf(string, "ksf = %.18e", kip / (ft*ft));   theInter->runCommand(string);
    sprintf(string, "pcf = %.18e", lb / (ft*ft*ft));   theInter->runCommand(string);
    sprintf(string, "in2 = %.18e", in*in);   theInter->runCommand(string);
    sprintf(string, "ft2 = %.18e", ft*ft);   theInter->runCommand(string);
    sprintf(string, "mm2 = %.18e", mm*mm);   theInter->runCommand(string);
    sprintf(string, "cm2 = %.18e", cm*cm);   theInter->runCommand(string);
    sprintf(string, "m2 = %.18e", m*m);   theInter->runCommand(string);
    sprintf(string, "in4 = %.18e", in*in*in*in);   theInter->runCommand(string);
    sprintf(string, "ft4 = %.18e", ft*ft*ft*ft);   theInter->runCommand(string);
    sprintf(string, "mm4 = %.18e", mm*mm*mm*mm);   theInter->runCommand(string);
    sprintf(string, "cm4 = %.18e", cm*cm*cm*cm);   theInter->runCommand(string);
    sprintf(string, "m4 = %.18e", m*m*m*m);   theInter->runCommand(string);
    sprintf(string, "pi = %.18e", 2.0*asin(1.0));   theInter->runCommand(string);
    sprintf(string, "PI = %.18e", 2.0*asin(1.0));   theInter->runCommand(string);

    SimulationInformation* simulationInfo = cmds->getSimulationInformation();
    if (simulationInfo == 0) return -1;

    simulationInfo->setForceUnit(force);
    simulationInfo->setLengthUnit(length);
    simulationInfo->setTimeUnit(time);
    simulationInfo->setTemperatureUnit(temperature);

    return 0;
}

int OPS_totalCPU()
{
    if (cmds == 0) return 0;
    EquiSolnAlgo* theAlgorithm = cmds->getAlgorithm();
    if (theAlgorithm == 0) {
	opserr << "WARNING no algorithm is set\n";
	return -1;
    }

    double value = theAlgorithm->getTotalTimeCPU();
    int numdata = 1;
    if (OPS_SetDoubleOutput(&numdata, &value, true) < 0) {
	opserr << "WARNING failed to set output\n";
	return -1;
    }

    return 0;
}

int OPS_solveCPU()
{
    if (cmds == 0) return 0;
    EquiSolnAlgo* theAlgorithm = cmds->getAlgorithm();
    if (theAlgorithm == 0) {
	opserr << "WARNING no algorithm is set\n";
	return -1;
    }

    double value = theAlgorithm->getSolveTimeCPU();
    int numdata = 1;
    if (OPS_SetDoubleOutput(&numdata, &value, true) < 0) {
	opserr << "WARNING failed to set output\n";
	return -1;
    }

    return 0;
}

int OPS_accelCPU()
{
    if (cmds == 0) return 0;
    EquiSolnAlgo* theAlgorithm = cmds->getAlgorithm();
    if (theAlgorithm == 0) {
	opserr << "WARNING no algorithm is set\n";
	return -1;
    }

    double value = theAlgorithm->getAccelTimeCPU();
    int numdata = 1;
    if (OPS_SetDoubleOutput(&numdata, &value, true) < 0) {
	opserr << "WARNING failed to set output\n";
	return -1;
    }

    return 0;
}

int OPS_numFact()
{
    if (cmds == 0) return 0;
    EquiSolnAlgo* theAlgorithm = cmds->getAlgorithm();
    if (theAlgorithm == 0) {
	opserr << "WARNING no algorithm is set\n";
	return -1;
    }

    double value = theAlgorithm->getNumFactorizations();
    int numdata = 1;
    if (OPS_SetDoubleOutput(&numdata, &value, true) < 0) {
	opserr << "WARNING failed to set output\n";
	return -1;
    }

    return 0;
}

int OPS_numIter()
{
    if (cmds == 0) return 0;
    EquiSolnAlgo* theAlgorithm = cmds->getAlgorithm();
    if (theAlgorithm == 0) {
	opserr << "WARNING no algorithm is set\n";
	return -1;
    }

    int value = theAlgorithm->getNumIterations();
    int numdata = 1;
    if (OPS_SetIntOutput(&numdata, &value, true) < 0) {
	opserr << "WARNING failed to set output\n";
	return -1;
    }

    return value;
}

int OPS_systemSize()
{
    if (cmds == 0) return 0;
    LinearSOE* theSOE = cmds->getSOE();
    if (theSOE == 0) {
	opserr << "WARNING no system is set\n";
	return -1;
    }

    int value = theSOE->getNumEqn();
    int numdata = 1;
    if (OPS_SetIntOutput(&numdata, &value, true) < 0) {
	opserr << "WARNING failed to set output\n";
	return -1;
    }

    return 0;
}

int OPS_domainCommitTag() {
    if (cmds == 0) {
        return 0;
    }

    int commitTag = cmds->getDomain()->getCommitTag();
    int numdata = 1;
    if (OPS_GetNumRemainingInputArgs() > 0) {
        if (OPS_GetIntInput(&numdata, &commitTag) < 0) {
            opserr << "WARNING: failed to get commitTag\n";
            return -1;
        }
        cmds->getDomain()->setCommitTag(commitTag);
    }
    if (OPS_SetIntOutput(&numdata, &commitTag, true) < 0) {
        opserr << "WARNING failed to set commitTag\n";
        return 0;
    }

    return 0;
}

void* OPS_ParallelRCM() {

#ifdef _PARALLEL_INTERPRETERS
    ParallelNumberer *theParallelNumberer = 0;
    if (cmds == 0) return theParallelNumberer;

    MachineBroker* machine = cmds->getMachineBroker();
    Channel** channels = cmds->getChannels();
    int numChannels = cmds->getNumChannels();

    int rank = machine->getPID();

    RCM *theRCM = new RCM(false);
    theParallelNumberer = new ParallelNumberer(*theRCM);
    theParallelNumberer->setProcessID(rank);
    theParallelNumberer->setChannels(numChannels, channels);

    return theParallelNumberer;
#else
    return 0;
#endif

}

void* OPS_ParallelNumberer() {

#ifdef _PARALLEL_INTERPRETERS
    ParallelNumberer *theParallelNumberer = 0;
    if (cmds == 0) return theParallelNumberer;

    MachineBroker* machine = cmds->getMachineBroker();
    Channel** channels = cmds->getChannels();
    int numChannels = cmds->getNumChannels();

    int rank = machine->getPID();

    theParallelNumberer = new ParallelNumberer;
    theParallelNumberer->setProcessID(rank);
    theParallelNumberer->setChannels(numChannels, channels);

    return theParallelNumberer;
#else
    return 0;
#endif
}

void* OPS_ParallelDisplacementControl() {
#ifdef _PARALLEL_INTERPRETERS
    DistributedDisplacementControl *theDDC = 0;
    if (cmds == 0) {
        return theDDC;
    }
    int idata[3];
    double ddata[3];

    if (OPS_GetNumRemainingInputArgs() < 3) {
        opserr << "WARNING integrator DistributedDisplacementControl node dof dU \n";
        opserr << "<Jd minIncrement maxIncrement>\n";
        return 0;
    }

    int num = 2;
    if (OPS_GetIntInput(&num, &idata[0]) < 0) {
        opserr << "WARNING: failed to get node and dof\n";
        return 0;
    }
    num = 1;
    if (OPS_GetDoubleInput(&num, &ddata[0]) < 0) {
        opserr << "WARNING: failed to get dU\n";
        return 0;
    }
    if (OPS_GetNumRemainingInputArgs() >= 3) {
        num = 1;
        if (OPS_GetIntInput(&num, &idata[2]) < 0) {
            opserr << "WARNING: failed to get Jd\n";
            return 0;
        }
        num = 2;
        if (OPS_GetDoubleInput(&num, &ddata[1]) < 0) {
            opserr << "WARNING: failed to get min and max\n";
            return 0;
        }
    } else {
        idata[2] = 1;
        ddata[1] = ddata[0];
        ddata[2] = ddata[0];
    }

    theDDC = new DistributedDisplacementControl(idata[0], idata[1]-1,
                                                ddata[0], idata[2],
                                                ddata[1], ddata[2]);
    MachineBroker* machine = cmds->getMachineBroker();
    Channel** channels = cmds->getChannels();
    int numChannels = cmds->getNumChannels();

    int rank = machine->getPID();
    theDDC->setProcessID(rank);
    theDDC->setChannels(numChannels, channels);
    return theDDC;
#else
    return 0;
#endif

}

void* OPS_MumpsSolver() {
    int icntl14 = 20;
    int icntl7 = 7;
    int matType = 0; // 0: unsymmetric, 1: symmetric positive definite, 2: symmetric general
    while (OPS_GetNumRemainingInputArgs() > 2) {
        const char* opt = OPS_GetString();
        int num = 1;
        if (strcmp(opt, "-ICNTL14") == 0) {
            if (OPS_GetIntInput(&num, &icntl14) < 0) {
                opserr << "WARNING: failed to get icntl14\n";
                return 0;
            }
        } else if (strcmp(opt, "-ICNTL7") == 0) {
            if (OPS_GetIntInput(&num, &icntl7) < 0) {
                opserr << "WARNING: failed to get icntl7\n";
                return 0;
            }
        } else if (strcmp(opt, "-matrixType") == 0) {
            if (OPS_GetIntInput(&num, &matType) < 0) {
                opserr << "WARNING: failed to get -matrixType. Unsymmetric matrix assumed\n";
                return 0;
            }
            if (matType < 0 || matType > 2) {
                opserr << "Mumps Warning: wrong -matrixType value (" << matType << "). Unsymmetric matrix assumed\n";
                matType = 0;
            }
        }
    }

#ifdef _PARALLEL_INTERPRETERS
#ifdef _MUMPS
    MumpsParallelSOE* soe = 0;

    MumpsParallelSolver *solver= new MumpsParallelSolver(icntl7, icntl14);
    soe = new MumpsParallelSOE(*solver, matType);

    MachineBroker* machine = cmds->getMachineBroker();
    Channel** channels = cmds->getChannels();
    int numChannels = cmds->getNumChannels();

    int rank = machine->getPID();
    soe->setProcessID(rank);
    soe->setChannels(numChannels, channels);
    return soe;
#endif
#else
#ifdef _MUMPS
    MumpsSolver *theSolver = new MumpsSolver(icntl7, icntl14);
    theSOE = new MumpsSOE(*theSolver, matType);
    return theSOE;
#endif
#endif
    return 0;
}

// Sensitivity:BEGIN /////////////////////////////////////////////

int OPS_computeGradients()
{
    Integrator* theIntegrator = 0;
    if(cmds->getStaticIntegrator() != 0) {
    	theIntegrator = cmds->getStaticIntegrator();
    } else if(cmds->getTransientIntegrator() != 0) {
    	theIntegrator = cmds->getTransientIntegrator();
    }

    if (theIntegrator == 0) {
    	opserr << "WARNING: No integrator is created\n";
    	return -1;
    }

    if (theIntegrator->computeSensitivities() < 0) {
	opserr << "WARNING: failed to compute sensitivities\n";
	return -1;
    }
    
    return 0;
}

int OPS_sensitivityAlgorithm()
{
    if (cmds == 0) return 0;

    int analysisTypeTag = 1;
    Integrator* theIntegrator = 0;
    if(cmds->getStaticIntegrator() != 0) {
    	theIntegrator = cmds->getStaticIntegrator();
    } else if(cmds->getTransientIntegrator() != 0) {
    	theIntegrator = cmds->getTransientIntegrator();
    }
    
    // 1: compute at each step (default); 2: compute by command; 
    if (OPS_GetNumRemainingInputArgs() < 1) {
    	opserr << "ERROR: Wrong number of parameters to sensitivity algorithm." << "\n";
    	return -1;
    }
    if (theIntegrator == 0) {
    	opserr << "The integrator needs to be instantiated before " << "\n"
    	       << " setting  sensitivity algorithm." << "\n";
    	return -1;
    }

    const char* type = OPS_GetString();
    if (strcmp(type,"-computeAtEachStep") == 0)
    	analysisTypeTag = 1;
    else if (strcmp(type,"-computeByCommand") == 0)
    	analysisTypeTag = 2;
    else {
    	opserr << "Unknown sensitivity algorithm option: " << type << "\n";
    	return -1;
    }

    theIntegrator->setComputeType(analysisTypeTag);
    theIntegrator->activateSensitivityKey();
	
    return 0;
}
// Sensitivity:END /////////////////////////////////////////////
