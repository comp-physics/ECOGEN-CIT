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

#ifndef MESHUNSTRUCT_H
#define MESHUNSTRUCT_H

//! \file      MeshUnStruct.h
//! \author    F. Petitpas, K. Schmidmayer
//! \version   1.1
//! \date      June 5 2019

#include <stdint.h>

#include "Mesh.h"
#include "MeshUnStruct/HeaderElements.h"
#include "../InputOutput/IO.h"

class MeshUnStruct : public Mesh
{
public:
  MeshUnStruct(const std::string &fichierMesh);
  ~MeshUnStruct();

  virtual void attributLimites(std::vector<BoundCond*> &boundCond);
  virtual int initializeGeometrie(TypeMeshContainer<Cell *> &cells, TypeMeshContainer<Cell *> &cellsGhost, TypeMeshContainer<CellInterface *> &cellInterfaces,
    const int &restartSimulation, bool pretraitementParallele = true, std::string ordreCalcul = "FIRSTORDER");
  virtual std::string whoAmI() const { return 0; };

  //Printing / Reading
  virtual void ecritHeaderPiece(std::ofstream &fileStream, TypeMeshContainer<Cell *> *cellsLvl) const;
  virtual void recupereNoeuds(std::vector<double> &jeuDonnees, std::vector<Cell *> *cellsLvl) const;
  virtual void recupereConnectivite(std::vector<double> &jeuDonnees, std::vector<Cell *> *cellsLvl) const;
  virtual void recupereOffsets(std::vector<double> &jeuDonnees, std::vector<Cell *> *cellsLvl) const;
  virtual void recupereTypeCell(std::vector<double> &jeuDonnees, std::vector<Cell *> *cellsLvl) const;
  virtual void recupereDonnees(TypeMeshContainer<Cell *> *cellsLvl, std::vector<double> &jeuDonnees, const int var, int phase) const;
  virtual void setDataSet(std::vector<double> &jeuDonnees, TypeMeshContainer<Cell *> *cellsLvl, const int var, int phase) const;
  virtual void extractAbsVeloxityMRF(TypeMeshContainer<Cell *> *cellsLvl, std::vector<double> &jeuDonnees, Source *sourceMRF) const;

private:
  void initializeGeometrieMonoCPU(TypeMeshContainer<Cell *> &cells, TypeMeshContainer<CellInterface *> &cellInterfaces, std::string ordreCalcul);
  void initializeGeometrieParallele(TypeMeshContainer<Cell *> &cells, TypeMeshContainer<Cell *> &cellsGhost, TypeMeshContainer<CellInterface *> &cellInterfaces, std::string ordreCalcul);
  void pretraitementFichierMeshGmsh();
  void lectureGeometrieGmsh(std::vector<ElementNS*>** voisinsNoeuds);
  void readGmshV2(std::vector<ElementNS*>** voisinsNoeuds, std::ifstream &meshFile);
  void readGmshV4(std::vector<ElementNS*>** voisinsNoeuds, std::ifstream &meshFile);
  void lectureGeometrieGmshParallele();
  void lectureElementGmshV2(const Coord *TableauNoeuds, std::ifstream &fichierMesh, ElementNS **element);
  void lectureElementGmshV4(const Coord *TableauNoeuds, std::ifstream &fichierMesh, ElementNS **element, const int &typeElement, int &indiceElement, const int & physicalEntity);

  void rechercheElementsArrieres(ElementNS *element, FaceNS *face, CellInterface *cellInterface, std::vector<ElementNS *> voisins, Cell **cells) const;
  void rechercheElementsAvants(ElementNS *element, FaceNS *face, CellInterface *cellInterface, std::vector<ElementNS *> voisins, Cell **cells) const;

  std::string m_fichierMesh;  /*name du file de mesh lu*/
  std::string m_nameMesh;

  int m_numberNoeuds;               /*number de noeuds definissant le domain geometrique*/
  int m_numberNoeudsInternes;       /*number de noeuds interne (hors fantomes)*/
  Coord *m_noeuds;                  /*Tableau des coordinates des noeuds du domain geometrique*/
  int m_numberElementsInternes;     /*Number d'elements de dimension n de compute internes */
  int m_numberElementsFantomes;     /*Number d'elements fantomes de dimension n pour compute parallele */
  int m_numberElementsCommunicants; /*Number reel d'elements communicants*/
  ElementNS **m_elements;           /*Tableau des elements geometriques internes*/
  FaceNS **m_faces;                 /*Tableau des face geometriques*/
  std::vector<BoundCond*> m_lim;      /*Tableau des conditions aux limites*/

  int m_numberFacesInternes;        /*number de faces entre deux cells de compute*/
  int m_numberFacesLimites;         /*number de faces entre une cell de compute et une limite*/
  int m_numberFacesParallele;       /*number de faces entre une cell de compute et une ghost cell*/

  int m_numberCellsFantomes;     /*number de cells fantomes*/

  int m_numberElements0D;
  int m_numberElements1D;
  int m_numberElements2D;
  int m_numberElements3D;
  int m_numberSegments;
  int m_numberTriangles;
  int m_numberQuadrangles;
  int m_numberTetrahedrons;
  int m_numberPyramids;
  int m_numberPoints;
  int m_numberHexahedrons;

  //statistics
  double m_totalSurface;    //!< sum of 2D element surfaces
  double m_totalVolume;     //!< sum of 3D element volumes
};

#endif // MESHUNSTRUCT_H