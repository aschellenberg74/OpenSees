/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */
                                                                        
// $Revision: 1.10 $
// $Date: 2014/09/30 $
// $Source: /usr/local/cvs/OpenSees/SRC/element/ShellNLDKGQ/ShellNLDKGQ.h,v $

// Written: Lisha Wang, Xinzheng Lu, Linlin Xie, Song Cen & Quan Gu
//
//  four node flat shell element with membrane and drill DOF
//  considering geometric nonlinear, form nonlinear shell element
//  using updated Lagrangian formula
// Ref: Plate Bending Part - DKQ, thin plate element
//      Membrane Part - GQ12, a membrane element with drilling DOF
//

#include <stdio.h> 
#include <stdlib.h> 
#include <math.h> 

#include <ID.h> 
#include <Vector.h>
#include <Matrix.h>
#include <Element.h>
#include <Node.h>
#include <SectionForceDeformation.h>
#include <Domain.h>
#include <ErrorHandler.h>
#include <ShellNLDKGQ.h>
#include <R3vectors.h>
#include <Renderer.h>
#include <ElementResponse.h>
#include <Parameter.h>

#include <Channel.h>
#include <FEM_ObjectBroker.h>
#include <elementAPI.h>
#include <map>

#define min(a,b) ( (a)<(b) ? (a):(b) )

static int numShellNLDKGQ = 0;

void *
OPS_ShellNLDKGQ(void)
{
  if (numShellNLDKGQ == 0) {
//    opserr << "Using ShellNLDKGQ - Developed by: Lisha Wang,Xinzheng Lu and Quan Gu\n";
    numShellNLDKGQ++;
  }

  Element *theElement = 0;
  
  int numArgs = OPS_GetNumRemainingInputArgs();
  
  if (numArgs < 6) {
    opserr << "Want: element ShellNLDKGQ $tag $iNode $jNoe $kNode $lNode $secTag";
    return 0;	
  }
  
  int iData[6];
  int numData = 6;
  if (OPS_GetIntInput(&numData, iData) != 0) {
    opserr << "WARNING invalid integer tag: element ShellNLDKGQ \n";
    return 0;
  }

  int dampingTag = 0;
  Damping *theDamping = 0;

  while(OPS_GetNumRemainingInputArgs() > 0) {
    const char* type = OPS_GetString();    
    if(strcmp(type,"-damp") == 0) {
	    if(OPS_GetNumRemainingInputArgs() > 0) {
	      numData = 1;
        if(OPS_GetIntInput(&numData,&dampingTag) < 0) return 0;
		    theDamping = OPS_getDamping(dampingTag);
        if(theDamping == 0) {
	        opserr<<"damping not found\n";
	        return 0;
        }
	    }
    } 
  }

  SectionForceDeformation *theSection = OPS_getSectionForceDeformation(iData[5]);

  if (theSection == 0) {
    opserr << "ERROR:  element ShellNLDKGQ " << iData[0] << "section " << iData[5] << " not found\n";
    return 0;
  }
  
  theElement = new ShellNLDKGQ(iData[0], iData[1], iData[2], iData[3],
			      iData[4], *theSection, theDamping);

  return theElement;
}

void *
OPS_ShellNLDKGQ(const ID& info)
{

    if (info.Size() == 0) {
	opserr << "WARNING: info is empty -- ShellNLDKGQ\n";
	return 0;
    }

    // save data
    static std::map<int,Vector> meshdata;
    if (info(0) == 1) {

	// check input
	if (info.Size() < 2) {
	    opserr << "WARNING: need info -- inmesh, meshtag\n";
	    return 0;
	}
	if (OPS_GetNumRemainingInputArgs() < 1) {
	    opserr << "WARNING: insuficient arguments -- secTag <-updateBasis>\n";
	    return 0;
	}

	// save data
	Vector& mdata = meshdata[info(1)];
	mdata.resize(2);
	mdata.Zero();

	// get secTag
	int numdata = 1;
	int secTag;
	if (OPS_GetIntInput(&numdata, &secTag) < 0) {
	    opserr << "WARNING: failed to get section tag -- ShellNLDKGQ\n";
	    return 0;
	}
	mdata(0) = (double)secTag;

	// update basis
	if (OPS_GetNumRemainingInputArgs() > 0) {
	    const char* type = OPS_GetString();
	    if (strcmp(type, "-updateBasis") == 0) {
		mdata(1) = 1;
	    }
	}

	return &meshdata;
    }

    // load data
    if (info(0) == 2) {
	if (info.Size() < 7) {
	    opserr << "WARNING: need info -- inmesh, meshtag, eleTag, nd1, nd2, nd3, nd4\n";
	    return 0;
	}
	int eleTag = info(2);

	// get data
	Vector& mdata = meshdata[info(1)];
	if (mdata.Size() < 2) {
	    return 0;
	}

	// get section
	int secTag = (int)mdata(0);
	SectionForceDeformation *theSection = OPS_getSectionForceDeformation(secTag);
	if (theSection == 0) {
	    opserr << "ERROR:  element ShellNLDKGQ " << info(2) << "section " << secTag << " not found\n";
	    return 0;
	}

	// update basis
	bool updateBasis = false;
	if (mdata(1) == 1) {
	    updateBasis = true;
	}

	return new ShellNLDKGQ(info(2), info(3), info(4), info(5),
			       info(6), *theSection);
    }
    

    return 0;
}

//static data
Matrix  ShellNLDKGQ::stiff(24,24) ;
Vector  ShellNLDKGQ::resid(24) ;
Matrix  ShellNLDKGQ::mass(24,24) ;


//quadrature data
const double  ShellNLDKGQ::root3 = sqrt(3.0) ;
const double  ShellNLDKGQ::one_over_root3 = 1.0 / root3 ;

double ShellNLDKGQ::sg[4] ;
double ShellNLDKGQ::tg[4] ;
double ShellNLDKGQ::wg[4] ;

 

//null constructor
ShellNLDKGQ::ShellNLDKGQ( ) :
Element( 0, ELE_TAG_ShellNLDKGQ),
connectedExternalNodes(4), CstrainGauss(32),TstrainGauss(32),load(0), Ki(0)  //modify for geometric nonlinearity
{ 
  for (int i = 0 ;  i < 4; i++ ) 
    materialPointers[i] = 0;

  for (int i = 0 ;  i < 4; i++ ) 
    theDamping[i] = 0;

  sg[0] = -one_over_root3;
  sg[1] = one_over_root3;
  sg[2] = one_over_root3;
  sg[3] = -one_over_root3;  

  tg[0] = -one_over_root3;
  tg[1] = -one_over_root3;
  tg[2] = one_over_root3;
  tg[3] = one_over_root3;  

  wg[0] = 1.0;
  wg[1] = 1.0;
  wg[2] = 1.0;
  wg[3] = 1.0;
}


//*********************************************************************
//full constructor
ShellNLDKGQ::ShellNLDKGQ(  int tag, 
                         int node1,
                         int node2,
   	                     int node3,
                         int node4,
	                     SectionForceDeformation &theMaterial,
                       Damping *damping) :
Element( tag, ELE_TAG_ShellNLDKGQ ),
connectedExternalNodes(4), CstrainGauss(32),TstrainGauss(32),load(0), Ki(0)//modify for geometric nonlinearity
{
  int i;

  connectedExternalNodes(0) = node1 ;
  connectedExternalNodes(1) = node2 ;
  connectedExternalNodes(2) = node3 ;
  connectedExternalNodes(3) = node4 ;

  for ( i = 0 ;  i < 4; i++ ) {

      materialPointers[i] = theMaterial.getCopy( ) ;

      if (materialPointers[i] == 0) {
	      opserr << "ShellNLDKGQ::constructor - failed to get a material of type: ShellSection\n";
      } //end if
      
  } //end for i 

  if (damping)
  {
    for (i = 0; i < 4; i++)
    {
      theDamping[i] =(*damping).getCopy();
    
      if (!theDamping[i]) {
        opserr << "ShellNLDKGQ::ShellNLDKGQ -- failed to get copy of damping\n";
      }
    }
  }
  else
  {
    for (i = 0; i < 4; i++) theDamping[i] = 0;
  }

  sg[0] = -one_over_root3;
  sg[1] = one_over_root3;
  sg[2] = one_over_root3;
  sg[3] = -one_over_root3;  

  tg[0] = -one_over_root3;
  tg[1] = -one_over_root3;
  tg[2] = one_over_root3;
  tg[3] = one_over_root3;  

  wg[0] = 1.0;
  wg[1] = 1.0;
  wg[2] = 1.0;
  wg[3] = 1.0;

 }
//******************************************************************

//destructor 
ShellNLDKGQ::~ShellNLDKGQ( )
{
  int i ;
  for ( i = 0 ;  i < 4; i++ ) {

    delete materialPointers[i] ;
    materialPointers[i] = 0 ; 

    nodePointers[i] = 0 ;

  } //end for i

  for (i = 0; i < 4; i++)
  {
    if (theDamping[i])
    {
      delete theDamping[i];
      theDamping[i] = 0;
    }
  }

  if (load != 0)
    delete load;

  if (Ki != 0)
    delete Ki;
}
//**************************************************************************


//set domain
void  ShellNLDKGQ::setDomain( Domain *theDomain ) 
{  
  int i;

  //node pointers
  for ( i = 0; i < 4; i++ ) {
     nodePointers[i] = theDomain->getNode( connectedExternalNodes(i) ) ;
     if (nodePointers[i] == 0) {
       opserr << "ShellNLDKGQ::setDomain - no node " << connectedExternalNodes(i);
       opserr << " exists in the model\n";
     }
     const Vector &nodeDisp=nodePointers[i]->getTrialDisp();
     if (nodeDisp.Size() != 6) {
       opserr << "ShellNLDKGQ::setDomain - node " << connectedExternalNodes(i);
       opserr << " NEEDS 6 dof - GARBAGE RESULTS or SEGMENTATION FAULT WILL FOLLOW\n";
     }       
  }

  //basis vectors and local coordinates
  updateBasis( ) ;
  //updateBasis( );

  for (i = 0; i < 4; i++)
  {
    if (theDamping[i] && theDamping[i]->setDomain(theDomain, 8)) {
      opserr << "ShellNLDKGQ::setDomain -- Error initializing damping\n";
      exit(-1);
    }
  }

  this->DomainComponent::setDomain(theDomain);
}


int
ShellNLDKGQ::setDamping(Domain *theDomain, Damping *damping)
{
  if (theDomain && damping)
  {
    for (int i = 0; i < 4; i++)
    {
      if (theDamping[i]) delete theDamping[i];

      theDamping[i] =(*damping).getCopy();
    
      if (!theDamping[i]) {
        opserr << "ShellNLDKGQ::setDamping -- failed to get copy of damping\n";
        return -1;
      }
      if (theDamping[i]->setDomain(theDomain, 8)) {
        opserr << "ShellNLDKGQ::setDamping -- Error initializing damping\n";
        return -2;
      }
    }
  }
  return 0;
}

//get the number of external nodes
int  ShellNLDKGQ::getNumExternalNodes( ) const
{
  return 4 ;
} 
 

//return connected external nodes
const ID&  ShellNLDKGQ::getExternalNodes( ) 
{
  return connectedExternalNodes ;
} 


Node **
ShellNLDKGQ::getNodePtrs(void) 
{
  return nodePointers;
} 

//return number of dofs
int  ShellNLDKGQ::getNumDOF( ) 
{
  return 24 ;
}


//commit state
int  ShellNLDKGQ::commitState( )
{
  int success = 0 ;

  // call element commitState to do any base class stuff
  if ((success = this->Element::commitState()) != 0) {
    opserr << "ShellNLDKGQ::commitState () - failed in base class";
  }    

  for (int i = 0; i < 4; i++ ) 
    success += materialPointers[i]->commitState( ) ;

  for (int i = 0; i < 4; i++ )
    if (theDamping[i]) success += theDamping[i]->commitState();

  //add for geometric nonlinearity
  //save the prev. step strain
  CstrainGauss = TstrainGauss;
  return success ;
}
 


//revert to last commit 
int  ShellNLDKGQ::revertToLastCommit( ) 
{
  int i ;
  int success = 0 ;

  for ( i = 0; i < 4; i++ ) 
    success += materialPointers[i]->revertToLastCommit( ) ;

  for (int i = 0; i < 4; i++ )
    if (theDamping[i]) success += theDamping[i]->revertToLastCommit();
  
  //add for geometric nonlinearity
 TstrainGauss = CstrainGauss;
  
  return success ;
}
    

//revert to start 
int  ShellNLDKGQ::revertToStart( ) 
{
  int i ;
  int success = 0 ;

  for ( i = 0; i < 4; i++ ) 
    success += materialPointers[i]->revertToStart( ) ;
  
  for (int i = 0; i < 4; i++ )
    if (theDamping[i]) success += theDamping[i]->revertToStart();
  
  //add for geometric nonlinearity
  CstrainGauss.Zero( );
  return success ;
}

//print out element data
void  ShellNLDKGQ::Print( OPS_Stream &s, int flag )
{
    if (flag == -1) {
        int eleTag = this->getTag();
        s << "EL_ShellNLDKGQ\t" << eleTag << "\t";
        s << eleTag << "\t" << 1;
        s << "\t" << connectedExternalNodes(0) << "\t" << connectedExternalNodes(1);
        s << "\t" << connectedExternalNodes(2) << "\t" << connectedExternalNodes(3) << "\t0.00";
        s << endln;
        s << "PROP_3D\t" << eleTag << "\t";
        s << eleTag << "\t" << 1;
        s << "\t" << -1 << "\tSHELL\t1.0\0.0";
        s << endln;
    }
    
    if (flag < -1) {
        
        int counter = (flag + 1) * -1;
        int eleTag = this->getTag();
        int i, j;
        for (i = 0; i < 4; i++) {
            const Vector &stress = materialPointers[i]->getStressResultant();
            
            s << "STRESS\t" << eleTag << "\t" << counter << "\t" << i << "\tTOP";
            for (j = 0; j < 6; j++)
                s << "\t" << stress(j);
            s << endln;
        }
    }
    
    if (flag == OPS_PRINT_CURRENTSTATE) {
        s << endln;
        s << "NLDKGQ Non-Locking Four Node Shell \n";
        s << "Element Number: " << this->getTag() << endln;
        s << "Node 1 : " << connectedExternalNodes(0) << endln;
        s << "Node 2 : " << connectedExternalNodes(1) << endln;
        s << "Node 3 : " << connectedExternalNodes(2) << endln;
        s << "Node 4 : " << connectedExternalNodes(3) << endln;
        
        s << "Material Information : \n ";
        materialPointers[0]->Print(s, flag);
        
        s << endln;
    }
    
    if (flag == OPS_PRINT_PRINTMODEL_JSON) {
        s << "\t\t\t{";
        s << "\"name\": " << this->getTag() << ", ";
        s << "\"type\": \"ShellNLDKGQ\", ";
        s << "\"nodes\": [" << connectedExternalNodes(0) << ", " << connectedExternalNodes(1) << ", ";
        s << connectedExternalNodes(2) << ", " << connectedExternalNodes(3) << "], ";
        s << "\"section\": \"" << materialPointers[0]->getTag() << "\"}";
    }
}

Response*
ShellNLDKGQ::setResponse(const char **argv, int argc, OPS_Stream &output)
{
  Response *theResponse = 0;

  output.tag("ElementOutput");
  output.attr("eleType", "ShellNLDKGQ");
  output.attr("eleTag",this->getTag());
  int numNodes = this->getNumExternalNodes();
  const ID &nodes = this->getExternalNodes();
  static char nodeData[32];

  for (int i=0; i<numNodes; i++) {
    sprintf(nodeData,"node%d",i+1);
    output.attr(nodeData,nodes(i));
  }

  if (strcmp(argv[0],"force") == 0 || strcmp(argv[0],"forces") == 0 ||
      strcmp(argv[0],"globalForce") == 0 || strcmp(argv[0],"globalForces") == 0) {
    const Vector &force = this->getResistingForce();
    int size = force.Size();
    for (int i=0; i<size; i++) {
      sprintf(nodeData,"P%d",i+1);
      output.tag("ResponseType",nodeData);
    }
    theResponse = new ElementResponse(this, 1, this->getResistingForce());
  } 

  else if (strcmp(argv[0],"material") == 0 || strcmp(argv[0],"Material") == 0 ||
	   strcmp(argv[0],"section") == 0) {
    if (argc < 2) {
      opserr << "ShellNLDKGQ::setResponse() - need to specify more data\n";
      return 0;
    }
    int pointNum = atoi(argv[1]);
    if (pointNum > 0 && pointNum <= 4) {
      
      output.tag("GaussPoint");
      output.attr("number",pointNum);
      output.attr("eta",sg[pointNum-1]);
      output.attr("neta",tg[pointNum-1]);
      
      theResponse =  materialPointers[pointNum-1]->setResponse(&argv[2], argc-2, output);
      
      output.endTag();
    }

  } else if (strcmp(argv[0],"stresses") ==0) {

    for (int i=0; i<4; i++) {
      output.tag("GaussPoint");
      output.attr("number",i+1);
      output.attr("eta",sg[i]);
      output.attr("neta",tg[i]);
      
      output.tag("SectionForceDeformation");
      output.attr("classType", materialPointers[i]->getClassTag());
      output.attr("tag", materialPointers[i]->getTag());
      
      output.tag("ResponseType","p11");
      output.tag("ResponseType","p22");
      output.tag("ResponseType","p12");
      output.tag("ResponseType","m11");
      output.tag("ResponseType","m22");
      output.tag("ResponseType","m12");
      output.tag("ResponseType","q1");
      output.tag("ResponseType","q2");
      
      output.endTag(); // GaussPoint
      output.endTag(); // NdMaterialOutput
    }
    
    theResponse =  new ElementResponse(this, 2, Vector(32));
  }
  
  else if (strcmp(argv[0],"strains") ==0) {

    for (int i=0; i<4; i++) {
      output.tag("GaussPoint");
      output.attr("number",i+1);
      output.attr("eta",sg[i]);
      output.attr("neta",tg[i]);
      
      output.tag("SectionForceDeformation");
      output.attr("classType", materialPointers[i]->getClassTag());
      output.attr("tag", materialPointers[i]->getTag());
      
      output.tag("ResponseType","eps11");
      output.tag("ResponseType","eps22");
      output.tag("ResponseType","gamma12");
      output.tag("ResponseType","theta11");
      output.tag("ResponseType","theta22");
      output.tag("ResponseType","theta33");
      output.tag("ResponseType","gamma13");
      output.tag("ResponseType","gamma23");
      
      output.endTag(); // GaussPoint
      output.endTag(); // NdMaterialOutput
    }
    
    theResponse =  new ElementResponse(this, 3, Vector(32));
  }

  else if (theDamping[0] && strcmp(argv[0],"dampingStresses") ==0) {

    for (int i=0; i<4; i++) {
      output.tag("GaussPoint");
      output.attr("number",i+1);
      output.attr("eta",sg[i]);
      output.attr("neta",tg[i]);
      
      output.tag("SectionForceDeformation");
      output.attr("classType", theDamping[i]->getClassTag());
      output.attr("tag", theDamping[i]->getTag());
      
      output.tag("ResponseType","p11");
      output.tag("ResponseType","p22");
      output.tag("ResponseType","p12");
      output.tag("ResponseType","m11");
      output.tag("ResponseType","m22");
      output.tag("ResponseType","m12");
      output.tag("ResponseType","q1");
      output.tag("ResponseType","q2");
      
      output.endTag(); // GaussPoint
      output.endTag(); // NdMaterialOutput
    }
    
    theResponse =  new ElementResponse(this, 4, Vector(32));
  }

  output.endTag();
  return theResponse;
}

int
ShellNLDKGQ::getResponse(int responseID, Information &eleInfo)
{
  int cnt = 0;

  static Vector stresses(32);
  static Vector strains(32);

  switch (responseID) {
  case 1: // global forces
    return eleInfo.setVector(this->getResistingForce());
    break;

  case 2: // stresses
    for (int i = 0; i < 4; i++) {

      // Get material stress response
      const Vector &sigma = materialPointers[i]->getStressResultant();
      stresses(cnt) = sigma(0);
      stresses(cnt+1) = sigma(1);
      stresses(cnt+2) = sigma(2);
      stresses(cnt+3) = sigma(3);
      stresses(cnt+4) = sigma(4);
      stresses(cnt+5) = sigma(5);
      stresses(cnt+6) = sigma(6);
      stresses(cnt+7) = sigma(7);
      cnt += 8;
    }
    return eleInfo.setVector(stresses);
    break;
  case 3: //strain
    for (int i = 0; i < 4; i++) {

      // Get section deformation
      const Vector &deformation = materialPointers[i]->getSectionDeformation();
      strains(cnt) = deformation(0);
      strains(cnt+1) = deformation(1);
      strains(cnt+2) = deformation(2);
      strains(cnt+3) = deformation(3);
      strains(cnt+4) = deformation(4);
      strains(cnt+5) = deformation(5);
      strains(cnt+6) = deformation(6);
      strains(cnt+7) = deformation(7);
      cnt += 8;
    }
    return eleInfo.setVector(strains);
    break;
  case 4: // damping stresses
    for (int i = 0; i < 4; i++) {

      // Get material stress response
      const Vector &sigma = theDamping[i]->getDampingForce();
      stresses(cnt) = sigma(0);
      stresses(cnt+1) = sigma(1);
      stresses(cnt+2) = sigma(2);
      stresses(cnt+3) = sigma(3);
      stresses(cnt+4) = sigma(4);
      stresses(cnt+5) = sigma(5);
      stresses(cnt+6) = sigma(6);
      stresses(cnt+7) = sigma(7);
      cnt += 8;
    }
    return eleInfo.setVector(stresses);
    break;
  default:
    return -1;
  }
  cnt=0;

  //return 0;
}

int
ShellNLDKGQ::setParameter(const char **argv, int argc, Parameter &param)
{
  int res = -1;

  // damping
  if (strstr(argv[0], "damp") != 0) {

    if (argc < 2 || !theDamping)
      return -1;

    for (int i=0; i<4; i++) {
      int dmpRes =  theDamping[i]->setParameter(argv, argc, param);
      if (dmpRes != -1)
        res = dmpRes;
    }
    return res;
  }

  // Send to all sections
  for (int i = 0; i < 4; i++) {
    int secRes = materialPointers[i]->setParameter(argv, argc, param);
    if (secRes != -1) {
      res = secRes;
    }
  }
  return res;
}

//return stiffness matrix 
const Matrix&  ShellNLDKGQ::getTangentStiff( ) 
{
  int tang_flag = 1 ; //get the tangent 

  //do tangent and residual here
  formResidAndTangent( tang_flag ) ; 

  //opserr<<stiff<<endln;
  return stiff ;
}    

//return secant matrix 
const Matrix&  ShellNLDKGQ::getInitialStiff( ) 
{
	if(Ki!=0)
		return *Ki;

   static const int ndf = 6; //two membrane + 3 moment +drill

	static const int nstress = 8; //3 membrane , 3 moment, 2 shear

	static const int ngauss = 4;

	static const int numnodes = 4;

	int i,j,k,p,q;
	int jj,kk;
	int jlast,jnew; //add for geometric nonlinearity

	int p1,q1;

	int p2,q2;

	int p3,q3;

	int success;

	double volume = 0.0;

	static double xsj; //determinant jacobian matrix

	static double dvol[ngauss]; //volume element

	//add for geometric nonlinearity
	static Vector incrDisp(ndf); //total displamcement

	static Vector Cstrain(nstress);//commit strain last step/ add for geometric nonlinearity

	static Vector strain(nstress); //strain
	//add for geometric nonlinearity
	static Vector dstrain(nstress);   //total strain increment
	static Vector dstrain_li(nstress); //linear incr strain
	static Vector dstrain_nl(3);//geometric nonlinear strain

	static double shp[3][numnodes]; //shape function 2d at a gauss point

	static double shpDrill[4][numnodes]; //shape function drilling dof at a gauss point

	static double shpBend[6][12]; //shape function - bending part at a gauss point

	//static Vector residJ(ndf); //nodeJ residual, global coordinates

	static Matrix stiffJK(ndf,ndf);//nodeJK stiffness, global coordinates

	//static Vector residJlocal(ndf); //nodeJ residual, local coordinates

	static Matrix stiffJKlinear(ndf,ndf);//nodeJK stiffness,for linear part

	static Matrix stiffJKgeo(3,3);//nodeJK stiffness,for geometric nonlinearity

	static Matrix stiffJKlocal(ndf,ndf); //nodeJK stiffness, local coordinates

	static Matrix stiffJK1(ndf,ndf);

	static Matrix stiffJK2(ndf,ndf);

	static Matrix stiffJK3(ndf,ndf);

	//static Vector residJ1(ndf);

	static Vector stress(nstress); //stress resultants
	//static Vector dstress(nstress); //add for geometric nonlinearity

	static Matrix dd(nstress,nstress);//material tangent

	//static Matrix J0(2,2); //Jacobian at center

	//static Matrix J0inv(2,2); //inverse of Jacobian at center
	static double sx[2][2];
	
	//add for geometric nonlinearity
	static Vector dispIncLocal(6);  //incr disp in local coordinates
	static Vector dispIncLocalBend(3);//incr disp of bending part in local coordinates

	//eleForce & eleForceLast: gauss stress 
	static Vector stressLast_gauss(8); //eleForceLast
	//static Vector stressNew_gauss(8);  //eleForce
	static Matrix membraneForce(2,2); //membrane force in gauss point

	Matrix Tmat(6,6);  //local-global coordinates transform matrix

	Matrix TmatTran(6,6);

	Matrix Pmat(6,6); //transform dofs order

	Matrix PmatTran(6,6);

	//-------------------B-matrices---------------------------------

	static Matrix BJ(nstress, ndf); // B matrix node J

	static Matrix BJtran(ndf, nstress);

	static Matrix BK(nstress, ndf); // B matrix node K

	static Matrix BJtranD(ndf, nstress); //BJtran * dd

	static Matrix BJP(nstress, ndf); //BJ * Pmat, transform the dof order

	//static Matrix BJPT(nstress,ndf); //BJP * Tmat, from global coordinates to local coordinates


	static Matrix Bmembrane(3,3); //membrane B matrix

	static Matrix Bbend(3,3); //bending B matrix

	static Matrix Bshear(2,3); //shear B matrix (zero)

	static double saveB[nstress][ndf][numnodes];

	//Added for geometric nonlinearity 
	//BG
	static Matrix BGJ(2,3);

	static Matrix BGJtran(3,2);

	static Matrix stiffBGM(3,2);// BGJtran * membraneForce

	static Matrix BGK(2,3);
	//---------------------------------------------------------------

	//zero stiffness and residual
	stiff.Zero( );
	//resid.Zero( );

	//start Yuli Huang & Xinzheng Lu
	updateBasis( );
	//end Yuli Huang & Xinzheng Lu

	//define Pmat- transpose the dofs
	Pmat.Zero();
	double one=1.00;
	Pmat(0,0) = one;
	Pmat(1,1) = one;
	Pmat(2,5) = one;
	Pmat(3,2) = one;
	Pmat(4,3) = one;
	Pmat(5,4) = one;
   //transpose PmatTran=transpose(Pmat)
	for (p1=0;p1<6;p1++){
		for (q1=0;q1<6;q1++){
			PmatTran(p1,q1) = Pmat(q1,p1);
		}
	}//end for p1

	//define Tmat xl=Tmat * x from global to local coordinates
	Tmat.Zero();
	Tmat(0,0) = g1[0];
	Tmat(0,1) = g2[0];
	Tmat(0,2) = g3[0];
	Tmat(1,0) = g1[1];
	Tmat(1,1) = g2[1];
	Tmat(1,2) = g3[1];
	Tmat(2,0) = g1[2];
	Tmat(2,1) = g2[2];
	Tmat(2,2) = g3[2];

	Tmat(3,3) = g1[0];
	Tmat(3,4) = g2[0];
	Tmat(3,5) = g3[0];
	Tmat(4,3) = g1[1];
	Tmat(4,4) = g2[1];
	Tmat(4,5) = g3[1];
	Tmat(5,3) = g1[2];
	Tmat(5,4) = g2[2];
	Tmat(5,5) = g3[2];

   //transpose TmatTran=transpose(Tmat)
	for (p2=0;p2<6;p2++){
		for (q2=0;q2<6;q2++){
			TmatTran(p2,q2) = Tmat(q2,p2);
		}
	}//end for p2

	//------------gauss loop--------------------------
	for (i=0; i<ngauss;i++){

		//get shape functions
		shape2d(sg[i],tg[i],xl,shp,xsj,sx);

		shapeDrill(sg[i],tg[i],xl,sx,shpDrill);

		shapeBend(sg[i],tg[i],xl,sx,shpBend);

		//volume element to be saved
		dvol[i] = wg[i] * xsj;
		volume += dvol[i];

		Bshear.Zero( );

		//zero the dstrains
		//add for geometric nonlinearity
		dstrain.Zero();
		dstrain_li.Zero();
		dstrain_nl.Zero();

		//add for geometric nonlinearity
		for (jlast=0;jlast < nstress;jlast++){
			
			Cstrain(jlast) = CstrainGauss(i*8+jlast);
			
		}
		//opserr<<CstrainGauss<<endln;

		// j-node loop to compute strain
		for (j = 0; j < numnodes; j++){

			//compute B matrix

			Bmembrane = computeBmembrane(j,shp,shpDrill);

			Bbend = computeBbend(j,shpBend);

			BJ = assembleB(Bmembrane, Bbend, Bshear);

			//save the B-matrix
			for (p=0;p<nstress; p++){
				for(q=0;q<ndf;q++){
					saveB[p][q][j] = BJ(p,q);
				}
			}//end for p
			//opserr<<Bmembrane<<endln;

			//add for geometric nonlinearity
			//BGJ
			BGJ = computeBG(j,shpBend);

			//nodal "displacements"  - need to be modified for geometric nonlinearity
			//delta displacements
			//in global coordinates
			const Vector &TrialDisp = nodePointers[j]->getTrialDisp( );
			const Vector &CommitDisp = nodePointers[j]->getDisp( );

			//incrDisp = TrialDisp - CommitDisp;
			for(p=0;p<ndf;p++){
				incrDisp(p) = TrialDisp(p) - CommitDisp(p);
			}// end for p

			//dispIncLocal = Tmat * dul
			dispIncLocal.addMatrixVector(0.0,Tmat,incrDisp,1.0);

			//bending part incr disp
			dispIncLocalBend(0) = dispIncLocal(2);
			dispIncLocalBend(1) = dispIncLocal(3);
			dispIncLocalBend(2) = dispIncLocal(4);
			//opserr<<dul<<endln;

			//compute the strain - modified for geometric nonlinearity:dstrain_li(8)
			//Note: transform the dof's order
			//BJP = BJ * P;
			BJP.addMatrixProduct(0.0, BJ, Pmat,1.0);
			//BJPT.addMatrixProduct(0.0,BJP,Tmat,1.0);//transform ul from global to local coordinates
			//dstrain_li += (BJ*dispIncLocal);
			dstrain_li.addMatrixVector(1.0, BJP, dispIncLocal,1.0);

			//add for geometric nonlinearity: dstrain_nl(3)
		   //dstrain_nl += (BGJ*dulbend);  
			//note: dstrain should be BGJ * dispIncLocalBend sum from j=0 to j=3
			dstrain_nl += computeNLdstrain(BGJ,dispIncLocalBend);
			//opserr<<dstrain_nl<<endln;


			//dstrain = dstrain_li + dstrain_nl
			dstrain(0) = dstrain_li(0) +dstrain_nl(0);
			dstrain(1) = dstrain_li(1) +dstrain_nl(1);
			dstrain(2) = dstrain_li(2) +dstrain_nl(2);
			dstrain(3) = dstrain_li(3);
			dstrain(4) = dstrain_li(4);
			dstrain(5) = dstrain_li(5);
			dstrain(6) = dstrain_li(6);
			dstrain(7) = dstrain_li(7);

		}//end j-node loop

		//strain = Cstrain + dstrain;
		for(q=0;q<nstress;q++){
			strain(q) = Cstrain(q) + dstrain(q);
		}//end for q

		//send the strain to the material
		success = materialPointers[i]->setTrialSectionDeformation(strain);

		//compute the stress
		stress = materialPointers[i]->getStressResultant( );

		//opserr<<dstrain<<endln;
		//opserr<<strain<<endln;

		//add for geometric nonlinearity
		//update strain in gauss points
		//define TstrainGauss
		for (jnew=0;jnew < nstress;jnew++){
			
			TstrainGauss(i*8+jnew) = strain(jnew);
			
		}
		//CstrainGauss = TstrainGauss;

		//add for geometric nonlinearity
		//from stress(0,1,2) to compute membraneForce at t 
		membraneForce(0,0) = stress(0);
	    membraneForce(1,1) = stress(1);
	    membraneForce(0,1) = stress(2);
	    membraneForce(1,0) = stress(2);

		stress *= dvol[i];//for Resid integration
		membraneForce *= dvol[i]; //for stiffJKgeo integration

    dd = materialPointers[i]->getInitialTangent( );
    if(theDamping[i]) dd *= theDamping[i]->getStiffnessMultiplier();
    dd *= dvol[i];//for stiffJKlinear integration

		//opserr<<tang_flag<<endln;

		//residual and tangent calculations node loops

		//jj loop----------------
		jj = 0; 
		for (j=0; j<numnodes;j++){

			//extract BJ
			for(p=0;p<nstress;p++){
				for (q=0; q<ndf;q++)
					BJ(p,q)=saveB[p][q][j];
			}// end for p
			//multiply bending terms by -1.0 for correct statement of equilibrium
			for (p=3;p<6;p++){
				for (q=3; q<6; q++)
					BJ(p,q) *= (-1.0);
			}//end for p

			//transpose BJtran=transpose(BJ);
			for (p=0;p<ndf;p++){
				for(q=0;q<nstress;q++)
					BJtran(p,q)=BJ(q,p);
			}//end for p

			//add for geometric nonlinearity
			BGJ = computeBG(j,shpBend);
			//transpose BGJ
			for (p3=0; p3<3; p3++){
				for(q3=0; q3<2; q3++)
					BGJtran(p3,q3)=BGJ(q3,p3);
			}//end for p3
			stiffBGM.addMatrixProduct(0.0,BGJtran,membraneForce,1.0);

            BJtranD.addMatrixProduct(0.0,BJtran,dd,1.0);


			//k loop------------
			kk=0;
			for(k=0; k<numnodes; k++){

				//extract BK
				for (p=0; p<nstress;p++){
					for(q=0;q<ndf;q++)
						BK(p,q) = saveB[p][q][k];
				}// end for p

				//stiffJKlinear = BJtranD * BK - modify for geometric nonlinearity
				stiffJKlinear.addMatrixProduct(0.0,BJtranD,BK,1.0);

				//add for geometric nonlinearity
				//extract BGK
				BGK = computeBG(k,shpBend);
				//stiffGeo = BGJtran * membraneForce *BGK
				stiffJKgeo.addMatrixProduct(0.0,stiffBGM,BGK,1.0);

				//opserr<<stiffJKlinear<<endln;
				//opserr<<stiffJKgeo<<endln;

				//stiffJKlocal = stiffJKlinear + stiffJKgeo
				stiffJKlocal = stiffJKlinear;
				/*for(pp=3;pp<6;pp++){
					for(qq=3;qq<6;qq++)
						stiffJKlocal(pp,qq) += stiffJKgeo(pp-3,qq-3);
				}//end for pp*/

				//transpose dof and coordinates
				stiffJK1.addMatrixProduct(0.0,PmatTran,stiffJKlocal,1.0);
				stiffJK2.addMatrixProduct(0.0,stiffJK1,Pmat,1.0);
				stiffJK3.addMatrixProduct(0.0,TmatTran,stiffJK2,1.0);
				stiffJK.addMatrixProduct(0.0,stiffJK3,Tmat,1.0);
              //opserr<<Bbend<<endln;

				//generate stiff
				//modify for geometric nonlinearity
				for(p=0;p<ndf;p++){
					for(q=0;q<ndf;q++){
						stiff(jj+p,kk+q) += stiffJK(p,q);  
					}//end for q
				}//end for p

				kk += ndf;
			}//end for k loop

			jj += ndf;

		}// end for j loop

	}//end for i gauss loop

	Ki = new Matrix(stiff);

	return stiff;

}
    

//return mass matrix
const Matrix&  ShellNLDKGQ::getMass( ) 
{
  int tangFlag = 1 ;

  formInertiaTerms( tangFlag ) ;

  return mass ;
} 


void  ShellNLDKGQ::zeroLoad( )
{
  if (load != 0)
    load->Zero();

  return ;
}


int 
ShellNLDKGQ::addLoad(ElementalLoad *theLoad, double loadFactor)
{
  opserr << "ShellNLDKGQ::addLoad - load type unknown for ele with tag: " << this->getTag() << endln;
  return -1;
}



int 
ShellNLDKGQ::addInertiaLoadToUnbalance(const Vector &accel)
{
  int tangFlag = 1 ;
  static Vector r(24);

  int i;

  int allRhoZero = 0;
  for (i=0; i<4; i++) {
    if (materialPointers[i]->getRho() != 0.0)
      allRhoZero = 1;
  }

  if (allRhoZero == 0) 
    return 0;

  formInertiaTerms( tangFlag ) ;

  int count = 0;
  for (i=0; i<4; i++) {
    const Vector &Raccel = nodePointers[i]->getRV(accel);
    for (int j=0; j<6; j++)
      r(count++) = Raccel(j);
  }

  if (load == 0) 
    load = new Vector(24);

  load->addMatrixVector(1.0, mass, r, -1.0);

  return 0;
}



//get residual
const Vector&  ShellNLDKGQ::getResistingForce( ) 
{
  int tang_flag = 0 ; //don't get the tangent

  formResidAndTangent( tang_flag ) ;

  // subtract external loads 
  if (load != 0)
    resid -= *load;

  return resid ;   
}


//get residual with inertia terms
const Vector&  ShellNLDKGQ::getResistingForceIncInertia( )
{
  static Vector res(24);
  int tang_flag = 0 ; //don't get the tangent

  //do tangent and residual here 
  formResidAndTangent( tang_flag ) ;

  formInertiaTerms( tang_flag ) ;

  res = resid;
  // add the damping forces if rayleigh damping
  if (alphaM != 0.0 || betaK != 0.0 || betaK0 != 0.0 || betaKc != 0.0)
    res += this->getRayleighDampingForces();

  // subtract external loads 
  if (load != 0)
    res -= *load;

  return res;
}

//*********************************************************************
//form inertia terms

void   
ShellNLDKGQ::formInertiaTerms( int tangFlag ) 
{

  //translational mass only
  //rotational inertia terms are neglected


  static const int ndf = 6 ; 

  static const int numberNodes = 4 ;

  static const int numberGauss = 4 ;

  static const int nShape = 3 ;

  static const int massIndex = nShape - 1 ;

  double xsj ;  // determinant jacaobian matrix 

  double sx[2][2]; //inverse jacobian matrix

  double dvol ; //volume element

  static double shp[nShape][numberNodes] ;  //shape functions at a gauss point

  static Vector momentum(ndf) ;


  int i, j, k, p;
  int jj, kk ;

  double temp, rhoH, massJK ;


  //zero mass 
  mass.Zero( ) ;

  //gauss loop 
  for ( i = 0; i < numberGauss; i++ ) {

    //get shape functions    
    shape2d( sg[i], tg[i], xl, shp, xsj ,sx) ;

    //volume element to also be saved
    dvol = wg[i] * xsj ;  


    //node loop to compute accelerations
    momentum.Zero( ) ;
    for ( j = 0; j < numberNodes; j++ ) 
      //momentum += ( shp[massIndex][j] * nodePointers[j]->getTrialAccel() ) ;
      momentum.addVector(1.0,  
			 nodePointers[j]->getTrialAccel(),
			 shp[massIndex][j] ) ;

      
    //density
    rhoH = materialPointers[i]->getRho() ;

    //multiply acceleration by density to form momentum
    momentum *= rhoH ;


    //residual and tangent calculations node loops
    for ( j=0, jj=0; j<numberNodes; j++, jj+=ndf ) {

      temp = shp[massIndex][j] * dvol ;

      for ( p = 0; p < 3; p++ )
        resid( jj+p ) += ( temp * momentum(p) ) ;

      
      if ( tangFlag == 1 && rhoH != 0.0) {

	 //multiply by density
	 temp *= rhoH ;

	 //node-node translational mass
         for ( k=0, kk=0; k<numberNodes; k++, kk+=ndf ) {

	   massJK = temp * shp[massIndex][k] ;

	   for ( p = 0; p < 3; p++ ) 
	      mass( jj+p, kk+p ) +=  massJK ;
            
          } // end for k loop

      } // end if tang_flag 

    } // end for j loop

  } //end for i gauss loop 

}

//*********************************************************************

//form residual and tangent
void  
ShellNLDKGQ::formResidAndTangent( int tang_flag ) 
{
	//
	//six(6) nodal dof's ordered:
	//-----------
	//|    u1    |   <---- plate membrane
	//|    u2    |
	//|----------|
	//|   w = u3 |  <----plate bending
	//|   theta1 |
	//|   theta2 |
	//|----------|
	//|   theta3 |  <- drill (tran from membrane)
	//|----------|
  // membrane strains ordered :
  //
  //            strain(0) =   eps00     i.e.   (11)-strain
  //            strain(1) =   eps11     i.e.   (22)-strain
  //            strain(2) =   gamma01   i.e.   (12)-shear
  //
  // curvatures and shear strains ordered  :
  //
  //            strain(3) =     kappa00  i.e.   (11)-curvature
  //            strain(4) =     kappa11  i.e.   (22)-curvature
  //            strain(5) =   2*kappa01  i.e. 2*(12)-curvature 
  //
  //            strain(6) =     gamma02  i.e.   (13)-shear
  //            strain(7) =     gamma12  i.e.   (23)-shear
  //
  //  same ordering for moments/shears but no 2 
  //  
  //  Then, 
  //              epsilon00 = -z * kappa00      +    eps00_membrane
  //              epsilon11 = -z * kappa11      +    eps11_membrane
  //  gamma01 = 2*epsilon01 = -z * (2*kappa01)  +  gamma01_membrane 
  //
  //  Shear strains gamma02, gamma12 constant through cross section
  //     neglected in this shell element   Zero();
  //
	static const int ndf = 6; //two membrane + 3 moment +drill

	static const int nstress = 8; //3 membrane , 3 moment, 2 shear

	static const int ngauss = 4;

	static const int numnodes = 4;

	int i,j,k,p,q;
	int jj,kk;
	int jlast,jnew; //add for geometric nonlinearity

	int p1,q1;

	int p2,q2;

	int p3,q3;

	int pp,qq;

	int success;

	double volume = 0.0;

	static double xsj; //determinant jacobian matrix

	static double dvol[ngauss]; //volume element

	//add for geometric nonlinearity
	static Vector incrDisp(ndf); //total displacement

	static Vector Cstrain(nstress);//commit strain last step/ add for geometric nonlinearity

	static Vector strain(nstress); //strain
	//add for geometric nonlinearity
	static Vector dstrain(nstress);   //total strain increment
	static Vector dstrain_li(nstress); //linear incr strain
	static Vector dstrain_nl(3);//geometric nonlinear strain

	static double shp[3][numnodes]; //shape function 2d at a gauss point

	static double shpDrill[4][numnodes]; //shape function drilling dof at a gauss point

	static double shpBend[6][12]; //shape function - bending part at a gauss point

	static Vector residJ(ndf); //nodeJ residual, global coordinates

	static Matrix stiffJK(ndf,ndf);//nodeJK stiffness, global coordinates

	static Vector residJlocal(ndf); //nodeJ residual, local coordinates

	static Matrix stiffJKlinear(ndf,ndf);//nodeJK stiffness,for linear part

	static Matrix stiffJKgeo(3,3);//nodeJK stiffness,for geometric nonlinearity

	static Matrix stiffJKlocal(ndf,ndf); //nodeJK stiffness, local coordinates

	static Matrix stiffJK1(ndf,ndf);

	static Matrix stiffJK2(ndf,ndf);

	static Matrix stiffJK3(ndf,ndf);

	static Vector residJ1(ndf);

	static Vector stress(nstress); //stress resultants
	static Vector Cstress(nstress);
	//static Vector dstress(nstress); //add for geometric nonlinearity

  static Vector dampingStress(nstress); // damping stress resultants

	static Matrix dd(nstress,nstress);//material tangent

	//static Matrix J0(2,2); //Jacobian at center

	//static Matrix J0inv(2,2); //inverse of Jacobian at center
	static double sx[2][2];
	
	//add for geometric nonlinearity
	static Vector dispIncLocal(6);  //incr disp in local coordinates
	static Vector dispIncLocalBend(3);//incr disp of bending part in local coordinates

	//eleForce & eleForceLast: gauss stress 
	static Matrix membraneForce(2,2); //membrane force in gauss point

	Matrix Tmat(6,6);  //local-global coordinates transform matrix

	Matrix TmatTran(6,6);

	Matrix Pmat(6,6); //transform dofs order

	Matrix PmatTran(6,6);

	//-------------------B-matrices---------------------------------

	static Matrix BJ(nstress, ndf); // B matrix node J

	static Matrix BJtran(ndf, nstress);

	static Matrix BK(nstress, ndf); // B matrix node K

	static Matrix BJtranD(ndf, nstress); //BJtran * dd

	static Matrix BJP(nstress, ndf); //BJ * Pmat, transform the dof order

	//static Matrix BJPT(nstress,ndf); //BJP * Tmat, from global coordinates to local coordinates


	static Matrix Bmembrane(3,3); //membrane B matrix

	static Matrix Bbend(3,3); //bending B matrix

	static Matrix Bshear(2,3); //shear B matrix (zero)

	static double saveB[nstress][ndf][numnodes];

	//Added for geometric nonlinearity 
	//BG
	static Matrix BGJ(2,3);

	static Matrix BGJtran(3,2);

	static Matrix stiffBGM(3,2);// BGJtran * membraneForce

	static Matrix BGK(2,3);
	//---------------------------------------------------------------

	//zero stiffness and residual
	stiff.Zero( );
	resid.Zero( );

	//start Yuli Huang & Xinzheng Lu
	updateBasis( );
	//end Yuli Huang & Xinzheng Lu

	//define Pmat- transpose the dofs
	Pmat.Zero();
	double one=1.00;
	Pmat(0,0) = one;
	Pmat(1,1) = one;
	Pmat(2,5) = one;
	Pmat(3,2) = one;
	Pmat(4,3) = one;
	Pmat(5,4) = one;
   //transpose PmatTran=transpose(Pmat)
	for (p1=0;p1<6;p1++){
		for (q1=0;q1<6;q1++){
			PmatTran(p1,q1) = Pmat(q1,p1);
		}
	}//end for p1

	//define Tmat xl=Tmat * x from global to local coordinates
	Tmat.Zero();
	Tmat(0,0) = g1[0];
	Tmat(0,1) = g1[1];
	Tmat(0,2) = g1[2];
	Tmat(1,0) = g2[0];
	Tmat(1,1) = g2[1];
	Tmat(1,2) = g2[2];
	Tmat(2,0) = g3[0];
	Tmat(2,1) = g3[1];
	Tmat(2,2) = g3[2];

	Tmat(3,3) = g1[0];
	Tmat(3,4) = g1[1];
	Tmat(3,5) = g1[2];
	Tmat(4,3) = g2[0];
	Tmat(4,4) = g2[1];
	Tmat(4,5) = g2[2];
	Tmat(5,3) = g3[0];
	Tmat(5,4) = g3[1];
	Tmat(5,5) = g3[2];
	
   //transpose TmatTran=transpose(Tmat)
	for (p2=0;p2<6;p2++){
		for (q2=0;q2<6;q2++){
			TmatTran(p2,q2) = Tmat(q2,p2);
		}
	}//end for p2

	//------------gauss loop--------------------------
	for (i=0; i<ngauss;i++){

		//get shape functions
		shape2d(sg[i],tg[i],xl,shp,xsj,sx);

		shapeDrill(sg[i],tg[i],xl,sx,shpDrill);

		shapeBend(sg[i],tg[i],xl,sx,shpBend);

		//volume element to be saved
		dvol[i] = wg[i] * xsj;
		volume += dvol[i];

		Bshear.Zero( );

		//zero the dstrains
		//add for geometric nonlinearity
		dstrain.Zero( );
		dstrain_li.Zero( );
		dstrain_nl.Zero( );

		//add for geometric nonlinearity
		for (jlast=0;jlast < nstress;jlast++){
			
			Cstrain(jlast) = CstrainGauss(i*8+jlast);
			
		}

		// j-node loop to compute strain
		for (j = 0; j < numnodes; j++){

			//compute B matrix

			Bmembrane = computeBmembrane(j,shp,shpDrill);

			Bbend = computeBbend(j,shpBend);

			BJ = assembleB(Bmembrane, Bbend, Bshear);

			//save the B-matrix
			for (p=0;p<nstress; p++){
				for(q=0;q<ndf;q++){
					saveB[p][q][j] = BJ(p,q);
				}
			}//end for p

			//add for geometric nonlinearity
			//BGJ
			BGJ = computeBG(j,shpBend);
			//opserr<<BGJ<<endln;

			//nodal "displacements"  - need to be modified for geometric nonlinearity
			//delta displacements
			//in global coordinates
			/*const Vector &TrialDisp = nodePointers[j]->getTrialDisp( );
			const Vector &CommitDisp = nodePointers[j]->getDisp( );
          
			//incrDisp = TrialDisp - CommitDisp;
			for(p=0;p<ndf;p++){
				incrDisp(p) = TrialDisp(p) - CommitDisp(p);
			}// end for p*/
			const Vector &incrDisp = nodePointers[j]->getIncrDisp( );

			//dispIncLocal = Tmat * dul
			dispIncLocal.addMatrixVector(0.0,Tmat,incrDisp,1.0);

			//bending part incr disp
			dispIncLocalBend(0) = dispIncLocal(2);
			dispIncLocalBend(1) = dispIncLocal(3);
			dispIncLocalBend(2) = dispIncLocal(4);

			//compute the strain - modified for geometric nonlinearity:dstrain_li(8)
			//Note: transform the dof's order
			//BJP = BJ * P;
			BJP.addMatrixProduct(0.0, BJ, Pmat,1.0);
			//dstrain_li += (BJ*dispIncLocal);
			dstrain_li.addMatrixVector(1.0, BJP, dispIncLocal,1.0);

			//add for geometric nonlinearity: dstrain_nl(3)
		   //dstrain_nl += (BGJ*dulbend);
			//note: dstrain should be BGJ * dispIncLocalBend sum from j=0 to j=3
			dstrain_nl += computeNLdstrain(BGJ,dispIncLocalBend);


		}//end j-node loop
		   dstrain_nl.Zero( );//add for accelerate the convergence,but not zero in stability analysis problems.
		   dstrain(0) = dstrain_li(0) +dstrain_nl(0);
			dstrain(1) = dstrain_li(1) +dstrain_nl(1);
			dstrain(2) = dstrain_li(2) +dstrain_nl(2);
			dstrain(3) = dstrain_li(3);
			dstrain(4) = dstrain_li(4);
			dstrain(5) = dstrain_li(5);
			dstrain(6) = dstrain_li(6);
			dstrain(7) = dstrain_li(7);

		//strain = Cstrain + dstrain;
		for(q=0;q<nstress;q++){
			strain(q) = Cstrain(q) + dstrain(q);
		}//end for q

		success = materialPointers[i]->setTrialSectionDeformation(strain);

		//compute the stress
		stress = materialPointers[i]->getStressResultant( );

    if (theDamping[i])
    {
      theDamping[i]->update(stress);
      dampingStress = theDamping[i]->getDampingForce();
      dampingStress *= dvol[i];
    }

		//add for geometric nonlinearity
		//update strain in gauss points
		//define TstrainGauss
		for (jnew=0;jnew < nstress;jnew++){
			
			TstrainGauss(i*8+jnew) = strain(jnew);
			
		}

		//add for geometric nonlinearity
		//from stress(0,1,2) to compute membraneForce at t 
		membraneForce(0,0) = stress(0);
	    membraneForce(1,1) = stress(1);
	    membraneForce(0,1) = stress(2);
	    membraneForce(1,0) = stress(2);

		stress *= dvol[i];//for Resid integration
		membraneForce *= dvol[i]; //for stiffJKgeo integration

		//dd *= dvol[i];
		if(tang_flag == 1){
			dd = materialPointers[i]->getSectionTangent( );
      if(theDamping[i]) dd *= theDamping[i]->getStiffnessMultiplier();
			dd *= dvol[i];//for stiffJKlinear integration
		}//end if tang_flag

		//residual and tangent calculations node loops

		//jj loop----------------
		jj = 0; 
		for (j=0; j<numnodes;j++){

			//extract BJ
			for(p=0;p<nstress;p++){
				for (q=0; q<ndf;q++)
					BJ(p,q)=saveB[p][q][j];
			}// end for p
			//multiply bending terms by -1.0 for correct statement of equilibrium
			for (p=3;p<6;p++){
				for (q=3; q<6; q++)
					BJ(p,q) *= (-1.0);
			}//end for p 

			//transpose BJtran=transpose(BJ);
			for (p=0;p<ndf;p++){
				for(q=0;q<nstress;q++)
					BJtran(p,q)=BJ(q,p);
			}//end for p

			//compute residual force - no need to modify for geometric nonlinearity
			residJlocal.addMatrixVector(0.0,BJtran,stress,1.0);
      if (theDamping[i]) residJlocal.addMatrixVector(1.0, BJtran,dampingStress,1.0 ) ;
			residJ1.addMatrixVector(0.0,PmatTran,residJlocal,1.0);
			residJ.addMatrixVector(0.0,TmatTran,residJ1,1.0);

			for(p=0; p<ndf; p++)
				resid(jj+p) += residJ(p);
			//add for geometric nonlinearity
			BGJ = computeBG(j,shpBend);
			//transpose BGJ
			for (p3=0; p3<3; p3++){
				for(q3=0; q3<2; q3++)
					BGJtran(p3,q3)=BGJ(q3,p3);
			}//end for p3
			stiffBGM.addMatrixProduct(0.0,BGJtran,membraneForce,1.0);

			//BJtranD = BJtran *dd;
			if(tang_flag == 1){

            BJtranD.addMatrixProduct(0.0,BJtran,dd,1.0);


			//k loop------------
			kk=0;
			for(k=0; k<numnodes; k++){

				//extract BK
				for (p=0; p<nstress;p++){
					for(q=0;q<ndf;q++)
						BK(p,q) = saveB[p][q][k];
				}// end for p

				//stiffJKlinear = BJtranD * BK - modify for geometric nonlinearity
				stiffJKlinear.addMatrixProduct(0.0,BJtranD,BK,1.0);

				//add for geometric nonlinearity
				//extract BGK
				BGK = computeBG(k,shpBend);
				//stiffGeo = BGJtran * membraneForce *BGK
				stiffJKgeo.addMatrixProduct(0.0,stiffBGM,BGK,1.0);

				//stiffJKlocal = stiffJKlinear + stiffJKgeo
				stiffJKlocal = stiffJKlinear;
				for(pp=3;pp<6;pp++){
					for(qq=3;qq<6;qq++)
						stiffJKlocal(pp,qq) += stiffJKgeo(pp-3,qq-3);
				}//end for pp

				//transpose dof and coordinates
				stiffJK1.addMatrixProduct(0.0,PmatTran,stiffJKlocal,1.0);
				stiffJK2.addMatrixProduct(0.0,stiffJK1,Pmat,1.0);
				stiffJK3.addMatrixProduct(0.0,TmatTran,stiffJK2,1.0);
				stiffJK.addMatrixProduct(0.0,stiffJK3,Tmat,1.0);

				//generate stiff
				//modify for geometric nonlinearity
				for(p=0;p<ndf;p++){
					for(q=0;q<ndf;q++){
						stiff(jj+p,kk+q) += stiffJK(p,q);  
					}//end for q
				}//end for p

				kk += ndf;
			}//end for k loop

			}//end if tang_flag

			jj += ndf;

		}// end for j loop

	}//end for i gauss loop

	return;

}


//************************************************************************
//compute local coordinates and basis

void   
ShellNLDKGQ::computeBasis( ) 
{  
  //could compute derivatives \frac{ \partial {\bf x} }{ \partial L_1 } 
  //                     and  \frac{ \partial {\bf x} }{ \partial L_2 }
  //and use those as basis vectors but this is easier 
  //and the shell is flat anyway.

  static Vector temp(3) ;

  static Vector v1(3) ;
  static Vector v2(3) ;
  static Vector v3(3) ;

  //get two vectors (v1, v2) in plane of shell by 
  // nodal coordinate differences

  const Vector &coor0 = nodePointers[0]->getCrds( ) ;

  const Vector &coor1 = nodePointers[1]->getCrds( ) ;

  const Vector &coor2 = nodePointers[2]->getCrds( ) ;
  
  const Vector &coor3 = nodePointers[3]->getCrds( ) ;

  v1.Zero( ) ;
  //v1 = 0.5 * ( coor2 + coor1 - coor3 - coor0 ) ;
  v1  = coor2 ;
  v1 += coor1 ;
  v1 -= coor3 ;
  v1 -= coor0 ;
  v1 *= 0.50 ;
  
  v2.Zero( ) ;
  //v2 = 0.5 * ( coor3 + coor2 - coor1 - coor0 ) ;
  v2  = coor3 ;
  v2 += coor2 ;
  v2 -= coor1 ;
  v2 -= coor0 ;
  v2 *= 0.50 ;
 
  //normalize v1 
  //double length = LovelyNorm( v1 ) ;
  double length = v1.Norm( ) ;
  v1 /= length ;

  //Gram-Schmidt process for v2 

  //double alpha = LovelyInnerProduct( v2, v1 ) ;
  double alpha = v2^v1 ;

  //v2 -= alpha*v1 ;
  temp = v1 ;
  temp *= alpha ;
  v2 -= temp ;

  //normalize v2 
  //length = LovelyNorm( v2 ) ;
  length = v2.Norm( ) ;
  v2 /= length ;

  //cross product for v3  
  v3 = LovelyCrossProduct( v1, v2 ) ;
  
  //local nodal coordinates in plane of shell

  int i ;
  for ( i = 0; i < 4; i++ ) {

       const Vector &coorI = nodePointers[i]->getCrds( ) ;
       xl[0][i] = coorI^v1 ;  
       xl[1][i] = coorI^v2 ;

  }  //end for i 

  //basis vectors stored as array of doubles
  for ( i = 0; i < 3; i++ ) {
      g1[i] = v1(i) ;
      g2[i] = v2(i) ;
      g3[i] = v3(i) ;
  }  //end for i 


}

//start Yuli Huang (yulihuang@gmail.com) & Xinzheng Lu (luxz@tsinghua.edu.cn)
//************************************************************************
//compute local coordinates and basis

void   
ShellNLDKGQ::updateBasis( ) 
{

  //could compute derivatives \frac{ \partial {\bf x} }{ \partial L_1 } 
  //                     and  \frac{ \partial {\bf x} }{ \partial L_2 }
  //and use those as basis vectors but this is easier 
  //and the shell is flat anyway.

  static Vector temp(3) ;

  static Vector v1(3) ;
  static Vector v2(3) ;
  static Vector v3(3) ;


  const Vector &coor0 = nodePointers[0]->getCrds( ) + nodePointers[0]->getTrialDisp( );
  const Vector &coor1 = nodePointers[1]->getCrds( ) + nodePointers[1]->getTrialDisp( );
  const Vector &coor2 = nodePointers[2]->getCrds( ) + nodePointers[2]->getTrialDisp( );
  const Vector &coor3 = nodePointers[3]->getCrds( ) + nodePointers[3]->getTrialDisp( );

  v1.Zero( ) ;
  //v1 = 0.5 * ( coor2 + coor1 - coor3 - coor0 ) ;
  v1  = coor2 ;
  v1 += coor1 ;
  v1 -= coor3 ;
  v1 -= coor0 ;
  v1 *= 0.50 ;
  
  v2.Zero( ) ;
  //v2 = 0.5 * ( coor3 + coor2 - coor1 - coor0 ) ;
  v2  = coor3 ;
  v2 += coor2 ;
  v2 -= coor1 ;
  v2 -= coor0 ;
  v2 *= 0.50 ;
  /*//normalize v1 
  //double length = LovelyNorm( v1 ) ;
  double length = v1.Norm( ) ;
  v1 /= length ;

  //Gram-Schmidt process for v2 

  //double alpha = LovelyInnerProduct( v2, v1 ) ;
  double alpha = v2^v1 ;

  //v2 -= alpha*v1 ;
  temp = v1 ;
  temp *= alpha ;
  v2 -= temp ;

  //normalize v2 
  //length = LovelyNorm( v2 ) ;
  length = v2.Norm( ) ;
  v2 /= length ;

  //cross product for v3  
  v3 = LovelyCrossProduct( v1, v2 ) ;*/

  // this procedure is simplified by Lisha Wang
  v3 = LovelyCrossProduct(v1,v2);
  v2 = LovelyCrossProduct(v3,v1);
  temp(0) = v1.Norm( );
  temp(1) = v2.Norm( );
  temp(2) = v3.Norm( );

  v1 /= temp(0);
  v2 /= temp(1);
  v3 /= temp(2);
  
  //local nodal coordinates in plane of shell

  int i ;
  for ( i = 0; i < 4; i++ ) {

       const Vector &coorI = nodePointers[i]->getCrds( ) + nodePointers[i]->getDisp( );//modify by Lisha Wang
       xl[0][i] = coorI^v1 ;  
       xl[1][i] = coorI^v2 ;

  }  //end for i 

  //basis vectors stored as array of doubles
  for ( i = 0; i < 3; i++ ) {
      g1[i] = v1(i) ;
      g2[i] = v2(i) ;
      g3[i] = v3(i) ;
  }  //end for i 


}
//end Yuli Huang (yulihuang@gmail.com) & Xinzheng Lu (luxz@tsinghua.edu.cn)


//********************************************************************
//assemble a B matrix

const Matrix&  
ShellNLDKGQ::assembleB( const Matrix &Bmembrane,
                               const Matrix &Bbend, 
                               const Matrix &Bshear ) 
{
	static Matrix B(8,6);
	
	int p,q;

	int pp;

// For Shell : 
//
//---B Matrices in standard {1,2,3} mechanics notation---------
//
//            -                     _          
//           | Bmembrane  |     0    |
//           | --------------------- |     
//    B =    |     0      |  Bbend   |   (8x6) 
//           | --------------------- |
//           |            |  Bshear  |
//            -           -         -
//
//-------------------------------------------------------------
	B.Zero( );
	//assemble B from sub-matrices

	//membrane parts
	for(p=0; p<3; p++) {

		for(q=0; q<3; q++)
			B(p,q) = Bmembrane(p,q);
	}//end for p

	//bending parts
	for (p=3; p<6; p++){
		pp = p-3;
		for(q=3; q<6;q++)
			B(p,q)=Bbend(pp,q-3);
	}//end for p

	//shear parts
	for(p=0;p<2;p++){
		pp = p+6;
       
		for(q=3; q<6; q++)
			B(pp,q)=Bshear(p,q-3);
	}//end for p

   return B;

}

//***********************************************************************
//compute Bmembrane matrix

const Matrix&
ShellNLDKGQ::computeBmembrane(int node, const double shp[3][4],
							const double shpDrill[4][4])
{
	static Matrix Bmembrane(3,3);

	// ------Bmembrane Matrix in standard {1,2,3} mechanics notation ---------------
	// 
	//               | N,1    0      Nu,1  |
	//   Bmembrane = | 0     N,2     Nv,2  |
	//               | N,2   N,1  Nv,1+Nu,2|
   // -----------------------------------------------------------------------------------

	Bmembrane.Zero( );

	Bmembrane(0,0) = shp[0][node];
	Bmembrane(0,2) = shpDrill[0][node];
	Bmembrane(1,1) = shp[1][node];
	Bmembrane(1,2) = shpDrill[3][node];
	Bmembrane(2,0) = shp[1][node];
	Bmembrane(2,1) = shp[0][node];
	Bmembrane(2,2) = shpDrill[1][node]+shpDrill[2][node];

	return Bmembrane;

}


//***********************************************************************
//compute Bbend matrix

const Matrix&
 ShellNLDKGQ::computeBbend(int node ,const double shpBend[6][12])
 {
	 static Matrix Bbend(3,3);

	 int i,j,k;

	 //----------Bbend Matrix in standard {1,2,3}mechanics notation------------------
	 //
	 //               |
	 //      Bbend =  |  Hx[3*node],1  Hx[3*node + 1],1  Hx[3*node + 2],1  |
	 //               |  Hy[3*node],2  Hy[3*node + 1],2  Hy[3*node + 2],2  |
	 //               |  Hx[3*node],2  Hx[3*node + 1],2  Hx[3*node + 2],2  |
	 //               |  + Hy[3*node],1 +Hy[3*node + 1],1 +Hy[3*node + 2],1|
	 //------------------------------------------------------------------------------
	 i = 3*node;
	 j = 3*node + 1;
	 k = 3*node + 2;

	 Bbend.Zero( );

	 Bbend(0,0) = shpBend[2][i];
	 Bbend(0,1) = shpBend[2][j];
	 Bbend(0,2) = shpBend[2][k];

	 Bbend(1,0) = shpBend[5][i];
	 Bbend(1,1) = shpBend[5][j];
	 Bbend(1,2) = shpBend[5][k];

 	 Bbend(2,0) = shpBend[3][i] + shpBend[4][i];
	 Bbend(2,1) = shpBend[3][j] + shpBend[4][j];
	 Bbend(2,2) = shpBend[3][k] + shpBend[4][k];

	 /*
	 bugfix: Massimo Petracca 02/26/2020. with the original implementation, the curvatures
	 sent to the section had the wrong sign.
	 */
	 Bbend *= -1.0;

	 return Bbend;
 }
//*************************************************************************
//compute BG matrix
const Matrix&
 ShellNLDKGQ::computeBG(int node ,const double shpBend[6][12])
 {
	 static Matrix BG(2,3);

	 int i,j,k;

	 //----------BG Matrix in standard {1,2,3}mechanics notation------------------
	 //
	 //               |
	 //      BG =     |  -Hx[3*node]  -Hx[3*node + 1]  -Hx[3*node + 2]  |
	 //               |  -Hy[3*node]  -Hy[3*node + 1]  -Hy[3*node + 2]  |
	 //------------------------------------------------------------------------------
	 i = 3*node;
	 j = 3*node + 1;
	 k = 3*node + 2;

	 BG.Zero( );

	 BG(0,0) = -shpBend[0][i];
	 BG(0,1) = -shpBend[0][j];
	 BG(0,2) = -shpBend[0][k];

	 BG(1,0) = -shpBend[1][i];
	 BG(1,1) = -shpBend[1][j];
	 BG(1,2) = -shpBend[1][k];

	 return BG;
 }
//************************************************************************
//compute the nonlinearity strain Increment associated with BG & bending
const Vector&
 ShellNLDKGQ::computeNLdstrain(const Matrix &BG,const Vector &dispIncLocalBend)
{
	static Vector dstrain_nl(3);
	static Vector strainInc(2);

	strainInc.addMatrixVector(0.0,BG,dispIncLocalBend,1.0);

	dstrain_nl(0) = strainInc(0) * strainInc(0) * 0.5;
	dstrain_nl(1) = strainInc(1) * strainInc(1) * 0.5;
	dstrain_nl(2) = strainInc(0) * strainInc(1);

	return dstrain_nl;
}
//shape function routine for four node quads

void  
ShellNLDKGQ::shape2d( double ss, double tt, 
		           const double x[2][4], 
		           double shp[3][4], 
		           double &xsj, double sx[2][2] )

{ 

  int i, j, k ;

  double temp ;
     
  static const double s[] = { -0.5,  0.5, 0.5, -0.5 } ;
  static const double t[] = { -0.5, -0.5, 0.5,  0.5 } ;

  static double xs[2][2] ;
 // static double sx[2][2] ;  //have been defined before

  for ( i = 0; i < 4; i++ ) {
      shp[2][i] = ( 0.5 + s[i]*ss )*( 0.5 + t[i]*tt ) ; //shape function for 2d isoparametric element
      shp[0][i] = s[i] * ( 0.5 + t[i]*tt ) ; // derivative to isoparametric coordinates 1
      shp[1][i] = t[i] * ( 0.5 + s[i]*ss ) ; // derivative to isoparametric coordinates 2
  } // end for i

  
  // Construct jacobian and its inverse
  
  for ( i = 0; i < 2; i++ ) {
    for ( j = 0; j < 2; j++ ) {

      xs[i][j] = 0.0 ;

      for ( k = 0; k < 4; k++ )
	  xs[i][j] +=  x[i][k] * shp[j][k] ;

    } //end for j
  }  // end for i 

  xsj = xs[0][0]*xs[1][1] - xs[0][1]*xs[1][0] ;

  //inverse jacobian
  double jinv = 1.0 / xsj ;
  sx[0][0] =  xs[1][1] * jinv ;
  sx[1][1] =  xs[0][0] * jinv ;
  sx[0][1] = -xs[0][1] * jinv ; 
  sx[1][0] = -xs[1][0] * jinv ; 

  //form global derivatives, local coordinates
  
  for ( i = 0; i < 4; i++ ) {
    temp      = shp[0][i]*sx[0][0] + shp[1][i]*sx[1][0] ;
    shp[1][i] = shp[0][i]*sx[0][1] + shp[1][i]*sx[1][1] ;
    shp[0][i] = temp ;
  } // end for i

  return ;
}
	   
//**********************************************************************

/*Matrix  
ShellNLDKGQ::transpose( int dim1, 
                                       int dim2, 
		                       const Matrix &M ) 
{
  int i ;
  int j ;

  Matrix Mtran( dim2, dim1 ) ;

  for ( i = 0; i < dim1; i++ ) {
     for ( j = 0; j < dim2; j++ ) 
         Mtran(j,i) = M(i,j) ;
  } // end for i

  return Mtran ;
}
*/
//**********************************************************************
// shape function for drill dof
void
ShellNLDKGQ::shapeDrill(double ss, double tt, 
					  const double x[2][4],
					  double sx[2][2], double shpDrill[4][4])
{
	double a1, a2, a3;
	double b1, b2, b3;

	static const double s[] = {-1.0, 1.0, 1.0, -1.0};
	static const double t[] = {-1.0, -1.0, 1.0, 1.0};

	double shptemp[4][4];   //derivative to xi,eta

	int i,j,k;

	const double one_over_four = 1/4.0;
    const double one_over_eight = 1/8.0;
	//compute a1-a3,b1-b3
   a1 = 0.0;
   a2 = 0.0;
   a3 = 0.0;
   b1 = 0.0;
   b2 = 0.0;
   b3 = 0.0;
	for (i=0;i<4;i++){
		a1 += s[i] * x[0][i] * one_over_four;
		a2 += t[i] * x[0][i] * one_over_four;
		a3 += s[i] * t[i] * x[0][i] * one_over_four;
		b1 += s[i] * x[1][i] * one_over_four;
		b2 += t[i] * x[1][i] * one_over_four;
		b3 += s[i] * t[i] * x[1][i] * one_over_four;		
	}//end for i

	//compute the derivatives of shape function to xi, eta
	// shptemp[0][j] = Nu,xi   shptemp[1][j] = Nu,eta
	// shptemp[2][j] = Nv,xi   shptemp[3][j] = Nv,eta
	for (j =0; j<4; j++){
		shptemp[0][j] = one_over_eight * (-2.0*s[j]*ss*(b1+b3*t[j])*(1.0+t[j]*tt)+
			          s[j]*t[j]*(1.0-tt*tt)*(b2+b3*s[j]));
		shptemp[1][j] = one_over_eight *(s[j]*t[j]*(1.0-ss*ss)*(b1+b3*t[j])-
			          2.0*t[j]*tt*(b2+b3*s[j])*(1.0+s[j]*ss));
		shptemp[2][j] = -one_over_eight * (-2.0*s[j]*ss*(a1+a3*t[j])*(1.0+t[j]*tt)+
			          s[j]*t[j]*(1.0-tt*tt)*(a2+a3*s[j]));
		shptemp[3][j] = -one_over_eight *(s[j]*t[j]*(1.0-ss*ss)*(a1+a3*t[j])-
			          2.0*t[j]*tt*(a2+a3*s[j])*(1.0+s[j]*ss));
	}// end for j

	//define shpdrill
	//           |  Nu,1  |
	//shpDrill = |  Nu,2  |
	//           |  Nv,1  |
	//           |  Nv,2  |
	for (k=0; k<4; k++){

		shpDrill[0][k] = shptemp[0][k] * sx[0][0]+
			            shptemp[1][k] * sx[1][0];
		shpDrill[1][k] = shptemp[0][k] * sx[0][1]+
			            shptemp[1][k] * sx[1][1];
		shpDrill[2][k] = shptemp[2][k] * sx[0][0]+
			            shptemp[3][k] * sx[1][0];
		shpDrill[3][k] = shptemp[2][k] * sx[0][1]+
			            shptemp[3][k] * sx[1][1];
	}//end for k

	return;
}


//*********************************************************************
//shape function for bending plate
void
ShellNLDKGQ::shapeBend(double ss, double tt, const double x[2][4],
					 double sx[2][2], double shpBend[6][12])
{
	//static Vector N(8);
	//static Vector Nxi(8);
	//static Vector Neta(8);
	static double N[3][8];

	double a5, a6, a7, a8;
	double b5, b6, b7, b8;
	double c5, c6, c7, c8;
	double d5, d6, d7, d8;
	double e5, e6, e7, e8;

	double x12, x23, x34, x41;
	double y12, y23, y34, y41;
	double L12, L23, L34, L41;

	static double temp[4][12];

	int i;

	const double one_over_four = 1.0/4.0;

	//define xij,yij,lij
	x12 = x[0][0] - x[0][1];
	x23 = x[0][1] - x[0][2];
	x34 = x[0][2] - x[0][3];
	x41 = x[0][3] - x[0][0];

	y12 = x[1][0] - x[1][1];
	y23 = x[1][1] - x[1][2];
	y34 = x[1][2] - x[1][3];
	y41 = x[1][3] - x[1][0];

	L12 = x12 * x12 + y12 * y12;
	L23 = x23 * x23 + y23 * y23;
	L34 = x34 * x34 + y34 * y34;
	L41 = x41 * x41 + y41 * y41;

	//a5-8,b5-8,c5-8,d5-8,e5-8
	a5 = - x12/L12;
	b5 = one_over_four * 3.0 * x12 * y12/L12;
	c5 = one_over_four *(x12*x12 - 2.0*y12*y12)/L12;
	d5 = - y12/L12;
	e5 = one_over_four *(y12*y12 - 2.0*x12*x12)/L12;

	a6 = - x23/L23;
	b6 = one_over_four * 3.0 * x23 * y23/L23;
	c6 = one_over_four *(x23*x23 - 2.0*y23*y23)/L23;
	d6 = - y23/L23;
	e6 = one_over_four *(y23*y23 - 2.0*x23*x23)/L23;

	a7 = - x34/L34;
	b7 = one_over_four * 3.0 * x34 * y34/L34;
	c7 = one_over_four *(x34*x34 - 2.0*y34*y34)/L34;
	d7 = - y34/L34;
	e7 = one_over_four *(y34*y34 - 2.0*x34*x34)/L34;

	a8 = - x41/L41;
	b8 = one_over_four * 3.0 * x41 * y41/L41;
	c8 = one_over_four *(x41*x41 - 2.0*y41*y41)/L41;
	d8 = - y41/L41;
	e8 = one_over_four *(y41*y41 - 2.0*x41*x41)/L41;

	//define 3-d isoparametric shape function
	N[0][0] = - one_over_four * (1.0-ss) * (1.0-tt) * (1.0+ss+tt);
	N[0][1] = - one_over_four * (1.0+ss) * (1.0-tt) * (1.0-ss+tt);
	N[0][2] = - one_over_four * (1.0+ss) * (1.0+tt) * (1.0-ss-tt);
	N[0][3] = - one_over_four * (1.0-ss) * (1.0+tt) * (1.0+ss-tt);
	N[0][4] = (1.0-ss*ss) * (1.0-tt) *2.0 * one_over_four;
	N[0][5] = (1.0+ss) * (1.0-tt*tt) *2.0 * one_over_four;
	N[0][6] = (1.0-ss*ss) * (1.0+tt) *2.0 * one_over_four;
	N[0][7] = (1.0-ss) * (1.0-tt*tt) *2.0 * one_over_four;
	//N,xi - deravatives
	N[1][0] = one_over_four * (1.0-tt) * (tt+2.0*ss);
	N[1][1] = one_over_four * (1.0-tt) * (2.0*ss-tt);
	N[1][2] = one_over_four * (1.0+tt) * (2.0*ss+tt);
	N[1][3] = one_over_four * (1.0+tt) * (2.0*ss-tt);
	N[1][4] = -ss * (1.0-tt);
	N[1][5] = (1.0-tt*tt)*2.0*one_over_four;
	N[1][6] = -ss * (1.0 + tt);
	N[1][7] = -(1.0-tt*tt)*2.0*one_over_four;
	//N,eta - deravitaves
	N[2][0] = (1.0-ss) * (ss+2.0*tt) * one_over_four;
	N[2][1] = (1.0+ss) * (2.0*tt-ss) * one_over_four;
	N[2][2] = (1.0+ss) * (2.0*tt+ss) * one_over_four;
	N[2][3] = (1.0-ss) * (2.0*tt-ss) * one_over_four;
	N[2][4] = -(1.0-ss*ss) * 2.0 * one_over_four;
	N[2][5] = - tt * (1.0 + ss);
	N[2][6] = (1.0-ss*ss) * 2.0 * one_over_four;
	N[2][7] = - tt * (1.0 - ss);
	//Hx, Hy
	const double three_over_two = 3.0/2.0;
	//Hx
	shpBend[0][0] = (a5*N[0][4]-a8*N[0][7]) * three_over_two;
	shpBend[0][1] = b5*N[0][4] + b8*N[0][7];
	shpBend[0][2] = N[0][0] - c5*N[0][4] - c8*N[0][7];

	shpBend[0][3] = (a6*N[0][5]-a5*N[0][4]) * three_over_two;
	shpBend[0][4] = b6*N[0][5] + b5*N[0][4];
	shpBend[0][5] = N[0][1] - c6*N[0][5] - c5*N[0][4];

	shpBend[0][6] = (a7*N[0][6]-a6*N[0][5]) * three_over_two;
	shpBend[0][7] = b7*N[0][6] + b6*N[0][5];
	shpBend[0][8] = N[0][2] - c7*N[0][6] - c6*N[0][5];

	shpBend[0][9] = (a8*N[0][7]-a7*N[0][6]) * three_over_two;
	shpBend[0][10] = b8*N[0][7] + b7*N[0][6];
	shpBend[0][11] = N[0][3] - c8*N[0][7] - c7*N[0][6];
	//Hy
	shpBend[1][0] = (d5*N[0][4]-d8*N[0][7])* three_over_two;
	shpBend[1][1] = -N[0][0] + e5*N[0][4] + e8*N[0][7];
	shpBend[1][2] = -b5*N[0][4] - b8*N[0][7];

	shpBend[1][3] = (d6*N[0][5]-d5*N[0][4])* three_over_two;
	shpBend[1][4] = -N[0][1] + e6*N[0][5] + e5*N[0][4];
	shpBend[1][5] = -b6*N[0][5] - b5*N[0][4];

	shpBend[1][6] = (d7*N[0][6]-d6*N[0][5])* three_over_two;
	shpBend[1][7] = -N[0][2] + e7*N[0][6] + e6*N[0][5];
	shpBend[1][8] = -b7*N[0][6] - b6*N[0][5];

	shpBend[1][9] = (d8*N[0][7]-d7*N[0][6])* three_over_two;
	shpBend[1][10] = -N[0][3] + e8*N[0][7] + e7*N[0][6];
	shpBend[1][11] = -b8*N[0][7] - b7*N[0][6];


	//Hx,xi  Hx,eta  Hy,xi  Hy,eta - temp[4][12]
	//Hx,xi
	temp[0][0] = (a5*N[1][4]-a8*N[1][7]) * three_over_two;
	temp[0][1] = b5*N[1][4] + b8*N[1][7];
	temp[0][2] = N[1][0] - c5*N[1][4] - c8*N[1][7];

	temp[0][3] = (a6*N[1][5]-a5*N[1][4]) * three_over_two;
	temp[0][4] = b6*N[1][5] + b5*N[1][4];
	temp[0][5] = N[1][1] - c6*N[1][5] - c5*N[1][4];

	temp[0][6] = (a7*N[1][6]-a6*N[1][5]) * three_over_two;
	temp[0][7] = b7*N[1][6] + b6*N[1][5];
	temp[0][8] = N[1][2] - c7*N[1][6] - c6*N[1][5];

	temp[0][9] = (a8*N[1][7]-a7*N[1][6]) * three_over_two;
	temp[0][10] = b8*N[1][7] + b7*N[1][6];
	temp[0][11] = N[1][3] - c8*N[1][7] - c7*N[1][6];
    //Hx,eta
	temp[1][0] = (a5*N[2][4]-a8*N[2][7]) * three_over_two;
	temp[1][1] = b5*N[2][4] + b8*N[2][7];
	temp[1][2] = N[2][0] - c5*N[2][4] - c8*N[2][7];

	temp[1][3] = (a6*N[2][5]-a5*N[2][4]) * three_over_two;
	temp[1][4] = b6*N[2][5] + b5*N[2][4];
	temp[1][5] = N[2][1] - c6*N[2][5] - c5*N[2][4];

	temp[1][6] = (a7*N[2][6]-a6*N[2][5]) * three_over_two;
	temp[1][7] = b7*N[2][6] + b6*N[2][5];
	temp[1][8] = N[2][2] - c7*N[2][6] - c6*N[2][5];

	temp[1][9] = (a8*N[2][7]-a7*N[2][6]) * three_over_two;
	temp[1][10] = b8*N[2][7] + b7*N[2][6];
	temp[1][11] = N[2][3] - c8*N[2][7] - c7*N[2][6];
	//Hy,xi
	temp[2][0] = (d5*N[1][4]-d8*N[1][7])* three_over_two;
	temp[2][1] = -N[1][0] + e5*N[1][4] + e8*N[1][7];
	temp[2][2] = -b5*N[1][4] - b8*N[1][7];

	temp[2][3] = (d6*N[1][5]-d5*N[1][4])* three_over_two;
	temp[2][4] = -N[1][1] + e6*N[1][5] + e5*N[1][4];
	temp[2][5] = -b6*N[1][5] - b5*N[1][4];

	temp[2][6] = (d7*N[1][6]-d6*N[1][5])* three_over_two;
	temp[2][7] = -N[1][2] + e7*N[1][6] + e6*N[1][5];
	temp[2][8] = -b7*N[1][6] - b6*N[1][5];

	temp[2][9] = (d8*N[1][7]-d7*N[1][6])* three_over_two;
	temp[2][10] = -N[1][3] + e8*N[1][7] + e7*N[1][6];
	temp[2][11] = -b8*N[1][7] - b7*N[1][6];


	//Hy,eta
	temp[3][0] = (d5*N[2][4]-d8*N[2][7])* three_over_two;
	temp[3][1] = -N[2][0] + e5*N[2][4] + e8*N[2][7];
	temp[3][2] = -b5*N[2][4] - b8*N[2][7];

	temp[3][3] = (d6*N[2][5]-d5*N[2][4])* three_over_two;
	temp[3][4] = -N[2][1] + e6*N[2][5] + e5*N[2][4];
	temp[3][5] = -b6*N[2][5] - b5*N[2][4];

	temp[3][6] = (d7*N[2][6]-d6*N[2][5])* three_over_two;
	temp[3][7] = -N[2][2] + e7*N[2][6] + e6*N[2][5];
	temp[3][8] = -b7*N[2][6] - b6*N[2][5];

	temp[3][9] = (d8*N[2][7]-d7*N[2][6])* three_over_two;
	temp[3][10] = -N[2][3] + e8*N[2][7] + e7*N[2][6];
	temp[3][11] = -b8*N[2][7] - b7*N[2][6];

	//Hx,x   Hx,y    Hy,x   Hy,y
	for (i=0;i<12;i++) {
		
		shpBend[2][i] = temp[0][i]*sx[0][0] +temp[1][i]*sx[1][0]; //Hx,x
		shpBend[3][i] = temp[0][i]*sx[0][1] +temp[1][i]*sx[1][1]; //Hx,y
		shpBend[4][i] = temp[2][i]*sx[0][0] +temp[3][i]*sx[1][0]; //Hy,x
		shpBend[5][i] = temp[2][i]*sx[0][1] +temp[3][i]*sx[1][1]; //Hy,y
	}//end for i
	return;
}

//**********************************************************************

int  ShellNLDKGQ::sendSelf (int commitTag, Channel &theChannel)
{
  int res = 0;

  // note: we don't check for dataTag == 0 for Element
  // objects as that is taken care of in a commit by the Domain
  // object - don't want to have to do the check if sending data
  int dataTag = this->getDbTag();
  

  // Now quad sends the ids of its materials
  int matDbTag;
  
  static ID idData(15);
  
  int i;
  for (i = 0; i < 4; i++) {
    idData(i) = materialPointers[i]->getClassTag();
    matDbTag = materialPointers[i]->getDbTag();
    // NOTE: we do have to ensure that the material has a database
    // tag if we are sending to a database channel.
    if (matDbTag == 0) {
      matDbTag = theChannel.getDbTag();
			if (matDbTag != 0)
			  materialPointers[i]->setDbTag(matDbTag);
    }
    idData(i+4) = matDbTag;
  }
  
  idData(8) = this->getTag();
  idData(9) = connectedExternalNodes(0);
  idData(10) = connectedExternalNodes(1);
  idData(11) = connectedExternalNodes(2);
  idData(12) = connectedExternalNodes(3);

  idData(13) = 0;
  idData(14) = 0;
  if (theDamping[0]) {
    idData(13) = theDamping[0]->getClassTag();
    int dbTag = theDamping[0]->getDbTag();
    if (dbTag == 0) {
      dbTag = theChannel.getDbTag();
      if (dbTag != 0)
        for (i = 0 ;  i < 4; i++)
	        theDamping[i]->setDbTag(dbTag);
	  }
    idData(14) = dbTag;
  }

  res += theChannel.sendID(dataTag, commitTag, idData);
  if (res < 0) {
    opserr << "WARNING ShellNLDKGQ::sendSelf() - " << this->getTag() << " failed to send ID\n";
    return res;
  }

  static Vector vectData(4);
  //vectData(0) = Ktt;
  vectData(0) = alphaM;
  vectData(1) = betaK;
  vectData(2) = betaK0;
  vectData(3) = betaKc;

  res += theChannel.sendVector(dataTag, commitTag, vectData);
  if (res < 0) {
    opserr << "WARNING ShellNLDKGQ::sendSelf() - " << this->getTag() << " failed to send ID\n";
    return res;
  }

  res += theChannel.sendVector(dataTag, commitTag, CstrainGauss);
  if (res < 0) {
    opserr << "WARNING ShellNLDKGT::sendSelf() - " << this->getTag() << " failed to send committed strains\n";
    return res;
  }
  
  // Finally, quad asks its material objects to send themselves
  for (i = 0; i < 4; i++) {
    res += materialPointers[i]->sendSelf(commitTag, theChannel);
    if (res < 0) {
      opserr << "WARNING ShellNLDKGQ::sendSelf() - " << this->getTag() << " failed to send its Material\n";
      return res;
    }
  }

  // Ask the Damping to send itself
  if (theDamping[0]) {
    for (int i = 0 ;  i < 4; i++) {
      res += theDamping[i]->sendSelf(commitTag, theChannel);
      if (res < 0) {
        opserr << "ShellNLDKGQ::sendSelf -- could not send Damping\n";
        return res;
      }
    }
  }

  return res;
}
    
int  ShellNLDKGQ::recvSelf (int commitTag, 
		       Channel &theChannel, 
		       FEM_ObjectBroker &theBroker)
{
  int res = 0;
  
  int dataTag = this->getDbTag();

  static ID idData(15);
  // Quad now receives the tags of its four external nodes
  res += theChannel.recvID(dataTag, commitTag, idData);
  if (res < 0) {
    opserr << "WARNING ShellNLDKGQ::recvSelf() - " << this->getTag() << " failed to receive ID\n";
    return res;
  }

  this->setTag(idData(8));
  connectedExternalNodes(0) = idData(9);
  connectedExternalNodes(1) = idData(10);
  connectedExternalNodes(2) = idData(11);
  connectedExternalNodes(3) = idData(12);

  static Vector vectData(4);
  res += theChannel.recvVector(dataTag, commitTag, vectData);
  if (res < 0) {
    opserr << "WARNING ShellNLDKGQ::sendSelf() - " << this->getTag() << " failed to send ID\n";
    return res;
  }

  //Ktt = vectData(0);
  alphaM = vectData(0);
  betaK = vectData(1);
  betaK0 = vectData(2);
  betaKc = vectData(3);

  res += theChannel.recvVector(dataTag, commitTag, CstrainGauss);
  if (res < 0) {
    opserr << "WARNING ShellNLDKGQ::sendSelf() - " << this->getTag() << " failed to send ID\n";
    return res;
  }
  TstrainGauss = CstrainGauss;
  
  int i;

  if (materialPointers[0] == 0) {
    for (i = 0; i < 4; i++) {
      int matClassTag = idData(i);
      int matDbTag = idData(i+4);
      // Allocate new material with the sent class tag
      materialPointers[i] = theBroker.getNewSection(matClassTag);
      if (materialPointers[i] == 0) {
	opserr << "ShellNLDKGQ::recvSelf() - Broker could not create NDMaterial of class type" << matClassTag << endln;;
	return -1;
      }
      // Now receive materials into the newly allocated space
      materialPointers[i]->setDbTag(matDbTag);
      res += materialPointers[i]->recvSelf(commitTag, theChannel, theBroker);
      if (res < 0) {
	opserr << "ShellNLDKGQ::recvSelf() - material " << i << "failed to recv itself\n";
	return res;
      }
    }
  }
  // Number of materials is the same, receive materials into current space
  else {
    for (i = 0; i < 4; i++) {
      int matClassTag = idData(i);
      int matDbTag = idData(i+4);
      // Check that material is of the right type; if not,
      // delete it and create a new one of the right type
      if (materialPointers[i]->getClassTag() != matClassTag) {
	delete materialPointers[i];
	materialPointers[i] = theBroker.getNewSection(matClassTag);
	if (materialPointers[i] == 0) {
	  opserr << "ShellNLDKGQ::recvSelf() - Broker could not create NDMaterial of class type" << matClassTag << endln;
	  exit(-1);
	}
      }
      // Receive the material
      materialPointers[i]->setDbTag(matDbTag);
      res += materialPointers[i]->recvSelf(commitTag, theChannel, theBroker);
      if (res < 0) {
	opserr << "ShellNLDKGQ::recvSelf() - material " << i << "failed to recv itself\n";
	return res;
      }
    }
  }
  
  int dmpTag = (int)idData(13);
  if (dmpTag) {
    for (i = 0 ;  i < 4; i++) {
      // Check if the Damping is null; if so, get a new one
      if (theDamping[i] == 0) {
        theDamping[i] = theBroker.getNewDamping(dmpTag);
        if (theDamping[i] == 0) {
          opserr << "ShellNLDKGQ::recvSelf -- could not get a Damping\n";
          exit(-1);
        }
      }
  
      // Check that the Damping is of the right type; if not, delete
      // the current one and get a new one of the right type
      if (theDamping[i]->getClassTag() != dmpTag) {
        delete theDamping[i];
        theDamping[i] = theBroker.getNewDamping(dmpTag);
        if (theDamping[i] == 0) {
          opserr << "ShellNLDKGQ::recvSelf -- could not get a Damping\n";
          exit(-1);
        }
      }
  
      // Now, receive the Damping
      theDamping[i]->setDbTag((int)idData(14));
      res += theDamping[i]->recvSelf(commitTag, theChannel, theBroker);
      if (res < 0) {
        opserr << "ShellNLDKGQ::recvSelf -- could not receive Damping\n";
        return res;
      }
    }
  }
  else {
    for (i = 0; i < 4; i++)
    {
      if (theDamping[i])
      {
        delete theDamping[i];
        theDamping[i] = 0;
      }
    }
  }
    
  return res;
}
//**************************************************************************

int
ShellNLDKGQ::displaySelf(Renderer &theViewer, int displayMode, float fact, const char **modes, int numMode)
{
	// get the end point display coords
	static Vector v1(3);
	static Vector v2(3);
	static Vector v3(3);
	static Vector v4(3);
	nodePointers[0]->getDisplayCrds(v1, fact, displayMode);
	nodePointers[1]->getDisplayCrds(v2, fact, displayMode);
	nodePointers[2]->getDisplayCrds(v3, fact, displayMode);
	nodePointers[3]->getDisplayCrds(v4, fact, displayMode);

	// place values in coords matrix
	static Matrix coords(4, 3);
	for (int i = 0; i < 3; i++) {
		coords(0, i) = v1(i);
		coords(1, i) = v2(i);
		coords(2, i) = v3(i);
		coords(3, i) = v4(i);
	}

	// Display mode is positive:
	// display mode = 0 -> plot no contour
	// display mode = 1-8 -> plot 1-8 stress resultant
	static Vector values(4);
	if (displayMode < 8 && displayMode > 0) {
		for (int i = 0; i < 4; i++) {
			const Vector& stress = materialPointers[i]->getStressResultant();
			values(i) = stress(displayMode - 1);
		}
	}
	else {
		for (int i = 0; i < 4; i++)
			values(i) = 0.0;
	}

	// draw the polygon
	return theViewer.drawPolygon(coords, values, this->getTag());
}
