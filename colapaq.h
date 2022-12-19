#ifndef COLAPAQ_H
#define COLAPAQ_H

// "COLA" cola FIFO (ENCOLAR/DESENCOLAR) o pila (APILAR/DESENCOLAR)
// con elementos de tipo genérico "Paq" y estadísticas empotradas

// necesita Time() de simulation.h
// necesita statvar.h
// statvar.h se encarga de descartar el periodo transitorio inicial,
// si lo hay, antes de empezar a registrar datos para las estadisticas

template <class Paq>
class COLA {
  protected:
  	struct Elem {
  		Elem *ant, *sig;
  		int    ocup;
  		double tll;
  		Paq    e;
  	};
  	Elem *prim, *ulti;
  	SEC_VALxT   n;   // cuenta de elementos en cola, con estadísticas
  	SEC_VAL		Ws;  // estadísticas de tiempos de espera
  public:
    	 COLA ();
    void ENCOLAR (Paq); // lo pone el último
    void APILAR (Paq);  // lo pone el primero
    Paq  DESENCOLAR (); // saca el primero

    int    LONG_COLA ();  // indica cuántos están esperando actualmente
    double LONG_MEDIA ();
    int    LONG_MAXIMA ();
    int	   NUM_ATENDIDOS (); // indica cuantos han salido de la cola
    double ESPERA_MEDIA ();
    double ESPERA_MAX ();
    void   REINICIAR_STATS ();
    void   IMPRIMIR_STATS (string txt);
};

// ejemplo de uso:
// struct PAQUETE {...}; define el tipo concreto PAQUETE
// COLA<PAQUETE> Q; declara una nueva cola de elementos de tipo PAQUETE

template <class Paq>
COLA<Paq>::COLA () {
	// inicia una nueva cola vacía
    prim = new (nothrow) Elem;
    if (prim == 0)
		cout << "No más colas!!\n";
    else {
	    ulti = prim;
	    prim->sig = prim;
	    prim->ant = prim;
	    prim->ocup = 0;
	    n.REGISTRAR (0);
	}
}

template <class Paq>
void COLA<Paq>::ENCOLAR (Paq x) {
	// lo pone el último
    if (ulti->sig->ocup == 0) {
		ulti = ulti->sig;
		ulti->ocup = 1;
		ulti->tll = Time();
		ulti->e = x;
	    n.INCREMENTAR ();
    }
	else {
		ulti->sig = new (nothrow) Elem;
		if (ulti->sig == 0)
			cout << "Cola llena!!\n";
    	else {
			ulti->sig->ant = ulti;
			ulti= ulti->sig;
			ulti->sig = prim;
			prim->ant = ulti;
			ulti->ocup = 1;
			ulti->tll = Time();
			ulti->e = x;
			n.INCREMENTAR ();
		}
	}
}

template <class Paq>
void COLA<Paq>::APILAR (Paq x) {
	// lo pone el primero
	if (prim->ant->ocup == 0) {
		prim = prim->ant;
		prim->ocup = 1;
		prim->tll = Time();
		prim->e = x;
		n.INCREMENTAR ();
	}
	else {
		prim->ant = new (nothrow) Elem;
		if (prim->ant == 0)
			cout << "Cola llena!!\n";
		else {
			prim->ant->sig = prim;
			prim = prim->ant;
			prim->ant = ulti;
			ulti->sig = prim;
			prim->ocup = 1;
			prim->tll = Time();
			prim->e = x;
			n.INCREMENTAR ();
		}
	}
}

template <class Paq>
Paq COLA<Paq>::DESENCOLAR () {
	// saca el primero
	Paq x;
	if (n.VAL () > 0) {
		x = prim->e;
		Ws.REGISTRAR (Time() - prim->tll);
		prim->ocup = 0;
		prim = prim->sig;
		n.DECREMENTAR ();
		return x;
	}
	else
		cout << "Cola vacía!!\n";
}

template <class Paq>
int COLA<Paq>::LONG_COLA () {
  	return int (n.VAL ());
}

template <class Paq>
int COLA<Paq>::NUM_ATENDIDOS () {
	return Ws.NUMVAL ();
}

template <class Paq>
double COLA<Paq>::LONG_MEDIA () {
	return n.MEDIA ();
}

template <class Paq>
int COLA<Paq>::LONG_MAXIMA () {
	return int (n.MAX ());
}

template <class Paq>
double COLA<Paq>::ESPERA_MEDIA () {
	return Ws.MEDIA ();
}

template <class Paq>
double COLA<Paq>::ESPERA_MAX () {
	return Ws.MAX ();
}

template <class Paq>
void COLA<Paq>::REINICIAR_STATS () {
	n.RESET ();
	Ws.RESET ();
}

template <class Paq>
void COLA<Paq>::IMPRIMIR_STATS (string txt) {
	cout << txt << endl;
	cout << "Num. de clientes esperando:   " << LONG_COLA () << "\n";
	cout << "Longitud media de la cola:    " << LONG_MEDIA () << "\n";
	cout << "Longitud maxima de la cola:   " << LONG_MAXIMA () << "\n";
	cout << "Num. de salidas de la cola:   " << NUM_ATENDIDOS () << "\n";
	cout << "Tiempo de espera medio:       " << ESPERA_MEDIA () << "\n";
	cout << "Tiempo de espera maximo:      " << ESPERA_MAX () << "\n\n";
}

#endif
