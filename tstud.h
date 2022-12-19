/*
--------------------------------------------------------------------------
--
--        SIMCTRL: RUN LENGTH CONTROL IN STEADY-STATE SIMULATIONS
--
--------------------------------------------------------------------------
--
-- File tstud.h Version 3.2 Date 6/29/93
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
-- T-Student and Normal distributions upper critical points (1-alpha/2)
--  for 'n' degrees of freedom.
--
--------------------------------------------------------------------------
*/
#ifndef SIMCTRL_TSTUD_H
#define SIMCTRL_TSTUD_H

extern double
t_student(
	 unsigned degrees,
	 double   ialfa,
	 double  *oalfa);

#endif
