#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>    // ver detalles en www.cplusplus.com/reference/clibrary/ctime/
using namespace std;

#include "simcontrol.h"
#include "rngstream.h"

double 			Ttransitorio;
double 			Tduracion;
unsigned long   Vsemilla;
int    			Trazado = 0;

time_t 	Tinic, Tfin;  // comienzo y final de la simulacion
double 	ss;			  // segundos que ha tardado la simulacion
extern int NumEnters; // numero de cambios de contexto entre procesos
extern int NumEvento; // igual al numero de eventos programados
extern int NumEventosEjecutados;
extern int NumEventosCancelados;


ifstream 	fich_params;		// fichero de parametros de entrada
int			estado_fich = -1; 	// para comprobar la primera vez si hay fichero de parametros
int		    arg_i = 1;			// indice al siguiente valor en línea de comandos

int		MAX_ARG;	// iniciada a argc-1 por simulacion.h
char  **ARGV;		// iniciada a argv por simulacion.h

void abrir_fich_params () {
  estado_fich = 0;
  if (MAX_ARG == 1) {
  	fich_params.open (ARGV[1], ios::in);
    if (fich_params.is_open())
		estado_fich = 1;
    else {
    	cout << "Error al abrir el fichero " << ARGV[1] << "\n";
    	MAX_ARG = 0;
	}
  }
}

string LeerLinea (string txt) {
	string 	line, subline1, subline;
	int ok, i;

	if (estado_fich == -1) abrir_fich_params ();

	if (estado_fich == 1) {
  	    do {
  	      line[0] = ' ';
  	      getline (fich_params, line);
  	      ok=fich_params.good();
		} while ((ok)&&(line[0]=='#'));

        if ( ok ) {
		  if (line.rfind (':') != string::npos)
            subline1 = line.substr(line.rfind (':')+1);
          else {
			cout << txt;
            subline1 = line;
	  		}
	  	  i=0; while (subline1[i] == ' ') i++;
	  	  subline = subline1.substr(i);
          cout << line << endl;
          arg_i++;
	    }
	};
	if ((estado_fich == 0)||(!ok)) {
		cout << txt;
	  	if (arg_i<=MAX_ARG) {
			subline = ARGV[arg_i];
			cout << subline << "\n";
		}
	    else
	    	getline (cin, subline);
	    arg_i++;
	};
	return (subline);
}

double LeerDouble (string txt) {
	string linea;
	double tmp;

	linea = LeerLinea (txt); stringstream (linea) >> tmp;
	return (tmp);
}

long LeerLong (string txt) {
	string linea;
	long tmp;

	linea = LeerLinea (txt); stringstream (linea) >> tmp;
	return (tmp);
}

int LeerInt (string txt) {
	string linea;
	int tmp;

	linea = LeerLinea (txt); stringstream (linea) >> tmp;
	return (tmp);
}

int LeerSiNo (string txt) {
	string linea;
	int tmp;

	linea = LeerLinea (txt);
	tmp = ((linea[0]=='S')||(linea[0]=='s'))? 1 : 0;
	return (tmp);
}

void LeerParametrosSim () {
	unsigned long seed[6];
	int i;

	time (&Tinic);

  	Ttransitorio =	LeerDouble ("Transitorio inicial:           ");
	Tduracion = 	LeerDouble ("Duracion maxima de simulacion: ");
	Vsemilla = 		(unsigned long)
					LeerLong   ("Semilla:                       ");
	Trazado =		LeerSiNo   ("Trazado (s|n):                 ");

	if (Vsemilla == 0)
		if (Tinic > 1360000000)
			Vsemilla = (unsigned long) (Tinic-1360000000);
		else
			Vsemilla = (unsigned long) (Tinic);

	for (i=0; i<6; i++)
		seed[i] = Vsemilla;
  	RngStream::SetPackageSeed (seed);
  	//cout << seed [0] << " " << seed [1] << " " << seed [2] << " " << seed [3] << " " << seed [4] << " " << seed [5] << endl;
}


double Duracion () { return Tduracion; };

double Transitorio () { return Ttransitorio; }

unsigned long Semilla () { return Vsemilla; };

void Trazar (int num, string txt1, double v1) {
   if (Trazado == 1)
     cout << num << "# " << txt1 << v1 << "\n";
}

void Trazar (int num, string txt1, double v1, string txt2, double v2) {
   if (Trazado == 1)
     cout << num << "# " << txt1 << v1 << ", " << txt2 << v2 << "\n";
}

const string FechayHora(time_t ts) {
    struct tm  	ahora;
    char   		buf[80];

    ahora = *localtime(&ts);
    strftime(buf, sizeof(buf), "%Y-%m-%d %X", &ahora);

    return buf;
}

void MarcarTinic () {
	time (&Tinic);
}

void ImprimirDatosExperim () {
    cout << "-----" << endl << FechayHora(Tinic) << ", semilla " << Semilla ();

	time (&Tfin);
	ss = difftime (Tfin, Tinic)+1;
	if (NumEnters>3) // NumEnters = 3 en modelos solo eventos
		printf (", %d cambios de proceso en %.0f s (%.0f c/s)\n\n",
				NumEnters, ss, NumEnters/ss);
	if (NumEvento>0)
 		if (Trazado == 1)
			printf (", %d(%d)/%d eventos ejecutados(cancelados)/programados en %.0f s (%.0f ev/s)\n\n",
					NumEventosEjecutados, NumEventosCancelados, NumEvento, ss, NumEventosEjecutados/ss);
		else
			printf (", %d eventos en %.0f s (%.0f ev/s)\n\n",
					NumEventosEjecutados, ss, NumEventosEjecutados/ss);
}

CI_VAR::CI_VAR (double alfa, double delta, int metodo) {
// delta como tanto por uno de la media (resto valores por defecto)
#define MAX_TRANSIT 0 // 1000
	if (metodo==BATCH_MEANS)
		V = InitBatchMeansSimControl(N_MAX,MAX_TRANSIT,0.05,2.0,0.5,alfa,delta);
	else
		V = InitSpectralSimControl(N_MAX,MAX_TRANSIT,0.05,2.0,0.5,1.5,alfa,delta);
};

void CI_VAR::REGISTRAR (double v) {
	SimControl (V, v);
};

void CI_VAR::IMPRIMIR_CI (string txt) {
	unsigned long simctrl_ntransient;
	unsigned long simctrl_nsteady;
	double simctrl_mean;
	double simctrl_cl;
	double simctrl_ep;

	unsigned long simctrl_step;
	unsigned long simctrl_n_batch;
	unsigned long simctrl_batch_s;

	GetSimControlResults (V,
						   &simctrl_ntransient,
						   &simctrl_nsteady,
						   &simctrl_mean,
						   &simctrl_cl,
						   &simctrl_ep,
						   &simctrl_step,
						   &simctrl_n_batch,
						   &simctrl_batch_s);
	if (simctrl_ep < 1) {
		cout << txt << "\n";
		cout << "      Valor:    " << simctrl_mean
			 << " [" << simctrl_mean*(1-simctrl_ep) << ", "
			 << simctrl_mean*(1+simctrl_ep) << "]\n";
		cout << "      Delta:    " << 100 * simctrl_ep << "%\n";
		cout << "      1-Alfa:   " << 100 * simctrl_cl << "%\n";
		cout << "      Muestras: " << simctrl_ntransient << "+" << simctrl_nsteady;
		if (simctrl_batch_s > 0)
			cout << " (batch " << simctrl_batch_s << ")";
		cout << "\n\n";
	}
}
