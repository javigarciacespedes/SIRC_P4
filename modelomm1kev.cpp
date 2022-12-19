#include "simulacion.h"

bool ServidorLibre = true; // variable de estado del servidor
int descartados = 0;

COLA<Paquete> Q;
// "Paquete" y "COLA" están definidos respectivamente en
// paquetes.h y colapaq.h (incluidos a traves de simulacion.h)

#include "colamm1k.h"
// incluye funciones LeerParametros(), Tllegada(), Tservicio() que son
// iguales para los distintos modelos de la cola M/M/1/k independientemente
// de que usen eventos, procesos o recursos

// declaración de tipos de datos de parámetros pasados a eventos

INFOEVENTO (Paquete)       // para eventos Llegada

// tratamiento de eventos

EVENTO (Salida, NOINFO)    // "NOINFO" indica evento sin parámetro
	Paquete sig;

    if (Q.LONG_COLA () == 0)
    	ServidorLibre = true;
    else {
       	sig = Q.DESENCOLAR ();
       	PROGRAMAR (Salida, TIEMPO_SIM () + sig.Ts);
	};
FINEVENTO

EVENTO (Llegada, Paquete)  // parámetro "INFO" es el paquete que llega
	Paquete sig, otro;
	paquetes_totales = paquetes_totales+1;
	
	if (Q.LONG_COLA() >= K && TIEMPO_SIM() > Transitorio()) // si la longitud de la cola es mayor que el valor de K. Se descarta el paquete.
		descartados = descartados+1;
    else {
    	Q.ENCOLAR (INFO); 
	}
    if (ServidorLibre) {
	   	sig = Q.DESENCOLAR ();
       	ServidorLibre = false;
       	PROGRAMAR (Salida, TIEMPO_SIM () + sig.Ts);
    };
    otro.Tll = TIEMPO_SIM () + Tllegada ();
    otro.Ts = Tservicio ();
    PROGRAMAR (Llegada, otro.Tll, otro);
FINEVENTO

INIC_SIMULACION
    cout << "MODELO de COLA MM1k con EVENTOS" << endl;
// lee transitorio, duración, semilla y parámetros de la cola M/M/1/k
    LeerParametros ();
    Paquete primer;
    primer.Tll = Tllegada ();
    primer.Ts = Tservicio ();
    PROGRAMAR (Llegada, primer.Tll, primer);

    EJECUTAR_EVENTOS (Transitorio ());
    ReiniciarContadores ();
    EJECUTAR_EVENTOS (Duracion ());

// transcurrido el transitorio, la cola calcula internamente estadísticas
	cout << endl;
	Q.IMPRIMIR_STATS ("RESULTADOS DE LA COLA");
// resultados adicionales sobre descarte
    EscribirResultados ();

FIN_SIMULACION

