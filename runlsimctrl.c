/*
--------------------------------------------------------------------------
--
--        SIMCTRL: RUN LENGTH CONTROL IN STEADY-STATE SIMULATIONS
--
--------------------------------------------------------------------------
--
-- File runlsimctrl.c Date 02/12/2013
--
--   simctrl.c 	Version 3.2 Date 6/29/93
--   batchm.c 	Version 3.2 Date 6/29/93
--   common.c 	Version 3.2 Date 6/29/93
--   spec.c 	Version 3.2 Date 6/29/93
--   tran.c 	Version 3.2 Date 6/29/93
--   var.c 		Version 3.2 Date 6/29/93
--
--   Manuel Alvarez-Campana Fernandez-Corredor
--   Pedro Luis Sierra Hernando
--   Departamento de Ingenieria de Sistemas Telematicos
--   ETS Ingenieros de Telecomunicacion
--   Universidad Politecnica de Madrid
--   E-mail: mac@dit.upm.es
--
--------------------------------------------------------------------------
*/

// Combines initial transient period analysis
// and steady state period analysis stages.

#include <cstdlib> // para exit ()
#include <malloc.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>

#include "runlsimctrl.h"
#include "simcoroutine.h"

//----------------------------------------------------------------
// Initial transient period length analysis for removal of samples.

#define N0_MAX        (N_MAX/2) /* maximum length of transient period */
#define A_T           0.05 /* significance level of stationarity test */
#define G_V           2.0 /* safety coefficient for estimator of variance */
#define G_E           0.5  /* exchange coefficient */

typedef void *TRANCTRL;

#define TRANERR     -1       /* transient period too long */
#define TRANEND      0       /* transient period is over */
#define TRANCON      1       /* transient period continues */

//----------------------------------------------------------------
// Sequential analysis of steady state simulation output data.

#define ALPHA     0.05                          /* 1 - ALPHA = conf. level */
#define E_MAX     0.05        /* max. relative precision of conf. interval */

// Spectral method.

#define G_A       1.5                /* checkpoint incremental coefficient */

typedef void *SPECCTRL;

#define SPECERR     -1       /* accuracy cannot be reached */
#define SPECEND      0       /* simulation may be stopped */
#define SPECCON      1       /* simulation continues */

// Batch means method.

#define BETA      0.1       /* significance level for autocorrelation test */

typedef void *BATCHMCTRL;

#define BATERR     -1       /* accuracy cannot be reached */
#define BATEND      0       /* simulation may be stopped */
#define BATCON      1       /* simulation continues */

//----------------------------------------------------------------
//  Common functions for error handling.
//----------------------------------------------------------------

#define adios(msg)  {fprintf( stderr, "SIMCTRL: %s\n", msg ); exit(1); }

static int SimCtrlMathError = 0;
static int SimCtrlCatchMathError = 0;

void
EnableCatchMathError()
{
  SimCtrlCatchMathError = 1;
  SimCtrlMathError = 0;
}

int
DetectedMathError()
{
  return( SimCtrlMathError );
}

void
DisableCatchMathError()
{
  SimCtrlCatchMathError = 0;
  SimCtrlMathError = 0;
}

int
matherr(register struct exception *x)
{
  if( SimCtrlCatchMathError ){
    SimCtrlMathError = 1;
    return( 1 );
  }
  return (0); /* all other exceptions, execute default procedure */
}

//----------------------------------------------------------------
// Spectral estimation of the variance of a random sequence.
//----------------------------------------------------------------

#define N_V          100L

#include "tstud.h" /* t_student() */

#define N_SP      25                   /* no. of points of the periodogram */
#define C1        0.882                /* Heidelberger and Weltch C1(25,2) */
#define C2        7                    /* Heidelberger and Weltch C2(25,2) */

#define N_SP2     (N_SP+N_SP)
#define N_V2      (N_V+N_V)

double
t_student_7( double  ialfa, double *oalfa)
{
  return( t_student( C2, ialfa, oalfa ) );
}

#define deter( eq, c0, c1, c2 ) ( \
		  (eq[0][c0]*(eq[1][c1]*eq[2][c2]-eq[1][c2]*eq[2][c1])) - \
		  (eq[1][c0]*(eq[0][c1]*eq[2][c2]-eq[0][c2]*eq[2][c1])) + \
		  (eq[2][c0]*(eq[0][c1]*eq[1][c2]-eq[0][c2]*eq[1][c1])) )

static double
compute_a0( double *L)
{
  /* least squares extrapolation procedure */
  double eq[3][4];

  int i,j;
  double x1,x2,y1;
  double c;

  for( i = 0; i < 3; i++ )
    for( j = 0; j < 4; j++ )
      eq[i][j] = 0.0;

  c = 4.0/N_V2;
  for( x1 = 3.0 / N_V2, i = 0; i < N_SP; i++, x1+=c ){ /* Fn = (4i+3)/2Nv; i= 0...Nsp-1 */
    x2 = x1 * x1;
    y1 = L[i]+0.270;

    eq[0][1] += x1;
    eq[0][2] += x2;
    eq[0][3] += y1;

    eq[1][2] += x2 * x1;
    eq[1][3] += y1 * x1;

    eq[2][2] += x2 * x2;

    eq[2][3] += y1 * x2;
  }

  eq[0][0] = N_SP;
  eq[1][0] = eq[0][1];
  eq[1][1] = eq[0][2];
  eq[2][0] = eq[0][2];
  eq[2][1] = eq[1][2];
  return( deter(eq,3,1,2) / deter(eq,0,1,2) ); /* kramer */
}

static double COS[N_V] = { /* cos( 2 * PI * k /N_V ), for k = 0...N_V-1 */
    1 ,0.998027 ,0.992115 ,0.982287 ,0.968583 ,0.951057 ,
    0.929776 ,0.904827 ,0.876307 ,0.844328 ,0.809017 ,0.770513 ,
    0.728969 ,0.684547 ,0.637424 ,0.587785 ,0.535827 ,0.481754 ,
    0.425779 ,0.368125 ,0.309017 ,0.24869 ,0.187381 ,0.125333 ,
    0.0627905 ,-1.60814e-16 ,-0.0627905 ,-0.125333 ,-0.187381 ,-0.24869 ,
    -0.309017 ,-0.368125 ,-0.425779 ,-0.481754 ,-0.535827 ,-0.587785 ,
    -0.637424 ,-0.684547 ,-0.728969 ,-0.770513 ,-0.809017 ,-0.844328 ,
    -0.876307 ,-0.904827 ,-0.929776 ,-0.951057 ,-0.968583 ,-0.982287 ,
    -0.992115 ,-0.998027 ,-1 ,-0.998027 ,-0.992115 ,-0.982287 ,
    -0.968583 ,-0.951057 ,-0.929776 ,-0.904827 ,-0.876307 ,-0.844328 ,
    -0.809017 ,-0.770513 ,-0.728969 ,-0.684547 ,-0.637424 ,-0.587785 ,
    -0.535827 ,-0.481754 ,-0.425779 ,-0.368125 ,-0.309017 ,-0.24869 ,
    -0.187381 ,-0.125333 ,-0.0627905 ,-1.83691e-16 ,0.0627905 ,0.125333 ,
    0.187381 ,0.24869 ,0.309017 ,0.368125 ,0.425779 ,0.481754 ,
    0.535827 ,0.587785 ,0.637424 ,0.684547 ,0.728969 ,0.770513 ,
    0.809017 ,0.844328 ,0.876307 ,0.904827 ,0.929776 ,0.951057 ,
    0.968583 ,0.982287 ,0.992115 ,0.998027
};

static double SIN[N_V] = { /* sin( 2 * PI * k /N_V ), for k = 0...N_V-1 */
    0 ,0.0627905 ,0.125333 ,0.187381 ,0.24869 ,0.309017 ,
    0.368125 ,0.425779 ,0.481754 ,0.535827 ,0.587785 ,0.637424 ,
    0.684547 ,0.728969 ,0.770513 ,0.809017 ,0.844328 ,0.876307 ,
    0.904827 ,0.929776 ,0.951057 ,0.968583 ,0.982287 ,0.992115 ,
    0.998027 ,1 ,0.998027 ,0.992115 ,0.982287 ,0.968583 ,
    0.951057 ,0.929776 ,0.904827 ,0.876307 ,0.844328 ,0.809017 ,
    0.770513 ,0.728969 ,0.684547 ,0.637424 ,0.587785 ,0.535827 ,
    0.481754 ,0.425779 ,0.368125 ,0.309017 ,0.24869 ,0.187381 ,
    0.125333 ,0.0627905 ,-3.21629e-16 ,-0.0627905 ,-0.125333 ,-0.187381 ,
    -0.24869 ,-0.309017 ,-0.368125 ,-0.425779 ,-0.481754 ,-0.535827 ,
    -0.587785 ,-0.637424 ,-0.684547 ,-0.728969 ,-0.770513 ,-0.809017 ,
    -0.844328 ,-0.876307 ,-0.904827 ,-0.929776 ,-0.951057 ,-0.968583 ,
    -0.982287 ,-0.992115 ,-0.998027 ,-1 ,-0.998027 ,-0.992115 ,
    -0.982287 ,-0.968583 ,-0.951057 ,-0.929776 ,-0.904827 ,-0.876307 ,
    -0.844328 ,-0.809017 ,-0.770513 ,-0.728969 ,-0.684547 ,-0.637424 ,
    -0.587785 ,-0.535827 ,-0.481754 ,-0.425779 ,-0.368125 ,-0.309017 ,
    -0.24869 ,-0.187381 ,-0.125333 ,-0.0627905
};

double
VarianceEstimation( double *x )
{
  double P[N_SP2];
  double L[N_SP];
  unsigned long   l;
  unsigned long   s;
  unsigned long   i;
  double x_sum;
  double y_sum;
  double a0;

  for( l = 0; l < N_SP2; l++ ){ /* B1 */
    for( x_sum = y_sum = 0.0, i = s = 0; s < N_V; s++ ){
	  /* i = (s * (l+1)) % N_V; */
      x_sum += x[s] * COS[ i ];
      y_sum += x[s] * SIN[ i ];
	  if( (i  += (l+1)) >= N_V )
		i -= N_V;
	}
    P[l] = (x_sum*x_sum) + (y_sum*y_sum);
  }

  for( l = 0; l < N_SP; l++ ) /* B2 */
    L[l] = log( (P[l+l] + P[l+l+1]) / N_V2 );


  /* now, we have to use least squares g = a0 + a1 * x + a2 * x^2 */

  a0 = compute_a0( L );

  /*
    p0 = C1 * exp(a0);
    var = p0/N_V = C1 * exp(a0) / N_V;
  */
  return( C1 * exp(a0) / N_V );
}

//----------------------------------------------------------------
// Initial transient period length analysis for removal of samples.
//----------------------------------------------------------------

#include <math.h> /* sqrt() */
#include <malloc.h>

#define MAX_FIRST_N0             100

typedef struct st_prv_tranctrl {
#define trUNDEF    -2
#define trERROR    -1
#define trEND       0
#define trINITIAL   1
#define trCOLLECT   2
  int               state; /* = trUNDEF;*/
  unsigned long     N0max;
  unsigned long     Nt;
  double            At;
  double            Gv;
  double            Ge;
  unsigned long     Dn;
  unsigned long     current_sample; /* = 0;*/
  unsigned long     n0;
  double            minval;
  double            maxval;
  double           *X; /* NULL; */
  double           *Xm; /*  = NULL; */
  double            Kt; /* sqrt(45)/(pow(Nt,1.5)*pow(Nv,0.5)) */
  double            TStudent7;
  double            T;
} *PRV_TRANCTRL;

TRANCTRL
InitTransitAnalysis( unsigned long n0_max, double a_t, double g_v, double g_e )
{
  PRV_TRANCTRL ps;

  if( (ps = (struct st_prv_tranctrl*)malloc(sizeof(struct st_prv_tranctrl)))==NULL)
	adios( "memory shortage" );

  ps->N0max = n0_max;
  ps->At = a_t;
  ps->Gv = g_v;
  ps->Ge = g_e;

  ps->state = trINITIAL;
  ps->current_sample = 0;

  ps->TStudent7 = t_student_7(a_t/2.0,NULL); /* assumes two-sided test */

  return( (TRANCTRL) ps );
}

int
TransitAnalysis( TRANCTRL sim, double val)
{
  PRV_TRANCTRL ps;

  ps = (PRV_TRANCTRL) sim;

  switch( ps->state ){
/*
   case trUNDEF:
    DefaultInitTransitAnalysis();
     no break
*/
   case trINITIAL:
    if( ps->current_sample == ps->N0max){
      ps->state = trERROR;
      break;
    }
    if( ps->current_sample == 0 ){
      ps->minval = ps->maxval = val;
      break;
    }
	if( ps->current_sample < (MAX_FIRST_N0 - 1) ){
      if( val >= ps->maxval ){
        ps->maxval = val;
        break;
      }
      if( val <= ps->minval ){
        ps->minval = val;
        break;
      }
    }
    ps->n0 =  ps->current_sample+1;
    ps->Dn = (unsigned long) (ps->Ge*ps->n0);
    ps->Nt = (unsigned long) (ps->Gv*N_V);
    if( ps->Dn > ps->Nt )
      ps->Nt = ps->Dn;
    if( (ps->n0+ps->Nt) > ps->N0max){
      ps->state = trERROR;
      break;
    }
    ps->Kt = sqrt( (double)45 / (ps->Nt*N_V)) / ps->Nt;
    if( (ps->X = (double *)malloc(ps->Nt*sizeof(double))) == NULL )
      adios( "not enough memory" );
    if( (ps->Xm = (double *)malloc(ps->Nt*sizeof(double))) == NULL )
      adios( "not enough memory" );

    ps->state = trCOLLECT;
    break;
   case trCOLLECT:
    {
      double var;
      double x;
      unsigned long k;
      int MathError;

      ps->X[ps->current_sample-ps->n0] = val;
      if( (ps->current_sample-ps->n0+1) < ps->Nt )
        break;
      /*
        when we get the Nt observations from No, we have to determine
        the variance of the last N_V observations out of the Nt ones
        */
      EnableCatchMathError();
      var = VarianceEstimation( &ps->X[ps->Nt-N_V] );
      MathError = DetectedMathError();
      DisableCatchMathError();
      if( !MathError ){
        /*
          Now we calculate the test statistic
        */
        for( ps->Xm[0] = ps->X[0], k = 1; k < ps->Nt; k++ )
          ps->Xm[k] = ps->Xm[k-1] + (ps->X[k]-ps->Xm[k-1])/(k+1);
        for( x = 0.0, k = 0; k < ps->Nt; k++ )
          x += (k+1) * (1 - (k+1)/ps->Nt) * (ps->Xm[ps->Nt-1]-ps->Xm[k]);
        ps->T = ps->Kt * x / sqrt(var);
        if( ps->T < 0.0 )
          ps->T = - ps->T; /* two-sided test */
        if( (ps->T <= ps->TStudent7)
            || ((ps->n0+ps->Dn+ps->Nt) > ps->N0max) // EE: max transient period reached should not preclude rest of simulation
        ){
          ps->state = trEND;
          break;
          /* WARNING: The Nt observations stored in X have to be used in
             SpectralAnalisys or BatchMeansAnalisys!!!
             */
        }
      }
      ps->n0 += ps->Dn;
      /*
        Now we discard Dn observations from the tested sequence. In this
        implementation, we just shift the values X[Dn]..X[Nt-1]
        to X[0]..X[Nt-Dn-1]
      */
      for( k = 0; k < ps->Nt-ps->Dn; k++ )
        ps->X[k] = ps->X[ps->Dn+k];
      if( (ps->n0+ps->Nt) > ps->N0max){
        ps->state = trEND; // trERROR; // EE: max transient period reached should not preclude rest of simulation
        break;
      }
    }
    break;
   default:
    break;
  }

  ps->current_sample++;

  if( ps->state == 0 )
    return( TRANEND );
  return( ( ps->state > 0 )? TRANCON: TRANERR );
}

void
GetResidualTransit( TRANCTRL sim, double **x, unsigned long  *n)
{
  PRV_TRANCTRL ps;

  ps = (PRV_TRANCTRL) sim;

  *x = ps->X;
  *n = ps->Nt;
}

void
FreeResidualTransit( TRANCTRL   sim)
{
  PRV_TRANCTRL ps;

  ps = (PRV_TRANCTRL) sim;

  free( ps->X );
  free( ps->Xm );
}

//----------------------------------------------------------------
// Sequential analysis of steady state simulation output data.
// Spectral method.
//----------------------------------------------------------------

#define max(x,y)                             (((x)>(y))?(x):(y))
#define min(x,y)                             (((x)<(y))?(x):(y))
#define lowint( d )                          ((unsigned long)(d))

#define M         100                                    /* no. of batches */
#define sN0        0L   /* no. of initial transitory observations discarded */

#define M2             (M+M)
#define DOUB(o)        ((o)+(o))

typedef struct st_prv_specctrl {
#define spUNDEF         -2                                /* Not initialized */
#define spERROR         -1                     /* Accuracy cannot be reached */
#define spEND            0                      /* Simulation may be stopped */
#define spCOLLECT        1                                  /* Initial state */
 int             state; /* = spUNDEF; */
 unsigned long   No;     /* Initial observations that were discarded */
 unsigned long   Nmax;             /*  max. length of simulation run */
 double          Ga;           /* checkpoint incremental coefficient */
 double          Alpha;                     /* 1 - ALPHA = conf. level */
 double          Emax;  /* max. relative precision of conf. interval */

 double          X[M2];             /* buffer for analysed sequence */
 unsigned long   batch_size;                     /* m in the article */
 unsigned long   last_checkpoint;             /* Wk-1 in the article */
 unsigned long   next_checkpoint;               /* Wk in the article */
 unsigned long   current_sample;                 /* i in the article */
 unsigned long   current_batch;                  /* j in the article */
 double          current_batch_sum;                /* sum in article */
 double          mean_so_far;     /* mean value of batches completed */

 double          last_Ep;
 double          last_mean;

 double          Ep;                  /* Relative precision obtained */
 double          TStudent7;/* upper critical point of t-distribution */
} *PRV_SPECCTRL;

SPECCTRL
InitSpectralAnalysis(
  unsigned long n_o,
  unsigned long n_max,
  double g_a,
  double alpha,
  double e_max
)
{
  PRV_SPECCTRL ps;

  if( (ps = (struct st_prv_specctrl*)malloc(sizeof(struct st_prv_specctrl)))==NULL)
	adios( "memory shortage" );

  ps->No = n_o;
  ps->Nmax = n_max;
  ps->Ga = g_a;

  ps->TStudent7 = t_student_7( alpha, &ps->Alpha );

  ps->Emax = e_max;
  ps->last_Ep = ps->Ep = 1.0; /* initially, the error is 100 % */

  ps->batch_size = 1; /* initial batch size */
  ps->current_sample = 0;
  ps->last_checkpoint = n_o; /* Wk-1 */
  ps->next_checkpoint = 2 * max(M,n_o); /* Wk */
  ps->current_batch_sum = 0.0;
  ps->current_batch = 0;
  ps->last_mean = ps->mean_so_far = 0.0;
  ps->state = spCOLLECT;

  return( (SPECCTRL) ps );
}

/*
#define DefaultInitSpectralAnalysis() InitSpectralAnalysis( sN0, N_MAX, G_A, ALPHA, E_MAX )
*/

static void
EvaluatePrecision( PRV_SPECCTRL ps)
/*
  The spectral analysis of the variance is done over the last N_V batch
  means,  that is, from X[current_batch-N_V] to X[current_batch-1].
  (X[current_batch] is not still completed).

  Note that current_batch is always >= M when estimation is called.

  Ep = T_Student(7,1-Alpha/2) * sqrt(var) / mean_so_far;
*/
{
  int MathError;
  double NewEp;

  EnableCatchMathError();
  NewEp = ps->TStudent7*sqrt(VarianceEstimation(&ps->X[ps->current_batch-N_V]))/ps->mean_so_far;
  MathError = DetectedMathError();
  DisableCatchMathError();

  if( !MathError ){
	if( NewEp < ps->last_Ep ){
	  ps->last_mean = ps->mean_so_far;
	  ps->last_Ep = NewEp;
	}
    ps->Ep = NewEp;
  }
}

void
GetSpectralAnalysisResults( SPECCTRL       sim, unsigned long *nsamples, double        *mean, double        *cl, double        *ep,
 unsigned long   *last_chk,              unsigned long   *next_chk,                unsigned long   *batch_s)

{
  PRV_SPECCTRL ps;

  ps = (PRV_SPECCTRL) sim;

/*  if( ps->state == spUNDEF )
    DefaultInitSpectralAnalysis();
*/
  if( (ps->current_sample + ps->No) != ps->last_checkpoint )
    EvaluatePrecision(ps);

  *nsamples = ps->current_sample;
  *mean = ps->last_mean;
  *cl = 1 - ps->Alpha;
  *ep = ps->last_Ep;
  *last_chk = ps->last_checkpoint;
  *next_chk = ps->next_checkpoint;
  *batch_s = ps->batch_size;
}

int
SpectralAnalysis(
  SPECCTRL       sim,
  double         val /* new observation value: x(wo+i) */
)
{
  PRV_SPECCTRL ps;

  ps = (PRV_SPECCTRL) sim;

  switch( ps->state ){
/*   case spUNDEF:
    DefaultInitSpectralAnalysis();
     no break
*/
   case spCOLLECT:
   case spEND: /* it will go on collecting samples if you insist */
   case spERROR: /* it will go on collecting samples if you insist */
    ps->current_batch_sum += val;

    if( ((ps->current_sample+1) % ps->batch_size ) == 0 ){ /* batching */
      ps->X[ps->current_batch] = ps->current_batch_sum / ps->batch_size;
      ps->mean_so_far += (ps->X[ps->current_batch]-ps->mean_so_far)/(double)(ps->current_batch+1);
      ps->current_batch_sum = 0.0;
      if( ++ps->current_batch == M2 ){ /* consolidate: 2M x m -> M x 2m */
        for( ps->current_batch = 0; ps->current_batch < M; ps->current_batch++ )
          ps->X[ps->current_batch] = 0.5 * (ps->X[ps->current_batch+ps->current_batch] +
									ps->X[1+ps->current_batch+ps->current_batch]);
        ps->batch_size += ps->batch_size; /* batch_size *= 2; */
      }
    }

    if( (ps->current_sample+1+ps->No) == ps->next_checkpoint ){ /* i == Wk */
      ps->last_checkpoint = ps->next_checkpoint;
      ps->next_checkpoint = lowint(ps->Ga * (ps->last_checkpoint-ps->No))+ps->No;
      if( ps->state == spCOLLECT && ps->next_checkpoint > ps->Nmax )
        ps->next_checkpoint = ps->Nmax;

      EvaluatePrecision(ps);
      if( ps->Ep <= ps->Emax ){
        ps->state = spEND;
        break;
      }
    }


    if( (ps->state == spCOLLECT) && (ps->current_sample > (ps->Nmax-ps->No)) ){
      ps->state = spERROR;
      break;
    }
    break;
   default:
    adios( "bad state" );
    break;
  }

  ps->current_sample++;

  if( ps->state == spEND )
	return( SPECEND );
  return( ( ps->state > 0 )? SPECCON: SPECERR );
}

//----------------------------------------------------------------
// Sequential analysis of steady state simulation output data.
// Batch means method.
//----------------------------------------------------------------

#define M0             50L              /* default batch size for ref seq. */
#define bN0             0L     /* no. of init. transitory samples discarded */
#define KB0            100L      /* no. of batches stored in analysed seq. */
#define N_LAGS         10             /* no. of autocorr. coeffs.: 0.1*KB0 */
#define ADD_KB0        30L         /* Kbo for additional test of precision */

typedef struct st_sequence{         /* linked list of arrays for ref. seq. */
  struct st_sequence  *next;
  unsigned             n;
  double               X[KB0];
} SEQUENCE;

typedef struct st_prv_batchmctrl {
#define bmUNDEF         -3                                /* Not initialized */
#define bmERROR         -2                     /* Accuracy cannot be reached */
#define bmERROR_SIZE    -1                /* Batch size cannot be determined */
#define bmEND            0                      /* Simulation may be stopped */
#define bmCOLLECT_SIZE   1         /* Initial state for determine batch size */
#define bmCOLLECT_EST    2                /* Estimate and test the precision */
  int             state; /* = bmUNDEF; */
/*-------------------------------------------------------------------------*/
  unsigned long       No;      /* Initial observations that were discarded */
  unsigned long       Nmax;              /*  max. length of simulation run */
  double              Alpha;                    /* 1 - ALPHA = conf. level */
  double              Emax;   /* max. relative precision of conf. interval */
/*-------------------------------------------------------------------------*/
  unsigned long        current_sample;                 /* i in the article */
  double               current_batch_sum;                /* sum in article */
  SEQUENCE            *ref_sequence; /* = NULL;  */      /* reference seq. */
  SEQUENCE            *current_batch; /* = NULL; */      /* ~ j in article */
  unsigned long        step;          /* sequential step, s in the article */
  unsigned long        Kbe;/* size of Reference Seq. when begin estimation */
  unsigned long        m_uncorrelated; /* batch size of uncorrelated means */
  int                  acceptable_size; /* = 0; */

  double               last_Kbe;  /* last kbe when precision was evaluated */
  double               last_mean;                    /* best mean obtained */
  double               last_Ep;        /* best relative precision obtained */
} *PRV_BATCHMCTRL;
/*-------------------------------------------------------------------------*/

static void
free_batchs( PRV_BATCHMCTRL ps)
{
  SEQUENCE *tmp;

  for( ; ps->ref_sequence != NULL; ){
    tmp = ps->ref_sequence;
    ps->ref_sequence = ps->ref_sequence->next;
    free( tmp );
  }
}

static void
store_batch( PRV_BATCHMCTRL ps, double         val)
{
  SEQUENCE *pt;

  if( ps->ref_sequence == NULL ){
    if( (pt = (SEQUENCE *)malloc(sizeof(SEQUENCE))) == NULL)
      adios(" not enough memory");
    pt->next = NULL;
    pt->X[0] = val;
    pt->n = 1;
    ps->ref_sequence = ps->current_batch = pt;
    return;
  }

  if( ps->current_batch->n < KB0 ){
    ps->current_batch->X[ps->current_batch->n] = val;
    ps->current_batch->n++;
    return;
  }

  /* else a new struct must be allocated */
  if( (pt = (SEQUENCE *)malloc(sizeof(SEQUENCE))) == NULL)
    adios(" not enough memory");
  pt->next = NULL;
  pt->X[0] = val;
  pt->n = 1;
  ps->current_batch->next = pt;
  ps->current_batch = pt;
}

static void
prepare_est( PRV_BATCHMCTRL ps, double        *y)
{
  unsigned long i;

  for( i = 0; i < KB0; i++ ) /* prepare data for estimation */
    store_batch( ps, y[i] );
}

static void
consolidate( PRV_BATCHMCTRL ps, double        *y, unsigned long  step)
{
  SEQUENCE *pt;
  unsigned long i,j,k;

 /* step*KB0 means X(Mo) -> KB0 means Y(step*M0) */
  for( pt = ps->ref_sequence, k = 0, i = 0; i < KB0; i++){
    for( y[i] = 0.0, j = 0; j < step ; j++){
      y[i] +=  pt->X[k];
      if( ++k == KB0 ){
        pt = pt->next;
        k = 0;
      }
    }
    y[i] /= step;
  }
}

static double
R( unsigned long  k,   /* lag */
     double        *x,   /* points to first sample to consider */
     unsigned long  n,   /* no. of samples to consider */
     double         m   /* mean value of the samples to consider */
)
{
  unsigned long i;
  double        r;

  for( r = 0.0, i = 0; i < n - k; i++ )
    r += (x[i]-m)*(x[i+k]-m);

  return( r );
}

static int
Uncorrelated( PRV_BATCHMCTRL ps, double        *x, unsigned long  kb)
{
  unsigned long k,u;
  unsigned long kb_2;                                              /* kb/2 */
  double m0, m1, m2; /* mean value of whole, left-half and right-half seqs.*/
  double R00, R01, R02;                          /* estimators of autocov. */
  double r0, r1, r2;                    /* estimators of autocorr. coeffs. */
  double r[N_LAGS];           /* jackknife estimators of autocorr. coeffs. */
  double var_r;                           /* variance of autocorr. coeffs. */
  double corr;
  double abs_r;
  double z;           /* upper 1-Bk/2 critical point standard distribution */

  /* calculates mean value of analysed sequence and two half-sequences */
  kb_2 = kb/2;
  for( m1 = 0.0, k = 0; k < kb_2; k++ )
    m1 += x[k];
  for( m2 = 0.0; k < kb; k++ )
    m2 += x[k];
  m0 = (m1 + m2)/kb;
  m1 /= kb_2;
  m2 /= kb_2;

  R00 = R( 0L, x, kb, m0 );
  R01 = R( 0L, &x[0], kb_2, m1 );
  R02 = R( 0L, &x[kb_2], kb_2, m2 );

  /* betak = beta / N_LAGS;   PETER: Mirar lo del Bk, pongo BETA cte. */
  z = t_student(0,BETA/N_LAGS,(double *)NULL);

  for( corr = 0.0, k = 0; k < N_LAGS; k++){
    r0 = R( k+1, x, kb, m0) / R00;
    r1 = R( k+1, &x[0], kb_2, m1 ) / R01;
    r2 = R( k+1, &x[kb_2], kb_2, m2 ) / R02;
    r[k] = (r0 + r0) - ((r1 + r2)/2);
    if( k == 0 )
      var_r = 1.0 / (double) kb;
    else{
      for( var_r = 0.0, u = 0; u < k-1; u++)
        var_r += (r[u] * r[u]);
      var_r = (1 +(var_r + var_r) ) / kb;
    }
    abs_r = ( (r[k] < 0.0)? (-r[k]):(r[k]) );

    if( abs_r < (z*sqrt(var_r)) )
      r[k] = abs_r = 0.0;
    corr += abs_r;
  }

  if( (corr == 0.0 ) && ps->acceptable_size)
    return(1); /* Uncorrelated */

  if( corr == 0.0 )
    ps->acceptable_size = 1;
  else
    ps->acceptable_size = 0;

  return(0); /* Still correlated */
}

static double
EvaluatePrecision( PRV_BATCHMCTRL ps)
{
  SEQUENCE *pt;        /* auxiliar pointer to Reference Sequence*/
  unsigned long i, j, k, m;
  double   Xmean;     /* mean of the Kbe batch means in Ref. Sec. */
  double   Xvar;
  double   NewEp;
  double   NewEp2;
  double   X[ADD_KB0]; /* analysed sequence for additional test */

  ps->last_Kbe = ps->Kbe; /* avoid recalculate prec. if not a new kbe */
  for( pt = ps->ref_sequence, Xmean = 0.0, Xvar = 0.0, j= i = 0; j < ps->Kbe; j++){
    Xmean += pt->X[i];
    Xvar += pt->X[i] * pt->X[i];
    if( ++i == KB0){
      pt = pt->next;
      i = 0;
    }
  }
  Xmean /= ps->Kbe;
  Xvar = ((Xvar/ps->Kbe)-Xmean*Xmean)/(ps->Kbe-1);
  NewEp = t_student((unsigned)(ps->Kbe-1),ps->Alpha,(double *)NULL)*sqrt(Xvar)/Xmean;

  if( (NewEp > ps->Emax) && ((ps->Kbe % ADD_KB0) == 0) ) {
    for( Xvar=0.0, m=ps->Kbe/ADD_KB0, pt=ps->ref_sequence, i = j = 0;
		j < ADD_KB0; j++ ){
      for( X[j] = 0.0, k = 0; k < m ; k++ ){
        X[j] +=  pt->X[i];
        if( ++i ==  KB0){
          i = 0;
          pt = pt->next;
        }
      }
      X[j] /= m;
	  Xvar += X[j]*X[j];
    }

	Xvar = ((Xvar/ADD_KB0)-Xmean*Xmean)/(ADD_KB0-1);

    NewEp2=t_student((unsigned)(ADD_KB0-1),ps->Alpha,(double*)NULL)
      * sqrt(Xvar) / Xmean;

    if( NewEp2 < NewEp )
      NewEp = NewEp2;
  }

  ps->last_mean = Xmean;
  ps->last_Ep = NewEp;

  return( NewEp );

}

BATCHMCTRL
InitBatchMAnalysis( unsigned long    n_o, unsigned long    n_max, double           alpha, double           e_max)
{
  PRV_BATCHMCTRL ps;

  if( (ps = (struct st_prv_batchmctrl*)malloc(sizeof(struct st_prv_batchmctrl)))==NULL)
	adios( "memory shortage" );

  ps->No = n_o;
  ps->Nmax = n_max;
  t_student(0,alpha,&ps->Alpha); /* get the nearest tabulated alpha */
  ps->Emax = e_max;

  ps->current_sample = 0;  /*i = 1*/
  ps->current_batch_sum = 0.0;
  ps->ref_sequence = NULL;
  ps->current_batch = NULL;
  ps->step = 1;
  ps->Kbe = 0;
  ps->m_uncorrelated = 0;
  ps->acceptable_size = 0;


  ps->last_Kbe = 0;
  ps->last_mean = 0.0;
  ps->last_Ep = 1; /* - Emax; ??? */

  ps->state = bmCOLLECT_SIZE;

  return( (BATCHMCTRL) ps );
}

/*
#define DefaultInitBatchMAnalysis() InitBatchMAnalysis(bN0, N_MAX, ALPHA, E_MAX)
*/

void
GetBatchMAnalysisResults(
     BATCHMCTRL     sim,
     unsigned long *nsamples,
     double        *mean,
     double        *cl,
     double        *ep,
     unsigned long *step,
     unsigned long *n_batch,
     unsigned long *batch_s)
{
  PRV_BATCHMCTRL ps;

  ps = (PRV_BATCHMCTRL) sim;

/*
  if( state == bmUNDEF )
    DefaultInitBatchMAnalysis();
*/

  if( ps->Kbe != ps->last_Kbe )
    EvaluatePrecision(ps);

  *nsamples = ps->current_sample;
  *mean = ps->last_mean;
  *cl = 1 - ps->Alpha;
  *ep = ps->last_Ep;

  *step = ps->step;
  *n_batch = ps->Kbe;
  *batch_s = ps->m_uncorrelated;
}

int
BatchMAnalysis(
  BATCHMCTRL     sim,
  double         val /* new observation value: x(wo+i) */
)
{
  PRV_BATCHMCTRL ps;
  double          Y[KB0];                  /* buffer for Analysed Sequence */


  ps = (PRV_BATCHMCTRL) sim;

  switch( ps->state ){
/*   case bmUNDEF:
    DefaultInitBatchMAnalysis();
     no break
*/

   case bmERROR_SIZE: /* if you insist ... */
   case bmCOLLECT_SIZE:
    ps->current_batch_sum += val;

    if( ((ps->current_sample + 1) % M0) != 0)
      break;  /* ps->state = bmCOLLECT_SIZE */

    store_batch( ps, ps->current_batch_sum/M0 );
    ps->current_batch_sum = 0.0;

    if( ps->current_batch->n < KB0 )
      break;  /* ps->state = bmCOLLECT_SIZE */

    consolidate( ps, Y, ps->step );

    if( !Uncorrelated( ps, Y, KB0 ) ){
      ps->step++;
      if( ps->state == bmCOLLECT_SIZE && ((ps->No + ps->current_sample +(KB0 * M0))> ps->Nmax) )
        ps->state = bmERROR_SIZE; /*  m_uncorr. cannot be determined (nmax) */
      break; /* ps->state = bmCOLLECT_SIZE */
    }

    ps->m_uncorrelated = ps->step * M0;
    ps->Kbe = KB0;

    free_batchs(ps);
	prepare_est( ps, Y ); /* prepare data for estimation */
    ps->current_batch_sum = 0.0;
    ps->state = bmCOLLECT_EST;

    if( EvaluatePrecision(ps) <= ps->Emax ){
      ps->state = bmEND;
      break;
    }

    if(ps->No + ps->current_sample + ps->m_uncorrelated +1 > ps->Nmax){
      ps->state = bmERROR;
      break;
    }

    break;


   case bmERROR: /* if you insist */
   case bmEND:   /* if you insist */
   case bmCOLLECT_EST:
    ps->current_batch_sum += val;

    if( ((ps->current_sample + 1) % ps->m_uncorrelated) != 0)
      break;  /*  ps->state = bmCOLLECT_EST */

    store_batch( ps, ps->current_batch_sum / ps->m_uncorrelated );
    ps->current_batch_sum = 0.0;
    ps->Kbe++;

    if( EvaluatePrecision(ps) <= ps->Emax ){
      ps->state = bmEND;
      break;
    }

    if( (ps->state != bmERROR) && (ps->No + ps->current_sample + ps->m_uncorrelated +1 > ps->Nmax)){
      ps->state = bmERROR;
      break;
    }

    break;
   default:
    adios( "unknown state" );
    break;
  }

  ps->current_sample++;

  if (ps->state == 0)
    return(BATEND);
  return( (ps->state > 0)? BATCON: BATERR);
}

//----------------------------------------------------------------
// Top level Part.
//----------------------------------------------------------------

#define CKEND  10000

typedef struct st_prv_simctrl {
#define UNDEF    -3
#define ERROR    -2
#define LONG     -1
#define END       0
#define TRANSIENT 1
#define STEADY    2
  int                            state; /* = UNDEF */
//define SPECTRAL         0
//define BATCH_MEANS      1
  int                            Method;
  unsigned long                  Nmax;
  unsigned long                  N0max;
  double                         At;
  double                         Gv;
  double                         Ge;
  double                         Ga;
  double                         Alpha;
  double                         Emax;
  unsigned long                  n0;
  unsigned long                  current_sample;  /* = 0;*/
  TRANCTRL                       tc;
  SPECCTRL                       sc;
  BATCHMCTRL                     bc;
  int                            finished;
  unsigned long                  ckend; /* mac 4/10/2002 check premature end */
} *PRV_SIMCTRL;

extern double Tduracion;
extern double Ttransitorio;

static int n_simctrl = 0;

//----------------------------------------------------------------

SIMCTRL
InitBatchMeansSimControl(
	 unsigned long    n_max,
	 unsigned long    n0_max,
	 double           a_t,
	 double           g_v,
	 double           g_e,
	 double           alpha,
	 double           e_max
)
{
  PRV_SIMCTRL ps;

  if( (ps = (struct st_prv_simctrl*)malloc(sizeof(struct st_prv_simctrl)))==NULL)
	adios( "memory shortage" );

  ps->Method = BATCH_MEANS;
  ps->Nmax = n_max;
  ps->N0max = n0_max;
  ps->At = a_t;
  ps->Gv = g_v;
  ps->Ge = g_e;
  ps->Alpha = alpha;
  ps->Emax = e_max;

  ps->n0 = 0;

  ps->tc = ps->sc = ps->bc = NULL;

  if( n0_max == 0 ){ /* no transitory analysis requested */
    ps->bc = InitBatchMAnalysis( 0L, n_max, alpha, e_max );
    ps->state = STEADY;
  } else {
    ps->tc = InitTransitAnalysis( n0_max, a_t, g_v, g_e );
    ps->state = TRANSIENT;
  }
  ps->current_sample = 0;

  ps->finished = 0;
  ps->ckend = 0;
  n_simctrl++;
  return( (SIMCTRL) ps );
}

//----------------------------------------------------------------

SIMCTRL
InitSpectralSimControl(
	 unsigned long    n_max,
	 unsigned long    n0_max,
	 double           a_t,
	 double           g_v,
	 double           g_e,
	 double           g_a,
	 double           alpha,
	 double           e_max
)
{
  PRV_SIMCTRL ps;

  if( (ps = (struct st_prv_simctrl*)malloc(sizeof(struct st_prv_simctrl)))==NULL)
	adios( "memory shortage" );

  ps->Method = SPECTRAL;
  ps->Nmax = n_max;
  ps->N0max = n0_max;
  ps->At = a_t;
  ps->Gv = g_v;
  ps->Ge = g_e;
  ps->Ga = g_a; /* unused in batch means method */
  ps->Alpha = alpha;
  ps->Emax = e_max;

  ps->n0 = 0;

  ps->tc = ps->sc = ps->bc = NULL;

  if( n0_max == 0 ){ /* no transitory analysis requested */
    ps->sc = InitSpectralAnalysis( 0L, n_max, g_a, alpha, e_max );
    ps->state = STEADY;
  } else {
    ps->tc = InitTransitAnalysis( n0_max, a_t, g_v, g_e );
    ps->state = TRANSIENT;
  }
  ps->current_sample = 0;

  ps->finished = 0;
  ps->ckend = 0;
  n_simctrl++;
  return( (SIMCTRL) ps );
}

/*
#define DefaultInitSimControl() InitSpectralSimControl( N_MAX, \
                                               N0_MAX, A_T, G_V, G_E, \
                                               G_A, ALPHA, E_MAX)
*/

//----------------------------------------------------------------

void
GetSimControlResults(
	 SIMCTRL        sim,
     unsigned long *ntransient,
     unsigned long *nsteady,
     double        *mean,
     double        *cl,
     double        *ep,

	 unsigned long *par1,
	 unsigned long *par2,
     unsigned long *batch_s
)
{
  PRV_SIMCTRL ps;

  ps = (PRV_SIMCTRL) sim;

/*  if( ps->state == UNDEF )
    ps = DefaultInitSimControl();
*/

  *ntransient = ps->n0;
  *nsteady = ps->current_sample;
  *ep = 1.0;
  *mean = 0.0;
  *cl = 0.0;
  if (ps->Method == BATCH_MEANS && ps->bc != NULL )
    GetBatchMAnalysisResults( ps->bc, nsteady, mean, cl, ep, par1, par2, batch_s );
  else if( ps->sc != NULL )
    GetSpectralAnalysisResults( ps->sc, nsteady, mean, cl, ep, par1, par2, batch_s );
  if (ps->state==TRANSIENT) *batch_s=0;
}

//----------------------------------------------------------------

static void
ck_prem_end(PRV_SIMCTRL ps)
{
  unsigned long ntransient;
  unsigned long nsteady;
  double        mean;
  double        cl;
  double        ep;

  unsigned long par1;
  unsigned long par2;
  unsigned long batch_s;

  ps->ckend++;
  if(ps->ckend<CKEND)
    return;
  ps->ckend = 0;

  ep = 1.0;
  if (ps->Method == BATCH_MEANS && ps->bc != NULL )
    GetBatchMAnalysisResults( ps->bc, &nsteady, &mean, &cl, &ep, &par1, &par2, &batch_s );
  else if( ps->sc != NULL )
    GetSpectralAnalysisResults( ps->sc, &nsteady, &mean, &cl, &ep, &par1, &par2, &batch_s );

  if(!(ep>ps->Emax))
    ps->state = END;
}

//----------------------------------------------------------------

int
SimControl(SIMCTRL sim, double val)
{
  double         *x;
  unsigned long   n;
  unsigned long   i;
  PRV_SIMCTRL     ps;

  if (Time() <= Ttransitorio) return (SIMCTRLCON);

  ps = (PRV_SIMCTRL) sim;

  switch( ps->state ){
/*
   case UNDEF:
     DefaultInitSimControl();
     no break
*/
   case TRANSIENT:
	switch( TransitAnalysis( ps->tc, val ) ){
	 case TRANERR:
	  ps->state = ERROR;
	  break;
	 case TRANEND:
	  ps->state = STEADY;
      /* process first Nt samples used in TransitAnalysis after n0 */
	  GetResidualTransit( ps->tc, &x, &n );
	  ps->n0 = ps->current_sample+1-n;
      if (ps->Method == BATCH_MEANS){
        ps->bc=InitBatchMAnalysis( ps->n0, ps->Nmax, ps->Alpha, ps->Emax );
	    for( i = 0; i < n; i++, x++ ){
		 switch( BatchMAnalysis( ps->bc, *x ) ){
		  case BATCON:
		   break;
		  case BATERR:
		   if( ps->state == STEADY ) ps->state = LONG;
		   break;
		  case BATEND:
		   ps->state = END;
		   break;
		 }
	    }
	  }
      else{
        ps->sc = InitSpectralAnalysis( ps->n0, ps->Nmax, ps->Ga, ps->Alpha, ps->Emax );
	    for( i = 0; i < n; i++, x++ ){
		 switch( SpectralAnalysis( ps->sc, *x ) ){
		  case SPECCON:
		   break;
		  case SPECERR:
		   if( ps->state == STEADY ) ps->state = LONG;
		   break;
		  case SPECEND:
		   ps->state = END;
		   break;
		 }
	    }
	  }
	  FreeResidualTransit(ps->tc);
	  break;
	 case TRANCON:
	  break;
	}
	break;
   case LONG: /* if you insist... */
   case END: /* if you insist... */
    if (ps->Method == BATCH_MEANS)
      BatchMAnalysis(ps->bc,val);
    else
      SpectralAnalysis(ps->sc,val);
	break;
   case STEADY:
    if (ps->Method == BATCH_MEANS){
	 switch( BatchMAnalysis( ps->bc, val ) ){
	  case BATERR:
	   ps->state = LONG;
	   break;
	  case BATEND:
	   ps->state = END;
	   break;
	  case BATCON:
	   break;
	 }
    } else {
     switch( SpectralAnalysis( ps->sc, val ) ){
	  case SPECERR:
	   ps->state = LONG;
	   break;
	  case SPECEND:
	   ps->state = END;
	   break;
	  case SPECCON:
	   break;
	 }
    }
	break;
   default:
	break;
  }

  ps->current_sample++;

  ck_prem_end(ps);

  if( ps->state == 0 ){
	if( ps->finished == 0 ) {
	  ps->finished = 1;
	  n_simctrl--;
	  if (n_simctrl == 0) {
		  Tduracion = Time() - Ttransitorio;
		  Reactivate(Main(), delay, 0.0);
	  }
	}
	return( SIMCTRLEND );
  }
  if( ps->state > 0 )
	return( SIMCTRLCON );

  return( ( ps->state == LONG )? SIMCTRLLON: SIMCTRLERR );
}

//----------------------------------------------------------------

int
FinishedSimControl()
{
  return( n_simctrl <= 0 );
}

//----------------------------------------------------------------
