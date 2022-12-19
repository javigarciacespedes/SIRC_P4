// Manejo integrado de eventos, procesos y recursos
// según primitivas de Simulación de Redes

#include <iostream>
using namespace std;
#include "simcoroutine.h"
#include "simcontrol.h"
#include "statvar.h"
// modulos adicionales que se usan en los modelos
#include "colapaq.h"
#include "paquetes.h"
#include "random.h"

///////////////////////// común ///////////////////////////

#define TIEMPO_SIM 	Time
#define T_SIM		Time()
// uso: TIEMPO_SIM() o, más corto, T_SIM sin paréntesis

///////////////////////// procesos ///////////////////////////

typedef Process*	PROCESO;

#define PROC(A) 	class A:public Process{double INFO1,INFO2,INFO3;public:A(double x=0,double y=0,double z=0){INFO1=x;INFO2=y;INFO3=z;}void Actions(){
// delimitan las acciones de un proceso A, con entre 0 y 3 parámetros
// numéricos llamados INFO1, INFO2 y INFO3 que tienen valor por defecto 0
// si se alcanza FINPROC el proceso termina sus acciones
#define	FINPROC 	}};

#define CREAR_PROCESO 	new
// ejemplos: P = CREAR_PROCESO A; crea un P que no usa parámetros y que
// ejecuta el bloque de acciones A definido con PROC(A)...FINPROC
// Q = CREAR_PROCESO A (3, 2.5); aquí INFO1 toma el valor 3,
// INFO2 el valor 2.5 y INFO3 no se usa (valor por defecto 0)
// el proceso creado queda en estado Pasivo

enum Estado {Pasivo, Activo, Esperando, Terminado};

inline Estado ESTADO_DE (PROCESO P) {
	if (P->Waiting()==2)
		return Terminado;
	else if (P->Waiting()==1)
		return Esperando;
	else if (P->Idle())
		return Pasivo;
	else
		return Activo;
}

#define PROCESO_ACTUAL 	Current
#define P_ACTUAL		Current()
// uso: PROCESO_ACTUAL() o, más corto, P_ACTUAL sin paréntesis
#define	P_PRINCIPAL		Main() // uso: P_PRINCIPAL sin paréntesis

inline void ACTIVAR (PROCESO P, double wt=0) {
	if (P->Waiting ())
		cout << "***error*** Ignorada activacion de proceso Esperando o Terminado\n";
	else if (wt < 0)
		cout << "***error*** Ignorada activacion con retardo negativo\n";
	else
		Reactivate(P, delay, wt);
//      if (wt == 0) Reactivate (P); else Reactivate(P, delay, wt);
//      haría que ACTIVAR(P) cediera control a P
}

int TratandoEvento ();

// los eventos NO deben usar ESPERAR
inline void ESPERAR (double wt) {
	if (TratandoEvento() == 0)
		Hold (wt);
	else
		cout << "***error*** Ignorada espera dentro de evento\n";
}

// los eventos NO deben desactivar P_ACTUAL
inline void DESACTIVAR (PROCESO P=0) {
	if ((P == 0)||(P == Current()))	{
		if (TratandoEvento() == 0)
			Passivate ();
		else
			cout << "***error*** Ignorada desactivacion del propio evento\n";
	}
	else if (P->Waiting())
		cout << "***error*** Ignorada desactivacion de proceso Esperando o Terminado\n";
	else if (P->Idle() == 0)
		Cancel(P);
}

void LIBERAR_RECURSOS (PROCESO p); // libera todos los ocupados por p

// los eventos NO deben terminar P_ACTUAL
inline void TERMINAR (PROCESO P=0) {
	PROCESO Q = P;

	if (Q == 0)
		Q = Current ();
	else if (Q->Waiting() == 2)
		return;
	if ((Q == Current()) && TratandoEvento ())
		cout << "***error*** Ignorada terminacion del propio evento\n";
	else {
		LIBERAR_RECURSOS (Q);
		Q->SetWaiting(2);
		if (Q == Current())
			Passivate ();
		else if (Q->Idle() == 0)
			Cancel(Q);
	};
}

///////////////////////// recursos ///////////////////////////

#include "colagen.h"

class RECURSO {
	int       Umax, U; // unidades existentes y unidades disponibles
	SEC_VALxT Ustats;  // estadísticas de unidades disponibles
	SEC_VALxT Nstats;  // cuenta de elementos en cola, con estadísticas
	SEC_VAL   Wstats;  // estadísticas de tiempo de espera por el recurso
	struct Peticion {
		PROCESO	p; // peticionario
		int		n; // unidades pedidas
		double  t; // cuándo se hizo la peticion
	};
	ColaGen<Peticion> pend[2]; // dos listas, uso alterno según i
	int i;  				   // indica la que está en uso
  public:
    	 RECURSO (int n=1);
    void PEDIR   (int n=1);
    void LIBERAR (int n=1);
    int  DISPONIBLES ();	// recursos disponibles actualmente
    int  OCUPADOS ();		// recursos ocupados actualmente

    int	   LONG_COLA ();  	 // peticiones esperando por recursos libres
    double LONG_MEDIA ();
    int    LONG_MAXIMA ();
    int    NUM_ATENDIDOS (); // peticiones concedidas
    double ESPERA_MEDIA ();
    double ESPERA_MAX ();
    double OCUP_MEDIA ();    // número medio de recursos ocupados
    int	   OCUP_MAX ();		 // número máximo de recursos ocupados a la vez
    void   REINICIAR_STATS ();
    void   IMPRIMIR_STATS (string txt);
};
// statvar.h se encarga de descartar el periodo transitorio inicial,
// si lo hay, antes de empezar a registrar datos para las estadisticas

///////////////////////// eventos ///////////////////////////

#define idEvento int	// eventos 1,2,3...
#define evNulo   0
#define NOINFO void*

idEvento Programar (int (*tratar_ev)(idEvento, void*, int), double t, void* pinfo);
// función interna que programa un evento que ocurrirá en el tiempo t y
// devuelve un identificador del evento programado (que puede usarse luego
// si fuera necesario cancelar el evento)
// si t < TIEMPO_SIM() no se programa el evento y se devuelve "evNulo"
//
// esta función se llama a través de PROGRAMAR, ver ejemplos:
//
// tmp = PROGRAMAR (eventoA, t1);
// 		programa un evento de tipo "eventoA" en t1 sin información asociada
//		y devuelve el identificador del evento programado
//
// el procedimiento que trata eventos de este tipo se define de la forma
// EVENTO (eventoA, NOINFO)
//    ...
// FINEVENTO
//
// tmp = PROGRAMAR (otroevento, t2, paq);
// 		programa evento de tipo "eventoB" en t2 con información asociada
//		"paq" donde "paq" es una variable de tipo "tipopaq"
//
// si uno o más tipos de evento manejan información asociada de tipo
// "tipopaq" debe declararse (una sola vez) mediante
// INFOEVENTO (tipopaq)
//
// en este caso el procedimiento de tratamiento se define de la forma
// EVENTO (eventoB, tipopaq)
//	  ... dentro del procedimiento se usa INFO (o INFO.campo1, INFO.campo2 ...
//    ... si tipopaq es un tipo estructurado) para acceder a la información
//    ... asociada al evento
// FINEVENTO

#define EVENTO(E,T)  int E(idEvento EV_ID,void* p,int x){static int npinfo=0;if(x==-1)npinfo=0;if(x==1){T INFO;if(p!=0)INFO=*((T*)p);delete(T*)p;{
// delimitan el procedimiento E de tratamiento de los eventos de tipo E,
// dentro del cual "EV_ID" es el identificador del evento tratado e "INFO"
// es de tipo T, donde T es el tipo de la información asociada al evento
// (si el evento no tiene información asociada se pone "NOINFO" en lugar
// del tipo T y en este caso INFO queda INDEFINIDO dentro del procedim. E)
//
// al llamar a PROGRAMAR, E se pone como primer parámetro, el instante en
// que ocurrirá el evento como segundo parámetro y el tercer parámetro,
// de tipo T contiene la información que se pasa al evento (si el evento
// no maneja información asociada se omite el tercer parámetro en la
// llamada a PROGRAMAR)
//
// el procedimiento que trata un evento debe ser "atómico" por lo que
// NO debe usar ESPERAR ni DESACTIVAR, ni PEDIR un recurso no disponible
#define FINEVENTO }; npinfo++;};return(npinfo);}

#define REFEVENTO(E,T)  int E(idEvento EV_ID,void* p,int x);
// usado para referirse a un evento definido más adelante, típicamente
// para programar eventos tipo E desde otro evento definido antes que E

idEvento PROGRAMAR (int (*tratar_ev)(idEvento, void*, int), double t);
// parámetro 1: procedim. que trata E, declarado con EVENTO(E,NOINFO)...
// parámetro 2: instante en el que se ejecutará el evento
// la función devuelve el identificador del evento programado
// (o bien "evNulo" si t < TIEMPO_SIM())

#define INFOEVENTO(T) idEvento PROGRAMAR(int(*tratar_ev)(idEvento,void*,int),double t,T x){T *y;y=new T;*y=x;return Programar(tratar_ev,t,y);};
// INFOEVENTO (T1)
// INFOEVENTO (T2) ...
// define procedimientos PROGRAMAR adicionales con un tercer parámetro
// para pasar información asociada de tipo T1, T2, ...

void CANCELAR (idEvento e);
// cancela el evento identificado por "e", identificador que habrá sido
// devuelto por la llamada a la función que programó el evento

void EJECUTAR_EVENTOS (double maxT);
// esta función "imita" la que ofrece simeventos.h/.cpp para modelos
// "solo eventos". Se llama desde el programa principal y equivale a
// ESPERAR (maxT-T_SIM) de forma que el proceso gestor de eventos va
// ejecutando eventos de la lista de eventos pendientes, pasando a cada
// procedimiento de tratamiento el identificador del evento ejecutado y
// el puntero a la información asociada al mismo, hasta que el tiempo
// simulado alcanza maxT

#define NUM_EVENTOS(E) E(0,0,0)
// devuelve el número de eventos de tipo E que se han ejecutado

#define RESET_NUM_EVENTOS(E) E(0,0,-1)
// pone a 0 el número de eventos de tipo E ejecutados

///////////////////// uso interno ///////////////////////////

void init_eXec_EVs ();  // no debe llamarla el usuario;
					    // se llama desde INIC_SIMULACION
void cerrar (); // Pide teclear algo para terminar; para evitarlo
                // compilar simulacion.cpp con la opción -D NOQUIT
			    // no debe llamarla el usuario;
			    // se llama desde FIN_SIMULACION
extern int		MAX_ARG; // usados internamente
extern char  **ARGV;	 // para lectura de parámetros

///////////////////////// común ///////////////////////////

#define INIC_SIMULACION 	int main(int argc, char *argv[]){char Dummy;StackBottom=&Dummy;MAX_ARG=argc-1;ARGV=argv;init_eXec_EVs();{
// delimita el comienzo del programa principal de la simulación

#define FIN_SIMULACION 		ImprimirDatosExperim ();cerrar();return 0;}}
// delimita el final del programa principal de la simulación

///////////////////////////////////////////////////////////
