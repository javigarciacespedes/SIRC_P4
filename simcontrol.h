#ifndef SIMCONTROL_H
#define SIMCONTROL_H

#include "runlsimctrl.h"
#include <string>

void 	LeerParametrosSim ();
// argc>2
//	los valores de parámetros son
//	transit=argv[1] duracionmáx=argv[2] semilla=argv[3] trazado=argv[4][0]
// 	(si faltan argumentos en línea de comando, se leen desde teclado)
// argc=2
//	lee valores del fichero de parámetros argv[1] en el mismo orden que
//  en el caso anterior
//  ejemplo de formato del fichero:
//		#texto ignorado
//		texto ignorado:      123
//		-2.5
//		mas texto ignorado: 55
//		si
//		...
// 		(si faltan parámetros en el fichero, se leen desde teclado)
// argc=1
//	sin argumentos: lee valores de parámetros desde teclado

string  LeerLinea (string txt);
// lee la siguiente linea de fichero de parámetros o el siguiente argumento
// de línea de comando si hay, ignorando líneas que empiezan con #
// y texto desde principio de linea hasta ':' y espacios siguientes
// si no hay, imprime el mensaje txt y lee de teclado
//
// las siguientes funciones usan la anterior y
//extraen un valor double, long,...
double	LeerDouble (string txt);
long	LeerLong (string txt);
int	    LeerInt (string txt);
int		LeerSiNo (string txt);
// si primer caracter es 'S' o 's' devuelve 1 (si), si no devuelve 0 (no)

double 	Transitorio ();
// duración de transitorio inicial (previa a cálculos de intervalos de
// confianza que pueden incluir a su vez periodos transitorios adicionales)

double 	Duracion ();
// duración de la simulación (se pone al máximo indicado al lanzar la
// simulación, pero puede reducirse si todos los intervalos de confianza
// se consiguen antes)

unsigned long 	Semilla ();

void Trazar (int num, string txt1, double v1);

void Trazar (int num, string txt1, double v1, string txt2, double v2);

void MarcarTinic ();
// usado para calcular cuanto tarda la simulacion

void ImprimirDatosExperim ();

// cálculo de intervalo de confianza con método Batch Means o Spectral

//define SPECTRAL         0
//define BATCH_MEANS      1
class CI_VAR {
	SIMCTRL V;
	public:
		// delta como tanto por uno de la media
		CI_VAR (double alfa, double delta, int metodo=BATCH_MEANS);
		void REGISTRAR (double v);
		void IMPRIMIR_CI (string txt);
};

#endif
