// Manejo integrado de eventos, procesos y recursos
// según primitivas de Simulación de Redes

#include <iostream>
using namespace std;

#include "simulacion.h"

///////////////////////// recursos ///////////////////////////

struct RecursoOcupado {
	PROCESO p;
	RECURSO* r;
	int n;
};

ColaGen<RecursoOcupado> RecursosOcupados[2];
int enUso = 0;

void VerRegistroRecursosOcupados (string txt) {
	RecursoOcupado rec;
	int tot = 0;

	cout << txt << "* en Uso " << enUso << "\n";
	while (RecursosOcupados[enUso].LONG_COLA () > 0) {
		rec = RecursosOcupados[enUso].DESENCOLAR ();
		cout << "* proceso " << rec.p << " recurso " << rec.r << " unidades " << rec.n << "\n";
		tot += rec.n;
		RecursosOcupados[1-enUso].ENCOLAR (rec);
	}
	cout << "* total unidades " << tot << "\n\n";
	enUso = 1 - enUso;
	while (RecursosOcupados[enUso].LONG_COLA () > 0) {
		rec = RecursosOcupados[enUso].DESENCOLAR ();
		RecursosOcupados[1-enUso].ENCOLAR (rec);
	}
	enUso = 1 - enUso;
}

void RegistrarRecursoOcupado (PROCESO p, RECURSO* r, int n=1) {
	RecursoOcupado rec;

	rec.p = p;
	rec.r = r;
	rec.n = n;
	RecursosOcupados[enUso].ENCOLAR (rec);
	// VerRegistroRecursosOcupados ("* Tras peticion ");
}

void RegistrarRecursoLiberado (RECURSO* r, int n) {
	RecursoOcupado rec;
	int faltan = n;

	while (RecursosOcupados[enUso].LONG_COLA () > 0) {
		rec = RecursosOcupados[enUso].DESENCOLAR ();
		if ((rec.r != r)||(rec.p != PROCESO_ACTUAL()))
			RecursosOcupados[1-enUso].ENCOLAR (rec);
		else {
			if (rec.n <= faltan)
				faltan -= rec.n;
			else {
				rec.n -= faltan;
				faltan = 0;
				RecursosOcupados[1-enUso].ENCOLAR (rec);
			}
		}
	}
	enUso = 1 - enUso;
	if (faltan > 0) cout << "***error*** Intento de liberar mas recursos de los asignados\n";
	// VerRegistroRecursosOcupados ("* Tras liberacion ");
}


RECURSO::RECURSO (int n) {
	Umax = n;
	if (Umax < 1) {
		Umax = 1; // NO se admite Umax < 1
		cout << "***error*** Numero de recursos <1 cambiado a 1\n";
	}
	U = Umax;
	Ustats.REGISTRAR (U);
	Nstats.REGISTRAR (0);
	i = 0;
}

void RECURSO::PEDIR (int n) {
	Peticion pet;

	if (n <= 0) {
		cout << "***error*** Peticion de recursos <= 0 ignorada\n";
		return;
	}
	if (U >= n) {
		U = U - n;
		Ustats.REGISTRAR (U);
		Wstats.REGISTRAR (0);
		RegistrarRecursoOcupado (PROCESO_ACTUAL (), this, n);
	}
	else {
		if (TratandoEvento ()) {
			cout << "***error*** Ignorada peticion de recursos no disponibles hecha en un evento \n";
			return;
		}
		pet.p = PROCESO_ACTUAL ();
		pet.n = n;
		pet.t = T_SIM;
		if (pet.n > Umax) {
			pet.n = Umax; // NO se admite pedir más de Umax
			cout << "***error*** Peticion de recursos > máximo cambiada a maximo\n";
		}
		pend[i].ENCOLAR (pet);
		pet.p->SetWaiting (1);
		Nstats.INCREMENTAR ();
		DESACTIVAR ();
	}
}

// LIBERAR traslada de una lista de peticiones a la otra las peticiones pendientes
// para las que no se liberan suficientes recursos; esto facilita la implementación
// de LIBERAR pero impide usar estadísticas empotradas que tenga la cola de peticiones,
// por ejemplo número medio de peticiones pendientes (o sea número medio de procesos
// esperando por el recurso) o tiempo de espera medio, máximo, etc.
void RECURSO::LIBERAR (int n) {
	Peticion pet;
	int prevU;

	if ((n <= 0)||(U == Umax)) {
		if (n <= 0)
			cout << "***error*** Peticion de liberar recursos <= 0 ignorada\n";
		else
			cout << "***error*** Intento de liberar recursos no asignados\n";
		return;
	}
	prevU = U;
	if (U + n > Umax) {
		U = Umax; // NO se admite que tras liberar quede U > Umax
		cout << "***error*** Intento de liberar mas recursos de los asignados\n";
	}
	else
		U = U + n;
	RegistrarRecursoLiberado (this, U-prevU);
	while (pend[i].LONG_COLA () > 0) {
		pet = pend[i].DESENCOLAR ();
		if (pet.p->Waiting()==1) { // si no, es que ha sido Terminado
			if (U >= pet.n) {
				U = U - pet.n;
				RegistrarRecursoOcupado (pet.p, this, pet.n);
				pet.p->SetWaiting (0);
				Wstats.REGISTRAR (T_SIM-pet.t);
				Nstats.DECREMENTAR ();
				ACTIVAR (pet.p);
			}
			else  // pasa a la otra cola; al final la actual queda vacía
				pend[1-i].ENCOLAR (pet);
		}
	};
	i = 1-i; // cambia cola en uso (cuando las dos vacías, da igual)
	if (U != prevU) Ustats.REGISTRAR (U);
}

void LIBERAR_RECURSOS (PROCESO p) {
	RecursoOcupado rec;

	if (p->Waiting()==2) {
		cout << "***error*** Intento de liberar recursos de proceso terminado\n";
		return;
	}
	while (RecursosOcupados[enUso].LONG_COLA () > 0) {
		rec = RecursosOcupados[enUso].DESENCOLAR ();
		if (rec.p == p)
			rec.r->LIBERAR (rec.n);
		else
			RecursosOcupados[1-enUso].ENCOLAR (rec);
	}
	enUso = 1 - enUso;
}

int RECURSO::DISPONIBLES () {
	return U;
}

int RECURSO::OCUPADOS () {
	return Umax-U;
}

int	RECURSO::NUM_ATENDIDOS () {
	return Wstats.NUMVAL ();
}

int	RECURSO::LONG_COLA () {
	return pend[i].LONG_COLA ();
}

double RECURSO::OCUP_MEDIA () {
	return Umax - Ustats.MEDIA ();
}

int RECURSO::OCUP_MAX () {
	return Umax - int (Ustats.MIN ());
}

double RECURSO::LONG_MEDIA () {
	return Nstats.MEDIA ();
}

int RECURSO::LONG_MAXIMA () {
	return int (Nstats.MAX ());
}

double RECURSO::ESPERA_MEDIA () {
	return Wstats.MEDIA ();
}

double RECURSO::ESPERA_MAX () {
	return Wstats.MAX ();
}

void RECURSO::REINICIAR_STATS () {
	Ustats.RESET ();
	Nstats.RESET ();
	Wstats.RESET ();
}

void RECURSO::IMPRIMIR_STATS (string txt) {
	cout << txt << endl;
	cout << "Num. de clientes esperando:   " << LONG_COLA () << "\n";
	cout << "Longitud media de la cola:    " << LONG_MEDIA () << "\n";
	cout << "Longitud maxima de la cola:   " << LONG_MAXIMA () << "\n";
	cout << "Num. de salidas de la cola:   " << NUM_ATENDIDOS () << "\n";
	cout << "Tiempo de espera medio:       " << ESPERA_MEDIA () << "\n";
	cout << "Tiempo de espera maximo:      " << ESPERA_MAX () << "\n";
	cout << "Utilizacion media:            " << OCUP_MEDIA () << "\n\n";
	//cout << "Utilizacion maxima:           " << OCUP_MAX () << "\n\n";
}

///////////////////////// eventos ///////////////////////////

PROCESO eXec_EVs; // gestor de eventos

struct CeldaEvento {
	CeldaEvento *sig;
	int cancelado;
	int (*p)(idEvento, void*, int);
	void *i;
	double t;
	idEvento id;
};

CeldaEvento *Prox = 0;
CeldaEvento *Ultimo = 0;

int NumEvento = 0; // igual al numero de eventos programados
int EventosPendientes = 0;
int	EventoMasProximo = 0;

int NumEventosEjecutados=0;
int NumEventosCancelados=0;

int EnEvento = 0;
int TratandoEvento () {	return EnEvento; }

PROC (eXec_EV_proc)
	int (*tratar)(idEvento, void*, int);
	int tmp;
	idEvento n;
	void *info;
	CeldaEvento *barre;

    for (;;) {
      	EventosPendientes--;
      	while (Prox->cancelado == 1)
         	Prox = Prox->sig;
      	tratar = Prox->p;
      	n = Prox->id;
      	info = Prox->i;
      	Prox = Prox->sig;
      	EnEvento = 1;
      	tmp = tratar (n, info, 1); // ejecuta el tratamiento del evento
	   	NumEventosEjecutados++;
	   	EnEvento = 0;
	   	if (EventosPendientes > 0) {
      		barre = Prox;
      		while (barre->cancelado == 1)
         		barre = barre->sig;
        	EventoMasProximo = barre->id;
			ESPERAR (barre->t - T_SIM);
		}
    	else
    		DESACTIVAR ();
	}
FINPROC

void init_eXec_EVs () {
	eXec_EVs = new eXec_EV_proc;
}

void PrepararEventoMasProximo () {
	CeldaEvento *barre;

   	if (EventosPendientes > 0) {
      	barre = Prox;
      	while (barre->cancelado == 1)
         	barre = barre->sig;
        if (barre->id != EventoMasProximo) {
			EventoMasProximo = barre->id;
			ACTIVAR (eXec_EVs, barre->t - T_SIM);
		}
    }
    else
    	DESACTIVAR (eXec_EVs);
}

idEvento Programar (int (*tratar_ev)(idEvento, void*, int), double t, void* pinfo) {
	CeldaEvento *tmp;
	CeldaEvento *barre;

	if (t < T_SIM) {
		cout << "***error*** Intento de programar evento en el pasado\n";
		return evNulo;
	}

	NumEvento++;
	if (EventosPendientes == 0) {
	    if (Prox == 0) {
			// crear lista
         	Prox = new CeldaEvento;
         	Prox->sig = Prox;
  	  	}
		Prox->cancelado = 0;
		Prox->p = tratar_ev;
		Prox->i = pinfo;
		Prox->t = t;
		Prox->id = NumEvento;
		Ultimo = Prox;
	}
	else {
      	if (Ultimo->sig == Prox) {
			// añadir celda
         	tmp = new CeldaEvento;
         	Ultimo->sig = tmp;
         	tmp->sig = Prox;
      	};
      	Ultimo->sig->cancelado = 0;
      	Ultimo->sig->p = tratar_ev;
      	Ultimo->sig->i = pinfo;
      	Ultimo->sig->t = t;
      	Ultimo->sig->id = NumEvento;
      	if (t >= Ultimo->t)
      		// el nuevo es el último y ya está en su sitio
         	Ultimo = Ultimo->sig;
      	else {
        	tmp = Ultimo->sig;
         	if ((t < Prox->t) && (tmp->sig == Prox))
         		// el nuevo es el primero y ya está en su sitio
            	Prox = tmp;
         	else {
				// ordenar
            	Ultimo->sig = tmp->sig;
            	if (t < Prox->t) {
					// el nuevo evento es el primero
               		barre = Ultimo;
               		while (barre->sig != Prox)
                  		barre = barre->sig;
					tmp->sig = Prox;
               		barre->sig = tmp;
               		Prox = tmp;
            	}
            	else {
               		barre = Prox;
               		while (t >= barre->sig->t)
                  		barre = barre->sig;
               		tmp->sig = barre->sig;
               		barre->sig = tmp;
				}
         	}
		}
   	};
   	EventosPendientes++;
   	if (PROCESO_ACTUAL() != eXec_EVs) PrepararEventoMasProximo ();
   	return NumEvento;
}

idEvento PROGRAMAR (int (*tratar_ev)(idEvento, void*, int), double t) {
	return Programar (tratar_ev, t, 0);
};

void CANCELAR (idEvento e) {
	CeldaEvento *barre;

   	if (EventosPendientes > 0) {
      	barre = Prox;
      	while ((barre->id != e) && (barre != Ultimo))
         	barre = barre->sig;
      	if ((barre->id == e) && (barre->cancelado == 0)) {
         	barre->cancelado = 1;
         	NumEventosCancelados++;
         	EventosPendientes--;
  	  	}
  	  	else
  	  		cout << "***error*** Intento de cancelar evento inexistente o ya cancelado\n";
  	  	if (PROCESO_ACTUAL() != eXec_EVs) PrepararEventoMasProximo ();
	};
}

void EJECUTAR_EVENTOS (double maxT) {
	ESPERAR (maxT-T_SIM);
}

/////////////////////////////////////////////////////////////////

void cerrar () {
#ifndef NOQUIT
	// system("PAUSE");
	char q;
	cout << "Teclee q para terminar... " ; cin >> q;
#endif
}
