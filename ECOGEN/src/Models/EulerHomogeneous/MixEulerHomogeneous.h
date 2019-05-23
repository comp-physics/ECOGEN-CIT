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

#ifndef MIXEULERHOMOGENEOUSOUS_H
#define MIXEULERHOMOGENEOUSOUS_H

//! \file      MixEulerHomogeneous.h
//! \author    K. Schmidmayer, F. Petitpas
//! \version   1.0
//! \date      December 19 2017

#include <vector>
#include "../Mixture.h"

//! \class     MixEulerHomogeneous
//! \brief     Mixture variables for Euler Homogeneous equations (velocity and thermodynamical equilibrium)
class MixEulerHomogeneous : public Mixture
{
public:
  MixEulerHomogeneous();
  //! \brief     Mixture constructor from a XML format reading
  //! \details   Reading data from XML file under the following format:
  //!           ex: <mixture>
  //!                 <dataMix pressure = "1.e5">
  //!                 <velocity x = "0." y = "0." z = "0." />
  //!               </mixture>
  //! \param     material       XML element to read for mixture data
  //! \param     fileName       string name of readed XML file
  MixEulerHomogeneous(tinyxml2::XMLElement *state, std::string fileName);
  virtual ~MixEulerHomogeneous();

  virtual void allocateAndCopyMixture(Mixture **mixture);
  virtual void copyMixture(Mixture &mixture);
  virtual double computeDensity(const double *alphak, const double *rhok, const int &numberPhases);
  virtual double computePressure(const double *alphak, const double *pk, const int &numberPhases);
  virtual double computeInternalEnergy(const double *Yk, const double *ek, const int &numberPhases);
  virtual double computeFrozenSoundSpeed(const double *Yk, const double *ck, const int &numberPhases);

  virtual double computePressure(double masse, const double &internalEnergy, Phase **phases, Mixture *mixture, const int &numberPhases, const int &liq, const int &vap);

  virtual void computeMixtureVariables(Phase **vecPhase, const int &numberPhases);
  virtual void internalEnergyToTotalEnergy(std::vector<QuantitiesAddPhys*> &vecGPA);
  virtual void totalEnergyToInternalEnergy(std::vector<QuantitiesAddPhys*> &vecGPA);

  virtual void localProjection(const Coord &normal, const Coord &tangent, const Coord &binormal);
  virtual void reverseProjection(const Coord &normal, const Coord &tangent, const Coord &binormal);

  //Specific methods for data printing
  //----------------------------------
  virtual int getNumberScalars() const { return 3; };
  virtual int getNumberVectors() const { return 1; };
  virtual double returnScalar(const int &numVar) const;
  virtual Coord returnVector(const int &numVar) const;
  virtual std::string returnNameScalar(const int &numVar) const;
  virtual std::string returnNameVector(const int &numVar) const;

  //Data reading
  virtual void setScalar(const int &numVar, const double &value);
  virtual void setVector(const int &numVar, const Coord &value);

  //Specific methods for parallel computing
  //---------------------------------------
  virtual int numberOfTransmittedVariables() const;
  virtual void fillBuffer(double *buffer, int &counter) const;
  virtual void fillBuffer(std::vector<double> &dataToSend) const;
  virtual void getBuffer(double *buffer, int &counter);
  virtual void getBuffer(std::vector<double> &dataToReceive, int &counter);

  //Specific methods for second order
  //---------------------------------
  virtual void computeSlopesMixture(const Mixture &sLeft, const Mixture &sRight, const double &distance);
  virtual void setToZero();
  virtual void extrapolate(const Mixture &slope, const double &distance);
  virtual void limitSlopes(const Mixture &slopeGauche, const Mixture &slopeDroite, Limiter &globalLimiter);

  //Specific methods for parallele computing at second order
  //--------------------------------------------------------
	virtual int numberOfTransmittedSlopes() const;
	virtual void fillBufferSlopes(double *buffer, int &counter) const;
	virtual void getBufferSlopes(double *buffer, int &counter);

  //Accessors
  //---------
  virtual const double& getDensity() const { return m_density; };
  virtual const double& getPressure() const { return m_pressure; };
  virtual const double& getU() const { return m_velocity.getX(); };
  virtual const double& getV() const { return m_velocity.getY(); };
  virtual const double& getW() const { return m_velocity.getZ(); };
  virtual const Coord& getVelocity() const { return m_velocity; };
  //virtual Coord getVelocity() const { return m_velocity; }; //KS//BD//
  virtual const double& getEnergy() const { return m_energie; };
  virtual const double& getTotalEnergy() const { return m_totalEnergy; };
  virtual const double& getMixSoundSpeed() const { return m_EqSoundSpeed; };

  virtual void setPressure(const double &p);
  virtual void setTemperature(const double &T);
  virtual void setVelocity(const double &u, const double &v, const double &w);
  virtual void setVelocity(const Coord &vit);
  virtual void setU(const double &u);
  virtual void setV(const double &v);
  virtual void setW(const double &w);
  virtual void setTotalEnergy(double &totalEnergy);

  //Operators
  //---------
  virtual void changeSign();
  virtual void multiplyAndAdd(const Mixture &slopesMixtureTemp, const double &coeff);
  virtual void divide(const double &coeff);

protected:
private:
  double m_density;              //!< mixture density
  double m_pressure;             //!< mixture pressure
  double m_temperature;          //!< mixture temperature
  Coord m_velocity;              //!< mixture velocity
  double m_energie;              //!< mixture internal specific energy
  double m_totalEnergy;          //!< mixture total specific energy
  double m_EqSoundSpeed;         //!< thermodynamical equilibrium sound speed
};

#endif // MIXEULERHOMOGENEOUSOUS_H
