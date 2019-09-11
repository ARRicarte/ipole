
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <gsl/gsl_math.h>
#include <gsl/gsl_sf_bessel.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_permutation.h>
#include <gsl/gsl_linalg.h>
#include "constants.h"
#include <complex.h> 
#include <omp.h>
#include "h5io.h"
#include "model.h"
#include "par.h"

#define NDIM	4

#define xstr(s) str(s)
#define str(s) #s

#define STRLEN (2048)

#define NIMG (4+1) // Stokes vector and faraday depth

/* mnemonics for primitive vars; conserved vars */
/*#define KRHO    0
#define UU      1
#define U1      2
#define U2      3
#define U3      4
#define B1      5
#define B2      6
#define B3      7
#define KEL     8
#define KTOT    9*/

/* numerical convenience */
#define SMALL	1.e-40
#define SIGMA_CUT (1.)

#define sign(x) (((x) < 0) ? -1 : ((x) > 0))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

#define IMLOOP for(int i=0;i<NDIM;i++) for(int j=0;j<NDIM;j++)
#define MULOOP for(int mu=0;mu<NDIM;mu++)                                        
#define MUNULOOP for(int mu=0;mu<NDIM;mu++) for(int nu=0;nu<NDIM;nu++) 

#define P4VEC(S,X) fprintf(stderr, "%s: %g %g %g %g\n", S, X[0], X[1], X[2], X[3]);

/* some coordinate parameters */
extern double a;

extern double theta_j;
extern double trat_j;
extern double trat_d;
extern int counterjet;
extern double rmax_geo;

//2d

extern double R0 ;
extern double Rin ;
extern double Rout ;
extern double Rh ;
extern double hslope ;
extern double th_len,th_beg;
extern double startx[NDIM], stopx[NDIM], dx[NDIM];
extern double gam ;
extern double DTd;
extern double t0;

/* HARM model globals */
extern double M_unit;
extern double L_unit;
extern double T_unit;
extern double RHO_unit;
extern double U_unit;
extern double B_unit;
extern double Te_unit;

extern int N1, N2, N3;

extern double freqcgs, thetacam, tf;

extern double levi_civita[NDIM][NDIM][NDIM][NDIM];
extern char fnam[STRLEN];

/** model-independent subroutines **/
/* core routines */
void init_XK(int i, int j, double Xcam[4], double fovx, double fovy, double X[4], double Kcon[4], double rotcam, double xoff, double yoff);
void null_normalize(double Kcon[NDIM], double fnorm) ;
void normalize(double *vcon, double gcov[][NDIM]);
double approximate_solve(double Ii, double ji, double ki, double jf, double kf, double dl, double *tau);
void get_jkinv(double X[NDIM], double Kcon[NDIM], double *jnuinv, double *knuinv);
int    stop_forward_integration(double X[NDIM], double Kcon[NDIM], double Xcam[NDIM]);
int    stop_backward_integration(double X[NDIM], double Kcon[NDIM], double Xcam[NDIM]);
void dump(double image[], double imageS[], double taus[], 
          const char *fname, double scale, double Dsource, double cam[NDIM], double DX, 
          double DY, double fovx, double fovy, double rcam, double thetacam, double phicam,
          double rotcam, double xoff, double yoff);

/* geodesic integration */
//void   push_photon(double X[NDIM], double Kcon[NDIM], double dl);
void   push_photon(double X[NDIM], double Kcon[NDIM], double dl, double Xhalf[NDIM],double Kconhalf[NDIM]);
void   push_photon_rk4(double X[NDIM], double Kcon[NDIM], double dl);
void   push_photon_gsl(double X[NDIM], double Kcon[NDIM], double dl);

/* model */
void   gcov_func(double *X, double gcov[][NDIM]);
void   gcov_func_rec(double *X, double gcov[][NDIM]);
int   gcon_func(double gcov[][NDIM], double gcon[][NDIM]);
double gdet_func(double gcov[][NDIM]);
void   get_connection(double *X, double lconn[][NDIM][NDIM]);
void   get_connection_num(double *X, double lconn[][NDIM][NDIM]);
void   lower(double *ucon, double Gcov[NDIM][NDIM], double *ucov);
void   raise(double *ucov, double Gcon[NDIM][NDIM], double *ucon);
double stepsize(double X[NDIM], double K[NDIM]);
void init_model(double *tA, double *tB);
double get_model_thetae(double X[NDIM]) ;
double get_model_b(double X[NDIM]) ;
double get_model_ne(double X[NDIM]) ;
void get_model_fourv(double X[NDIM], double Ucon[NDIM], double Ucov[NDIM], double Bcon[NDIM], double Bcov[NDIM]);
void get_model_bcov(double X[NDIM], double Bcov[NDIM]) ;
void get_model_bcon(double X[NDIM], double Bcon[NDIM]) ;
void get_model_ucov(double X[NDIM], double Ucov[NDIM]) ;
void get_model_ucon(double X[NDIM], double Ucon[NDIM]) ;
void update_data(double *tA, double *tB);
void update_data_until(double *tA, double *tB, double tgt);
void output_hdf5(hid_t fid);

/* harm utilities */
/*
void interp_fourv(double X[NDIM], double ***fourv, double Fourv[NDIM]) ;
double interp_scalar(double X[NDIM], double **var) ;
void Xtoij(double X[NDIM], int *i, int *j, double del[NDIM]) ;
*/
void   bl_coord(double *X, double *r, double *th);
void set_units();
//void init_physical_quantities(int n) ;

//for 3d coment out
/*
void *malloc_rank1(int n1, int size) ;
void **malloc_rank2(int n1, int n2, int size) ;
void ***malloc_rank3(int n1, int n2, int n3, int size) ;
void ****malloc_rank4(int n1, int n2, int n3, int n4, int size) ;
*/
void parse_input(int argc, char *argv[], Params *params);
void init_storage(void) ;

/* tetrad related */
void   coordinate_to_tetrad(double Ecov[NDIM][NDIM], double K[NDIM], double K_tetrad[NDIM]);
void   tetrad_to_coordinate(double Ecov[NDIM][NDIM], double K_tetrad[NDIM], double K[NDIM]);
double delta(int i, int j);
void   make_camera_tetrad(double X[NDIM], double Econ[NDIM][NDIM], double Ecov[NDIM][NDIM]);
void   make_plasma_tetrad(double Ucon[NDIM], double Kcon[NDIM], double Bcon[NDIM],
	        double Gcov[NDIM][NDIM], double Econ[NDIM][NDIM], double Ecov[NDIM][NDIM]) ;
void set_levi_civita();

/* imaging */
void make_ppm(double p[], double freq, char filename[]) ;
void rainbow_palette(double data, double min, double max, int *pRed, int *pGreen, int *pBlue) ;

/* radiation */
double Bnu_inv(double nu, double Thetae) ;
double jnu_inv(double nu, double Thetae, double Ne, double B, double theta) ;
double get_fluid_nu(double Kcon[NDIM], double Ucov[NDIM]) ;
double get_bk_angle(double X[NDIM], double Kcon[NDIM], double Ucov[NDIM], double Bcon[NDIM], double Bcov[NDIM]);

/* emissivity */ 
double jnu_synch(double nu, double Ne, double Thetae, double B, double theta) ;

#define DLOOP  for(k=0;k<NDIM;k++)for(l=0;l<NDIM;l++)

void init_N(double Xi[NDIM],double Kconi[NDIM],double complex Ncon[NDIM][NDIM]);
void evolve_N(double Xi[NDIM],double Kconi[NDIM],
	      double Xf[NDIM],double Kconf[NDIM],
	      double Xhalf[NDIM],double Kconhalf[NDIM],
	      double dlam,
	      double complex N_coord[NDIM][NDIM],
        double *tauF);
void project_N(double X[NDIM],double Kcon[NDIM],
	double complex Ncon[NDIM][NDIM],
	double *Stokes_I, double *Stokes_Q,double *Stokes_U,double *Stokes_V, double rotcam);





