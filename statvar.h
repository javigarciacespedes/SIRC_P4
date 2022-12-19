#ifndef STATVAR_H
#define STATVAR_H

#include <math.h>
// usa Time() de simulation.h
extern double Ttransitorio;

class SEC_VAL {
// SEC_VAL V; define una secuencia V de valores, por ejemplo
// longitudes de paquetes o tiempos de espera
// sobre la que se calculan mínimo, máximo, media, varianza...
  protected:
  	double	init_t;
	double	S, S2, prev_v, min_v, max_v;
	int		num_v;
    virtual
    void	actualizar		() {}; // redefinida en SEC_VALxT
    virtual
    double	denominador		() { return double(num_v); };
	double 	sumacuadrados  	() { actualizar (); return S2; };
	double 	mediacuadrados 	() { if (denominador () > 0) {
									actualizar ();
									return S2/denominador ();
								 }
								 else return 0.0;
							   };
  public:
    virtual
    void	RESET 		()	{ S=0.0; S2=0.0; init_t=Time();
    				          num_v=0; prev_v=0.0; min_v=0.0; max_v=0.0;
    				        };
			SEC_VAL 	()	{ RESET (); };
	virtual
	void 	REGISTRAR 	(double x)	{ if (Time() > Ttransitorio) {
										num_v++; prev_v=x;
									  	S += x; S2 += (x*x);
									  	if (num_v==1) { min_v=x; max_v=x; };
									  	if (x>max_v) max_v=x;
									  	if (x<min_v) min_v=x;
									  }
									};
	void 	operator = 	(double x)	{ REGISTRAR (x); };
	// V=x equivale a V.REGISTRAR(x)
	double	VAL			()			{ return prev_v; };

	// las funciones siguientes registran un nuevo valor obtenido
	// de incrementar o decrementar el valor anterior
	// (o bien el valor 0 si aún no se había registrado ningún valor)
	void 	INCREMENTAR (double x=1)	{ REGISTRAR (prev_v+x); };
	void 	DECREMENTAR (double x=1)	{ REGISTRAR (prev_v-x); };
	void 	operator += (double x)		{ REGISTRAR (prev_v+x); };
	void 	operator -= (double x)		{ REGISTRAR (prev_v-x); };

	// estadísticas
	int 	NUMVAL		() { return num_v; };
	// si V.NUMVAL()==0, las demás estadísticas devuelven también 0
	double 	MAX			() { return max_v; };
	double 	MIN			() { return min_v; };
	double 	SUMA		() { actualizar (); return S; };
	double 	MEDIA		() { if (denominador () > 0) {
								actualizar ();
								return S/denominador ();
							 }
							 else return 0.0;
						   };
	double 	VARIANZA 	() { return (mediacuadrados () - MEDIA()*MEDIA()); };
	double	STDDEV   	() { return pow (VARIANZA (), 0.5); };
	void    IMPRIMIR 	() {
		if ((init_t < Ttransitorio) && (Time() >= Ttransitorio)) init_t = Ttransitorio;
		cout << "intervalo de tiempo = "
			 << init_t << " ... " << Time () << "\n";
		cout << "numero de valores   = " << NUMVAL () << "\n";
		cout << "ultimo valor        = " << VAL () << "\n";
		cout << "minimo              = " << MIN () << "\n";
		cout << "maximo              = " << MAX () << "\n";
		cout << "suma                = " << SUMA () << "\n";
		cout << "media               = " << MEDIA () << "\n";
		cout << "varianza            = " << VARIANZA () << "\n";
		cout << "std dev             = " << STDDEV () << "\n\n";
	};
};

class SEC_VALxT: public SEC_VAL {
// SEC_VALxT W; define una secuencia W de valores que forman
// una curva en escalera en función del tiempo
// por ejemplo, la longitud de una cola
//
// al declarar o reiniciar una SEC_VALxT se especifica su
// valor inicial, que por defecto es 0
//
// las estadísticas de SEC_VALxT se calculan para el periodo
// de cómputo que va desde el instante inicial (o desde el último
// reinicio) hasta el instante en que se hace la consulta
//
// para calcular la media y la varianza cada valor se pondera
// por el tiempo que se ha mantenido, de forma que,
// por ejemplo, la "suma" es el área bajo la curva en escalera
// que definen los valores registrados, no la suma directa de
// los valores, y la media se obtiene dividiendo esta "suma"
// por el periodo de cómputo en lugar de por el número de valores
// (si el periodo de cómputo es nulo, devuelven 0)

	double	prev_t;
	void 	actualizar () 	{ if ((prev_t < Ttransitorio) && (Time() >= Ttransitorio))
								prev_t = Ttransitorio;
							  if ((Time() > Ttransitorio) && (Time() > prev_t)) {
								S  += prev_v * (Time()-prev_t);
								S2 += prev_v*prev_v * (Time()-prev_t);
								prev_t=Time();
							  }
						  	};
	double	denominador	()	{ if ((init_t < Ttransitorio) && (Time() >= Ttransitorio))
								init_t = Ttransitorio;
							  return (Time()-init_t);
							};
  public:
    		SEC_VALxT 	(double x=0){ S=0.0; S2=0.0;
    								  init_t=Time(); prev_t=Time();
    								  num_v=1; prev_v=x; min_v=x; max_v=x;
    								};
	void	RESET 		()			{ S=0.0; S2=0.0;
									  init_t=Time(); prev_t=Time();
									  num_v=1; min_v=prev_v; max_v=prev_v;
    								};
	void 	REGISTRAR 	(double x)	{ if (Time() >= Ttransitorio) {
										actualizar ();
									    num_v++; prev_v=x;
									    if (x>max_v) max_v=x;
									    if (x<min_v) min_v=x;
									  }
									  else {
									    prev_v=x; min_v=x; max_v=x;
									  }
									};
	void 	operator = 	(double x)	{ REGISTRAR (x); };  // NO borrar!!
	// W=x equivale a W.REGISTRAR(x)
	void 	operator += (double x)	{ REGISTRAR (prev_v+x); };
	void 	operator -= (double x)	{ REGISTRAR (prev_v-x); };
};

#endif
