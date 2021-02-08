/*
 * This file is part of the GROMACS molecular simulation package.
 *
 * Copyright (c) 2021, by the GROMACS development team, led by
 * Mark Abraham, David van der Spoel, Berk Hess, and Erik Lindahl,
 * and including many others, as listed in the AUTHORS file in the
 * top-level source directory and at http://www.gromacs.org.
 *
 * GROMACS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1
 * of the License, or (at your option) any later version.
 *
 * GROMACS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with GROMACS; if not, see
 * http://www.gnu.org/licenses, or write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA.
 *
 * If you want to redistribute modifications to GROMACS, please
 * consider that scientific software is very special. Version
 * control is crucial - bugs must be traceable. We will be happy to
 * consider code for inclusion in the official distribution, but
 * derived work must not be called official GROMACS. Details are found
 * in the README & COPYING files - if they are missing, get the
 * official version at http://www.gromacs.org.
 *
 * To help us fund GROMACS development, we humbly ask that you cite
 * the research papers on the package. Check out http://www.gromacs.org.
 */
/*! \internal \file
 *
 * \brief
 * Declares the free-energy kernel dispatch class
 *
 * \author Berk Hess <hess@kth.se>
 * \ingroup module_nbnxm
 */
#ifndef GMX_NBNXM_FREEENERGYDISPATCH_H
#define GMX_NBNXM_FREEENERGYDISPATCH_H

#include <memory>

#include "gromacs/math/vectypes.h"
#include "gromacs/mdtypes/enerdata.h"
#include "gromacs/mdtypes/threaded_force_buffer.h"
#include "gromacs/utility/arrayref.h"

struct gmx_enerdata_t;
struct gmx_wallcycle;
struct interaction_const_t;
class PairlistSets;
struct t_lambda;
struct t_nrnb;

namespace gmx
{
template<typename>
class ArrayRefWithPadding;
class ForceWithShiftForces;
class StepWorkload;
} // namespace gmx

/*! \libinternal
 *  \brief Temporary data and methods for handling dispatching of the nbnxm free-energy kernels
 */
class FreeEnergyDispatch
{
public:
    //! Constructor
    FreeEnergyDispatch(int numEnergyGroups);

    //! Sets up the threaded force buffer and the reduction, should be called after constructing the pair lists
    void setupFepThreadedForceBuffer(int numAtomsForce, const PairlistSets& pairlistSets);

    //! Dispatches the non-bonded free-energy kernels, thread parallel and reduces the output
    void dispatchFreeEnergyKernels(const PairlistSets&                              pairlistSets,
                                   const gmx::ArrayRefWithPadding<const gmx::RVec>& coords,
                                   gmx::ForceWithShiftForces*     forceWithShiftForces,
                                   bool                           useSimd,
                                   int                            ntype,
                                   real                           rlist,
                                   const interaction_const_t&     ic,
                                   gmx::ArrayRef<const gmx::RVec> shiftvec,
                                   gmx::ArrayRef<const real>      nbfp,
                                   gmx::ArrayRef<const real>      nbfp_grid,
                                   gmx::ArrayRef<const real>      chargeA,
                                   gmx::ArrayRef<const real>      chargeB,
                                   gmx::ArrayRef<const int>       typeA,
                                   gmx::ArrayRef<const int>       typeB,
                                   t_lambda*                      fepvals,
                                   gmx::ArrayRef<const real>      lambda,
                                   gmx_enerdata_t*                enerd,
                                   const gmx::StepWorkload&       stepWork,
                                   t_nrnb*                        nrnb,
                                   gmx_wallcycle*                 wcycle);

private:
    //! Temporary array for storing foreign lambda group pair energies
    gmx_grppairener_t foreignGroupPairEnergies_;

    //! Threaded force buffer for nonbonded FEP
    gmx::ThreadedForceBuffer<gmx::RVec> threadedForceBuffer_;
    //! Threaded buffer for nonbonded FEP foreign energies and dVdl, no forces, so numAtoms = 0
    gmx::ThreadedForceBuffer<gmx::RVec> threadedForeignEnergyBuffer_;
};

#endif // GMX_NBNXN_FREEENERGYDISPATCH_H
