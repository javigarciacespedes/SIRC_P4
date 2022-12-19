#ifndef PAQ_H
#define PAQ_H

struct Paquete {
	int num;
	double Tll;
	double Ts;
	int longitud;
};

Paquete SIG_PAQ ();
// rellena el campo num a 1,2,3... y el resto a 0

// funciones para paquetes manejados como punteros a la estructura Paquete

typedef Paquete* PaquetePtr;

PaquetePtr CREAR_PAQ ();
// asigna memoria y rellena el campo num a 1,2,3... y el resto a 0

void LIBERAR_PAQ (PaquetePtr p);
// libera memoria

#endif



