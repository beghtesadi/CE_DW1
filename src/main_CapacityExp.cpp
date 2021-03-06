#include <stdio.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <math.h>
#include <algorithm>

/* scip includes */
#include "objscip/objscip.h"
#include "objscip/objscipdefplugins.h"

/* user defined includes */
#include "pricer_CapacityExp.h"


/* namespace usage */
using namespace std;
using namespace scip;

void GenerateDemand (vector<int> &x , vector<double> &y);
double CalProb (int n, double mean);
double mean=5;
double K=2;





int main ()
{

  vector<double> purchcost = {1,4,7};
  vector<int >vialsize = {1,5,10};
  double ConfRate=0.75;
  double Shortbeta=1;
  double Wastbeta=1;
  int I=3;
  int Nper_to_spoil=1;
  int T=6; //T is the number of stages
  int NScen=pow(K, T-1);
  int Nnodes=0;
  for (int i=0; i<T ; i++){
	Nnodes=Nnodes+pow(K,i);}

  vector<double>Dprob;
  vector<int>Demand;
  vector<double>Nodedemand;
  vector<int>ScenDemand;
  vector<double>Scenprob;
  vector<double>Scenprob2;
  vector<double>Nodeprobe;
  vector < SCIP_Real > x_ub = {50,10,5};

  GenerateDemand(Demand, Dprob);

  /*for (int i=0; i<K ; i++)
  {
	  cout << Dprob[i] << endl;
  }*/
  //generating the nodes matrix
  vector< vector<int> > nodemat;
  vector< vector<int> > jmat;

	int n=1;
	for (int t=1 ; t<T+1 ; t++)
	{
		//creat an empty vector
		vector<int>row;
		vector<int>jrow;
		int len=NScen/pow(K,t-1);
		for (int i=0 ; i<pow(K,t-1) ; i++)
		{
			for (int j=0 ; j<len ; j++)
			{
				row.push_back(n);
				jrow.push_back(i);
			}
			n++;
		}
		nodemat.push_back(row);
		jmat.push_back(jrow);
	}


	for (int i=0 ; i<pow(K,T-1) ; i++)
	{
		Scenprob2.push_back(0);
		Scenprob.push_back(0);
	}


	for (int j=0 ; j<K ; j++)
	{
		Nodedemand.push_back(Demand[j]);
		Nodeprobe.push_back(Dprob[j]);
		Scenprob[j]=Dprob[j];
	}

	for (int t=3; t<T+1 ; t++)
	{
		int n=0;
		for (int i=0 ; i<pow(K,t-2) ; i++)
		{
			for (int j=0 ; j < K ; j++)
			{
				Nodedemand.push_back(Demand[j]);
				Nodeprobe.push_back(Dprob[j]);
				Scenprob2[n]=Scenprob[i]* Dprob[j];
				n++;
			}
		}
		int ssize= (int)Scenprob2.size();
		for(int ii=0 ; ii<ssize ; ii++)
		{
			Scenprob[ii]=Scenprob2[ii];
		}
	}

	//Generating ScenDemand
	for (int i=0 ; i<pow(K,T-2) ; i++)
	{
		for (int j=0; j<K ; j++)
		{
			ScenDemand.push_back(Demand[j]);
		}
	}
  SCIP* scip = NULL;

  static const char* vaccine_PRICER_NAME = "vaccine_Pricer";

  SCIP_CALL( SCIPcreate(&scip) );

   /***********************
    * Version information *
    ***********************/

   SCIPprintVersion(scip, NULL);
   //SCIPinfoMessage(scip, NULL, "\n");

   /* include default plugins */
   SCIP_CALL( SCIPincludeDefaultPlugins(scip) );

   /* set verbosity parameter */
   //SCIP_Real presolveparam = 0.0;
   SCIP_CALL( SCIPsetIntParam(scip, "display/verblevel", 5) );
   SCIP_CALL( SCIPsetIntParam(scip, "presolving/maxrounds", 0 ) );
   //SCIP_CALL( SCIPsetRealParam(scip, "presolving/restartminred", 0.0) );
   SCIP_CALL( SCIPsetBoolParam(scip, "display/lpinfo", TRUE) );

   SCIP_CALL( SCIPsetSeparating(scip, SCIP_PARAMSETTING_OFF, TRUE) );

   /* create empty problem */
   SCIP_CALL( SCIPcreateProb(scip, "vaccine", 0, 0, 0, 0, 0, 0, 0) );

   SCIP_CALL( SCIPsetSeparating(scip, SCIP_PARAMSETTING_OFF, TRUE) );

   //Add x
   vector < vector < vector < SCIP_VAR* > > > x_var(T-1);
   for (int t=2 ; t<T+1; t++)
   {
	   x_var[t-2].resize(pow(K,t-1));
	   int lenght = NScen/pow(K,t-1);
	   for (int j=0; j<pow(K,t-1) ; j++)
	   {
		   x_var[t-2][j].resize(I);
		   char var_name[255];
		   for (int i = 0; i <I ; ++i)
		   {
			   SCIP_VAR* xvar;
			   SCIPsnprintf(var_name, 255, "x%d_%d_%d", t, j, i );
			   SCIP_CALL( SCIPcreateVar(scip,
					   &xvar,                                                      // returns new index
					   var_name,                                                   // name
					   0.0,                                                        // lower bound
					   x_ub[i],                                      // upper bound
					   Nodeprobe[nodemat[t-1][j*lenght]-2] * purchcost[i],      // objective
					   SCIP_VARTYPE_INTEGER,                                       // variable type
					   true,                                                       // initial
					   false,                                                      // forget the rest ...
					   0, 0, 0, 0, 0) );
			   SCIP_CALL( SCIPaddVar(scip, xvar) );
			   x_var[t-2][j][i] = xvar;
		   }
	   }
   }
  
  /*Add variables*/
  char var_name[255];
  vector< SCIP_VAR* > psi_var(NScen);
  for (int i=0 ; i < NScen ; i++)
  {
     SCIP_VAR* var;
     SCIPsnprintf(var_name, 255, "psi%d", i );
     SCIP_CALL( SCIPcreateVar(scip,
                     &var,                   // returns new index
                     var_name,               // name
                     0.0,                    // lower bound
                     1.0,                    // upper bound
                     0,                      // objective
                     SCIP_VARTYPE_BINARY,   // variable type
                     true,                   // initial
                     false,                  // forget the rest ...
                     0, 0, 0, 0, 0) );
    SCIP_CALL( SCIPaddVar(scip, var) );
    psi_var[i] = var;
   }

  /* Add w variables*/
  vector < vector < SCIP_VAR * > > wj_var(T-1);
  for (int t=2; t<T+1 ; t++)
  {
	  wj_var[t-2].resize(pow(K,t-1));
	  int lenght  = NScen/ pow(K,t-1);
	  for (int j=0 ; j<pow(K,t-1); j++)
	  {
		  SCIP_Real objj = 0;
		  SCIP_VAR * var;
		  SCIPsnprintf(var_name, 255, "wt%d_%d", t, j );
		  SCIP_CALL( SCIPcreateVar(scip,
				  &var,                   // returns new index
				  var_name,               // name
				  0.0,                    // lower bound
				  SCIPinfinity(scip),     // upper bound
				  objj,                      // objective
				  SCIP_VARTYPE_CONTINUOUS,   // variable type
				  true,                   // initial
				  false,                  // forget the rest ...
				  0, 0, 0, 0, 0) );
		  SCIP_CALL( SCIPaddVar(scip, var) );
		  wj_var[t-2][j] = var;
	  }
  }


  //Add wastage constraint
  char con_name[255];
  vector < vector < vector < vector < SCIP_CONS* > > > > wt_con(T-1);
  for (int t=2; t<T+1 ; t++)
  {
	  int NN=min(Nper_to_spoil,T-t);
	  int lenght = NScen/pow(K,t-1);
	  wt_con[t-2].resize(pow(K,t-1));
	  for (int j=0 ; j<pow(K,t-1); j++)
	  {
		  wt_con[t-2][j].resize(pow(K,NN));
		  for (int n=0; n<pow(K,NN); n++)
		  {
			  wt_con[t-2][j][n].resize(pow(K,T-t));
			  for (int q=0 ; q< pow(K,T-t) ; q++)
			  {
				  SCIP_CONS* ww_con = NULL;
				  SCIPsnprintf(con_name, 255, "w_con%d_%d_%d_%d", t,j, n, q);
				  SCIP_CALL( SCIPcreateConsLinear(scip, & ww_con, con_name, 0, NULL, NULL,
						  -SCIPinfinity(scip),    /* lhs */
						  Wastbeta,               /* rhs */
						  true,                   /* initial */
						  false,                  /* separate */
						  true,                   /* enforce */
						  true,                   /* check */
						  true,                   /* propagate */
						  false,                  /* local */
						  true,                   /* modifiable */
						  false,                  /* dynamic */
						  false,                  /* removable */
						  false) );               /* stickingatnode*/
				  wt_con[t-2][j][n][q]=ww_con;
				  //Add the initial column
				  SCIP_CALL( SCIPaddCoefLinear(scip, wt_con[t-2][j][n][q], wj_var[t-2][j], -1*(Nodedemand[nodemat[t-1][j*lenght]-2])+Wastbeta) );
				  //Add x vars
				  for (int ii=0; ii<I; ii++)
				  {
					  SCIP_CALL( SCIPaddCoefLinear(scip, wt_con[t-2][j][n][q], x_var[t-2][j][ii], vialsize[ii]) );
				  }
				  assert (wt_con[t-2][j][n][q] != NULL);
				  SCIP_CALL( SCIPaddCoefLinear(scip, wt_con[t-2][j][n][q], psi_var[j*lenght+q], -1000) );
				  SCIPinfoMessage(scip, NULL, "z at this constraint Z" );
				  SCIP_CALL( SCIPaddCons(scip, wt_con[t-2][j][n][q]) );
			  }
		  }
	  }
  }

  //Add the second constraint
  vector < vector < vector < vector < SCIP_CONS* > > > > sec_con(I);
  for (int i=0; i<I; i++)
  {
	  sec_con[i].resize(T-1);
	  for (int t=2; t<T+1 ; t++)
	  {
		  int NN = min (T-t,Nper_to_spoil);
		  int lenght = NScen/pow(K,t-1);
		  sec_con[i][t-2].resize(pow(K,t-1));
		  for (int j=0 ; j<pow(K,t-1); j++)
		  {
			  sec_con[i][t-2][j].resize(pow(K,NN));
			  for (int n=0; n<pow(K,NN); n++)
			  {
				  SCIP_CONS* s_con = NULL;
				  SCIPsnprintf(con_name, 255, "sec_con%d_%d_%d_%d", i ,t,j, n);
				  SCIP_CALL( SCIPcreateConsLinear(scip, & s_con, con_name, 0, NULL, NULL,
						  0,
						  SCIPinfinity(scip),
						  true,
						  false,
						  true,
						  true,
						  true,
						  false,
						  true,
						  false,
						  false,
						  false) );
				  sec_con[i][t-2][j][n] = s_con;
				  SCIP_CALL( SCIPaddCoefLinear(scip, sec_con[i][t-2][j][n], x_var[t-2][j][i], vialsize[i]) );
				  if (i==0)
					  SCIP_CALL( SCIPaddCoefLinear(scip, sec_con[i][t-2][j][n], wj_var[t-2][j], -1*(Nodedemand[nodemat[t-1][j*lenght]-2])+Wastbeta) );
				  SCIP_CALL( SCIPaddCons(scip,sec_con[i][t-2][j][n]) );
			  }
		  }
	  }
  }

   /*Add constraint 5c*/
   vector < vector < SCIP_CONS* > > z_con (NScen);
   SCIP_Real mwjj = 0;
   SCIP_Real mwjj2 = -1;
   //char con_name[255];  
   for (int i=0; i<NScen; i++)
   {  
      z_con[i].resize(T-1);
      for (int j=2; j<T+1 ; j++)
       {
          SCIP_CONS* con;
          SCIPsnprintf(con_name, 255, "z%d_%d", i, j);
          SCIP_VAR* index = psi_var[i];
          SCIP_Real coeff = 1;
          SCIP_CALL( SCIPcreateConsLinear(scip, &con, con_name, 1, &index, &coeff,
                     0,                      /* lhs */
                     SCIPinfinity(scip),     /* rhs */
                     true,                   /* initial */
                     false,                  /* separate */
                     true,                   /* enforce */
                     true,                   /* check */
                     true,                   /* propagate */
                     false,                  /* local */
                     true,                   /* modifiable */
                     false,                  /* dynamic */
                     false,                  /* removable */
                     false) );               /* stickingatnode */
                SCIP_CALL( SCIPaddCons(scip, con) );
                z_con[i][j-2] = con;
                int jjjmat= jmat [j-1][i];
                //if (j==T && i==0)
                   //SCIP_CALL( SCIPaddCoefLinear(scip, z_con[i][j-2], wj_var[j-2][jjjmat], mwjj2) );
              //  else
                   SCIP_CALL( SCIPaddCoefLinear(scip, z_con[i][j-2], wj_var[j-2][jjjmat], mwjj) );           
        }
    }

   /*Add constraint 5d */
   //char con_name[255];
   SCIP_CONS*  alpha_con = NULL;
   SCIP_CALL( SCIPcreateConsLinear(scip, & alpha_con, "alpha" , 0, NULL, NULL,
                     -SCIPinfinity(scip),    /* lhs */
                     1-ConfRate,                    /* rhs */
                     true,                   /* initial */
                     false,                  /* separate */
                     true,                   /* enforce */
                     true,                   /* check */
                     true,                   /* propagate */
                     false,                  /* local */
                     false,                   /* modifiable */
                     false,                  /* dynamic */
                     false,                  /* removable */
                     false) );               /* stickingatnode */

    for (int i = 0; i < NScen; ++i)
	SCIP_CALL( SCIPaddCoefLinear(scip, alpha_con, psi_var[i], Scenprob[i]) );
    SCIP_CALL( SCIPaddCons(scip, alpha_con) );

   /*Add constraint 5e */
   //char con_name[255]; 
   SCIP_Real wje =1;
   vector < vector < SCIP_CONS* > > w_con(T-1);
   for (int t=2; t< T+1 ; t++)
   {
    w_con[t-2].resize(pow (K, t-1));
    for (int j=0; j < pow (K, t-1); j++)
     {
      SCIP_CONS* con = NULL;
      SCIPsnprintf(con_name, 255, "w%d_%d", t, j);
      SCIP_CALL( SCIPcreateConsLinear( scip, &con, con_name, 0, NULL, NULL,
                                       1.0,   /* lhs */
                                       1.0,   /* rhs */
                                       true,  /* initial */
                                       false, /* separate */
                                       true,  /* enforce */
                                       true,  /* check */
                                       true,  /* propagate */
                                       false, /* local */
                                       true,  /* modifiable */
                                       false, /* dynamic */
                                       false, /* removable */
                                       false  /* stickingatnode */ ) );
      SCIP_CALL( SCIPaddCons(scip, con) );
      w_con[t-2][j] = con;
      SCIP_CALL( SCIPaddCoefLinear(scip, w_con[t-2][j], wj_var[t-2][j],wje ) );
     }
   }

   int n_pricing =0;
   int nrestart = 0;
         
   /* include pricer */
   ObjPricervaccine* vaccine_pricer_ptr = new ObjPricervaccine(scip, vaccine_PRICER_NAME, purchcost, vialsize, Nper_to_spoil,        
                           Wastbeta, T, NScen, Nnodes, K, I, n_pricing , nrestart , Nodedemand, nodemat, jmat, x_var, psi_var, wj_var, wt_con, sec_con, z_con, alpha_con, w_con);



   SCIP_CALL( SCIPincludeObjPricer(scip, vaccine_pricer_ptr, true) );

   /* activate pricer */
   SCIP_CALL( SCIPactivatePricer(scip, SCIPfindPricer(scip, vaccine_PRICER_NAME)) );

   SCIP_CALL( SCIPwriteOrigProblem(scip, "CapacitExp_init.lp", "lp", FALSE) );

   /*************
    *  Solve    *
    *************/

  SCIP_CALL( SCIPsolve(scip) );


   /**************
    * Statistics *
    *************/
   
   FILE* ffp2 =fopen("/home/beghtesadi/SCIPproject/CE_DW1/results","w");
   SCIP_CALL( SCIPprintStatistics(scip, ffp2) );
  //printPresolverStatistics(scip,ffp2);

   SCIP_CALL( SCIPprintBestSol(scip, ffp2, FALSE) );



   /********************
    * Deinitialization *
    ********************/

   SCIP_CALL( SCIPfree(&scip) );

   BMScheckEmptyMemory();

   return 0;
}//for main 

void GenerateDemand (vector<int> &x , vector<double> &y){
	int a, b;
        a = min(mean,ceil((K-1)/2));
        if (mean > ceil ((K-1)/2))
           b= floor((K-1)/2);
        else
           b= K-mean-1;
        double prob = 0;
        for (int i= 0; i< mean-a +1 ; i++){
          prob = prob +CalProb (i , mean);
          }
        double probb = prob;
        x.push_back(mean-a);
        y.push_back(prob);
	for (int i=1 ; i<a+b ; i++){
           y.push_back(CalProb(mean-a+i , mean)); 
	   x.push_back(mean-a+i);
           probb = probb +y[i];}
        x.push_back (mean+b);
        y.push_back(1-probb);
}
double CalProb (int n, double mean) 
 {
  double prob=exp(-1*mean);
  if (n > 0)
  {
    for (int i=1 ; i<n+1 ; i++)
    {
     prob = prob * mean / i ;
    }
   } 
  return (prob);
 }

