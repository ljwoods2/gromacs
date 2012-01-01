/* -*- mode: c; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; c-file-style: "stroustrup"; -*-
 *
 *
 *                This source code is part of
 *
 *                 G   R   O   M   A   C   S
 *
 * Copyright (c) 1991-2000, University of Groningen, The Netherlands.
 * Copyright (c) 2001-2009, The GROMACS Development Team
 *
 * Gromacs is a library for molecular simulation and trajectory analysis,
 * written by Erik Lindahl, David van der Spoel, Berk Hess, and others - for
 * a full list of developers and information, check out http://www.gromacs.org
 *
 * This program is free software; you can redistribute it and/or modify it under 
 * the terms of the GNU Lesser General Public License as published by the Free 
 * Software Foundation; either version 2 of the License, or (at your option) any 
 * later version.
 * As a special exception, you may use this file as part of a free software
 * library without restriction.  Specifically, if other files instantiate
 * templates or use macros or inline functions from this file, or you compile
 * this file and link it with other files to produce an executable, this
 * file does not by itself cause the resulting executable to be covered by
 * the GNU Lesser General Public License.  
 *
 * In plain-speak: do not worry about classes/macros/templates either - only
 * changes to the library have to be LGPL, not an application linking with it.
 *
 * To help fund GROMACS development, we humbly ask that you cite
 * the papers people have written on it - you can find them on the website!
 */


/* When calculating RF or Ewald interactions we calculate the electrostatic
 * forces on excluded atom pairs here in the non-bonded loops.
 * But when energies and/or virial is required we calculate them
 * separately to as then it is easier to separate the energy and virial
 * contributions.
 */
#if defined CHECK_EXCLS && defined CALC_COULOMB
#define EXCL_FORCES
#endif

        {
            int cj;
#ifdef ENERGY_GROUPS
            int egp_cj;
#endif
            int i;

            cj = l_cj[cjind].c;

#ifdef ENERGY_GROUPS
            egp_cj = nbat->energrp[cj];
#endif
            for(i=0; i<UNROLLI; i++)
            {
                int ai;
                int type_i_off;
                int j;

                ai = ci*UNROLLI + i;

                type_i_off = type[ai]*ntype2;

                for(j=0; j<UNROLLJ; j++)
                {
                    int  aj;
                    real dx,dy,dz;
                    real rsq,rinv;
                    real rinvsq,rinvsix;
                    real Vvdw6=0,Vvdw12=0;
#ifdef CALC_COULOMB
                    real qq;
                    real fcoul;
#ifndef CALC_COUL_RF
                    real rs,frac;
                    int  ri;
                    real fexcl;
#endif
#ifdef CALC_ENERGIES
                    real vcoul;
#endif
#endif
                    real fx,fy,fz;

#ifdef CHECK_EXCLS
                    int interact;

                    interact = ((l_cj[cjind].excl>>(i*UNROLLI + j)) & 1);
#ifndef EXCL_FORCES
                    /* Remove all exclused atom pairs from the list */
                    if (interact == 0)
                    {
                        continue;
                    }
#else
                    /* Remove the (sub-)diagonal to avoid double counting */
                    if (cj == ci_sh && j <= i)
                    {
                        continue;
                    }
#endif
#else
#define interact 1
#endif

                    aj = cj*UNROLLJ + j;

                    dx  = xi[i*3+0] - x[aj*3+0];
                    dy  = xi[i*3+1] - x[aj*3+1];
                    dz  = xi[i*3+2] - x[aj*3+2];

                    rsq = dx*dx + dy*dy + dz*dz;

                    if (rsq >= rcut2)
                    {
                        continue;
                    }
#ifdef EXCL_FORCES
                    /* Avoid overflow of rinvsix */
                    if (rsq < 1e-12)
                    {
                        continue;
                    }
#endif

#ifdef COUNT_PAIRS
                    npair++;
#endif

                    rinv = gmx_invsqrt(rsq);

                    rinvsq  = rinv*rinv;

#ifdef HALF_LJ
                    if (i < UNROLLI/2)
#endif
                    {
                        rinvsix = interact*rinvsq*rinvsq*rinvsq;

                        Vvdw6   = nbfp[type_i_off+type[aj]*2  ]*rinvsix;
                        Vvdw12  = nbfp[type_i_off+type[aj]*2+1]*rinvsix*rinvsix;
#ifdef CALC_ENERGIES
#ifdef ENERGY_GROUPS
                        Vvdw[egp_sh_i[i]+((egp_cj>>(8*j)) & 255)] +=
                            Vvdw12/12 - Vvdw6/6;
#else
                        Vvdw_ci += Vvdw12/12 - Vvdw6/6;
#endif
#endif
                    }

#ifdef CALC_COULOMB
                    qq = qi[i]*q[aj];

#ifdef  CALC_COUL_RF
                    fcoul  = qq*(interact*rinv*rinvsq - k_rf2);
#ifdef CALC_ENERGIES
                    vcoul  = qq*(interact*rinv + k_rf*rsq - c_rf);
#endif
#else
                    rs     = rsq*rinv*ic->tabq_scale;
                    ri     = (int)rs;
                    frac   = rs - ri;
                    fexcl  = tab_coul_FDV0[ri*4] + frac*tab_coul_FDV0[ri*4+1];
                    fcoul  = interact*rinvsq - fexcl;
#ifdef CALC_ENERGIES
                    vcoul  = qq*(interact*rinv
                                 -(tab_coul_FDV0[ri*4+2]
                                   -halfsp*frac*(tab_coul_FDV0[ri*4] + fexcl)));
#endif
                    fcoul *= qq*rinv;
#endif

#ifdef CALC_ENERGIES
#ifdef ENERGY_GROUPS
                    //printf("egp = %3d + %3d\n",egp_sh_i[i],((egp_cj>>(8*j)) & 255));
                    Vc[egp_sh_i[i]+((egp_cj>>(8*j)) & 255)] += vcoul;
#else
                    Vc_ci += vcoul;
#endif
#endif
#endif

#ifdef CALC_COULOMB
#ifdef HALF_LJ
                    if (i < UNROLLI/2)
#endif
                    {
                        fx = ((Vvdw12 - Vvdw6)*rinvsq + fcoul)*dx;
                        fy = ((Vvdw12 - Vvdw6)*rinvsq + fcoul)*dy;
                        fz = ((Vvdw12 - Vvdw6)*rinvsq + fcoul)*dz;
                    }
#ifdef HALF_LJ
                    else
                    {
                        fx = fcoul*dx;
                        fy = fcoul*dy;
                        fz = fcoul*dz;
                    }
#endif
#else
                    fx = (Vvdw12 - Vvdw6)*rinvsq*dx;
                    fy = (Vvdw12 - Vvdw6)*rinvsq*dy;
                    fz = (Vvdw12 - Vvdw6)*rinvsq*dz;
#endif

                    /* Increment i-atom force */
                    fi[i*3+0] += fx;
                    fi[i*3+1] += fy;
                    fi[i*3+2] += fz;
                    /* Decrement j-atom force */
                    f[aj*3+0] -= fx;
                    f[aj*3+1] -= fy;
                    f[aj*3+2] -= fz;
                }
            }
        }

#undef interact
#undef EXCL_FORCES
