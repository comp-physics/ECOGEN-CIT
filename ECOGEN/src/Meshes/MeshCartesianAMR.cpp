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

//! \file      MeshCartesianAMR.cpp
//! \author    K. Schmidmayer, F. Petitpas, B. Dorschner
//! \version   1.0
//! \date      February 19 2019

#include <algorithm>

#include "MeshCartesianAMR.h"

//***********************************************************************

MeshCartesianAMR::MeshCartesianAMR(double lX, int numberCellsX, double lY, int numberCellsY, double lZ, int numberCellsZ,
  std::vector<stretchZone> stretchX, std::vector<stretchZone> stretchY, std::vector<stretchZone> stretchZ,
	int lvlMax, double criteriaVar, bool varRho, bool varP, bool varU, bool varAlpha, double xiSplit, double xiJoin) :
  MeshCartesian(lX, numberCellsX, lY, numberCellsY, lZ, numberCellsZ, stretchX, stretchY, stretchZ),
  m_lvlMax(lvlMax), m_criteriaVar(criteriaVar), m_varRho(varRho), m_varP(varP), m_varU(varU), m_varAlpha(varAlpha), m_xiSplit(xiSplit), m_xiJoin(xiJoin)
{
  m_type = AMR;
}

//***********************************************************************

MeshCartesianAMR::~MeshCartesianAMR(){
  if (Ncpu > 1) delete[] m_cellsLvlGhost;
}

//***********************************************************************

int MeshCartesianAMR::initializeGeometrie(TypeMeshContainer<Cell *> &cells, TypeMeshContainer<CellInterface *> &cellInterfaces, bool pretraitementParallele, std::string ordreCalcul)
{
  this->meshStretching();
  this->initializeGeometrieAMR(cells, cellInterfaces, ordreCalcul);
  return m_geometrie;
}


//***********************************************************************

void MeshCartesianAMR::initializeGeometrieAMR(TypeMeshContainer<Cell *> &cells, TypeMeshContainer<CellInterface *> &cellInterfaces, std::string ordreCalcul)
{
  int ix, iy, iz;

  m_numberCellsX = m_numberCellsXGlobal;
  m_numberCellsY = m_numberCellsYGlobal;
  m_numberCellsZ = m_numberCellsZGlobal;
  
  //Domain decomposition
  //--------------------
  decomposition::Decomposition decomp({{m_numberCellsXGlobal,m_numberCellsYGlobal,m_numberCellsZGlobal}}, Ncpu);
  auto keys = decomp.get_keys(rankCpu);

  for(unsigned int i = 0; i < keys.size(); ++i)
  {    
    if (ordreCalcul == "FIRSTORDER") { cells.push_back(new Cell); }
    else { cells.push_back(new CellO2); }
    m_elements.push_back(new ElementCartesian());
    m_elements[i]->setKey(keys[i]);
    cells[i]->setElement(m_elements[i], i);
  }

  //Create cells and elements
  //-------------------------
  double volume(0.);
  for(unsigned int i = 0; i < keys.size(); ++i)
  {
    auto coord = keys[i].coordinate();
    ix = coord.x(); iy = coord.y(); iz = coord.z();
    volume = m_dXi[ix] * m_dYj[iy] * m_dZk[iz];
    cells[i]->getElement()->setVolume(volume);

    //CFL lenght
    double lCFL(1.e10);
    if (m_numberCellsX != 1) { lCFL = std::min(lCFL, m_dXi[ix]); }
    if (m_numberCellsY != 1) { lCFL = std::min(lCFL, m_dYj[iy]); }
    if (m_numberCellsZ != 1) { lCFL = std::min(lCFL, m_dZk[iz]); }
    if (m_geometrie > 1) lCFL *= 0.6;

    cells[i]->getElement()->setLCFL(lCFL);
    cells[i]->getElement()->setPos(m_posXi[ix], m_posYj[iy], m_posZk[iz]);
    cells[i]->getElement()->setSize(m_dXi[ix], m_dYj[iy], m_dZk[iz]);
  }

  //Create cell interfaces, faces and ghost cells
  //---------------------------------------------
  m_numberCellsCalcul = cells.size(); //KS//BD// Update this after balancing
  createCellInterfacesFacesAndGhostCells(cells, cellInterfaces, ordreCalcul, &decomp);
  m_numberCellsTotal = cells.size(); //KS//BD// Update this after balancing
  m_numberFacesTotal = cellInterfaces.size(); //KS//BD// Update this after balancing

  std::cout
    << "numberCellsCalcul "<<m_numberCellsCalcul<<" "
    << "m_numberCellsTotal "<<m_numberCellsTotal<<" "
    << "m_numberFacesTotal "<<m_numberFacesTotal<<" "
    <<std::endl; //KS//BD// Erase 3D faces for 1D and 2D simulations
}

//***********************************************************************

void MeshCartesianAMR::createCellInterfacesFacesAndGhostCells(TypeMeshContainer<Cell *> &cells,     
TypeMeshContainer<CellInterface*>& cellInterfaces, 
std::string ordreCalcul, decomposition::Decomposition* decomp)
{
   using coordinate_type = decomposition::Key<3>::coordinate_type;
   std::array<decomposition::Key<3>::coordinate_type,6> offsets;
   std::fill(offsets.begin(), offsets.end(), coordinate_type(0));

   double posX=0, posY=0., posZ=0.;

   for(int d = 0; d < 3; d++)
   {
       offsets[2*d][d] =-1;
       offsets[2*d+1][d] =+1;
   }

   const auto sizeNonGhostCells=cells.size();
   for(unsigned int i = 0; i < sizeNonGhostCells; ++i)
   {
       const auto coord = cells[i]->getElement()->getKey().coordinate();
       const auto ix = coord.x(), iy = coord.y(), iz = coord.z();
       for(int idx = 0; idx < offsets.size(); idx++)
       {
           const auto offset=offsets[idx];

           posX = m_posXi[ix] + 0.5*m_dXi[ix]*offset[0];
           posY = m_posYj[iy] + 0.5*m_dYj[iy]*offset[1];
           posZ = m_posZk[iz] + 0.5*m_dZk[iz]*offset[2];

           Coord normal, tangent,binormal;
           normal.setXYZ(static_cast<double>(offset[0]), 
                   static_cast<double>(offset[1]), 
                   static_cast<double>(offset[2])); 

           //Xdir
           if (offset[0] == 1) 
           {
               tangent.setXYZ( 0.,1.,0.); 
               binormal.setXYZ(0.,0.,1.); 
           }
           if (offset[0] == -1)
           {
               tangent.setXYZ( 0.,-1.,0.); 
               binormal.setXYZ(0.,0.,1.); 
           }

           //Ydir
           if (offset[1] == 1)
           {
               tangent.setXYZ( -1.,0.,0.); 
               binormal.setXYZ(0.,0.,1.); 
           }
           if (offset[1] == -1)
           {
               tangent.setXYZ( 1.,0.,0.); 
               binormal.setXYZ(0.,0.,1.); 
           }

           //Zdir
           if (offset[2] == 1) 
           {
               tangent.setXYZ( 1.,0.,0.); 
               binormal.setXYZ(0.,1.,0.); 
           }
           if (offset[2] == -1)
           {
               tangent.setXYZ(-1.,0.,0.); 
               binormal.setXYZ(0.,1.,0.); 
           }

           auto neighborCell = cells[i]->getElement()->getKey().coordinate() + offset;
           if (!decomp->is_inside(neighborCell)) //Offset is at a physical boundary
           {
               //Create boundary cell interface
               if (offset[0] == 1) //xDir=N
                   m_limXp->creeLimite(cellInterfaces);
               if (offset[0] == -1) //xDir=0
                   m_limXm->creeLimite(cellInterfaces);
               if (offset[1] == 1) //yDir=N
                   m_limYp->creeLimite(cellInterfaces);
               if (offset[1] == -1) //yDir=0
                   m_limYm->creeLimite(cellInterfaces);
               if (offset[2] == 1) //zDir=N
                   m_limZp->creeLimite(cellInterfaces);
               if (offset[2] == -1) //zDir=0
                   m_limZm->creeLimite(cellInterfaces);

               cellInterfaces.back()->initialize(cells[i], nullptr);

               cells[i]->addCellInterface(cellInterfaces.back());
               m_faces.push_back(new FaceCartesian());
               cellInterfaces.back()->setFace(m_faces.back());

               if (offset[0])
               {
                   m_faces.back()->setSize(0.0, m_dYj[iy], m_dZk[iz]);
                   m_faces.back()->initializeAutres(m_dYj[iy] * m_dZk[iz], normal, tangent, binormal);
               }
               if (offset[1])
               {
                   m_faces.back()->setSize(m_dXi[ix], 0.0, m_dZk[iz]);
                   m_faces.back()->initializeAutres(m_dXi[ix] * m_dZk[iz], normal, tangent, binormal);
               }
               if (offset[2])
               {
                   m_faces.back()->setSize(m_dXi[ix], m_dYj[iy], 0.0);
                   m_faces.back()->initializeAutres(m_dYj[iy] * m_dXi[ix], normal, tangent, binormal);
               }
               m_faces.back()->setPos(posX, posY, posZ);

           }
           else //Offset is an internal cell (ghost or not)
           {
               //Get neighbor key
               auto nKey = cells[i]->getElement()->getKey().neighbor(offset);
               int neighbour = decomp->get_rank(nKey);

               if (offset[0]>0 || offset[1]>0 || offset[2]>0) //Positive offset
               {
                   //Create cell interface
                   if (ordreCalcul == "FIRSTORDER") { cellInterfaces.push_back(new CellInterface); }
                   else { cellInterfaces.push_back(new CellInterfaceO2); }     

                   m_faces.push_back(new FaceCartesian());
                   cellInterfaces.back()->setFace(m_faces.back());

                   if (offset[0])
                   {
                       m_faces.back()->setSize(0.0, m_dYj[iy], m_dZk[iz]);
                       m_faces.back()->initializeAutres(m_dYj[iy] * m_dZk[iz], normal, tangent, binormal);
                   }
                   if (offset[1])
                   {
                       m_faces.back()->setSize(m_dXi[ix], 0.0, m_dZk[iz]);
                       m_faces.back()->initializeAutres(m_dXi[ix] * m_dZk[iz], normal, tangent, binormal);
                   }
                   if (offset[2])
                   {
                       m_faces.back()->setSize(m_dXi[ix], m_dYj[iy], 0.0);
                       m_faces.back()->initializeAutres(m_dYj[iy] * m_dXi[ix], normal, tangent, binormal);
                   }
                   m_faces.back()->setPos(posX, posY, posZ);

                   //Try to find the neighbor cell into the non-ghost cells
                   auto it = std::find_if(cells.begin(), cells.begin()+sizeNonGhostCells,
                           [&nKey](Cell* _k0){ 
                           return _k0->getElement()->getKey() == nKey;
                            });

                   if (it != cells.begin()+sizeNonGhostCells) //Neighbor cell is a non-ghost cell
                   {
                       //Update cell interface
                       cellInterfaces.back()->initialize(cells[i], *it);
                       cells[i]->addCellInterface(cellInterfaces.back());
                       (*it)->addCellInterface(cellInterfaces.back());
                   }
                   else //Neighbor cell is a ghost cell
                   {
                       //Try to find the neighbor cell into the already created ghost cells
                       auto it2 = std::find_if(cells.begin()+sizeNonGhostCells, cells.end(),
                         [&nKey](Cell* _k0){ 
                         return _k0->getElement()->getKey() == nKey;
                          });

                       if (it2 == cells.end()) //Ghost cell does not exist
                       {
                          //Create ghost cell and update cell interface
                         if (ordreCalcul == "FIRSTORDER") { cells.push_back(new CellGhost); }
                         else { cells.push_back(new CellO2Ghost); }
                         m_elements.push_back(new ElementCartesian());
                         m_elements.back()->setKey(nKey);
                         cells.back()->setElement(m_elements.back(), cells.size()-1);
                         cells.back()->pushBackSlope();
                         parallel.addSlopesToSend(neighbour);
                         parallel.addSlopesToReceive(neighbour);

                         //Update parallel communications
                         parallel.setNeighbour(neighbour);
                         //Try to find the current cell into the already added cells to send
                         auto cKey = cells[i]->getElement()->getKey();
                         auto it3 = std::find_if(parallel.getElementsToSend(neighbour).begin(), parallel.getElementsToSend(neighbour).end(),
                           [&cKey](Cell* _k0){ 
                           return _k0->getElement()->getKey() == cKey;
                            });
                         if (it3 == parallel.getElementsToSend(neighbour).end()) //Current cell not added in send vector
                         {
                           parallel.addElementToSend(neighbour,cells[i]);
                         }
                         parallel.addElementToReceive(neighbour, cells.back());
                         cells.back()->setRankOfNeighborCPU(neighbour);

                         const auto coord = nKey.coordinate();
                         const auto nix = coord.x(), niy = coord.y(), niz = coord.z();

                         const double volume = m_dXi[nix] * m_dYj[niy] * m_dZk[niz];
                         cells.back()->getElement()->setVolume(volume);

                         double lCFL(1.e10);
                         if (m_numberCellsX != 1) { lCFL = std::min(lCFL, m_dXi[nix]); }
                         if (m_numberCellsY != 1) { lCFL = std::min(lCFL, m_dYj[niy]); }
                         if (m_numberCellsZ != 1) { lCFL = std::min(lCFL, m_dZk[niz]); }
                         if (m_geometrie > 1) lCFL *= 0.6;

                         cells.back()->getElement()->setLCFL(lCFL);
                         cells.back()->getElement()->setPos(m_posXi[nix], m_posYj[niy], m_posZk[niz]);
                         cells.back()->getElement()->setSize(m_dXi[nix], m_dYj[niy], m_dZk[niz]);

                         //Update pointers cells <-> cell interfaces
                         cellInterfaces.back()->initialize(cells[i], cells.back());
                         cells[i]->addCellInterface(cellInterfaces.back());
                         cells.back()->addCellInterface(cellInterfaces.back());
                       }
                       else { //Ghost cell exists
                         //Update parallel communications
                         //Try to find the current cell into the already added cells to send
                         auto cKey = cells[i]->getElement()->getKey();
                         auto it3 = std::find_if(parallel.getElementsToSend(neighbour).begin(), parallel.getElementsToSend(neighbour).end(),
                           [&cKey](Cell* _k0){ 
                           return _k0->getElement()->getKey() == cKey;
                            });
                         if (it3 == parallel.getElementsToSend(neighbour).end()) //Current cell not added in send vector
                         {
                           parallel.addElementToSend(neighbour,cells[i]);
                         }

                         //Update pointers cells <-> cell interfaces
                         cellInterfaces.back()->initialize(cells[i], *it2);
                         cells[i]->addCellInterface(cellInterfaces.back());
                         (*it2)->addCellInterface(cellInterfaces.back());
                         (*it2)->pushBackSlope();
                         parallel.addSlopesToSend(neighbour);
                         parallel.addSlopesToReceive(neighbour);
                       }
                   }
               }
               else //Negative offset
               {
                   //Try to find the neighbor cell into the non-ghost cells
                   auto it = std::find_if(cells.begin(), cells.begin()+sizeNonGhostCells,
                                             [&nKey](Cell* _k0){ 
                                             return _k0->getElement()->getKey() == nKey;
                                              });

                   if (it == cells.begin()+sizeNonGhostCells) //Neighbor cell is a ghost cell
                   {
                     //Create cell interface related to the ghost cell
                     if (ordreCalcul == "FIRSTORDER") { cellInterfaces.push_back(new CellInterface); }
                     else { cellInterfaces.push_back(new CellInterfaceO2); }     

                     m_faces.push_back(new FaceCartesian());
                     cellInterfaces.back()->setFace(m_faces.back());

                     if (offset[0])
                     {
                         normal.setXYZ( 1.,0.,0.); 
                         tangent.setXYZ( 0.,1.,0.); 
                         binormal.setXYZ(0.,0.,1.); 
                         m_faces.back()->setSize(0.0, m_dYj[iy], m_dZk[iz]);
                         m_faces.back()->initializeAutres(m_dYj[iy] * m_dZk[iz], normal, tangent, binormal);
                     }
                     if (offset[1])
                     {
                         normal.setXYZ( 0.,1.,0.); 
                         tangent.setXYZ( -1.,0.,0.); 
                         binormal.setXYZ(0.,0.,1.); 
                         m_faces.back()->setSize(m_dXi[ix], 0.0, m_dZk[iz]);
                         m_faces.back()->initializeAutres(m_dXi[ix] * m_dZk[iz], normal, tangent, binormal);
                     }
                     if (offset[2])
                     {
                         normal.setXYZ( 0.,0.,1.); 
                         tangent.setXYZ( 1.,0.,0.); 
                         binormal.setXYZ(0.,1.,0.); 
                         m_faces.back()->setSize(m_dXi[ix], m_dYj[iy], 0.0);
                         m_faces.back()->initializeAutres(m_dYj[iy] * m_dXi[ix], normal, tangent, binormal);
                     }
                     m_faces.back()->setPos(posX, posY, posZ);

                     //Try to find the neighbor cell into the already created ghost cells
                     auto it2 = std::find_if(cells.begin()+sizeNonGhostCells, cells.end(),
                                            [&nKey](Cell* _k0){ 
                                            return _k0->getElement()->getKey() == nKey;
                                             });

                     if (it2 == cells.end()) //Ghost cell does not exist
                     {
                       //Create ghost cell
                       if (ordreCalcul == "FIRSTORDER") { cells.push_back(new CellGhost); }
                       else { cells.push_back(new CellO2Ghost); }
                       m_elements.push_back(new ElementCartesian());
                       m_elements.back()->setKey(nKey);
                       cells.back()->setElement(m_elements.back(), cells.size()-1);
                       cells.back()->pushBackSlope();
                       parallel.addSlopesToSend(neighbour);
                       parallel.addSlopesToReceive(neighbour);

                       //Update parallel communications
                       parallel.setNeighbour(neighbour);
                       //Try to find the current cell into the already added cells to send
                       auto cKey = cells[i]->getElement()->getKey();
                       auto it3 = std::find_if(parallel.getElementsToSend(neighbour).begin(), parallel.getElementsToSend(neighbour).end(),
                         [&cKey](Cell* _k0){ 
                         return _k0->getElement()->getKey() == cKey;
                          });
                       if (it3 == parallel.getElementsToSend(neighbour).end()) //Current cell not added in send vector
                       {
                         parallel.addElementToSend(neighbour,cells[i]);
                       }
                       parallel.addElementToReceive(neighbour, cells.back());
                       cells.back()->setRankOfNeighborCPU(neighbour);

                       const auto coord = nKey.coordinate();
                       const auto nix = coord.x(), niy = coord.y(), niz = coord.z();

                       const double volume = m_dXi[nix] * m_dYj[niy] * m_dZk[niz];
                       cells.back()->getElement()->setVolume(volume);

                       double lCFL(1.e10);
                       if (m_numberCellsX != 1) { lCFL = std::min(lCFL, m_dXi[nix]); }
                       if (m_numberCellsY != 1) { lCFL = std::min(lCFL, m_dYj[niy]); }
                       if (m_numberCellsZ != 1) { lCFL = std::min(lCFL, m_dZk[niz]); }
                       if (m_geometrie > 1) lCFL *= 0.6;

                       cells.back()->getElement()->setLCFL(lCFL);
                       cells.back()->getElement()->setPos(m_posXi[nix], m_posYj[niy], m_posZk[niz]);
                       cells.back()->getElement()->setSize(m_dXi[nix], m_dYj[niy], m_dZk[niz]);

                       //Update pointers cells <-> cell interfaces
                       cellInterfaces.back()->initialize(cells.back(), cells[i]);
                       cells[i]->addCellInterface(cellInterfaces.back());
                       cells.back()->addCellInterface(cellInterfaces.back());
                     }
                     else //Ghost cell exists
                     {
                       //Update parallel communications
                       //Try to find the current cell into the already added cells to send
                       auto cKey = cells[i]->getElement()->getKey();
                       auto it3 = std::find_if(parallel.getElementsToSend(neighbour).begin(), parallel.getElementsToSend(neighbour).end(),
                         [&cKey](Cell* _k0){ 
                         return _k0->getElement()->getKey() == cKey;
                          });
                       if (it3 == parallel.getElementsToSend(neighbour).end()) //Current cell not added in send vector
                       {
                         parallel.addElementToSend(neighbour,cells[i]);
                       }

                       //Update pointers cells <-> cell interfaces
                       cellInterfaces.back()->initialize(*it2, cells[i]);
                       cells[i]->addCellInterface(cellInterfaces.back());
                       (*it2)->addCellInterface(cellInterfaces.back());
                       (*it2)->pushBackSlope();
                       parallel.addSlopesToSend(neighbour);
                       parallel.addSlopesToReceive(neighbour);
                     }
                   }
               } //Negative offset
           } //Offset is an internal cell (ghost or not)
       } //Offsets
   } //Internal, non-ghost cells
   if (Ncpu > 1) {
     for(int i=0;i<Ncpu;++i)
     {
      std::sort(parallel.getElementsToReceive(i).begin(),parallel.getElementsToReceive(i).end(),[&]( Cell* child0, Cell* child1 )
      {
        return child0->getElement()->getKey()< child1->getElement()->getKey();
      });
     }
   }
}

//***********************************************************************

void MeshCartesianAMR::genereTableauxCellsCellInterfacesLvl(TypeMeshContainer<Cell *> &cells, TypeMeshContainer<CellInterface *> &cellInterfaces, std::vector<Cell *> **cellsLvl,
	std::vector<CellInterface *> **cellInterfacesLvl)
{
	(*cellsLvl) = new std::vector<Cell *>[m_lvlMax + 1];
	for (int i = 0; i < m_numberCellsCalcul; i++) { (*cellsLvl)[0].push_back(cells[i]); }

	(*cellInterfacesLvl) = new std::vector<CellInterface *>[m_lvlMax + 1];
	for (int i = 0; i < m_numberFacesTotal; i++) { (*cellInterfacesLvl)[0].push_back(cellInterfaces[i]); }

	m_cellsLvl = cellsLvl;

	if (Ncpu > 1) {
		//Genere les tableaux de cells fantomes par niveau
		m_cellsLvlGhost = new std::vector<Cell *>[m_lvlMax + 1];
		for (int i = m_numberCellsCalcul; i < m_numberCellsTotal; i++) {
			m_cellsLvlGhost[0].push_back(cells[i]);
		}
	}
}

//***********************************************************************

void MeshCartesianAMR::procedureRaffinementInitialization(std::vector<Cell *> *cellsLvl, std::vector<CellInterface *> *cellInterfacesLvl,
  const std::vector<AddPhys*> &addPhys, Model *model, int &nbCellsTotalAMR, std::vector<GeometricalDomain*> &domains,	TypeMeshContainer<Cell *> &cells, Eos **eos, const int &resumeSimulation)
{
  nbCellsTotalAMR = m_numberCellsCalcul;

  if (resumeSimulation == 0) { //Only for simulation from input files
    for (int iterInit = 0; iterInit < 2; iterInit++) {
      for (int lvl = 0; lvl < m_lvlMax; lvl++) {
        if (Ncpu > 1) { parallel.communicationsPrimitivesAMR(eos, lvl); }
        this->procedureRaffinement(cellsLvl, cellInterfacesLvl, lvl, addPhys, model, nbCellsTotalAMR, cells, eos);
        for (unsigned int i = 0; i < cellsLvl[lvl + 1].size(); i++) {
          cellsLvl[lvl + 1][i]->fill(domains, m_lvlMax);
        }
        for (unsigned int i = 0; i < cellsLvl[lvl + 1].size(); i++) {
          cellsLvl[lvl + 1][i]->completeFulfillState();
        }
        for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) {
          cellsLvl[lvl][i]->averageChildrenInParent();
        }
      }
    }
    for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
      if (Ncpu > 1) { parallel.communicationsPrimitivesAMR(eos, lvl); }
      for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) {
        if (!cellsLvl[lvl][i]->getSplit()) { cellsLvl[lvl][i]->completeFulfillState(); }
      }
    }
  }
}

//***********************************************************************

void MeshCartesianAMR::procedureRaffinement(std::vector<Cell *> *cellsLvl, std::vector<CellInterface *> *cellInterfacesLvl, const int &lvl,
  const std::vector<AddPhys*> &addPhys, Model *model, int &nbCellsTotalAMR, TypeMeshContainer<Cell *> &cells, Eos **eos)
{
  //1) Calcul de Xi dans chaque cell de niveau lvl
  //-------------------------------------------------
  for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) { cellsLvl[lvl][i]->setToZeroXi(); }
  for (unsigned int i = 0; i < cellInterfacesLvl[lvl].size(); i++) { cellInterfacesLvl[lvl][i]->computeXi(m_criteriaVar, m_varRho, m_varP, m_varU, m_varAlpha); }
  //bool varP2(true);
  //if (lvl >= 5) { varP2 = false; }
  //for (unsigned int i = 0; i < cellInterfacesLvl[lvl].size(); i++) { cellInterfacesLvl[lvl][i]->computeXi(m_criteriaVar, m_varRho, varP2, m_varU, m_varAlpha); }
  //for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) {
  //  double x(0.), y(0.), z(0.);
  //  x = cellsLvl[lvl][i]->getPosition().getX();
  //  y = cellsLvl[lvl][i]->getPosition().getY();
  //  //z = cellsLvl[lvl][i]->getPosition().getZ();
  //  //if (std::pow((x*x + y*y + z*z), 0.5) > 500.e-6) {
  //  //if (std::pow((x*x + y*y), 0.5) > 6.e-4) {
  //  //if ((x > 250e-6) || (y > 200.e-6)) {
  //  //if (x > 15.) {
  //  if (std::pow((x*x + y * y), 0.5) > 5.) {
  //      cellsLvl[lvl][i]->setToZeroXi();
  //  }
  //}
  if (Ncpu > 1) { parallel.communicationsXi( lvl); }
  
  //2) Smoothing de Xi
  //------------------
  for (int iterDiff = 0; iterDiff < 2; iterDiff++) { //Arbitrary number of iterations
		//Mise a zero cons xi
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) { cellsLvl[lvl][i]->setToZeroConsXi(); }

    //Calcul des "flux"
    for (unsigned int i = 0; i < cellInterfacesLvl[lvl].size(); i++) { cellInterfacesLvl[lvl][i]->computeFluxXi(); }

    //Evolution temporelle
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) { cellsLvl[lvl][i]->timeEvolutionXi(); }
		if (Ncpu > 1) { parallel.communicationsXi( lvl); }
  }

	if (lvl < m_lvlMax) {
    int lvlPlus1 = lvl + 1;
    //3) Raffinement des cells et cell interfaces
    //-------------------------------------------
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) { cellsLvl[lvl][i]->chooseRefine(m_xiSplit, m_numberCellsY, m_numberCellsZ, addPhys, model, nbCellsTotalAMR); }

    //4) Deraffinement des cells et cell interfaces
    //---------------------------------------------
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) { cellsLvl[lvl][i]->chooseUnrefine(m_xiJoin, nbCellsTotalAMR); }

    if (Ncpu > 1) {
      //5) Raffinement et deraffinement des cells fantomes
      //-----------------------------------------------------
      //Communication split + Raffinement et deraffinement des cells fantomes + Reconstruction du tableau de cells fantomes de niveau lvl + 1
      parallel.communicationsSplit(lvl);
      m_cellsLvlGhost[lvlPlus1].clear();
      for (unsigned int i = 0; i < m_cellsLvlGhost[lvl].size(); i++) { m_cellsLvlGhost[lvl][i]->chooseRefineDeraffineGhost(m_numberCellsY, m_numberCellsZ, addPhys, model, m_cellsLvlGhost); }
      //Communications primitives pour mettre a jour les cells deraffinees
      parallel.communicationsPrimitivesAMR(eos, lvl);

      //6) Mise a jour des communications persistantes au niveau lvl + 1
      //----------------------------------------------------------------
      parallel.communicationsNumberGhostCells(lvlPlus1);	//Communication des numbers d'elements a envoyer et a recevoir de chaque cote de la limite parallele
      parallel.updatePersistentCommunicationsLvl(lvlPlus1, m_geometrie);
    }

    //7) Reconstruction des tableaux de cells et cell interfaces lvl + 1
    //------------------------------------------------------------------
    cellsLvl[lvlPlus1].clear();
    cellInterfacesLvl[lvlPlus1].clear();
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) { cellsLvl[lvl][i]->buildLvlCellsAndLvlInternalCellInterfacesArrays(cellsLvl, cellInterfacesLvl); }
    for (unsigned int i = 0; i < cellInterfacesLvl[lvl].size(); i++) { cellInterfacesLvl[lvl][i]->constructionTableauCellInterfacesExternesLvl(cellInterfacesLvl); }
  }
}

//***********************************************************************

std::string MeshCartesianAMR::whoAmI() const
{
  return "CARTESIAN_AMR";
}

//**************************************************************************
//******************************** PRINTING ********************************
//**************************************************************************

void MeshCartesianAMR::ecritHeaderPiece(std::ofstream &fileStream, std::vector<Cell *> *cellsLvl) const
{
  int numberCells = 0, numberPointsParMaille = 4;
  for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) {
      if (!cellsLvl[lvl][i]->getSplit()) { numberCells += 1; }
    }
  }
  if (m_numberCellsZ > 1) { numberPointsParMaille = 8; }

  fileStream << "    <Piece NumberOfPoints=\"" << numberPointsParMaille*numberCells << "\" NumberOfCells=\"" << numberCells << "\">" << std::endl;
}

//***********************************************************************

void MeshCartesianAMR::recupereNoeuds(std::vector<double> &jeuDonnees) const
{
  int dimZ = 0;
  if (m_numberCellsZ > 1) dimZ = 1;

  double dXsur2(0.), dYsur2(0.), dZsur2(0.);
  for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
    dXsur2 = 0.; dYsur2 = 0.; dZsur2 = 0.;
    for (unsigned int i = 0; i < (*m_cellsLvl)[lvl].size(); i++) {
      if (!(*m_cellsLvl)[lvl][i]->getSplit()) {

        dXsur2 = 0.5*(*m_cellsLvl)[lvl][i]->getSizeX();
        dYsur2 = 0.5*(*m_cellsLvl)[lvl][i]->getSizeY();
        dZsur2 = 0.5*(*m_cellsLvl)[lvl][i]->getSizeZ();
        //Point 0
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() - dXsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() - dYsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() - dZsur2*dimZ);
        //Point 1
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() + dXsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() - dYsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() - dZsur2*dimZ);
        //Point 2
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() + dXsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() + dYsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() - dZsur2*dimZ);
        //Point 3
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() - dXsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() + dYsur2);
        jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() - dZsur2*dimZ);

        if (dimZ > 0.99) {
          //Point 4
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() - dXsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() - dYsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() + dZsur2);
          //Point 5
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() + dXsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() - dYsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() + dZsur2);
          //Point 6
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() + dXsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() + dYsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() + dZsur2);
          //Point 7
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getX() - dXsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getY() + dYsur2);
          jeuDonnees.push_back((*m_cellsLvl)[lvl][i]->getPosition().getZ() + dZsur2);
        }
      } //Fin cell non split
    } //Fin Cells
  } //Fin Levels
}

//***********************************************************************

void MeshCartesianAMR::recupereConnectivite(std::vector<double> &jeuDonnees) const
{
  int dimZ(0);
  int numberPointsParMaille(4);
  if (m_numberCellsZ > 1) { dimZ = 1; numberPointsParMaille = 8; }

  if (dimZ < 0.99) {
    int numCell(0);
    for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
      for (unsigned int i = 0; i < (*m_cellsLvl)[lvl].size(); i++) {
        if (!(*m_cellsLvl)[lvl][i]->getSplit()) {
          jeuDonnees.push_back(numCell*numberPointsParMaille);
          jeuDonnees.push_back(numCell*numberPointsParMaille+1);
          jeuDonnees.push_back(numCell*numberPointsParMaille+2);
          jeuDonnees.push_back(numCell*numberPointsParMaille+3);
          numCell++;
        }
      }
    }
  }
  else {
    int numCell(0);
    for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
      for (unsigned int i = 0; i < (*m_cellsLvl)[lvl].size(); i++) {
        if (!(*m_cellsLvl)[lvl][i]->getSplit()) {
          jeuDonnees.push_back(numCell*numberPointsParMaille);
          jeuDonnees.push_back(numCell*numberPointsParMaille + 1);
          jeuDonnees.push_back(numCell*numberPointsParMaille + 2);
          jeuDonnees.push_back(numCell*numberPointsParMaille + 3);
          jeuDonnees.push_back(numCell*numberPointsParMaille + 4);
          jeuDonnees.push_back(numCell*numberPointsParMaille + 5);
          jeuDonnees.push_back(numCell*numberPointsParMaille + 6);
          jeuDonnees.push_back(numCell*numberPointsParMaille + 7);
          numCell++;
        }
      }
    }
  }
}

//***********************************************************************

void MeshCartesianAMR::recupereOffsets(std::vector<double> &jeuDonnees) const
{
  int numberPointsParMaille(4);
  if (m_numberCellsZ > 1) { numberPointsParMaille = 8; }
  int numCell(0);
  for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
    for (unsigned int i = 0; i < (*m_cellsLvl)[lvl].size(); i++) {
      if (!(*m_cellsLvl)[lvl][i]->getSplit()) {
        jeuDonnees.push_back((numCell + 1)*numberPointsParMaille);
        numCell++;
      }
    }
  }
}

//****************************************************************************

void MeshCartesianAMR::recupereTypeCell(std::vector<double> &jeuDonnees) const
{
  int type(9);
  if (m_numberCellsZ > 1) { type = 12; }
  int numCell(0);
  for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
    for (unsigned int i = 0; i < (*m_cellsLvl)[lvl].size(); i++) {
      if (!(*m_cellsLvl)[lvl][i]->getSplit()) {
        jeuDonnees.push_back(type);
        numCell++;
      }
    }
  }
}

//***********************************************************************

void MeshCartesianAMR::recupereDonnees(std::vector<Cell *> *cellsLvl, std::vector<double> &jeuDonnees, const int var, int phase) const
{
  jeuDonnees.clear();
  for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) {
      if (!cellsLvl[lvl][i]->getSplit()) {
        if (var > 0) { //On veut recuperer les donnees scalars
          if (phase >= 0) { jeuDonnees.push_back(cellsLvl[lvl][i]->getPhase(phase)->returnScalar(var)); }      //Donnees de phases
          else if (phase == -1) { jeuDonnees.push_back(cellsLvl[lvl][i]->getMixture()->returnScalar(var)); }   //Donnees de mixture
          else if (phase == -2) { jeuDonnees.push_back(cellsLvl[lvl][i]->getTransport(var - 1).getValue()); }
          else if (phase == -3) { jeuDonnees.push_back(cellsLvl[lvl][i]->getXi()); }
          else if (phase == -4) { jeuDonnees.push_back(cellsLvl[lvl][i]->getGradient()); }
          else { Errors::errorMessage("MeshCartesianAMR::recupereDonnees: unknown number of phase: ", phase); }
        }
        else { //On veut recuperer les donnees vectorielles
          if (phase >= 0) { //Donnees de phases
            jeuDonnees.push_back(cellsLvl[lvl][i]->getPhase(phase)->returnVector(-var).getX());
            jeuDonnees.push_back(cellsLvl[lvl][i]->getPhase(phase)->returnVector(-var).getY());
            jeuDonnees.push_back(cellsLvl[lvl][i]->getPhase(phase)->returnVector(-var).getZ());
          }
          else if(phase == -1){  //Donnees de mixture
            jeuDonnees.push_back(cellsLvl[lvl][i]->getMixture()->returnVector(-var).getX());
            jeuDonnees.push_back(cellsLvl[lvl][i]->getMixture()->returnVector(-var).getY());
            jeuDonnees.push_back(cellsLvl[lvl][i]->getMixture()->returnVector(-var).getZ());
          }
          else { Errors::errorMessage("MeshCartesianAMR::recupereDonnees: unknown number of phase: ", phase); }
        } //Fin vecteur
      } //Fin split
    } //fin lvl
  } //fin levels
}

//****************************************************************************

void MeshCartesianAMR::setDataSet(std::vector<double> &jeuDonnees, std::vector<Cell *> *cellsLvl, const int var, int phase) const
{
  int iterDataSet(0);
  Coord vec;
  for (int lvl = 0; lvl <= m_lvlMax; lvl++) {
    for (unsigned int i = 0; i < cellsLvl[lvl].size(); i++) {
      if (!cellsLvl[lvl][i]->getSplit()) {
        if (var > 0) { //Scalars data are first set
          if (phase >= 0) { cellsLvl[lvl][i]->getPhase(phase)->setScalar(var, jeuDonnees[iterDataSet++]); } //phases data
          else if (phase == -1) { cellsLvl[lvl][i]->getMixture()->setScalar(var, jeuDonnees[iterDataSet++]); }  //mixture data
          else if (phase == -2) { cellsLvl[lvl][i]->getTransport(var - 1).setValue(jeuDonnees[iterDataSet++]); } //transport data
          else if (phase == -3) { cellsLvl[lvl][i]->setXi(jeuDonnees[iterDataSet++]); } //xi indicator
          else { Errors::errorMessage("MeshCartesianAMR::setDataSet: unknown phase number: ", phase); }
        }
        else { //On veut recuperer les donnees vectorielles
          if (phase >= 0) { //Phases data
            vec.setXYZ(jeuDonnees[iterDataSet], jeuDonnees[iterDataSet + 1], jeuDonnees[iterDataSet + 2]);
            cellsLvl[lvl][i]->getPhase(phase)->setVector(-var, vec);
            iterDataSet += 3;
          }
          else if (phase == -1) {  //Mixture data
            vec.setXYZ(jeuDonnees[iterDataSet], jeuDonnees[iterDataSet + 1], jeuDonnees[iterDataSet + 2]);
            cellsLvl[lvl][i]->getMixture()->setVector(-var, vec);
            iterDataSet += 3;
          }
          else { Errors::errorMessage("MeshCartesianAMR::setDataSet: unknown phase number: ", phase); }
        } //Fin vecteur
      } // Fin split
    } // Fin lvl
  } // Fin levels
}

//***********************************************************************

void MeshCartesianAMR::refineCell(Cell *cell, const std::vector<AddPhys*> &addPhys, Model *model, int &nbCellsTotalAMR)
{
  cell->refineCellAndCellInterfaces(m_numberCellsY, m_numberCellsZ, addPhys, model);
  nbCellsTotalAMR += cell->getNumberCellsChildren() - 1;
}

//****************************************************************************
//****************************** Parallele ***********************************
//****************************************************************************

void MeshCartesianAMR::initializePersistentCommunications(const int numberPhases, const int numberTransports, const TypeMeshContainer<Cell *> &cells, std::string ordreCalcul)
{
	m_numberPhases = numberPhases;
	m_numberTransports = numberTransports;
	int numberVariablesPhaseATransmettre = cells[0]->getPhase(0)->numberOfTransmittedVariables();
	numberVariablesPhaseATransmettre *= m_numberPhases;
	int numberVariablesMixtureATransmettre = cells[0]->getMixture()->numberOfTransmittedVariables();
	int m_numberPrimitiveVariables = numberVariablesPhaseATransmettre + numberVariablesMixtureATransmettre + m_numberTransports;
  int m_numberSlopeVariables(0);
  if (ordreCalcul == "SECONDORDER") {
    int numberSlopesPhaseATransmettre = cells[0]->getPhase(0)->numberOfTransmittedSlopes();
    numberSlopesPhaseATransmettre *= m_numberPhases;
    int numberSlopesMixtureATransmettre = cells[0]->getMixture()->numberOfTransmittedSlopes();
    m_numberSlopeVariables = numberSlopesPhaseATransmettre + numberSlopesMixtureATransmettre + m_numberTransports + 1 + 1; //+1 for the interface detection + 1 for slope index
  }
	parallel.initializePersistentCommunicationsAMR(m_numberPrimitiveVariables, m_numberSlopeVariables, m_numberTransports, m_geometrie, m_lvlMax);
}

//***********************************************************************

void MeshCartesianAMR::communicationsPrimitives(Eos **eos, const int &lvl, Prim type)
{
	parallel.communicationsPrimitivesAMR(eos, lvl, type);
}

//***********************************************************************

void MeshCartesianAMR::communicationsVector(std::string nameVector, const int &dim, const int &lvl, int num, int index)
{
	parallel.communicationsVectorAMR(nameVector, m_geometrie, lvl, num, index);
}

//***********************************************************************

void MeshCartesianAMR::communicationsAddPhys(const std::vector<AddPhys*> &addPhys,  const int &lvl)
{
	for (unsigned int pa = 0; pa < addPhys.size(); pa++) { addPhys[pa]->communicationsAddPhysAMR(m_numberPhases, m_geometrie, lvl); }
}

//***********************************************************************

void MeshCartesianAMR::communicationsTransports(const int &lvl)
{
  parallel.communicationsTransportsAMR( lvl);
}

//***********************************************************************

void MeshCartesianAMR::finalizeParallele(const int &lvlMax)
{
	parallel.finalizeAMR(lvlMax);
}

//***********************************************************************
