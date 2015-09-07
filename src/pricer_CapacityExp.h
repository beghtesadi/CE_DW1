/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                           */
/*                  This file is part of the program and library             */
/*         SCIP --- Solving Constraint Integer Programs                      */
/*                                                                           */
/*    Copyright (C) 2002-2014 Konrad-Zuse-Zentrum                            */
/*                            fuer Informationstechnik Berlin                */
/*                                                                           */
/*  SCIP is distributed under the terms of the ZIB Academic License.         */
/*                                                                           */
/*  You should have received a copy of the ZIB Academic License.             */
/*  along with SCIP; see the file COPYING. If not email to scip@zib.de.      */
/*                                                                           */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/**@file pricer_vaccine.h
 * @brief VRP pricer plugin
 * @author Andreas Bley
 * @author Marc Pcacch
 */

#ifndef __SCIP_PRICER_vaccine_H__
#define __SCIP_PRICER_vaccine_H__

#include "objscip/objscip.h"
#include "scip/pub_var.h"

#include <vector>
#include <list>

using namespace std;
using namespace scip;


/** pricer class */
class ObjPricervaccine : public ObjPricer
{
public:

   /** Constructs the pricer object with the data needed */
   ObjPricervaccine(
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
   vector < vector < int > > &                          p_nodemat,
   vector < vector < int > > &                          p_jmat,
   vector < vector < vector < SCIP_VAR* > > > &     p_x_var,
   vector < SCIP_VAR* > &                            p_psi_var,
   vector < vector< SCIP_VAR * > > &                 p_wj_var,
   //vector< vector< SCIP_VAR * > > &                 p_wj_var2,
   vector < vector < vector < vector < SCIP_CONS* > > > > &   p_wt_con,
   vector < vector < vector < vector < SCIP_CONS* > > > > & p_sec_con,
   vector < vector < SCIP_CONS* > > &                     p_z_con,
   SCIP_CONS*                                     & alpha_con,
   vector< vector<SCIP_CONS* > > &                   p_w_con    
   );

   /** Destructs the pricer object. */
   virtual ~ObjPricervaccine();

   /** initialization method of variable pricer (called after problem was transformed) */
   virtual SCIP_DECL_PRICERINIT(scip_init);
   //virtual SCIP_DECL_PRICERINITSOL(initsol);

   virtual SCIP_DECL_PRICEREXITSOL(scip_exitsol);

   /** reduced cost pricing method of variable pricer for feasible LPs */
   virtual SCIP_DECL_PRICERREDCOST(scip_redcost);

   /** farkas pricing method of variable pricer for infeasible LPs */
   //virtual SCIP_DECL_PRICERFARKAS(scip_farkas);

   /** perform pricing */
   SCIP_RETCODE pricing(SCIP*   scip);            /**< whether we perform Farkas pricing */


   /** add tour variable to problem */
   SCIP_RETCODE add_newcolumn_variable(
		   SCIP*                                                           scip,
		      vector< vector< vector < vector < vector < SCIP_Real > > > > >  & y_newcolumn,
		      SCIP_Real                                                      z_newcolumn,
		      int                                                             t,
		      int                                                             j ) ;

   /** return negative reduced cost tour (uses restricted shortest path dynamic programming algorithm) */
   SCIP_RETCODE find_new_column(
		   vector < vector < vector < vector <SCIP_Real > > > > pi,
		   vector< vector < vector < vector <  SCIP_Real > > > > lambda,     /**< dual variable value*/
		   vector< vector < SCIP_Real > > gamma,
		   vector< vector <vector< vector< vector< SCIP_Real > > > > > & y_newcolumn,
		   vector< vector < SCIP_Real > > & z_newcolumn,
		   vector< vector < SCIP_Real > > & objval );


private:

   vector<double>                                 _purchcost;
   vector<int >                                   _vialsize;
   int                                            _Nper_to_spoil;
   double                                         _Wastbeta;
   int                                            _T;
   int                                            _n_pricing;
   int                                            _nrestart;
   int                                            _NScen;
   int                                            _Nnodes;
   int                                            _K;
   int                                            _I;
   vector<double>                                 _Nodedemand;
   vector< vector<int> >                          _nodemat; 
   vector< vector<int> >                          _jmat;

   vector < vector < vector < SCIP_VAR* > > >      _x_var;
   vector< SCIP_VAR* >                         _psi_var;
   vector< vector< SCIP_VAR * > >              _wj_var;  
  // vector< vector< SCIP_VAR * > >              _wj_var2;
   vector < vector < vector < vector < SCIP_CONS* > > > >    _wt_con;
   vector < vector < vector < vector < SCIP_CONS* > > > >   _sec_con;  /**< array of partitioning constraints */
   vector<vector<SCIP_CONS* > >                _z_con;  
   SCIP_CONS*                                  _alpha_con;
   vector< vector<SCIP_CONS* > >               _w_con;
};
#endif

