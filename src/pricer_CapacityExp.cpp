#include "pricer_CapacityExp.h"
#include "scip/scipdefplugins.h"

#include <math.h>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <vector>

#include "objscip/objscip.h"
#include "scip/cons_linear.h"


using namespace std;
using namespace scip;

#define PRICER_NAME            "vaccine"
#define PRICER_DESC            "variable pricer template"
#define PRICER_PRIORITY        0
#define PRICER_DELAY           TRUE     /* only call pricer if all problem variables have non-negative reduced costs */


/** Constructs the pricer object with the data needed
 *
 *  An alternative is to have a problem data class which allows to access the data.
 */
ObjPricervaccine::ObjPricervaccine(
		SCIP*                                            scip,          /**< SCIP pointer */
		const char*                                      p_name,        /**< name of pricer */
		vector<double> &                                 p_purchcost,
		vector<int > &                                   p_vialsize,
		int                                              p_Nper_to_spoil,
		double                                           p_Wastbeta,
		int                                              p_T,
		int                                              p_NScen,
		int                                              p_Nnodes,
		int                                              p_K,
		int                                              p_I,
		int                                              p_n_pricing,
		int                                              p_nrestart,
		vector<double> &                                 p_Nodedemand,
		vector< vector<int> > &                          p_nodemat,
		vector< vector<int> > &                          p_jmat,
		vector < vector < vector < SCIP_VAR* > > > &     p_x_var,
		vector< SCIP_VAR* > &                            p_psi_var,
		vector< vector< SCIP_VAR * > > &                 p_wj_var,
		vector < vector < vector < vector < SCIP_CONS* > > > > &  p_wt_con,
		vector < vector < vector < vector < SCIP_CONS* > > > > & p_sec_con,
		vector<vector<SCIP_CONS* > > &                   p_z_con,
		SCIP_CONS*                                     & p_alpha_con,
		vector< vector<SCIP_CONS* > > &                   p_w_con
):
				   ObjPricer(scip, p_name, "Finds tour with negative reduced cost.", 0, TRUE),
				   _purchcost(p_purchcost),
				   _vialsize(p_vialsize),
				   _Nper_to_spoil(p_Nper_to_spoil),
				   _Wastbeta(p_Wastbeta),
				   _T(p_T),
				   _NScen(p_NScen),
				   _Nnodes(p_Nnodes),
				   _K(p_K),
				   _I(p_I),
				   _n_pricing(p_n_pricing),
				   _nrestart(p_nrestart),
				   _Nodedemand(p_Nodedemand),
				   _nodemat(p_nodemat),
				   _jmat(p_jmat),
				   _x_var(p_x_var),
				   _psi_var(p_psi_var),
				   _wj_var(p_wj_var),
				//   _wj_var2(p_wj_var2),
				   _wt_con(p_wt_con),
				   _sec_con(p_sec_con),
				   _z_con(p_z_con),
				   _alpha_con(p_alpha_con),
				   _w_con(p_w_con)
{}


/** Destructs the pricer object. */
ObjPricervaccine::~ObjPricervaccine()
{}

SCIP_DECL_PRICERINIT(ObjPricervaccine::scip_init)
{
	for (int i=0; i<_NScen; i++)
	{  //gets the transformed var and z_con
		SCIP_CALL( SCIPgetTransformedVar(scip, _psi_var[i], &_psi_var[i]) );
		for (int j=2; j<_T+1 ; j++)
		{
			SCIP_CALL( SCIPgetTransformedCons(scip, _z_con[i][j-2], &_z_con[i][j-2]) );
		}
	}


	for(int t=2; t<_T+1; t++)
	{
		int NN= min(_Nper_to_spoil,_T-t);
		for (int j=0 ; j<pow(_K,t-1) ; j++)
		{
			for (int i=0 ; i< _I ; i++)
			{
				SCIP_CALL( SCIPgetTransformedVar(scip, _x_var[t-2][j][i], &_x_var[t-2][j][i]) );
			}
			SCIP_CALL( SCIPgetTransformedCons(scip, _w_con[t-2][j], &_w_con[t-2][j]));
			SCIP_CALL( SCIPgetTransformedVar(scip, _wj_var[t-2][j], &_wj_var[t-2][j]) );
			//SCIP_CALL( SCIPgetTransformedVar(scip, _wj_var2[t-2][j], &_wj_var2[t-2][j]) );
			for (int n=0; n<pow(_K,NN); n++)
			{
				for (int q=0 ; q< pow(_K,_T-t) ; q++)
				{
					SCIP_CALL( SCIPgetTransformedCons(scip, _wt_con[t-2][j][n][q], &_wt_con[t-2][j][n][q]));
				}
			}
		}
	}

	for (int i=0 ; i< _I ; i++)
	{
		for(int t=2; t<_T+1; t++)
			{
				int NN= min(_Nper_to_spoil,_T-t);
				for (int j=0 ; j<pow(_K,t-1) ; j++)
				{
					for (int n=0; n<pow(_K,NN); n++)
					{
						SCIP_CALL( SCIPgetTransformedCons(scip, _sec_con[i][t-2][j][n], &_sec_con[i][t-2][j][n]));
					}
				}
			}

	}
	//gets transformed for alpha_con
	SCIP_CALL( SCIPgetTransformedCons(scip, _alpha_con, &_alpha_con));


	return SCIP_OKAY;
}

SCIP_DECL_PRICEREXITSOL(ObjPricervaccine::scip_exitsol)
{
	if( SCIPisInRestart(scip) )
	{
		for (int i=0; i<_NScen; i++)
		{
			for (int j=2; j<_T+1 ; j++)
			{
				SCIP_CALL( SCIPsetConsModifiable(scip, _z_con[i][j-2], FALSE) );
			}
		}

		for(int t=2; t<_T+1; t++)
		{
			int NN= min(_Nper_to_spoil,_T-t);
			for (int j=0 ; j<pow(_K,t-1) ; j++)
			{
				SCIP_CALL( SCIPsetConsModifiable(scip, _w_con[t-2][j], FALSE));
				//SCIP_CALL( SCIPgetTransformedVar(scip, _wj_var2[t-2][j], &_wj_var2[t-2][j]) );
				for (int n=0; n<pow(_K,NN); n++)
				{
					for (int q=0 ; q< pow(_K,_T-t) ; q++)
					{
						SCIP_CALL( SCIPsetConsModifiable(scip, _wt_con[t-2][j][n][q], FALSE));
					}
				}
			}
		}

		for (int i=0 ; i< _I ; i++)
		{
			for(int t=2; t<_T+1; t++)
			{
				int NN= min(_Nper_to_spoil,_T-t);
				for (int j=0 ; j<pow(_K,t-1) ; j++)
				{
					for (int n=0; n<pow(_K,NN); n++)
					{
						SCIP_CALL( SCIPsetConsModifiable(scip, _sec_con[i][t-2][j][n], FALSE));
					}
				}
			}

		}

		SCIP_CALL( SCIPdeactivatePricer(scip, pricer) );
	}
	return SCIP_OKAY;
}

/** perform pricing*/
SCIP_RETCODE ObjPricervaccine::pricing(SCIP*  scip)               /**< SCIP data structure */
{
	//Get the duals of wastage constraint
	vector < vector < vector < vector <  SCIP_Real > > > > pi(_T-1);
	for(int t=2; t<_T+1; t++)
	{
		int NN= min(_Nper_to_spoil,_T-t);
		pi[t-2].resize(pow(_K,t-1));
		for (int j=0 ; j<pow(_K,t-1) ; j++)
		{
			pi[t-2][j].resize(pow(_K,_T-t));
			for (int n=0; n<pow(_K,NN); n++)
			{
				pi[t-2][j][n].resize(pow(_K,_T-t));
				for (int q=0 ; q< pow(_K,_T-t) ; q++)
				{
					pi[t-2][j][n][q] = SCIPgetDualsolLinear(scip, _wt_con[t-2][j][n][q]);
					cout << "pi" << pi[t-2][j][n][q] << endl;
				}

			}
		}
	}

	//Get the duals og the second constraint
	vector< vector < vector < vector <  SCIP_Real > > > > lambda(_I);
	for (int i=0 ; i<_I ; i++)
	{
		lambda[i].resize(_T-1);
		for(int t=2; t<_T+1; t++)
		{
			int NN= min(_Nper_to_spoil,_T-t);
			lambda[i][t-2].resize(pow(_K,t-1));
			for (int j=0 ; j<pow(_K,t-1) ; j++)
			{
				lambda[i][t-2][j].resize(pow(_K,NN));
				for (int n=0; n<pow(_K,NN); n++)
				{
					lambda[i][t-2][j][n] = SCIPgetDualsolLinear(scip, _sec_con[i][t-2][j][n]);
					cout << "lamda" << lambda[i][t-2][j][n] << endl;
				}
			}
		}
	}

	//Get the duals of 5c*/
	vector < vector < SCIP_Real > > gamma(_NScen);
	for (int i=0; i<_NScen; i++)
	{
		gamma[i].resize(_T-1);
		for (int j=2; j<_T+1 ; j++)
		{
			gamma[i][j-2] = SCIPgetDualsolLinear(scip, _z_con[i][j-2]);
			cout << "gamma" << gamma[i][j-2] << endl;
		}
	}
	//Gets the dual of w_con
	vector < vector < SCIP_Real > > mu(_T-1);
	for (int t=2; t< _T+1 ; t++)
	{
		mu[t-2].resize(pow(_K,t-1));
		for (int j=0; j < pow (_K, t-1); j++)
		{
			mu[t-2][j]= SCIPgetDualsolLinear(scip, _w_con[t-2][j]);
			cout << "mu" << mu[t-2][j] << endl;
		}
	}

	vector < vector < vector < vector < vector < SCIP_Real > > > > > y_newcolumn (_I);
	vector< vector < SCIP_Real > > z_newcolumn (_T-1);
	vector< vector < SCIP_Real > > objval (_T-1);
	vector< vector < SCIP_Real > > redcost(_T-1);
	for (int i=0; i<_I ; i++)
	{
		y_newcolumn[i].resize(_T-1);
		for (int t=2; t<_T+1; t++)
		{
			y_newcolumn[i][t-2].resize(pow(_K, t-1));
			for (int j=0; j<pow(_K, t-1); j++)
			{
				int YNN = min(_T-t,_Nper_to_spoil);
				y_newcolumn[i][t-2][j].resize(YNN+1);
				for (int kk=0; kk < YNN+1 ; kk++)
				{
					y_newcolumn[i][t-2][j][kk].resize(pow(_K,kk));
				}
			}
		}
	}
	for (int t=2; t<_T+1; t++)
	{
		z_newcolumn[t-2].resize(pow(_K, t-1));
		objval[t-2].resize(pow(_K,t-1));
		redcost[t-2].resize(pow(_K,t-1));
	}

	SCIP_CALL( find_new_column(pi , lambda , gamma, y_newcolumn, z_newcolumn, objval));
	int n_added_var =0;
	vector < vector < vector < SCIP_Real > > > ycolumn (_I);
	for (int t=2; t<_T+1; t++)
	{
		for (int j=0 ; j<pow(_K,t-1); j++)
		{
			redcost[t-2][j] = objval[t-2][j] - mu[t-2][j];
			cout << "objval" << objval[t-2][j] << endl;
			if ( SCIPisNegative(scip, redcost[t-2][j]) )
			{
				SCIP_CALL(add_newcolumn_variable(scip, y_newcolumn, z_newcolumn[t-2][j], t, j));
				n_added_var++;
			}
		}
	}
	if (n_added_var > 0 )
	{
		_n_pricing ++;
		return SCIP_OKAY;
	}

	SCIP_CALL( SCIPwriteTransProblem(scip, "CapacityExp.lp", "lp", FALSE) );


	return SCIP_OKAY;
}

SCIP_DECL_PRICERREDCOST(ObjPricervaccine::scip_redcost)
{
	if( SCIPgetNRuns(scip) > 1 )
	{
	    *result = SCIP_SUCCESS;
	    return SCIP_OKAY;
	}

	if( SCIPgetDepth(scip) != 0 )
	{
	    *result = SCIP_SUCCESS;
	    SCIP_CALL( SCIPsetBoolParam(scip, "display/lpinfo", FALSE) );
	     SCIP_CALL( SCIPsetIntParam(scip, "presolving/maxrounds", -1 ) );
	     SCIP_CALL( SCIPsetSeparating(scip, SCIP_PARAMSETTING_DEFAULT, TRUE) );
	     SCIP_CALL( SCIPrestartSolve(scip));

	     return SCIP_OKAY;
	}

	SCIPdebugMessage("call scip_redcost ...\n");

	/* set result pointer, see above */
	*result = SCIP_SUCCESS;

	/* call pricing routine */
	SCIP_CALL( pricing(scip) );

	return SCIP_OKAY;
}

/** add new variable to problem */
SCIP_RETCODE ObjPricervaccine::add_newcolumn_variable(
		SCIP*                                                           scip,
		vector < vector< vector < vector < vector < SCIP_Real > > > > >    & y_newcolumn,
		SCIP_Real                                                      z_newcolumn,
		int                                                             t,
		int                                                             j )        /**< list of nodes in tour */
{

	char var_name[255];
	SCIP_VAR * w_var;
	//int lenght = pow(_K,_T-1)/pow(_K, t-1);

	SCIPsnprintf(var_name, 255, "Alpha%d_%d_%d", t, j , _n_pricing);
	SCIP_CALL( SCIPcreateVar(scip, & w_var, var_name,
			0.0,                      // lower bound
			SCIPinfinity(scip),      // upper bound
			0,                       // objective
			SCIP_VARTYPE_CONTINUOUS, // variable type
			false, false, 0, 0, 0, 0, 0) );
	SCIPdebugMessage("new variable <%s>\n", var_name);

	/* add new variable to the list of variables to price into LP (score: leave 1 here) */
	SCIP_CALL( SCIPaddPricedVar(scip, w_var, 1.0) );

	// Add the new variable to the wastage constraint
	int NNN = min(t-2,_Nper_to_spoil);
	for (int kk=0 ; kk<NNN+1 ; kk++)
	{
		int NN = min(_T-t+kk , _Nper_to_spoil);
		for (int n=0 ; n < pow(_K,t-kk+NN-t) ; n++)
		{
			int jjmat = _jmat[t-kk-1][j*pow(_K,_T-1)/pow(_K,t-1)];
			for (int q=0 ; q<pow(_K,_T-t+kk) ; q++)
			{
				for (int i=0 ; i<_I ; i++)
				{
					SCIP_CALL( SCIPaddCoefLinear(scip, _wt_con[t-kk-2][jjmat][(j*pow(_K,t-kk+NN-1)/pow(_K,t-1))-(jjmat*pow(_K,t-kk+NN-1)/pow(_K,t-kk-1))+n][q], w_var,
							-1*y_newcolumn[i][t-kk-2][jjmat][kk][j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1)]));
					//cout << "wcon" << t-kk << "-" << jjmat << "-" << n << "-" << q << endl;
					//cout << "y" << i << "-" << t-kk << "-" << jjmat <<  "-" << kk << "-" << j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1) << "added" << endl;
					//cout << "Yis" << y_newcolumn[i][t-kk-2][jjmat][kk][j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1)] << endl;
				}
			}
		}
	}
	//Add it to the second constraint
	for (int i=0 ; i<_I ; i++)
	{
		for (int kk=0 ; kk<NNN+1 ; kk++)
		{
			int NN = min(_T-t+kk , _Nper_to_spoil);
			for (int n=0 ; n <pow(_K,t-kk+NN-t) ; n++)
			{
				int jjmat = _jmat[t-kk-1][j*pow(_K,_T-1)/pow(_K,t-1)];
				SCIP_CALL( SCIPaddCoefLinear(scip, _sec_con[i][t-kk-2][jjmat][(j*pow(_K,t-kk+NN-1)/pow(_K,t-1))-(jjmat*pow(_K,t-kk+NN-1)/pow(_K,t-kk-1))+n] , w_var,
						-1*y_newcolumn[i][t-kk-2][jjmat][kk][j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1)]));
				//cout << "wcon" << t-kk << "-" << jjmat << "-" << n << endl;
				//cout << "y" << i << "-" << t-kk << "-" << jjmat << "-" << kk << "-" << j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1) << "added" << endl;
			}
		}
	}

	// Add to constraint 5c
	for (int jj=0 ; jj< pow(_K,_T-1)/pow(_K,t-1); jj++)
	{
		SCIP_CALL( SCIPaddCoefLinear(scip, _z_con[j*pow(_K,_T-1)/pow(_K,t-1)+jj][t-2], w_var, -1*z_newcolumn ) );
	}

	//Add to constraint 5e
	SCIP_CALL( SCIPaddCoefLinear(scip, _w_con[t-2][j], w_var, 1 ) );

	//cleanup

	SCIP_CALL( SCIPreleaseVar(scip, &w_var) );

	return SCIP_OKAY;
}

//Finds new columns
SCIP_RETCODE ObjPricervaccine::find_new_column(
		vector < vector < vector < vector <SCIP_Real > > > > pi,
		vector< vector < vector < vector <  SCIP_Real > > > > lambda,/**< dual variable value*/
		vector < vector < SCIP_Real > > gamma,
		vector < vector <vector< vector< vector< SCIP_Real > > > > > & y_newcolumn,
		vector < vector < SCIP_Real > > & z_newcolumn,
		vector < vector < SCIP_Real > > & objval )
{

	vector < SCIP_Real > x_ub = {50,10,5};
	for (int t=2 ; t<_T+1; t++)
	{
		int lenght = _NScen/pow(_K,t-1);
		for (int j=0; j<pow(_K,t-1) ; j++)
		{
			if(_n_pricing == 9)
				cout << "Here" << endl;
			SCIP* subscip = NULL;
			SCIP_CALL( SCIPcreate(&subscip) );
			SCIPprintVersion(subscip, NULL);
			SCIPinfoMessage(subscip, NULL, "\n");

			/* include default plugins */
			SCIP_CALL( SCIPincludeDefaultPlugins(subscip) );

			/* set verbosity parameter */
			SCIP_CALL( SCIPsetIntParam(subscip, "display/verblevel", 5) );
			//SCIP_CALL( SCIPsetBoolParabm(subscip, "display/lpinfo", TRUE) );

			/* create empty problem */
			SCIP_CALL( SCIPcreateProb(subscip, "p_vaccine", 0, 0, 0, 0, 0, 0, 0) );


			/*Add variables Yi(m)k */
			char var_name[255];
			vector < vector < SCIP_VAR* > > y_var(_I);
			vector< vector < SCIP_Real > > picoeff(_I);
			vector< vector < SCIP_Real > > lambdacoeff(_I);
			int NNN = min(t-2,_Nper_to_spoil);
			for (int i=0 ; i<_I ; i++)
			{
				picoeff[i].resize(NNN+1);
				lambdacoeff[i].resize(NNN+1);
				y_var[i].resize(NNN+1);
				for (int kk=0; kk <NNN+1; kk++)
				{
					int NN = min(_T-t+kk,_Nper_to_spoil);
					for (int n=0 ; n <pow(_K,t-kk+NN-t) ; n++)
					{
						int jjmat = _jmat[t-kk-1][j*pow(_K,_T-1)/pow(_K,t-1)];
						//cout << (j*pow(_K,t-kk+NN-1))*(1/pow(_K,t-1)-1/pow(_K,t-kk-1))+n << endl;
						lambdacoeff[i][kk] += lambda[i][t-kk-2][jjmat][(j*pow(_K,t-kk+NN-1)/pow(_K,t-1))-(jjmat*pow(_K,t-kk+NN-1)/pow(_K,t-kk-1))+n];
						for (int q=0 ; q<pow(_K,_T-t+kk) ; q++)
						{
							picoeff[i][kk] += pi[t-kk-2][jjmat][(j*pow(_K,t-kk+NN-1)/pow(_K,t-1))-(jjmat*pow(_K,t-kk+NN-1)/pow(_K,t-kk-1))+n][q];
						}
					}
					cout << "lambda" << lambdacoeff[i][kk] << endl;
					//Add
					SCIP_VAR* yvar;
					int jjmat = _jmat[t-kk-1][j*pow(_K,_T-1)/pow(_K,t-1)];
					//cout << "jj " << jjmat << " j" << j << " " << jjmat*pow(_K,t-1)/pow(_K,t-kk-1) << endl;
					int jj = j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1);
					//cout << "jj" << jj << endl;
					SCIPsnprintf(var_name, 255, "y%d_%d_%d_%d_%d", i , t-kk , jjmat , kk,  jj );
					cout << j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1) << endl;
					SCIP_CALL( SCIPcreateVar(subscip,
							&yvar,                                                      // returns new index
							var_name,                                                   // name
							0.0,                                                        // lower bound
							50,                                      // upper bound
							lambdacoeff[i][kk]+picoeff[i][kk],                                         // objective
							SCIP_VARTYPE_CONTINUOUS,                                       // variable type
							true,                                                       // initial
							false,                                                      // forget the rest ...
							0, 0, 0, 0, 0) );
					SCIP_CALL( SCIPaddVar(subscip, yvar) );
					y_var[i][kk] = yvar;
					//SCIPinfoMessage(subscip, NULL, "New Y variable is Y_%d_%d_%d_%d_%d", i, t, j, t+kk, j * (pow(_K, t+kk-1)/pow(_K, t-1))+jj);
				} // jj
			}


			/*Add variables z_m */
			SCIP_VAR* pz_var;
			SCIP_Real zcoef=0;
			for (int q=0; q<pow(_K,_T-t); q++)
			{
				zcoef += gamma[j*lenght+q][t-2];
			}
			SCIP_CALL( SCIPcreateVar(subscip,
					&pz_var,                                                      // returns new index
					"z",                                                   // name
					0.0,                                                        // lower bound
					1,                                                          // upper bound
					zcoef,                                                      // objective
					SCIP_VARTYPE_INTEGER,                                        // variable type
					true,                                                       // initial
					false,                                                      // forget the rest ...
					0, 0, 0, 0, 0) );
			SCIP_CALL( SCIPaddVar(subscip, pz_var) );


			// Add the shortage constraint constraint
			char con_name[255]="shcon";
			//int NNN = CalMin(t-2,_Nper_to_spoil);
			SCIP_CONS* sh_con = NULL;
			//SCIP_VAR* index = pz_var;
			SCIP_Real coeff = _Nodedemand[_nodemat[t-1][j*lenght]-2];
			SCIP_CALL( SCIPcreateConsLinear(subscip, & sh_con, con_name, 1, &pz_var, &coeff,
					_Nodedemand[_nodemat[t-1][j*lenght]-2]-_Wastbeta,    /* lhs */
					SCIPinfinity(subscip),                    /* rhs */
					true,                   /* initial */
					false,                  /* separate */
					true,                   /* enforce */
					true,                   /* check */
					true,                   /* propagate */
					false,                  /* local */
					false,                   /* modifiable */
					false,                  /* dynamic */
					false,                  /* removable */
					false) );
			for (int i=0 ; i<_I ; i++)
			{
				//y_var[i].resize(NNN+1);
				for (int kk=0; kk <NNN+1; kk++)
				{
					SCIP_CALL( SCIPaddCoefLinear(subscip, sh_con, y_var[i][kk], 1) );
				}
			}
			assert (pz_var != NULL );
			assert (sh_con != NULL);
			SCIP_CALL( SCIPaddCons(subscip, sh_con) );


			//SCIP_CALL( SCIPwriteTransProblem(subscip, "pricer.lp", "lp", FALSE));


			//SCIP_CALL( SCIPwriteOrigProblem(subscip, "pricer_init.lp", "lp", FALSE) );

			SCIP_CALL( SCIPsolve(subscip) );

			SCIP_CALL( SCIPwriteTransProblem(subscip, "pricer.lp", "lp", FALSE));

			SCIPinfoMessage(subscip, NULL, "solution status", SCIPgetStatus (subscip) );
			SCIP_CALL( SCIPprintStatistics(subscip, NULL) );

			SCIP_CALL( SCIPprintBestSol(subscip, NULL, FALSE) );


			//Get objective value
			objval[t-2][j] = SCIPgetPrimalbound(subscip);
			SCIPinfoMessage(subscip, NULL, "obj value", objval[t-2][j]);
			SCIP_SOL* sol;
			sol = SCIPgetBestSol (subscip);
			for (int i=0 ; i<_I ; i++)
			{
				for (int kk=0; kk <NNN+1; kk++)
				{
					int jjmat = _jmat[t-kk-1][j*pow(_K,_T-1)/pow(_K,t-1)];
					y_newcolumn[i][t-kk-2][jjmat][kk][j-jjmat*pow(_K,t-1)/pow(_K,t-kk-1)] = SCIPgetSolVal(subscip, sol, y_var[i][kk]);
				}
			}
			z_newcolumn[t-2][j]= SCIPgetSolVal(subscip, sol, pz_var);

			SCIP_CALL( SCIPfree(&subscip) );

			BMScheckEmptyMemory();

		}

	}

	return SCIP_OKAY;
}
