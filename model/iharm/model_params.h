#ifndef MODEL_PARAMS_H
#define MODEL_PARAMS_H

#define SLOW_LIGHT (0)

// Model-specific definitions and globals
#define KRHO 0
#define UU   1
#define U1   2
#define U2   3
#define U3   4
#define B1   5
#define B2   6
#define B3   7
#define KEL  8
#define KTOT 9

extern double DTd;
extern double sigma_cut;
extern double sigma_transition;
extern int counterjet;
extern int electronModel;
extern double rmax_geo;
extern double constant_beta_e0;
extern double beta_crit;
extern double beta_crit_coefficient;

#endif // MODEL_PARAMS_H
