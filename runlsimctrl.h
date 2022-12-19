/*
--------------------------------------------------------------------------
--
--        SIMCTRL: RUN LENGTH CONTROL IN STEADY-STATE SIMULATIONS
--
--------------------------------------------------------------------------
--
-- File simctrl.h Version 3.2 Date 6/29/93
--
-- Manuel Alvarez-Campana Fernandez-Corredor
-- Pedro Luis Sierra Hernando
-- Departamento de Ingenieria de Sistemas Telematicos
-- ETS Ingenieros de Telecomunicacion
-- Ciudad Universitaria s/n
-- E-28040 Madrid SPAIN
--
-- tel: +34 1 549 57 00 x.366 - fax: +34 1 243 20 77 - tlx: 47430 ETSIT E
-- E-mail: mac@dit.upm.es
--
--------------------------------------------------------------------------
--
-- The main header file for SIMCTRL package. To be included by any program
-- using the package.
--
--------------------------------------------------------------------------
*/
#ifndef SIMCTRL_H
#define SIMCTRL_H

#define N_MAX     10000000L            /* max. length of simulation run */
#define SPECTRAL         0
#define BATCH_MEANS      1

typedef void *SIMCTRL;

extern SIMCTRL
InitBatchMeansSimControl( unsigned long    n_max, unsigned long    n0_max, double           a_t, double           g_v, double           g_e, double           alpha, double           e_max);

extern SIMCTRL
InitSpectralSimControl( unsigned long    n_max, unsigned long    n0_max, double           a_t, double           g_v, double           g_e, double           g_a, double           alpha, double           e_max);


#define SIMCTRLERR     -2       /* initial transient period too long */
#define SIMCTRLLON     -1       /* simulation too long */
#define SIMCTRLEND      0       /* simulation may be stopped */
#define SIMCTRLCON      1       /* simulation control continues */

extern int
SimControl( SIMCTRL sim, double  val);


extern void
GetSimControlResults( SIMCTRL        sim, unsigned long *ntransient, unsigned long *nsteady, double        *mean, double        *cl, double        *ep,
     unsigned long *par1,     unsigned long *par2,     unsigned long *batch_s);
/* batch means: par1=step, par2=n_batch
   spectral:    par1=last checkp, par2=next_checkp
*/

extern int
FinishedSimControl();

#endif



