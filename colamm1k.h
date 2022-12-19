// cola M/M/1/k: parte independiente de como se haga el modelo
// (eventos, procesos, etc.)

// declara los parámetros de cola M/M/1/k
double 	Landa;	// tasa de llegadas
double 	Mu;	    // tasa de servicio (Mu = 1/tiempo medio de servicio)
double  K;
double paquetes_totales = 0;
// declara los generadores pseudoaleatorios U1 y U2
RND_GEN U1;
RND_GEN U2;
RND_GEN U3;
// U1 y U2 se iniciaran en LeerParametros(), una vez leidos los valores
// de los parametros Landa y MuInverso
// previamente se habra leido en LeerParametrosSim() un valor de semilla
// a partir de la cual se generan semillas diferentes para U1 y U2,
// de forma la secuencia generada con U1 (tiempos entre llegadas) y la
// generada con U2 (tiempos de servicio) sean independientes entre si

void LeerParametros () {
// lee transitorio, duración, semilla (se debe llamar siempre antes de
// pasar a leer los parametros especificos del modelo)
	LeerParametrosSim (); cout << "\n";
// inicia los generadores pseudoaleatorios con distribucion
// exponencial negativa, con los parametros indicados
// y cada uno con una semilla distinta calculada internamente a partir
// de la semilla general introducida al ejecutar la simulacion
  	Landa =    LeerDouble ("Tasa de llegadas (paq/s): ");
  	U1 = INIT_NEGEXP (Landa);
  	Mu = 1.0 / LeerDouble ("Servicio medio (s):       ");
  	U2 = INIT_NEGEXP (Mu);
  	K = LeerDouble ("Cola maxima (paq): ");
  	U3 = INIT_NEGEXP (K);
  	MarcarTinic (); // usado para calcular cuanto tarda la simulacion
}

// declara las funciones Tllegada() y Tservicio() que devuelven los
// valores generados mediante U1 y U2 respectivamente
inline double Tllegada() { return SIG_VAL(U1); }
inline double Tservicio() { return SIG_VAL(U2); }

// contadores para calculo de resultados, que se pueden comparar con
// los calculados automaticamente por los objetos COLA y RECURSO
int    NumClientesAtendidos = 0;
double AcumEspera = 0.0;
double AcumServicio = 0.0;

void ReiniciarContadores () {
    NumClientesAtendidos = 0;
    AcumEspera = 0.0;
    AcumServicio = 0.0;
    Trazar (0, "   Reinicio de contadores en ", TIEMPO_SIM ());
}

void EscribirResultados () {
	cout << "\n" << "Resultados del periodo:       " << Transitorio() << " ... " << Transitorio()+Duracion() << "\n" ;
	cout << "Clientes descartados: " << descartados << "\n" ;
	cout << "Clientes descartados/total: " << (float) (descartados)/(Q.LONG_COLA()+Q.NUM_ATENDIDOS()+descartados) << "\n";
	
	
	
// completar lo nercesario para escribir el cociente entre el número de paquetes descartados por estar la cola llena
// y el número total de paquetes simulados (paquetes descartados+paquetes esperando en cola+paquetes que han salido de la cola)
}
