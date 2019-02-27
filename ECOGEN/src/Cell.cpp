//  
//       ,---.     ,--,    .---.     ,--,    ,---.    .-. .-. 
//       | .-'   .' .')   / .-. )  .' .'     | .-'    |  \| | 
//       | `-.   |  |(_)  | | |(_) |  |  __  | `-.    |   | | 
//       | .-'   \  \     | | | |  \  \ ( _) | .-'    | |\  | 
//       |  `--.  \  `-.  \ `-' /   \  `-) ) |  `--.  | | |)| 
//       /( __.'   \____\  )---'    )\____/  /( __.'  /(  (_) 
//      (__)              (_)      (__)     (__)     (__)     
//
//  This file is part of ECOGEN.
//
//  ECOGEN is the legal property of its developers, whose names 
//  are listed in the copyright file included with this source 
//  distribution.
//
//  ECOGEN is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published 
//  by the Free Software Foundation, either version 3 of the License, 
//  or (at your option) any later version.
//  
//  ECOGEN is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//  
//  You should have received a copy of the GNU General Public License
//  along with ECOGEN (file LICENSE).  
//  If not, see <http://www.gnu.org/licenses/>.

//! \file      Cell.cpp
//! \author    F. Petitpas, K. Schmidmayer, S. Le Martelot
//! \version   1.0
//! \date      July 30 2018

#include "Cell.h"

//***********************************************************************

Cell::Cell() : m_vecPhases(0), m_mixture(0), m_cons(0), m_vecTransports(0), m_consTransports(0), m_childrenCells(0),m_element(0)
{
  m_lvl = 0;
  m_xi = 0.;
	m_split = false;
}

//***********************************************************************

Cell::Cell(int lvl) : m_vecPhases(0), m_mixture(0), m_cons(0), m_vecTransports(0), m_consTransports(0), m_childrenCells(0), m_element(0)
{
  m_lvl = lvl;
  m_xi = 0.;
	m_split = false;
}

//***********************************************************************

Cell::~Cell()
{
  for (int k = 0; k < m_numberPhases; k++) {
    delete m_vecPhases[k];
  }
  delete[] m_vecPhases;
  delete m_mixture;
  delete[] m_vecTransports;
  delete m_cons;
  delete[] m_consTransports;
  for (unsigned int qpa = 0; qpa < m_vecQuantitiesAddPhys.size(); qpa++) {
    delete m_vecQuantitiesAddPhys[qpa];
  }
  for (unsigned int i = 0; i < m_childrenInternalCellInterfaces.size(); i++) {
    m_childrenInternalCellInterfaces[i]->finalizeFace();
    delete m_childrenInternalCellInterfaces[i];
  }
  m_childrenInternalCellInterfaces.clear();
  for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
    delete m_childrenCells[i];
  }
  m_childrenCells.clear();
}

//***********************************************************************

void Cell::addCellInterface(CellInterface *cellInterface)
{
  m_cellInterfaces.push_back(cellInterface);
}

//***********************************************************************

void Cell::deleteCellInterface(CellInterface *cellInterface)
{
  for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
    if (m_cellInterfaces[b] == cellInterface) { m_cellInterfaces.erase(m_cellInterfaces.begin() + b); }
  }
}

//***********************************************************************

void Cell::allocate(const int &numberPhases, const int &numberTransports, const std::vector<AddPhys*> &addPhys, Model *model)
{
  m_numberPhases = numberPhases;
  m_numberTransports = numberTransports;
  m_vecPhases = new Phase*[numberPhases];
  for (int k = 0; k < numberPhases; k++) {
    model->allocatePhase(&m_vecPhases[k]);
  }
  model->allocateMixture(&m_mixture);
  model->allocateCons(&m_cons,numberPhases);
  if (numberTransports > 0) {
    m_vecTransports = new Transport[numberTransports];
    m_consTransports = new Transport[numberTransports];
  }
  for (unsigned int k = 0; k < addPhys.size(); k++) {
    addPhys[k]->addQuantityAddPhys(this);
  }
  m_model = model;
}

//***********************************************************************

void Cell::allocateEos(const int &numberPhases, Model *model)
{
	model->allocateEos(*this, numberPhases);
}

//***********************************************************************

void Cell::fill(std::vector<GeometricalDomain*> &domains, const int &lvlMax)
{
  Coord coordinates;
  coordinates = m_element->getPosition();
  for (unsigned int geom = 0; geom < domains.size(); geom++) {
      domains[geom]->fillIn(this, m_numberPhases, m_numberTransports);
  }

  //Initial smearing of the interface. Uncomment only when needed.
  // double radius;
  // double x_init(0.), y_init(0.), z_init(0.);
  // //radius = std::pow(std::pow(coordinates.getX() - x_init, 2.), 0.5); //1D
  // x_init = 3175.e-6;
  // radius = std::pow(std::pow(coordinates.getX() - x_init, 2.) + std::pow(coordinates.getY() - y_init, 2.), 0.5); //2D
  // //radius = std::pow(std::pow(coordinates.getX() - x_init, 2.) + std::pow(coordinates.getY() - y_init, 2.) + std::pow(coordinates.getZ() - z_init, 2.), 0.5); //3D
  // double alphaAir(0.), alphaEau(0.);
  // //double h(3200.e-6/50./std::pow(2., (double)lvlMax)); //0.04e-3
  // //double h(3.e-4/150./std::pow(2., (double)lvlMax));
  // double h(350.e-6/100./std::pow(2., (double)lvlMax));
  // alphaAir = 1. / 2.*(1. - tanh((radius - 100.e-6)/1.5/h));
  // if (alphaAir > 1.) alphaAir = 1.;
  // if (alphaAir < 0.) alphaAir = 0.;
  // alphaEau = 1. - alphaAir;
  // m_vecPhases[1]->setAlpha(alphaAir);
  // m_vecPhases[0]->setAlpha(alphaEau);
  // double pressure(alphaAir*3.55e3+alphaEau*50.6625e5);
  // m_vecPhases[1]->setPressure(pressure);
  // m_vecPhases[0]->setPressure(pressure);
  // m_mixture->setPressure(pressure);

  //Initial smearing for 1D cavitation test case. Uncomment only when needed.
  // double coordX;
  // coordX = std::pow(std::pow(coordinates.getX(), 2.), 0.5); //1D
  // double variation(0.);
  // double h(1./500./std::pow(2., (double)lvlMax));
  // variation = 1. / 2.*(1. - tanh((coordX - 0.5)/1.5/h));
  // double velocity(variation*(-100.)+(1.-variation)*100.);
  // m_mixture->setVelocity(velocity, 0., 0.);
}

//***********************************************************************

void Cell::allocateAndCopyPhase(const int &phaseNumber, Phase *phase)
{
  phase->allocateAndCopyPhase(&m_vecPhases[phaseNumber]);
}

//***********************************************************************

void Cell::copyPhase(const int &phaseNumber, Phase *phase)
{
  m_vecPhases[phaseNumber]->copyPhase(*phase);
}

//***********************************************************************

void Cell::copyMixture(Mixture *mixture)
{
  m_mixture->copyMixture(*mixture);
}

//***********************************************************************

void Cell::setToZeroCons(const int &numberPhases, const int &numberTransports)
{
  m_cons->setToZero(numberPhases);
  for (int k = 0; k < numberTransports; k++) {
    m_consTransports[k].setValue(0.);
  }
}

//***********************************************************************

void Cell::setToZeroConsGlobal(const int &numberPhases, const int &numberTransports)
{
  if (!m_split) {
    m_cons->setToZero(numberPhases);
    for (int k = 0; k < numberTransports; k++) {
      m_consTransports[k].setValue(0.);
    }
  }
  else {
    for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
      m_childrenCells[i]->setToZeroConsGlobal(numberPhases, numberTransports);
    }
  }
}

//***********************************************************************

void Cell::setToZeroBufferFlux(const int &numberPhases)
{
  m_cons->setToZeroBufferFlux(numberPhases);
}

//***********************************************************************

void Cell::timeEvolution(const double &dt, const int &numberPhases, const int &numberTransports, Symmetry *symmetry, Prim type)
{
  m_cons->setBufferFlux(*this, numberPhases);                //fluxTempXXX receive conservative variables at time n : Un
  symmetry->addSymmetricTerms(this, numberPhases, type);     //m_cons is incremented by the symmetric terms from primitive variables at time n
  m_cons->multiply(dt, numberPhases);                        //m_cons is multiplied by dt
  m_cons->addFlux(1., numberPhases);                         //Adding the buffer fluxTempXXX to obtain Un+1 in m_cons
  m_cons->schemeCorrection(this, numberPhases);              //Specific correction for non-conservative models

  //Same process for transport (Un construction not needed)
  for (int k = 0; k < numberTransports; k++) {
    m_consTransports[k].multiply(dt);
    m_vecTransports[k].add(m_consTransports[k].getValue());
  }
}

//***********************************************************************

void Cell::timeEvolutionAddPhys(const double &dt, const int &numberPhases, const int &numberTransports)
{
  m_cons->setBufferFlux(*this, numberPhases);                //fluxTempXXX receive conservative variables at time n : Un
  m_cons->multiply(dt, numberPhases);                        //m_cons is multiplied by dt
  m_cons->addFlux(1., numberPhases);                         //Adding the buffer fluxTempXXX to obtain Un+1 in m_cons
}

//***********************************************************************

void Cell::buildPrim(const int &numberPhases)
{
  m_cons->buildPrim(m_vecPhases, m_mixture, numberPhases); 
}

//***********************************************************************

void Cell::buildCons(const int &numberPhases)
{
  m_cons->buildCons(m_vecPhases, numberPhases, m_mixture);
}

//***********************************************************************

void Cell::correctionEnergy(const int &numberPhases)
{
  m_mixture->totalEnergyToInternalEnergy(m_vecQuantitiesAddPhys);    //Building specific internal energy form totale one
  m_cons->correctionEnergy(this, numberPhases);                      //Pressure correction
}

//***********************************************************************

void Cell::printPhasesMixture(const int &numberPhases, const int &numberTransports, std::ofstream &fileStream) const
{
  for (int k = 0; k < numberPhases; k++) { m_vecPhases[k]->printPhase(fileStream); }
  m_mixture->printMixture(fileStream);
  for (int k = 0; k < numberTransports; k++) { fileStream << m_vecTransports[k].getValue() << " "; }
}

//***********************************************************************

void Cell::completeFulfillState(Prim type)
{
  //Complete thermodynamical variables
  m_model->fulfillState(m_vecPhases, m_mixture, m_numberPhases, type);
  //Extended energies depending on additional physics
  this->prepareAddPhys();
  m_mixture->internalEnergyToTotalEnergy(m_vecQuantitiesAddPhys);
}

//***********************************************************************

void Cell::fulfillState(Prim type)
{
  //Complete thermodynamical variables
  m_model->fulfillState(m_vecPhases, m_mixture, m_numberPhases, type);
  //This routine is used in different configurations and a short note correspond to each one:
  //- Riemann solver: No need to reconstruct the total energy there because it isn't grabbed during the Riemann problem. The total energy is directly reconstruct there.
  //The reason is to avoid calculations on the gradients of additional physics which are not necessary and furthermore wrongly computed.
  //Note that the capillary energy is not required during the Riemann problem because the models are splitted.
  //- Parallel: No need to reconstruct the total energy there because it is already communicated.
  //Note that the gradients of additional physics would also be wrongly computed if done in the ghost cells.
  //- Relaxation or correction: The total energy doesn't have to be updated there.
}

//***********************************************************************

void Cell::localProjection(const Coord &normal, const Coord &tangent, const Coord &binormal, const int &numberPhases, Prim type)
{
  for (int k = 0; k < numberPhases; k++) {
    m_vecPhases[k]->localProjection(normal, tangent, binormal);
  }
  m_mixture->localProjection(normal, tangent, binormal);
}

//***********************************************************************

void Cell::reverseProjection(const Coord &normal, const Coord &tangent, const Coord &binormal, const int &numberPhases, Prim type)
{
  for (int k = 0; k < numberPhases; k++) {
    m_vecPhases[k]->reverseProjection(normal, tangent, binormal);
  }
  m_mixture->reverseProjection(normal, tangent, binormal);
}

//***********************************************************************

void Cell::copyVec(Phase **vecPhases, Mixture *mixture, Transport *vecTransports)
{
  for (int k = 0; k < m_numberPhases; k++) {
    m_vecPhases[k]->copyPhase(*vecPhases[k]);
  }
  m_mixture->copyMixture(*mixture);
  for (int k = 0; k < m_numberTransports; k++) {
    m_vecTransports[k] = vecTransports[k];
  }
}

//***********************************************************************

//void Cell::printCut1Dde2D(std::ofstream &fileStream, std::string variableConstanteCut, const double &valueCut, const double &dL)
//{
//  if (m_childrenCells.size() == 0) {
//    bool imprX(false), imprY(false), imprZ(false);
//    double dLsur2, position, epsilon;
//    dLsur2 = dL / std::pow(2., (double)m_lvl) / 2.;
//    epsilon = 1.e-3*dLsur2;
//    if (variableConstanteCut == "x") {
//      imprY = true;
//      position = m_element->getPosition().getX();
//    }
//    else if (variableConstanteCut == "y") {
//      imprX = true;
//      position = m_element->getPosition().getY();
//    }
//
//    //imprX = true;
//    //imprY = false;
//    //if (std::fabs(m_element->getPosition().getX() - m_element->getPosition().getY()) < 1.e-6) {
//    if (std::fabs(position - valueCut - epsilon) < dLsur2) {
//      m_element->ecritPos(fileStream, imprX, imprY, imprZ);
//      this->printPhasesMixture(m_numberPhases, m_numberTransports, fileStream);
//      fileStream << m_lvl << " " << m_xi << " ";
//      fileStream << endl;
//    }
//  }
//  else {
//    for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
//      m_childrenCells[i]->printCut1Dde2D(fileStream, variableConstanteCut, valueCut, dL);
//    }
//  }
//}
//
////***********************************************************************
//
//void Cell::printCut1Dde3D(std::ofstream &fileStream, std::string variableConstanteCut1, std::string variableConstanteCut2,
//  const double &valueCut1, const double &valueCut2, const double &dL1, const double &dL2)
//{
//  if (m_childrenCells.size() == 0) {
//    bool imprX(true), imprY(true), imprZ(true);
//    double dL1sur2, dL2sur2, position1, position2, epsilon1, epsilon2;
//    dL1sur2 = dL1 / std::pow(2., (double)m_lvl) / 2.;
//    dL2sur2 = dL2 / std::pow(2., (double)m_lvl) / 2.;
//    epsilon1 = 1.e-3*dL1sur2;
//    epsilon2 = 1.e-3*dL2sur2;
//
//    if (variableConstanteCut1 == "x") {
//      imprX = false;
//      position1 = m_element->getPosition().getX();
//    }
//    else if (variableConstanteCut1 == "y") {
//      imprY = false;
//      position1 = m_element->getPosition().getY();
//    }
//    else {
//      imprZ = false;
//      position1 = m_element->getPosition().getZ();
//    }
//
//    if (variableConstanteCut2 == "x") {
//      imprX = false;
//      position2 = m_element->getPosition().getX();
//    }
//    else if (variableConstanteCut2 == "y") {
//      imprY = false;
//      position2 = m_element->getPosition().getY();
//    }
//    else {
//      imprZ = false;
//      position2 = m_element->getPosition().getZ();
//    }
//
//    if ((std::fabs(position1 - valueCut1 - epsilon1) < dL1sur2) && (std::fabs(position2 - valueCut2 - epsilon2) < dL2sur2)) {
//      m_element->ecritPos(fileStream, imprX, imprY, imprZ);
//      this->printPhasesMixture(m_numberPhases, m_numberTransports, fileStream);
//      fileStream << m_lvl << " " << m_xi << " ";
//      fileStream << endl;
//    }
//  }
//  else {
//    for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
//      m_childrenCells[i]->printCut1Dde3D(fileStream, variableConstanteCut1, variableConstanteCut2, valueCut1, valueCut2, dL1, dL2);
//    }
//  }
//}

//****************************************************************************
//***************************Additional Physics*******************************
//****************************************************************************

void Cell::prepareAddPhys()
{
  for (unsigned int qpa = 0; qpa < m_vecQuantitiesAddPhys.size(); qpa++) {
    m_vecQuantitiesAddPhys[qpa]->computeQuantities(this);
  }
}

//***********************************************************************

double Cell::selectScalar(std::string nameVariable, int num) const
{
  //Selection scalar
  if (nameVariable == "TR") {
    return m_vecTransports[num].getValue();
    //double psi(0.), coeff(0.75);
    //psi = std::pow(m_vecTransports[num].getValue(), coeff) / (std::pow(m_vecTransports[num].getValue(), coeff) + std::pow((1 - m_vecTransports[num].getValue()), coeff));
    //return psi;
  }
  else if (nameVariable == "P") {
    if (m_numberPhases > 1) {
      return m_mixture->getPressure();
    }
    else {
      return m_vecPhases[num]->getPressure();
    }
  }
  else if (nameVariable == "RHO") {
    if (m_numberPhases > 1) {
      return m_mixture->getDensity();
    }
    else {
      return m_vecPhases[num]->getDensity();
    }
  }
  else if (nameVariable == "ALPHA") {
    if (m_numberPhases > 1) {
      return m_vecPhases[num]->getAlpha();
    }
    else { return 1.; }
  }
  else if (nameVariable == "u") {
    if (m_numberPhases > 1) {
      return m_mixture->getVelocity().getX();
    }
    else {
      return m_vecPhases[num]->getU();
    }
  }
  else if (nameVariable == "v") {
    if (m_numberPhases > 1) {
      return m_mixture->getVelocity().getY();
    }
    else {
      return m_vecPhases[num]->getV();
    }
  }
  else if (nameVariable == "w") {
    if (m_numberPhases > 1) {
      return m_mixture->getVelocity().getZ();
    }
    else {
      return m_vecPhases[num]->getW();
    }
  }
  else if (nameVariable == "T") {
    return m_vecPhases[num]->getTemperature();
  }
  //FP//TODO// faire QPA et Phases
  else { Errors::errorMessage("nameVariable unknown in selectScalar (linked to QuantitiesAddPhys)"); return 0; }
}

//***********************************************************************

void Cell::setScalar(std::string nameVariable, const double &value, int num, int subscript)
{
  //Selection scalar
  if (nameVariable == "TR") { //transport
    m_vecTransports[num].setValue(value);
  }
  //FP//TODO// faire QPA et Phases
  else { Errors::errorMessage("nameVariable unknown in setScalar (linked to QuantitiesAddPhys)"); }
}

//***********************************************************************

Coord Cell::selectVector(std::string nameVector, int num, int subscript) const
{
  //Selection vector
  if (nameVector == "QPA") { //additional physics
    return m_vecQuantitiesAddPhys[num]->getGrad(subscript);
  }
  //FP//TODO// faire vecteur pour les Phases
  else { Errors::errorMessage("nameVector unknown in selectVector (linked to QuantitiesAddPhys)"); return 0; }
}

//***********************************************************************

void Cell::setVector(std::string nameVector, const Coord &value, int num, int subscript)
{
  //Selection vector
  if (nameVector == "QPA") { //additional physics
    m_vecQuantitiesAddPhys[num]->setGrad(value, subscript);
  }
  //FP//TODO// faire vecteur pour les Phases
  else { Errors::errorMessage("nameVector unknown in setVector (linked to QuantitiesAddPhys)"); }
}

//***********************************************************************

Coord Cell::computeGradient(std::string nameVariable, int numPhase)
{
  int typeCellInterface(0);
  double sommeDistanceX = 0.;
  double sommeDistanceY = 0.;
  double sommeDistanceZ = 0.;
  Coord grad(0.);               /*!< gradient vector for needed variable on the cell*/
  Coord gradProjectedFace(0.);  /*!< gradient vector for the needed variable on a cell interface in the absolute system of coordinate*/             
  double gradCellInterface(0.);          /*!< gradient for needed variable on the cell interface in the face direction*/
 
  for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
    if (!m_cellInterfaces[b]->getSplit()) {
      typeCellInterface = m_cellInterfaces[b]->whoAmI();
      if (typeCellInterface == 0) //Cell interface type CellInterface/O2
      {
        // Extracting left and right variables values for each cell interface
        // and calculus of the gradient normal to the face
        double cg = m_cellInterfaces[b]->getCellGauche()->selectScalar(nameVariable, numPhase);
        double cd = m_cellInterfaces[b]->getCellDroite()->selectScalar(nameVariable, numPhase);

        double distance(m_cellInterfaces[b]->getCellGauche()->distance(m_cellInterfaces[b]->getCellDroite()));
        gradCellInterface = (cd - cg) / distance;

        // Projection in the absolute system of coordinate
        gradProjectedFace.setX(m_cellInterfaces[b]->getFace()->getNormal().getX()*gradCellInterface);
        gradProjectedFace.setY(m_cellInterfaces[b]->getFace()->getNormal().getY()*gradCellInterface);
        gradProjectedFace.setZ(m_cellInterfaces[b]->getFace()->getNormal().getZ()*gradCellInterface);

        // Summ for each cell interface with ponderation using distance in each direction
        // then the cell gradient is normalized by summ of distances.
        double distanceX(m_cellInterfaces[b]->getCellGauche()->distanceX(m_cellInterfaces[b]->getCellDroite()));
        double distanceY(m_cellInterfaces[b]->getCellGauche()->distanceY(m_cellInterfaces[b]->getCellDroite()));
        double distanceZ(m_cellInterfaces[b]->getCellGauche()->distanceZ(m_cellInterfaces[b]->getCellDroite()));
        distanceX = std::fabs(distanceX);
        distanceY = std::fabs(distanceY);
        distanceZ = std::fabs(distanceZ);

        gradProjectedFace.setXYZ(gradProjectedFace.getX()*distanceX, gradProjectedFace.getY()*distanceY, gradProjectedFace.getZ()*distanceZ);

        sommeDistanceX += distanceX;
        sommeDistanceY += distanceY;
        sommeDistanceZ += distanceZ;

        grad += gradProjectedFace;
      }
      else if (typeCellInterface == 1) { //Cell interface of type Abs
        double distanceX(this->distanceX(m_cellInterfaces[b]));
        double distanceY(this->distanceY(m_cellInterfaces[b]));
        double distanceZ(this->distanceZ(m_cellInterfaces[b]));
        distanceX = std::fabs(distanceX)*2.;
        distanceY = std::fabs(distanceY)*2.;
        distanceZ = std::fabs(distanceZ)*2.;
        sommeDistanceX += distanceX;
        sommeDistanceY += distanceY;
        sommeDistanceZ += distanceZ;
      }
      else if (typeCellInterface == 6) { //Cell Interface of type Symetrie
        if (nameVariable == "u" || nameVariable == "v" || nameVariable == "w") {
          // Extracting left variables values
          // and calculus of the gradient normal to the face
          double cg = m_cellInterfaces[b]->getCellGauche()->selectScalar(nameVariable, numPhase);

          double distance(this->distance(m_cellInterfaces[b]));
          gradCellInterface = cg / distance;

          // Multiplication of the gradient by the normal direction to guarantee symmetry
          if (nameVariable == "u") { gradCellInterface = gradCellInterface* m_cellInterfaces[b]->getFace()->getNormal().getX(); }
          if (nameVariable == "v") { gradCellInterface = gradCellInterface* m_cellInterfaces[b]->getFace()->getNormal().getY(); }
          if (nameVariable == "w") { gradCellInterface = gradCellInterface* m_cellInterfaces[b]->getFace()->getNormal().getZ(); }

          // Projection in the absolute system of coordinate
          gradProjectedFace.setX(m_cellInterfaces[b]->getFace()->getNormal().getX()*gradCellInterface);
          gradProjectedFace.setY(m_cellInterfaces[b]->getFace()->getNormal().getY()*gradCellInterface);
          gradProjectedFace.setZ(m_cellInterfaces[b]->getFace()->getNormal().getZ()*gradCellInterface);

          // Summ for each cell interface with ponderation using distance in each direction
          // then the cell gradient is normalized by summ of distances.
          double distanceX(this->distanceX(m_cellInterfaces[b]));
          double distanceY(this->distanceY(m_cellInterfaces[b]));
          double distanceZ(this->distanceZ(m_cellInterfaces[b]));
          distanceX = std::fabs(distanceX)*2.;
          distanceY = std::fabs(distanceY)*2.;
          distanceZ = std::fabs(distanceZ)*2.;

          gradProjectedFace.setXYZ(gradProjectedFace.getX()*distanceX, gradProjectedFace.getY()*distanceY, gradProjectedFace.getZ()*distanceZ);

          sommeDistanceX += distanceX;
          sommeDistanceY += distanceY;
          sommeDistanceZ += distanceZ;

          grad += gradProjectedFace;
        }
        else {
          double distanceX(this->distanceX(m_cellInterfaces[b]));
          double distanceY(this->distanceY(m_cellInterfaces[b]));
          double distanceZ(this->distanceZ(m_cellInterfaces[b]));
          distanceX = std::fabs(distanceX)*2.;
          distanceY = std::fabs(distanceY)*2.;
          distanceZ = std::fabs(distanceZ)*2.;
          sommeDistanceX += distanceX;
          sommeDistanceY += distanceY;
          sommeDistanceZ += distanceZ;
        }
      }
      else if (typeCellInterface == 2) { //Cell interface of type Wall
        if (nameVariable == "u" || nameVariable == "v" || nameVariable == "w") {
          // Extracting left variables values
          // and calculus of the gradient normal to the face
          double cg = m_cellInterfaces[b]->getCellGauche()->selectScalar(nameVariable, numPhase);

          double distance(this->distance(m_cellInterfaces[b]));
          gradCellInterface = cg / distance;

          // Projection in the absolute system of coordinate
          gradProjectedFace.setX(m_cellInterfaces[b]->getFace()->getNormal().getX()*gradCellInterface);
          gradProjectedFace.setY(m_cellInterfaces[b]->getFace()->getNormal().getY()*gradCellInterface);
          gradProjectedFace.setZ(m_cellInterfaces[b]->getFace()->getNormal().getZ()*gradCellInterface);

          // Summ for each cell interface with ponderation using distance in each direction
          // then the cell gradient is normalized by summ of distances.
          double distanceX(this->distanceX(m_cellInterfaces[b]));
          double distanceY(this->distanceY(m_cellInterfaces[b]));
          double distanceZ(this->distanceZ(m_cellInterfaces[b]));
          distanceX = std::fabs(distanceX)*2.;
          distanceY = std::fabs(distanceY)*2.;
          distanceZ = std::fabs(distanceZ)*2.;

          gradProjectedFace.setXYZ(gradProjectedFace.getX()*distanceX, gradProjectedFace.getY()*distanceY, gradProjectedFace.getZ()*distanceZ);

          sommeDistanceX += distanceX;
          sommeDistanceY += distanceY;
          sommeDistanceZ += distanceZ;

          grad += gradProjectedFace;
        }
        else {
          double distanceX(this->distanceX(m_cellInterfaces[b]));
          double distanceY(this->distanceY(m_cellInterfaces[b]));
          double distanceZ(this->distanceZ(m_cellInterfaces[b]));
          distanceX = std::fabs(distanceX)*2.;
          distanceY = std::fabs(distanceY)*2.;
          distanceZ = std::fabs(distanceZ)*2.;
          sommeDistanceX += distanceX;
          sommeDistanceY += distanceY;
          sommeDistanceZ += distanceZ;
        }
      }
    }
  }

  // Verifications in multiD
  if (sommeDistanceX <= 1.e-12) { sommeDistanceX = 1.; }
  if (sommeDistanceY <= 1.e-12) { sommeDistanceY = 1.; }
  if (sommeDistanceZ <= 1.e-12) { sommeDistanceZ = 1.; }

  // Final normalized gradient on the cell
  grad.setXYZ(grad.getX() / sommeDistanceX, grad.getY() / sommeDistanceY, grad.getZ() / sommeDistanceZ);

  return grad;
}

//***********************************************************************

QuantitiesAddPhys* Cell::getQPA(int &numGPA) const
{
  return m_vecQuantitiesAddPhys[numGPA];
}

//***********************************************************************

Coord Cell::getGradTk(int &numPhase, int &numAddPhys) const
{
  return m_vecQuantitiesAddPhys[numAddPhys]->getGradTk(numPhase);
}

//***********************************************************************

void Cell::setGradTk(int &numPhase, int &numAddPhys, double *buffer, int &counter)
{

  Coord grad(0.);
  grad.setX(buffer[++counter]);
  grad.setY(buffer[++counter]);
  grad.setZ(buffer[++counter]);
  m_vecQuantitiesAddPhys[numAddPhys]->setGradTk(numPhase, grad);
}

//***********************************************************************

void Cell::addNonConsAddPhys(const int &numberPhases, AddPhys &addPhys, Symmetry *symmetry)
{
  addPhys.addNonConsAddPhys(this, numberPhases);
  symmetry->addSymmetricTermsAddPhys(this, numberPhases, addPhys);
}

//***********************************************************************

void Cell::reinitializeColorFunction(const int &numTransport, const int &numPhase)
{
	m_vecTransports[numTransport].setValue(m_vecPhases[numPhase]->getAlpha());
}

//****************************************************************************
//*****************************Accessors**************************************
//****************************************************************************

int Cell::getCellInterfacesSize() const
{
  return m_cellInterfaces.size();
}

//***********************************************************************

CellInterface* Cell::getCellInterface(int &b)
{
  return m_cellInterfaces[b];
}

//***********************************************************************

Phase* Cell::getPhase(const int &phaseNumber, Prim type) const
{
  return m_vecPhases[phaseNumber];
}

//***********************************************************************

Phase** Cell::getPhases(Prim type) const
{
  return m_vecPhases;
}

//***********************************************************************

Mixture* Cell::getMixture(Prim type) const
{
  return m_mixture;
}

//***********************************************************************

Flux* Cell::getCons() const
{
  return m_cons;
}

//***********************************************************************

void Cell::setCons(Flux *cons)
{
  m_cons->setCons(cons, m_numberPhases);
}

//***********************************************************************

Coord Cell::getPosition() const
{
  if (m_element != 0) { return m_element->getPosition(); }
  return 0.;
}

//***********************************************************************

Coord Cell::getSize() const
{
  return m_element->getSize();
}

//***********************************************************************

double Cell::getSizeX() const
{
  return m_element->getSizeX();
}

//***********************************************************************

double Cell::getSizeY() const
{
  return m_element->getSizeY();
}

//***********************************************************************

double Cell::getSizeZ() const
{
  return m_element->getSizeZ();
}

//***********************************************************************

void Cell::setElement(Element *element, const int &numCell)
{
  m_element = element;
  m_element->setCellAssociee(numCell);
}

//***********************************************************************

Element* Cell::getElement()
{
  return m_element;
}

//***********************************************************************

void Cell::setTransport(double value, int &numTransport, Prim type)
{
  m_vecTransports[numTransport].setValue(value);
}

//***********************************************************************

Transport& Cell::getTransport(const int &numTransport, Prim type) const
{
	return m_vecTransports[numTransport];
}

//***********************************************************************

Transport* Cell::getTransports(Prim type) const
{
	return m_vecTransports;
}

//***********************************************************************

Transport* Cell::getConsTransport(const int &numTransport) const
{
  return &m_consTransports[numTransport];
}

//***********************************************************************

void Cell::setConsTransport(double value, const int &numTransport)
{
  m_consTransports[numTransport].setValue(value);
}

//***********************************************************************

int Cell::getNumberPhases() const
{
  return m_numberPhases;
}

//***********************************************************************

int Cell::getNumberTransports() const
{
  return m_numberTransports;
}

//***********************************************************************

double Cell::getGradient()
{
  std::string nameVariable = "RHO";
  Coord grad(0.);
  int var = 0; //only for single phase
  grad = this->computeGradient(nameVariable, var);
  return grad.norm();
}

//***********************************************************************

Model* Cell::getModel()
{
  return m_model;
}

//***********************************************************************

Coord Cell::getVelocity()
{
  return m_model->getVelocity(this);
}

//***********************************************************************

std::vector<QuantitiesAddPhys*>& Cell::getVecQuantitiesAddPhys()
{
  return m_vecQuantitiesAddPhys;
}

//***********************************************************************

void Cell::printInfo() const
{
  m_element->printInfo();
}

//****************************************************************************
//******************************Distances*************************************
//****************************************************************************

double Cell::distance(Cell *c)
{
  return m_element->distance(c->getElement());
}

//***********************************************************************

double Cell::distanceX(Cell *c)
{
	return m_element->distanceX(c->getElement());
}

//***********************************************************************

double Cell::distanceY(Cell *c)
{
	return m_element->distanceY(c->getElement());
}

//***********************************************************************

double Cell::distanceZ(Cell *c)
{
	return m_element->distanceZ(c->getElement());
}

//***********************************************************************

double Cell::distance(CellInterface *b)
{
  return m_element->distance(b->getFace());
}

//***********************************************************************

double Cell::distanceX(CellInterface *b)
{
  return m_element->distanceX(b->getFace());
}

//***********************************************************************

double Cell::distanceY(CellInterface *b)
{
  return m_element->distanceY(b->getFace());
}

//***********************************************************************

double Cell::distanceZ(CellInterface *b)
{
  return m_element->distanceZ(b->getFace());
}

//***********************************************************************

bool Cell::traverseObjet(const GeometricObject &objet) const
{
  return m_element->traverseObjet(objet);
}

//****************************************************************************
//*****************************      AMR    **********************************
//****************************************************************************

void Cell::setToZeroXi()
{
  m_xi = 0.;
}

//***********************************************************************

void Cell::setToZeroConsXi()
{
  m_consXi = 0.;
}

//***********************************************************************

void Cell::timeEvolutionXi()
{
  m_xi += m_consXi;
}

//***********************************************************************

void Cell::chooseRefine(const double &xiSplit, const int &nbCellsY, const int &nbCellsZ,
  const std::vector<AddPhys*> &addPhys, Model *model, int &nbCellsTotalAMR)
{
  if (!m_split) {
    if (m_xi >= xiSplit) {
      if (!this->lvlNeighborTooLow()) {
        this->refineCellAndCellInterfaces(nbCellsY, nbCellsZ, addPhys, model);
        nbCellsTotalAMR += m_childrenCells.size() - 1;
      }
    }
  }
}

//***********************************************************************

void Cell::chooseUnrefine(const double &xiJoin, int &nbCellsTotalAMR)
{
  if (m_split) {
    bool deraffineGlobal(false);
    if (m_xi < xiJoin) {
      deraffineGlobal = true;
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //if one of my child possesses children, then unrefinement impossible
        if (m_childrenCells[i]->getNumberCellsChildren() > 0) { deraffineGlobal = false; }
      }
      //if one of my neighboor s level is too high, then unrefinement impossible
      if (deraffineGlobal) { if (this->lvlNeighborTooHigh()) { deraffineGlobal = false; }; }
    }
    //if all of my children can be unrefined, then unrefinement
    if (deraffineGlobal) {
      nbCellsTotalAMR -= m_childrenCells.size() - 1;
      this->unrefineCellAndCellInterfaces();
    }
  }
}

//***********************************************************************

void Cell::refineCellAndCellInterfaces(const int &nbCellsY, const int &nbCellsZ, const std::vector<AddPhys*> &addPhys, Model *model)
{
	m_split = true;

  //--------------------------------------
  //Initializations (children and dimension)
  //--------------------------------------

  double dimX(1.), dimY(0.), dimZ(0.);
  int numberCellsChildren(2);
  int dim(1);
  if (nbCellsZ != 1) {
    numberCellsChildren = 8;
    dimY = 1.;
    dimZ = 1.;
    dim = 3;
  }
  else if (nbCellsY != 1) {
    numberCellsChildren = 4;
    dimY = 1.;
    dim = 2;
  }
  CellInterface* cellInterfaceRef(0);
  for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
    if (m_cellInterfaces[b]->whoAmI() == 0) { cellInterfaceRef = m_cellInterfaces[b]; break; } //Cell interface type CellInterface/O2
  }
  int allocateSlopeLocal = 1;
  
  //----------------
  //Cells refinement 
  //----------------

  //Mesh data initialization for children cells
  //-------------------------------------------
  double posXCellParent, posYCellParent, posZCellParent;
  double dXParent, dYParent, dZParent;
  double posXChild, posYChild, posZChild;
  posXCellParent = m_element->getPosition().getX();
  posYCellParent = m_element->getPosition().getY();
  posZCellParent = m_element->getPosition().getZ();
  dXParent = m_element->getSizeX();
  dYParent = m_element->getSizeY();
  dZParent = m_element->getSizeZ();
  double volumeCellParent, lCFLCellParent;
  volumeCellParent = m_element->getVolume();
  lCFLCellParent = m_element->getLCFL();

  for (int i = 0; i < numberCellsChildren; i++) {


    //Children cells creation
    //-----------------------
    this->createChildCell(i, m_lvl);
    m_element->creerElementChild();
    m_childrenCells[i]->setElement(m_element->getElementChild(i), i);
    m_childrenCells[i]->getElement()->setVolume(volumeCellParent / (double)numberCellsChildren);
    m_childrenCells[i]->getElement()->setLCFL(0.5*lCFLCellParent);
    m_childrenCells[i]->getElement()->setSize((1-dimX*0.5)*getSizeX(), (1 - dimY*0.5)*getSizeY(), (1 - dimZ*0.5)*getSizeZ());
    posXChild = posXCellParent + dimX*dXParent*(double)(-0.25 + 0.5 * (i % 2));
    posYChild = posYCellParent + dimY*dYParent*(double)(-0.25 + 0.5 * ((i / 2) % 2));
    posZChild = posZCellParent + dimZ*dZParent*(double)(-0.25 + 0.5 * ((i / 4) % 2));
    m_childrenCells[i]->getElement()->setPos(posXChild, posYChild, posZChild);

    //Set the key for the child
    //-----------------------
    const auto child_key_0= this->getElement()->getKey().child(0);
    auto coord_i= child_key_0.coordinate();
    coord_i.x()+=(i % 2);
    coord_i.y()+=((i / 2) % 2);
    coord_i.z()+=((i / 4) % 2);
    const decomposition::Key<3> child_key( coord_i,child_key_0.level());
    m_childrenCells[i]->getElement()->setKey(child_key);


    //Initialization of main arrays according to model and number of phases
    //+ physical initialization: physical data for children cells
    //---------------------------------------------------------------------
    m_childrenCells[i]->allocate(m_numberPhases, m_numberTransports, addPhys, model);
    for (int k = 0; k < m_numberPhases; k++) {
      m_childrenCells[i]->copyPhase(k, m_vecPhases[k]);
    }
    m_childrenCells[i]->copyMixture(m_mixture);
    m_childrenCells[i]->getCons()->setToZero(m_numberPhases);
    for (int k = 0; k < m_numberTransports; k++) { m_childrenCells[i]->setTransport(m_vecTransports[k].getValue(), k); }
    for (int k = 0; k < m_numberTransports; k++) { m_childrenCells[i]->setConsTransport(0., k); }
    m_childrenCells[i]->setXi(m_xi);
  }

  //-----------------------------------
  //Internal cell-interfaces refinement
  //-----------------------------------

  if (nbCellsZ == 1) {
    if (nbCellsY == 1) {

      //Case 1D
      //-------

      // |-------------|-------------| X
      // 1      0      0      1      2

      //Internal cell interface child number 0 (face on X)
      //--------------------------------------------------
      cellInterfaceRef->creerCellInterfaceChildInterne(m_lvl, &m_childrenInternalCellInterfaces);
      m_childrenInternalCellInterfaces[0]->creerFaceChild(cellInterfaceRef);
      m_childrenInternalCellInterfaces[0]->getFace()->setNormal(1., 0., 0.);
      m_childrenInternalCellInterfaces[0]->getFace()->setTangent(0., 1., 0.);
      m_childrenInternalCellInterfaces[0]->getFace()->setBinormal(0., 0., 1.);
      m_childrenInternalCellInterfaces[0]->getFace()->setPos(posXCellParent, posYCellParent, posZCellParent);
      m_childrenInternalCellInterfaces[0]->getFace()->setSize(0., m_element->getSizeY(), m_element->getSizeZ());
      m_childrenInternalCellInterfaces[0]->getFace()->setSurface(m_element->getSizeY()*m_element->getSizeZ());
      m_childrenInternalCellInterfaces[0]->initializeGauche(m_childrenCells[0]);
      m_childrenInternalCellInterfaces[0]->initializeDroite(m_childrenCells[1]);
      m_childrenCells[0]->addCellInterface(m_childrenInternalCellInterfaces[0]);
      m_childrenCells[1]->addCellInterface(m_childrenInternalCellInterfaces[0]);

      //Attribution model and slopes
      //----------------------------
      m_childrenInternalCellInterfaces[0]->associeModel(model);
      m_childrenInternalCellInterfaces[0]->allocateSlopes(m_numberPhases, m_numberTransports, allocateSlopeLocal);
    }
    else {

      //Case 2D
      //-------
      // Y
      // |--------------|--------------|
      // |              |              |
      // |              |              |
      // |      2      1|      3       |
      // |              |              |
      // |              |              |
      // |--------------|--------------|
      // |      2       |      3       |
      // |              |              |
      // |      0      0|      1       |
      // |              |              |
      // |              |              |
      // |--------------|--------------|X

      for (int i = 0; i < 4; i++) {
        cellInterfaceRef->creerCellInterfaceChildInterne(m_lvl, &m_childrenInternalCellInterfaces);
        m_childrenInternalCellInterfaces[i]->creerFaceChild(cellInterfaceRef);
        if (i < 2) {
          //Internal cell interface 0 and 1 (face on X)
          //-------------------------------------------
          m_childrenInternalCellInterfaces[i]->getFace()->setNormal(1., 0., 0.);
          m_childrenInternalCellInterfaces[i]->getFace()->setTangent(0., 1., 0.);
          m_childrenInternalCellInterfaces[i]->getFace()->setBinormal(0., 0., 1.);
          m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent, posYCellParent + dYParent*(-0.25 + 0.5 * (double)i), posZCellParent);
          m_childrenInternalCellInterfaces[i]->getFace()->setSize(0., 0.5*m_element->getSizeY(), m_element->getSizeZ());
          m_childrenInternalCellInterfaces[i]->getFace()->setSurface(0.5*m_element->getSizeY()*m_element->getSizeZ());
          m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[2 * i]);
          m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[1 + 2 * i]);
          m_childrenCells[2 * i]->addCellInterface(m_childrenInternalCellInterfaces[i]);
          m_childrenCells[1 + 2 * i]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        }
        else {
          //Internal cell interface 2 and 3 (face on Y)
          //-------------------------------------------
          m_childrenInternalCellInterfaces[i]->getFace()->setNormal(0., 1., 0.);
          m_childrenInternalCellInterfaces[i]->getFace()->setTangent(-1., 0., 0.);
          m_childrenInternalCellInterfaces[i]->getFace()->setBinormal(0., 0., 1.);
          m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent + dXParent*(-0.25 + 0.5 * (double)(i % 2)), posYCellParent, posZCellParent);
          m_childrenInternalCellInterfaces[i]->getFace()->setSize(0.5*m_element->getSizeX(), 0., m_element->getSizeZ());
          m_childrenInternalCellInterfaces[i]->getFace()->setSurface(0.5*m_element->getSizeX()*m_element->getSizeZ());
          m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[i % 2]);
          m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[2 + i % 2]);
          m_childrenCells[i % 2]->addCellInterface(m_childrenInternalCellInterfaces[i]);
          m_childrenCells[2 + i % 2]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        }
        //Attribution model and slopes
        //----------------------------
        m_childrenInternalCellInterfaces[i]->associeModel(model);
        m_childrenInternalCellInterfaces[i]->allocateSlopes(m_numberPhases, m_numberTransports, allocateSlopeLocal);
      }
    }
  }
  else {

    //Case 3D
    //-------

    //3 times the 2D plane on X, Y and Z with following numbering:
    //- Number au center correspond ici au number de la face (normal vers nous),
    //- Number dans le coin bas gauche pour cells devant le plan (cell droite),
    //- Number dans le coin haut droite pour cells derriere le plan (cell gauche).

    //             Selon X :                          Selon Y:                           Selon Z :
    //
    // |--------------|--------------|    |--------------|--------------|    |--------------|--------------|
    // |             6|             2|    |             4|             0|    |             2|             3|
    // |              |              |    |              |              |    |              |              |
    // |      2       |      3       |    |      2       |      3       |    |      2       |      3       |
    // |              |              |    |              |              |    |              |              |
    // |7             |3             |    |6             |2             |    |6             |7             |
    // |--------------|--------------|    |--------------|--------------|    |--------------|--------------|
    // |             4|             0|    |             5|             1|    |             0|             1|
    // |              |              |    |              |              |    |              |              |
    // |      0       |      1       |    |      0       |      1       |    |      0       |      1       |
    // |              |              |    |              |              |    |              |              |
    // |5             |1             |    |7             |3             |    |4             |5             |
    // |--------------|--------------|    |--------------|--------------|    |--------------|--------------|
    //
    //                y                              z---|                                  y
    //                |                                  |                                  |
    //            z---|                                  x                                  |---x

    //Face on X
    for (int i = 0; i < 4; i++) {
      cellInterfaceRef->creerCellInterfaceChildInterne(m_lvl, &m_childrenInternalCellInterfaces);
      m_childrenInternalCellInterfaces[i]->creerFaceChild(cellInterfaceRef);
      m_childrenInternalCellInterfaces[i]->getFace()->setNormal(1., 0., 0.);
      m_childrenInternalCellInterfaces[i]->getFace()->setTangent(0., 1., 0.);
      m_childrenInternalCellInterfaces[i]->getFace()->setBinormal(0., 0., 1.);
      if (i == 0) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent, posYCellParent - 0.25*dYParent, posZCellParent + 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[4]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[5]);
        m_childrenCells[4]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[5]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else if (i == 1) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent, posYCellParent - 0.25*dYParent, posZCellParent - 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[0]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[1]);
        m_childrenCells[0]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[1]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else if (i == 2) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent, posYCellParent + 0.25*dYParent, posZCellParent + 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[6]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[7]);
        m_childrenCells[6]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[7]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent, posYCellParent + 0.25*dYParent, posZCellParent - 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[2]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[3]);
        m_childrenCells[2]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[3]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      m_childrenInternalCellInterfaces[i]->getFace()->setSize(0., 0.5*m_element->getSizeY(), 0.5*m_element->getSizeZ());
      m_childrenInternalCellInterfaces[i]->getFace()->setSurface(0.5*m_element->getSizeY()*0.5*m_element->getSizeZ());
      //Attribution model and slopes
      m_childrenInternalCellInterfaces[i]->associeModel(model);
      m_childrenInternalCellInterfaces[i]->allocateSlopes(m_numberPhases, m_numberTransports, allocateSlopeLocal);
    }

    //Face on Y
    for (int i = 4; i < 8; i++) {
      cellInterfaceRef->creerCellInterfaceChildInterne(m_lvl, &m_childrenInternalCellInterfaces);
      m_childrenInternalCellInterfaces[i]->creerFaceChild(cellInterfaceRef);
      m_childrenInternalCellInterfaces[i]->getFace()->setNormal(0., 1., 0.);
      m_childrenInternalCellInterfaces[i]->getFace()->setTangent(-1., 0., 0.);
      m_childrenInternalCellInterfaces[i]->getFace()->setBinormal(0., 0., 1.);
      if (i == 4) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent + 0.25*dXParent, posYCellParent, posZCellParent + 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[5]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[7]);
        m_childrenCells[5]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[7]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else if (i == 5) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent + 0.25*dXParent, posYCellParent, posZCellParent - 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[1]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[3]);
        m_childrenCells[1]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[3]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else if (i == 6) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent - 0.25*dXParent, posYCellParent, posZCellParent + 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[4]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[6]);
        m_childrenCells[4]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[6]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent - 0.25*dXParent, posYCellParent, posZCellParent - 0.25*dZParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[0]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[2]);
        m_childrenCells[0]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[2]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      m_childrenInternalCellInterfaces[i]->getFace()->setSize(0.5*m_element->getSizeX(), 0., 0.5*m_element->getSizeZ());
      m_childrenInternalCellInterfaces[i]->getFace()->setSurface(0.5*m_element->getSizeX()*0.5*m_element->getSizeZ());
      //Attribution model and slopes
      m_childrenInternalCellInterfaces[i]->associeModel(model);
      m_childrenInternalCellInterfaces[i]->allocateSlopes(m_numberPhases, m_numberTransports, allocateSlopeLocal);
    }

    //Face on Z
    for (int i = 8; i < 12; i++) {
      cellInterfaceRef->creerCellInterfaceChildInterne(m_lvl, &m_childrenInternalCellInterfaces);
      m_childrenInternalCellInterfaces[i]->creerFaceChild(cellInterfaceRef);
      m_childrenInternalCellInterfaces[i]->getFace()->setNormal(0., 0., 1.);
      m_childrenInternalCellInterfaces[i]->getFace()->setTangent(1., 0., 0.);
      m_childrenInternalCellInterfaces[i]->getFace()->setBinormal(0., 1., 0.);
      if (i == 8) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent - 0.25*dXParent, posYCellParent - 0.25*dYParent, posZCellParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[0]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[4]);
        m_childrenCells[0]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[4]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else if (i == 9) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent + 0.25*dXParent, posYCellParent - 0.25*dYParent, posZCellParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[1]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[5]);
        m_childrenCells[1]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[5]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else if (i == 10) {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent - 0.25*dXParent, posYCellParent + 0.25*dYParent, posZCellParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[2]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[6]);
        m_childrenCells[2]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[6]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      else {
        m_childrenInternalCellInterfaces[i]->getFace()->setPos(posXCellParent + 0.25*dXParent, posYCellParent + 0.25*dYParent, posZCellParent);
        m_childrenInternalCellInterfaces[i]->initializeGauche(m_childrenCells[3]);
        m_childrenInternalCellInterfaces[i]->initializeDroite(m_childrenCells[7]);
        m_childrenCells[3]->addCellInterface(m_childrenInternalCellInterfaces[i]);
        m_childrenCells[7]->addCellInterface(m_childrenInternalCellInterfaces[i]);
      }
      m_childrenInternalCellInterfaces[i]->getFace()->setSize(0.5*m_element->getSizeX(), 0.5*m_element->getSizeY(), 0.);
      m_childrenInternalCellInterfaces[i]->getFace()->setSurface(0.5*m_element->getSizeX()*0.5*m_element->getSizeY());
      //Attribution model and slopes
      m_childrenInternalCellInterfaces[i]->associeModel(model);
      m_childrenInternalCellInterfaces[i]->allocateSlopes(m_numberPhases, m_numberTransports, allocateSlopeLocal);
    }
  }

  //----------------------------------
  //External cell-interface refinement
  //----------------------------------

  for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
    if (!m_cellInterfaces[b]->getSplit()) { m_cellInterfaces[b]->raffineCellInterfaceExterne(nbCellsY, nbCellsZ, dXParent, dYParent, dZParent, this, dim); }
  }
}

//***********************************************************************

void Cell::createChildCell(const int &num, const int &lvl)
{
  m_childrenCells.push_back(new Cell(lvl + 1));
}

//***********************************************************************

void Cell::unrefineCellAndCellInterfaces()
{
  //---------------------
  //Update of parent cell
  //---------------------

  this->averageChildrenInParent();

  //--------------------------------------------
  //Internal children cell-interface destruction
  //--------------------------------------------

  for (unsigned int i = 0; i < m_childrenInternalCellInterfaces.size(); i++) {
    m_childrenInternalCellInterfaces[i]->finalizeFace();
    delete m_childrenInternalCellInterfaces[i];
  }
  m_childrenInternalCellInterfaces.clear();

  //--------------------------------------------
  //External children cell-interface destruction
  //--------------------------------------------

  for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
    m_cellInterfaces[b]->deraffineCellInterfaceExterne(this);
  }

  //--------------------------
  //Children cells destruction
  //--------------------------

  for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
    delete m_childrenCells[i];
  }
  m_childrenCells.clear();
  m_element->finalizeElementsChildren();

	m_split = false;
}

//***********************************************************************

void Cell::averageChildrenInParent()
{
  int numberCellsChildren(m_childrenCells.size());
  if (numberCellsChildren > 0) {
    //Thermodynamical reconstruction of parent cell
    //Averaging conservative variables
    m_cons->setToZero(m_numberPhases);
    for (int i = 0; i < numberCellsChildren; i++) {
      m_cons->setBufferFlux(*m_childrenCells[i], m_numberPhases);              //Building children cells conservative variables in fluxTempXXX
      m_cons->addFlux(1., m_numberPhases);                                     //Adding them in m_cons
    }
    m_cons->multiply(1. / (double)numberCellsChildren, m_numberPhases);        //Division of m_cons by the children number to obatin average
    //Primitive variables reconstruction using relaxation
    m_cons->buildPrim(m_vecPhases, m_mixture, m_numberPhases);
	m_model->relaxations(this, m_numberPhases);

    //Transport
    for (int k = 0; k < m_numberTransports; k++) {
      double transport(0.);
      for (int i = 0; i < numberCellsChildren; i++) {
        transport += m_childrenCells[i]->getTransport(k).getValue();
      }
      transport = transport / (double)numberCellsChildren;
      m_vecTransports[k].setValue(transport);
    }

    //setting m_cons to zero for next
    m_cons->setToZero(m_numberPhases);
    for (int k = 0; k < m_numberTransports; k++) {
      m_consTransports[k].setValue(0.);
    }
  }
}

//***********************************************************************

bool Cell::lvlNeighborTooHigh()
{
  bool criteria(false);
  for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
    if (m_cellInterfaces[b]->getLvl() == m_lvl) {
      for (int bChild = 0; bChild < m_cellInterfaces[b]->getNumberCellInterfacesChildren(); bChild++) {
        if (m_cellInterfaces[b]->getCellInterfaceChild(bChild)->getSplit()) { criteria = true; return criteria; }
      }
    }
    else {
      if (m_cellInterfaces[b]->getSplit()) { criteria = true; return criteria; }
    }
  }
  return criteria;
}

//***********************************************************************

bool Cell::lvlNeighborTooLow()
{
  bool criteria(false);
  for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
    if (!m_cellInterfaces[b]->getSplit()) {
      if (m_cellInterfaces[b]->whoAmI() == 0) //Cell interface type CellInterface/O2
      {
        //Getting left and right AMR levels for each cell interface
        int lvlg = m_cellInterfaces[b]->getCellGauche()->getLvl();
        int lvld = m_cellInterfaces[b]->getCellDroite()->getLvl();

        //Looking for neighbor level to be not too low
        if (lvlg < m_lvl) { criteria = true; return criteria; }
        if (lvld < m_lvl) { criteria = true; return criteria; }
      }
      else
      {
        //Getting Left AMR levels for cell-interface conditions
        int lvlg = m_cellInterfaces[b]->getCellGauche()->getLvl();

        //Looking for neighbor level to be not too low
        if (lvlg < m_lvl) { criteria = true; return criteria; }
      }
    }
  }
  return criteria;
}

//***********************************************************************

void Cell::buildLvlCellsAndLvlInternalCellInterfacesArrays(std::vector<Cell *> *cellsLvl, std::vector<CellInterface *> *cellInterfacesLvl) //KS// name!
{
  for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
    cellsLvl[m_lvl + 1].push_back(m_childrenCells[i]);
  }
  for (unsigned int i = 0; i < m_childrenInternalCellInterfaces.size(); i++) {
    cellInterfacesLvl[m_lvl + 1].push_back(m_childrenInternalCellInterfaces[i]);
  }
}

//***********************************************************************

bool Cell::printGnuplotAMR(std::ofstream &fileStream, const int &dim, GeometricObject *objet)
{
  bool ecrit(true);
  int dimension(dim);
  Coord position = m_element->getPosition();
  //for cut printing
  if (objet != 0) 
  {
    if (objet->getType() != 0) { //For non probes objects
      ecrit = m_element->traverseObjet(*objet);
      position = objet->projectionPoint(position);
      dimension = objet->getType();
    }
  }
  if (ecrit) {
    if (!m_split) {
      //CAUTION: the order has to be kept for conformity reasons with gnuplot scripts
      if (dimension >= 1) fileStream << position.getX() << " ";
      if (dimension >= 2) fileStream << position.getY() << " ";
      if (dimension == 3)fileStream << position.getZ() << " ";
      this->printPhasesMixture(m_numberPhases, m_numberTransports, fileStream);
      fileStream << m_lvl << " " << m_xi << " ";
      fileStream << std::endl;
      if (objet != 0) { if (objet->getType() == 0) return true; } //probe specificity, unique.
    }
    else {
      
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        m_childrenCells[i]->printGnuplotAMR(fileStream, dim, objet);
      }
    }
  } 
  return false;
}

//***********************************************************************

void Cell::computeIntegration(double &integration)
{
  if (!m_split) {
    integration += m_element->getVolume()*m_vecPhases[1]->getAlpha();
  }
  else {
    for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
      m_childrenCells[i]->computeIntegration(integration);
    }
  }
}

//***********************************************************************

void Cell::lookForPmax(double *pMax, double*pMaxWall)
{
  if (!m_split) {
    if (m_mixture->getPressure() > pMax[0]) {
      pMax[0] = m_mixture->getPressure();
      pMax[1] = m_element->getPosition().getX();
      pMax[2] = m_element->getPosition().getY();
      pMax[3] = m_element->getPosition().getZ();
    }
    if (m_mixture->getPressure() > pMaxWall[0] && m_element->getPosition().getX() < 0.005) {
      pMaxWall[0] = m_mixture->getPressure();
      pMaxWall[1] = m_element->getPosition().getX();
      pMaxWall[2] = m_element->getPosition().getY();
      pMaxWall[3] = m_element->getPosition().getZ();
    }
  }
  else {
    for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
      m_childrenCells[i]->lookForPmax(pMax, pMaxWall);
    }
  }
}

//***********************************************************************

int Cell::getLvl()
{
  return m_lvl;
}

//***********************************************************************

bool Cell::getSplit()
{
	return m_split;
}

//***********************************************************************

double Cell::getXi()
{
  return m_xi;
}

//***********************************************************************

void Cell::setXi(double value)
{
  m_xi = value;
}

//***********************************************************************

void Cell::addFluxXi(double value)
{
  m_consXi += value;
}

//***********************************************************************

void Cell::subtractFluxXi(double value)
{
  m_consXi -= value;
}

//***********************************************************************

int Cell::getNumberCellsChildren()
{
  return m_childrenCells.size();
}

//***********************************************************************

Cell* Cell::getCellChild(const int &num)
{
  return m_childrenCells[num];
}

//***********************************************************************

std::vector<Cell *>* Cell::getChildVector()
{
  return &m_childrenCells;
}

//****************************************************************************
//************************** Parallel non-AMR *******************************
//****************************************************************************

void Cell::fillBufferPrimitives(double *buffer, int &counter, Prim type) const
{
	for (int k = 0; k < m_numberPhases; k++) {
		this->getPhase(k, type)->fillBuffer(buffer, counter);
	}
	this->getMixture(type)->fillBuffer(buffer, counter);
	for (int k = 0; k < m_numberTransports; k++) {
		buffer[++counter] = this->getTransport(k, type).getValue();
	}
}

//***********************************************************************

void Cell::getBufferPrimitives(double *buffer, int &counter, Eos **eos, Prim type)
{
	for (int k = 0; k < m_numberPhases; k++) {
		this->getPhase(k, type)->getBuffer(buffer, counter, eos);
	}
	this->getMixture(type)->getBuffer(buffer, counter);
	for (int k = 0; k < m_numberTransports; k++) {
		this->setTransport(buffer[++counter], k, type);
	}
  this->fulfillState(type);
}

//***********************************************************************

void Cell::fillBufferVector(double *buffer, int &counter, const int &dim, std::string nameVector, int num, int index) const
{
	buffer[++counter] = this->selectVector(nameVector, num, index).getX();
	if (dim > 1) buffer[++counter] = this->selectVector(nameVector, num, index).getY();
	if (dim > 2) buffer[++counter] = this->selectVector(nameVector, num, index).getZ();
}

//***********************************************************************

void Cell::getBufferVector(double *buffer, int &counter, const int &dim, std::string nameVector, int num, int index)
{
	Coord temp;
	temp.setX(buffer[++counter]);
	if (dim > 1) temp.setY(buffer[++counter]);
	if (dim > 2) temp.setZ(buffer[++counter]);
	this->setVector(nameVector, temp, num, index);
}

//***********************************************************************

void Cell::fillBufferTransports(double *buffer, int &counter) const
{
  for (int k = 0; k < m_numberTransports; k++) {
    buffer[++counter] = this->getTransport(k).getValue();
  }
}

//***********************************************************************

void Cell::getBufferTransports(double *buffer, int &counter)
{
  for (int k = 0; k < m_numberTransports; k++) {
    this->setTransport(buffer[++counter], k);
  }
}

//****************************************************************************
//**************************** AMR Parallel **********************************
//****************************************************************************

void Cell::fillBufferPrimitivesAMR(double *buffer, int &counter, const int &lvl, std::string whichCpuAmIForNeighbour, Prim type) const
{
  if (m_lvl == lvl) {
    for (int k = 0; k < m_numberPhases; k++) {
      this->getPhase(k, type)->fillBuffer(buffer, counter);
    }
    this->getMixture(type)->fillBuffer(buffer, counter);
    for (int k = 0; k < m_numberTransports; k++) {
      buffer[++counter] = this->getTransport(k, type).getValue();
    }
  }
  else {
    if (whichCpuAmIForNeighbour == "LEFT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am left CPU, I send all right cells
        if ((i % 2) == 1) { m_childrenCells[i]->fillBufferPrimitivesAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, type); }
      }
    }
    else if (whichCpuAmIForNeighbour == "RIGHT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am right CPU, I send all left cells
        if ((i % 2) == 0) { m_childrenCells[i]->fillBufferPrimitivesAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, type); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BOTTOM") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am bottom CPU, I send all top cells
        if ((i % 4) > 1) { m_childrenCells[i]->fillBufferPrimitivesAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, type); }
      }
    }
    else if (whichCpuAmIForNeighbour == "TOP") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am top CPU, I send all bottom cells
        if ((i % 4) <= 1) { m_childrenCells[i]->fillBufferPrimitivesAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, type); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BACK") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am back CPU, I send all front cells
        if (i > 3) { m_childrenCells[i]->fillBufferPrimitivesAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, type); }
      }
    }
    else if (whichCpuAmIForNeighbour == "FRONT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am front CPU, I send all back cells
        if (i <= 3) { m_childrenCells[i]->fillBufferPrimitivesAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, type); }
      }
    }
  }
}

//***********************************************************************

void Cell::getBufferPrimitivesAMR(double *buffer, int &counter, const int &lvl, Eos **eos, Prim type)
{
	if (m_lvl == lvl) {
		for (int k = 0; k < m_numberPhases; k++) {
			this->getPhase(k, type)->getBuffer(buffer, counter, eos);
		}
		this->getMixture(type)->getBuffer(buffer, counter);
		for (int k = 0; k < m_numberTransports; k++) {
			this->setTransport(buffer[++counter], k, type);
		}
    this->fulfillState(type);
	}
	else {
		for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
			m_childrenCells[i]->getBufferPrimitivesAMR(buffer, counter, lvl, eos, type);
		}
	}
}

//***********************************************************************

void Cell::fillBufferVectorAMR(double *buffer, int &counter, const int &lvl, std::string whichCpuAmIForNeighbour, const int &dim, std::string nameVector, int num, int index) const
{
	if (m_lvl == lvl) {
		buffer[++counter] = this->selectVector(nameVector, num, index).getX();
		if (dim > 1) buffer[++counter] = this->selectVector(nameVector, num, index).getY();
		if (dim > 2) buffer[++counter] = this->selectVector(nameVector, num, index).getZ();
	}
	else {
    if (whichCpuAmIForNeighbour == "LEFT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am left CPU, I send all right cells
        if ((i % 2) == 1) { m_childrenCells[i]->fillBufferVectorAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, dim, nameVector, num, index); }
      }
    }
    else if (whichCpuAmIForNeighbour == "RIGHT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am right CPU, I send all left cells
        if ((i % 2) == 0) { m_childrenCells[i]->fillBufferVectorAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, dim, nameVector, num, index); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BOTTOM") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am bottom CPU, I send all top cells
        if ((i % 4) > 1) { m_childrenCells[i]->fillBufferVectorAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, dim, nameVector, num, index); }
      }
    }
    else if (whichCpuAmIForNeighbour == "TOP") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am top CPU, I send all bottom cells
        if ((i % 4) <= 1) { m_childrenCells[i]->fillBufferVectorAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, dim, nameVector, num, index); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BACK") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am back CPU, I send all front cells
        if (i > 3) { m_childrenCells[i]->fillBufferVectorAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, dim, nameVector, num, index); }
      }
    }
    else if (whichCpuAmIForNeighbour == "FRONT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am front CPU, I send all back cells
        if (i <= 3) { m_childrenCells[i]->fillBufferVectorAMR(buffer, counter, lvl, whichCpuAmIForNeighbour, dim, nameVector, num, index); }
      }
    }
	}
}

//***********************************************************************

void Cell::getBufferVectorAMR(double *buffer, int &counter, const int &lvl, const int &dim, std::string nameVector, int num, int index)
{
	if (m_lvl == lvl) {
		Coord temp;
		temp.setX(buffer[++counter]);
		if (dim > 1) temp.setY(buffer[++counter]);
		if (dim > 2) temp.setZ(buffer[++counter]);
		this->setVector(nameVector, temp, num, index);
	}
	else {
		for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
			m_childrenCells[i]->getBufferVectorAMR(buffer, counter, lvl, dim, nameVector, num, index);
		}
	}
}

//***********************************************************************

void Cell::fillBufferTransportsAMR(double *buffer, int &counter, const int &lvl, std::string whichCpuAmIForNeighbour) const
{
  if (m_lvl == lvl) {
    for (int k = 0; k < m_numberTransports; k++) {
      buffer[++counter] = this->getTransport(k).getValue();
    }
  }
  else {
    if (whichCpuAmIForNeighbour == "LEFT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am left CPU, I send all right cells
        if ((i % 2) == 1) { m_childrenCells[i]->fillBufferTransportsAMR(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "RIGHT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am right CPU, I send all left cells
        if ((i % 2) == 0) { m_childrenCells[i]->fillBufferTransportsAMR(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BOTTOM") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am bottom CPU, I send all top cells
        if ((i % 4) > 1) { m_childrenCells[i]->fillBufferTransportsAMR(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "TOP") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am top CPU, I send all bottom cells
        if ((i % 4) <= 1) { m_childrenCells[i]->fillBufferTransportsAMR(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BACK") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am back CPU, I send all front cells
        if (i > 3) { m_childrenCells[i]->fillBufferTransportsAMR(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "FRONT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am front CPU, I send all back cells
        if (i <= 3) { m_childrenCells[i]->fillBufferTransportsAMR(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
  }
}

//***********************************************************************

void Cell::getBufferTransportsAMR(double *buffer, int &counter, const int &lvl)
{
  if (m_lvl == lvl) {
    for (int k = 0; k < m_numberTransports; k++) {
      this->setTransport(buffer[++counter], k);
    }
  }
  else {
    for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
      m_childrenCells[i]->getBufferTransportsAMR(buffer, counter, lvl);
    }
  }
}

//***********************************************************************

void Cell::chooseRefineDeraffineGhost(const int &nbCellsY, const int &nbCellsZ,	const std::vector<AddPhys*> &addPhys, Model *model, std::vector<Cell *> *cellsLvlGhost)
{
  if (m_split) {
    //std::cout << "cpu " << rankCpu << " split" << std::endl;
    if (m_childrenCells.size() == 0) { this->refineCellAndCellInterfacesGhost(nbCellsY, nbCellsZ, addPhys, model); }
  }
  else {
    //std::cout << "cpu " << rankCpu << " non-split" << std::endl;
    if (m_childrenCells.size() > 0) { this->unrefineCellAndCellInterfacesGhost(); }
  }
  //std::cout << "cpu " << rankCpu << " push-back" << std::endl;
  for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
    cellsLvlGhost[m_lvl + 1].push_back(m_childrenCells[i]);
  }
}

//***********************************************************************

void Cell::refineCellAndCellInterfacesGhost(const int &nbCellsY, const int &nbCellsZ, const std::vector<AddPhys*> &addPhys, Model *model)
{
    //--------------------------------------
    //Initializations (children and dimension)
    //--------------------------------------

  //Notice that the children number of ghost cells is different than for internal cells
	double dimX(1.), dimY(0.), dimZ(0.);
	//int numberCellsChildren(2);
	int dim(1);
	if (nbCellsZ != 1) {
		//numberCellsChildren = 8;
		dimY = 1.;
		dimZ = 1.;
		dim = 3;
	}
	else if (nbCellsY != 1) {
		//numberCellsChildren = 4;
		dimY = 1.;
		dim = 2;
	}
	CellInterface* cellInterfaceRef(0); //cellInterfaceRef always is the one with the level corresponding to the current ghost-cell level
	for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
		if (m_cellInterfaces[b]->whoAmI() == 0) { cellInterfaceRef = m_cellInterfaces[b]; break; } //Cell interface type CellInterface/O2
	}
	int allocateSlopeLocal = 1;

	//---------------
	//Cell refinement
	//---------------

	//Initialization of mesh data for children cells
	//----------------------------------------------
    double posXCellParent, posYCellParent, posZCellParent;
    double dXParent, dYParent, dZParent;
    double posXChild, posYChild, posZChild;
    posXCellParent = m_element->getPosition().getX();
    posYCellParent = m_element->getPosition().getY();
    posZCellParent = m_element->getPosition().getZ();
    dXParent = m_element->getSizeX();
    dYParent = m_element->getSizeY();
    dZParent = m_element->getSizeZ();
    double volumeCellParent, lCFLCellParent;
    volumeCellParent = m_element->getVolume();
	lCFLCellParent = m_element->getLCFL();


	for (unsigned int b = 0; b < m_cellInterfaces.size(); b++)     
    {
        if(m_cellInterfaces[b]->whoAmI()==0) //Inner face 
        {
            if (!m_cellInterfaces[b]->getSplit()) //Not split
            {
                auto const key=this->getElement()->getKey();
                bool GhostCellIsLeft=false;
                Cell* GhostCellNeighbor=nullptr;

                auto child_coord=key.child(0).coordinate();

                if(this==m_cellInterfaces[b]->getCellGauche())
                {
                    GhostCellNeighbor=m_cellInterfaces[b]->getCellDroite();
                    GhostCellIsLeft=true;

                    child_coord[0]+= m_cellInterfaces[b]->getFace()->getNormal().getX();
                    child_coord[1]+= m_cellInterfaces[b]->getFace()->getNormal().getY();
                    child_coord[2]+= m_cellInterfaces[b]->getFace()->getNormal().getZ();
                }
                else
                {
                    GhostCellNeighbor=m_cellInterfaces[b]->getCellGauche();
                    GhostCellIsLeft=false;
                }

                //Iterate over directions other than normal
                int idx=0;
                if( std::fabs(m_cellInterfaces[b]->getFace()->getNormal().getY()-1.0) < 1e-10 )
                    idx=1;
                if( std::fabs(m_cellInterfaces[b]->getFace()->getNormal().getZ()-1.0) < 1e-10 )
                    idx=2;

                int direction_j = dim==3 ? 2:1; 
                int direction_i = (dim==2 || dim==3) ? 2:1; 
                for(int i =0; i<direction_i; ++i)
                {
                    for(int j =0; j<direction_j; ++j)
                    {
                        auto next=child_coord;
                        next[(idx+1)%dim]+=i;
                        next[(idx+2)%dim]+=j;
                        decomposition::Key<3> next_key(next,key.level()+1);

                        Cell* childCellGhost;

                        bool cellExists=false;
                        for (int c = 0; c < m_childrenCells.size(); c++) 
                        {
                            if(m_childrenCells[c]->getElement()->getKey()==next_key)
                            {
                                cellExists=true;
                                childCellGhost = m_childrenCells[c];
                                break;
                            } //found existing cell 
                        }

                        //Create the cell
                        if(!cellExists)
                        {
                            //Children cells/element creation
                            //-------------------------------
                            this->createChildCell(i, m_lvl);
                            childCellGhost = m_childrenCells.back();
                            m_element->creerElementChild();
                            childCellGhost->setElement(m_element->getElementChildBack(), 0);
                            childCellGhost->getElement()->setVolume(volumeCellParent / std::pow(2,dim));
                            childCellGhost->getElement()->setLCFL(0.5*lCFLCellParent);
                            childCellGhost->getElement()->setSize((1 - dimX*0.5)*dXParent, (1 - dimY*0.5)*dYParent, (1 - dimZ*0.5)*dZParent);
                            childCellGhost->getElement()->setKey( next_key );

                            auto cell_direction = next-child_coord;
                            childCellGhost->getElement()->setPos(posXCellParent + 0.25*cell_direction[0]*dimX*dXParent,
                                    posYCellParent + 0.25*cell_direction[1]*dimY*dYParent,
                                    posZCellParent + 0.25*cell_direction[2]*dimZ*dZParent);

                            //Initialization of main arrays according to model and number of phases
                            //+ physical initialization: physical data for children cells
                            //-------------------------------------------------------------------------------------
                            childCellGhost->allocate(m_numberPhases, m_numberTransports, addPhys, model);
                            for (int k = 0; k < m_numberPhases; k++) {
                                childCellGhost->copyPhase(k, m_vecPhases[k]);
                            }
                            childCellGhost->copyMixture(m_mixture);
                            childCellGhost->getCons()->setToZero(m_numberPhases);
                            for (int k = 0; k < m_numberTransports; k++) { childCellGhost->setTransport(m_vecTransports[k].getValue(), k); }
                            for (int k = 0; k < m_numberTransports; k++) { childCellGhost->setConsTransport(0., k); }
                            childCellGhost->setXi(m_xi);
                        }



                        //Update pointer with current cell and split face
                        m_cellInterfaces[b]->initialize( this,childCellGhost);
                        const auto face= m_cellInterfaces[b]->getFace();
                        const double surfaceChild(std::pow(0.5, dim - 1.)*face->getSurface());

                        //Same level 
                        if(GhostCellNeighbor->getLvl()==m_lvl)
                        {
                            //Push back faces
                            m_cellInterfaces[b]->creerCellInterfaceChild();
                            Face* f = new FaceCartesian;
                            m_cellInterfaces[b]->getCellInterfaceChildBack()->setFace(f);

                            //face properties
                            m_cellInterfaces[b]->getCellInterfaceChildBack()->getFace()->initializeAutres(surfaceChild, face->getNormal(), face->getTangent(), face->getBinormal());

                            auto cellInterfacePosition=childCellGhost->getPosition();
                            cellInterfacePosition.setX( cellInterfacePosition.getX()+ face->getNormal().getX()*0.5*childCellGhost->getSizeX() );
                            cellInterfacePosition.setY( cellInterfacePosition.getY()+ face->getNormal().getY()*0.5*childCellGhost->getSizeY() );
                            cellInterfacePosition.setZ( cellInterfacePosition.getZ()+ face->getNormal().getZ()*0.5*childCellGhost->getSizeZ() );
                            m_cellInterfaces[b]->getCellInterfaceChildBack()->getFace()->setPos(cellInterfacePosition.getX(),cellInterfacePosition.getY(),cellInterfacePosition.getZ());

                            auto FaceSize=face->getSize();
                            FaceSize.setX( FaceSize.getX()*0.5  );
                            FaceSize.setY( FaceSize.getY()*0.5  );
                            FaceSize.setZ( FaceSize.getZ()*0.5  );
                            m_cellInterfaces[b]->getCellInterfaceChildBack()->getFace()->setSize(FaceSize);

                            if(GhostCellIsLeft)
                            {
                                m_cellInterfaces[b]->getCellInterfaceChildBack()->initializeGauche(childCellGhost);
                                m_cellInterfaces[b]->getCellInterfaceChildBack()->initializeDroite(GhostCellNeighbor);
                            }
                            else
                            {
                                m_cellInterfaces[b]->getCellInterfaceChildBack()->initializeDroite(childCellGhost);
                                m_cellInterfaces[b]->getCellInterfaceChildBack()->initializeGauche(GhostCellNeighbor);
                            }

                            childCellGhost->addCellInterface(m_cellInterfaces[b]->getCellInterfaceChildBack());
                            GhostCellNeighbor->addCellInterface(m_cellInterfaces[b]->getCellInterfaceChildBack());

                            //Association du model et des slopes
                            //-----------------------------------
                            m_cellInterfaces[b]->getCellInterfaceChildBack()->associeModel(model);
                            m_cellInterfaces[b]->getCellInterfaceChildBack()->allocateSlopes(this->getNumberPhases(), this->getNumberTransports(), allocateSlopeLocal);
                        }
                        else  // different level 
                        {
                            if(GhostCellIsLeft)
                            {
                                m_cellInterfaces[b]->initializeGauche(childCellGhost);
                            }
                            else
                            {
                                m_cellInterfaces[b]->initializeDroite(childCellGhost);
                            }
                            childCellGhost->addCellInterface(m_cellInterfaces[b]);
                        }
                    }
                }
            }
        }
	}


    //Sort the children according to the flattened index
    const auto child0Coord=this->getElement()->getKey().child(0).coordinate();
    auto getIndex=[](const decltype(child0Coord) dir){return dir[0]+2*dir[1]+4*dir[1]; } ;
    std::sort(m_childrenCells.begin(),m_childrenCells.end(),[&child0Coord,&getIndex]( Cell* child0, Cell* child1 )
    {
        auto dir0=child0->getElement()->getKey().coordinate()-child0Coord;
        auto dir1=child1->getElement()->getKey().coordinate()-child0Coord;
        return getIndex(dir0)< getIndex(dir1);
    });



    //FIXME: 
    //       Communication AMR:
    //          Update fillBufferAMR to check send elements
    //                  => has ghostneighbor instead of index
}

//***********************************************************************

void Cell::unrefineCellAndCellInterfacesGhost()
{
	//--------------------------------------------
	//External children cell-interface destruction
	//--------------------------------------------

	for (unsigned int b = 0; b < m_cellInterfaces.size(); b++) {
		m_cellInterfaces[b]->deraffineCellInterfaceExterne(this);
	}

	//--------------------------
	//Children cells destruction
	//--------------------------

	for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
		delete m_childrenCells[i];
	}
	m_childrenCells.clear();
	m_element->finalizeElementsChildren();
}

//***********************************************************************

void Cell::fillBufferXi(double *buffer, int &counter, const int &lvl, std::string whichCpuAmIForNeighbour) const
{
	if (m_lvl == lvl) {
		buffer[++counter] = m_xi;
	}
	else {
    if (whichCpuAmIForNeighbour == "LEFT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am left CPU, I send all right cells
        if ((i % 2) == 1) { m_childrenCells[i]->fillBufferXi(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "RIGHT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am right CPU, I send all left cells
        if ((i % 2) == 0) { m_childrenCells[i]->fillBufferXi(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BOTTOM") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am bottom CPU, I send all top cells
        if ((i % 4) > 1) { m_childrenCells[i]->fillBufferXi(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "TOP") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am top CPU, I send all bottom cells
        if ((i % 4) <= 1) { m_childrenCells[i]->fillBufferXi(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BACK") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am back CPU, I send all front cells
        if (i > 3) { m_childrenCells[i]->fillBufferXi(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "FRONT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am front CPU, I send all back cells
        if (i <= 3) { m_childrenCells[i]->fillBufferXi(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
	}
}

//***********************************************************************

void Cell::getBufferXi(double *buffer, int &counter, const int &lvl)
{
	if (m_lvl == lvl) {
		m_xi = buffer[++counter];
	}
	else {
		for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
			m_childrenCells[i]->getBufferXi(buffer, counter, lvl);
		}
	}
}

//***********************************************************************

void Cell::fillBufferSplit(bool *buffer, int &counter, const int &lvl, std::string whichCpuAmIForNeighbour) const
{
	if (m_lvl == lvl) {
		buffer[++counter] = m_split;
	}
	else {
    if (whichCpuAmIForNeighbour == "LEFT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am left CPU, I send all right cells
        if ((i % 2) == 1) { m_childrenCells[i]->fillBufferSplit(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "RIGHT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am right CPU, I send all left cells
        if ((i % 2) == 0) { m_childrenCells[i]->fillBufferSplit(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BOTTOM") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am bottom CPU, I send all top cells
        if ((i % 4) > 1) { m_childrenCells[i]->fillBufferSplit(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "TOP") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am top CPU, I send all bottom cells
        if ((i % 4) <= 1) { m_childrenCells[i]->fillBufferSplit(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BACK") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am back CPU, I send all front cells
        if (i > 3) { m_childrenCells[i]->fillBufferSplit(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "FRONT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am front CPU, I send all back cells
        if (i <= 3) { m_childrenCells[i]->fillBufferSplit(buffer, counter, lvl, whichCpuAmIForNeighbour); }
      }
    }
	}
}

//***********************************************************************

void Cell::getBufferSplit(bool *buffer, int &counter, const int &lvl)
{
	if (m_lvl == lvl) {
		m_split = buffer[++counter];
	}
	else {
		for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
			m_childrenCells[i]->getBufferSplit(buffer, counter, lvl);
		}
	}
}

//***********************************************************************

void Cell::fillNumberElementsToSendToNeighbour(int &numberElementsToSendToNeighbor, const int &lvl, std::string whichCpuAmIForNeighbour)
{
	if (m_lvl == lvl) {
		numberElementsToSendToNeighbor++;
	}
	else {
    if (whichCpuAmIForNeighbour == "LEFT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am left CPU, I send all right cells
        if ((i % 2) == 1) { m_childrenCells[i]->fillNumberElementsToSendToNeighbour(numberElementsToSendToNeighbor, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "RIGHT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am right CPU, I send all left cells
        if ((i % 2) == 0) { m_childrenCells[i]->fillNumberElementsToSendToNeighbour(numberElementsToSendToNeighbor, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BOTTOM") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am bottom CPU, I send all top cells
        if ((i % 4) > 1) { m_childrenCells[i]->fillNumberElementsToSendToNeighbour(numberElementsToSendToNeighbor, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "TOP") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am top CPU, I send all bottom cells
        if ((i % 4) <= 1) { m_childrenCells[i]->fillNumberElementsToSendToNeighbour(numberElementsToSendToNeighbor, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "BACK") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am back CPU, I send all front cells
        if (i > 3) { m_childrenCells[i]->fillNumberElementsToSendToNeighbour(numberElementsToSendToNeighbor, lvl, whichCpuAmIForNeighbour); }
      }
    }
    else if (whichCpuAmIForNeighbour == "FRONT") {
      for (unsigned int i = 0; i < m_childrenCells.size(); i++) {
        //I am front CPU, I send all back cells
        if (i <= 3) { m_childrenCells[i]->fillNumberElementsToSendToNeighbour(numberElementsToSendToNeighbor, lvl, whichCpuAmIForNeighbour); }
      }
    }
	}
}

//***************************************************************************
