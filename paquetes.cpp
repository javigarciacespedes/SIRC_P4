#include <iostream>
using namespace std;

#include "paquetes.h"
#include "colagen.h"

int num_secuencia = 0;

ColaGen<PaquetePtr> paq_reutilizados;

Paquete SIG_PAQ () {
	Paquete p;

	num_secuencia++;
	p.num = num_secuencia;
	p.Tll = 0.0;
	p.Ts = 0.0;
	p.longitud = 0;
	return p;
}

PaquetePtr CREAR_PAQ () {
	PaquetePtr p;

	if (paq_reutilizados.LONG_COLA() > 0)
		p = paq_reutilizados.DESENCOLAR ();
	else
		p = new Paquete;
	num_secuencia++;
	p->num = num_secuencia;
	p->Tll = 0.0;
	p->Ts = 0.0;
	p->longitud = 0;
	return p;
}

void LIBERAR_PAQ (PaquetePtr p) {
	paq_reutilizados.ENCOLAR (p);
}




