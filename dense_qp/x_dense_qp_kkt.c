/**************************************************************************************************
*                                                                                                 *
* This file is part of HPIPM.                                                                     *
*                                                                                                 *
* HPIPM -- High-Performance Interior Point Method.                                                *
* Copyright (C) 2019 by Gianluca Frison.                                                          *
* Developed at IMTEK (University of Freiburg) under the supervision of Moritz Diehl.              *
* All rights reserved.                                                                            *
*                                                                                                 *
* The 2-Clause BSD License                                                                        *
*                                                                                                 *
* Redistribution and use in source and binary forms, with or without                              *
* modification, are permitted provided that the following conditions are met:                     *
*                                                                                                 *
* 1. Redistributions of source code must retain the above copyright notice, this                  *
*    list of conditions and the following disclaimer.                                             *
* 2. Redistributions in binary form must reproduce the above copyright notice,                    *
*    this list of conditions and the following disclaimer in the documentation                    *
*    and/or other materials provided with the distribution.                                       *
*                                                                                                 *
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND                 *
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED                   *
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE                          *
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR                 *
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES                  *
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;                    *
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND                     *
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT                      *
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS                   *
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.                                    *
*                                                                                                 *
* Author: Gianluca Frison, gianluca.frison (at) imtek.uni-freiburg.de                             *
*                                                                                                 *
**************************************************************************************************/



// range-space (Schur complement) method
void FACT_SOLVE_KKT_UNCONSTR_DENSE_QP(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{

	int ii, jj;

	int nv = qp->dim->nv;
	int ne = qp->dim->ne;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;

	struct STRMAT *Hg = qp->Hv;
	struct STRMAT *A = qp->A;
	struct STRVEC *gz = qp->gz;
	struct STRVEC *b = qp->b;

	struct STRVEC *v = qp_sol->v;
	struct STRVEC *pi = qp_sol->pi;

	struct STRMAT *Lv = ws->Lv;
	struct STRMAT *Le = ws->Le;
	struct STRMAT *Ctx = ws->Ctx;
	struct STRMAT *AL = ws->AL;
	struct STRVEC *lv = ws->lv;

	// null space
	struct STRMAT *A_LQ = ws->A_LQ;
	struct STRMAT *A_Q = ws->A_Q;
	struct STRMAT *Zt = ws->Zt;
	struct STRMAT *ZtH = ws->ZtH;
	struct STRMAT *ZtHZ = ws->ZtHZ;
	struct STRVEC *xy = ws->xy;
	struct STRVEC *Yxy = ws->Yxy;
	struct STRVEC *xz = ws->xz;
	struct STRVEC *tmp_nv = ws->tmp_nv;
	void *lq_work_null = ws->lq_work_null;
	void *orglq_work_null = ws->orglq_work_null;

	if(ne>0)
		{
		if(arg->kkt_fact_alg==0) // null space method
			{
			// TODO check to cache LQ across IPM iterations !!!!!
			GELQF(ne, nv, A, 0, 0, A_LQ, 0, 0, lq_work_null);
//			printf("\nA_LQ\n");
//			blasfeo_print_dmat(ne, nv, A_LQ, 0, 0);

			// TODO cache dA containing tau into another vector !!!!!

			// TODO change dorglq API to pass tau explicitly as a vector !!!!!
			// TODO allocate its dedicated workspace !!!!!
			ORGLQ(nv, nv, ne, A_LQ, 0, 0, A_Q, 0, 0, orglq_work_null);
//			printf("\nA_Q\n");
//			blasfeo_print_dmat(nv, nv, A_Q, 0, 0);

			GECP(nv-ne, nv, A_Q, ne, 0, Zt, 0, 0);
//			printf("\nZt\n");
//			blasfeo_print_dmat(nv-ne, nv, Zt, 0, 0);

#if 0
//printf("\nA\n");
//blasfeo_print_dmat(ne, nv, A, 0, 0);
//printf("\nA_LQ\n");
//blasfeo_print_dmat(ne, nv, A_LQ, 0, 0);
//printf("\nA_Q\n");
//blasfeo_print_dmat(nv, nv, A_Q, 0, 0);

for(ii=0; ii<ne; ii++)
	for(jj=ii+1; jj<nv; jj++)
		BLASFEO_DMATEL(A_LQ, ii, jj) = 0;
blasfeo_dgemm_nn(ne, nv, nv, 1.0, A_LQ, 0, 0, A_Q, 0, 0, -1.0, A, 0, 0, AL, 0, 0);
double max_err = 0.0;
double tmp;
for(ii=0; ii<ne; ii++)
	for(jj=ii+1; jj<nv; jj++)
		{
		tmp = BLASFEO_DMATEL(AL, ii, jj);
		max_err = fabs(tmp)>max_err ? fabs(tmp) : max_err;
		}
printf("\nA_LQ * A_Q - A max err %e\n", max_err);
//blasfeo_print_exp_dmat(ne, nv, AL, 0, 0);
//exit(1);
#endif

			GEMM_NT(nv-ne, nv, nv, 1.0, Zt, 0, 0, Hg, 0, 0, 0.0, ZtH, 0, 0, ZtH, 0, 0);
//			printf("\nZtH\n");
//			blasfeo_print_dmat(nv-ne, nv, ZtH, 0, 0);

			SYRK_LN(nv-ne, nv, 1.0, ZtH, 0, 0, Zt, 0, 0, 0.0, ZtHZ, 0, 0, ZtHZ, 0, 0);
//			printf("\nZtHZ\n");
//			blasfeo_print_dmat(nv-ne, nv-ne, ZtHZ, 0, 0);

			POTRF_L(nv-ne, ZtHZ, 0, 0, ZtHZ, 0, 0);
//			printf("\nZtHZ\n");
//			blasfeo_print_dmat(nv-ne, nv-ne, ZtHZ, 0, 0);

			TRSV_LNN(ne, A_LQ, 0, 0, b, 0, xy, 0);
//			printf("\nxy\n");
//			blasfeo_print_dvec(ne, xy, 0);

			GEMV_T(ne, nv, 1.0, A_Q, 0, 0, xy, 0, 0.0, Yxy, 0, Yxy, 0);
//			printf("\nYxy\n");
//			blasfeo_print_dvec(nv, Yxy, 0);

			GEMV_N(nv-ne, nv, -1.0, ZtH, 0, 0, Yxy, 0, 0.0, xz, 0, xz, 0);
//			printf("\nxz\n");
//			blasfeo_print_dvec(nv-ne, xz, 0);

			GEMV_N(nv-ne, nv, -1.0, Zt, 0, 0, gz, 0, 1.0, xz, 0, xz, 0);
//			printf("\nxz\n");
//			blasfeo_print_dvec(nv-ne, xz, 0);

			TRSV_LNN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);
//			printf("\nxz\n");
//			blasfeo_print_dvec(nv-ne, xz, 0);

			TRSV_LTN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);
//			printf("\nxz\n");
//			blasfeo_print_dvec(nv-ne, xz, 0);

			GEMV_T(nv-ne, nv, 1.0, Zt, 0, 0, xz, 0, 1.0, Yxy, 0, v, 0);
//			printf("\nv\n");
//			blasfeo_print_dvec(nv, v, 0);

			SYMV_L(nv, nv, 1.0, Hg, 0, 0, v, 0, 1.0, gz, 0, tmp_nv, 0);
//			printf("\ntmp_nv\n");
//			blasfeo_print_dvec(nv, tmp_nv, 0);

			GEMV_N(ne, nv, 1.0, A_Q, 0, 0, tmp_nv, 0, 0.0, pi, 0, pi, 0);
//			printf("\npi\n");
//			blasfeo_print_dvec(ne, pi, 0);

			TRSV_LTN(ne, A_LQ, 0, 0, pi, 0, pi, 0);
//			printf("\npi\n");
//			blasfeo_print_dvec(ne, pi, 0);

			// TODO
//			printf("\ndone!\n");
//			exit(1);
			}
		else // range space method
			{
			POTRF_L(nv, Hg, 0, 0, Lv, 0, 0);

//			GECP(ne, nv, A, 0, 0, AL, 0, 0);
			TRSM_RLTN(ne, nv, 1.0, Lv, 0, 0, A, 0, 0, AL, 0, 0);

			GESE(ne, ne, 0.0, Le, 0, 0);
			SYRK_POTRF_LN(ne, nv, AL, 0, 0, AL, 0, 0, Le, 0, 0, Le, 0, 0);

			TRSV_LNN(nv, Lv, 0, 0, gz, 0, lv, 0);

			GEMV_N(ne, nv, 1.0, AL, 0, 0, lv, 0, 1.0, b, 0, pi, 0);

			TRSV_LNN(ne, Le, 0, 0, pi, 0, pi, 0);
			TRSV_LTN(ne, Le, 0, 0, pi, 0, pi, 0);

			GEMV_T(ne, nv, 1.0, A, 0, 0, pi, 0, -1.0, gz, 0, v, 0);

			TRSV_LNN(nv, Lv, 0, 0, v, 0, v, 0);
			TRSV_LTN(nv, Lv, 0, 0, v, 0, v, 0);
			}
		}
	else
		{
#if 0
		POTRF_L(nv, Hg, 0, 0, Lv, 0, 0);

		VECCP(nv, gz, 0, v, 0);
		VECSC(nv, -1.0, v, 0);

		TRSV_LNN(nv, Lv, 0, 0, v, 0, v, 0);
		TRSV_LTN(nv, Lv, 0, 0, v, 0, v, 0);
#else
		ROWIN(nv, 1.0, gz, 0, Hg, nv, 0);
		POTRF_L_MN(nv+1, nv, Hg, 0, 0, Lv, 0, 0);

		ROWEX(nv, -1.0, Lv, nv, 0, v, 0);
		TRSV_LTN(nv, Lv, 0, 0, v, 0, v, 0);
#endif
		}

	return;

	}



static void COND_SLACKS_FACT_SOLVE(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{

	int ii, idx;

	int nv = qp->dim->nv;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;
	int ns = qp->dim->ns;

	struct STRVEC *Z = qp->Z;
	int *idxs = qp->idxs;

//	struct STRVEC *dv = ws->sol_step->v;
	struct STRVEC *dv = qp_sol->v;

//	struct STRVEC *res_g = ws->res->res_g;
	struct STRVEC *res_g = qp->gz;

	struct STRVEC *Gamma = ws->Gamma;
	struct STRVEC *gamma = ws->gamma;
	struct STRVEC *Zs_inv = ws->Zs_inv;
	struct STRVEC *tmp_nbg = ws->tmp_nbg;

	REAL *ptr_Gamma = Gamma->pa;
	REAL *ptr_gamma = gamma->pa;
	REAL *ptr_Z = Z->pa;
	REAL *ptr_Zs_inv = Zs_inv->pa;
	REAL *ptr_dv = dv->pa;
	REAL *ptr_res_g = res_g->pa;
	REAL *ptr_tmp0 = (tmp_nbg+0)->pa;
	REAL *ptr_tmp1 = (tmp_nbg+1)->pa;
	REAL *ptr_tmp2 = (tmp_nbg+2)->pa;
	REAL *ptr_tmp3 = (tmp_nbg+3)->pa;

	REAL tmp0, tmp1;

	VECCP(nb+ng, Gamma, 0, tmp_nbg+0, 0);
	VECCP(nb+ng, Gamma, nb+ng, tmp_nbg+1, 0);
	VECCP(nb+ng, gamma, 0, tmp_nbg+2, 0);
	VECCP(nb+ng, gamma, nb+ng, tmp_nbg+3, 0);

	for(ii=0; ii<ns; ii++)
		{
		idx = idxs[ii];
		ptr_Zs_inv[0+ii]  = ptr_Z[0+ii]  + arg->reg_prim + ptr_Gamma[0+idx]     + ptr_Gamma[2*nb+2*ng+ii];
		ptr_Zs_inv[ns+ii] = ptr_Z[ns+ii] + arg->reg_prim + ptr_Gamma[nb+ng+idx] + ptr_Gamma[2*nb+2*ng+ns+ii];
		ptr_dv[nv+ii]     = ptr_res_g[nv+ii]    + ptr_gamma[0+idx]     + ptr_gamma[2*nb+2*ng+ii];
		ptr_dv[nv+ns+ii]  = ptr_res_g[nv+ns+ii] + ptr_gamma[nb+ng+idx] + ptr_gamma[2*nb+2*ng+ns+ii];
		ptr_Zs_inv[0+ii]  = 1.0/ptr_Zs_inv[0+ii];
		ptr_Zs_inv[ns+ii] = 1.0/ptr_Zs_inv[ns+ii];
		tmp0 = ptr_dv[nv+ii]*ptr_Zs_inv[0+ii];
		tmp1 = ptr_dv[nv+ns+ii]*ptr_Zs_inv[ns+ii];
		ptr_tmp0[idx] = ptr_tmp0[idx] - ptr_tmp0[idx]*ptr_Zs_inv[0+ii]*ptr_tmp0[idx];
		ptr_tmp1[idx] = ptr_tmp1[idx] - ptr_tmp1[idx]*ptr_Zs_inv[ns+ii]*ptr_tmp1[idx];
		ptr_tmp2[idx] = ptr_tmp2[idx] - ptr_Gamma[0+idx]*tmp0;
		ptr_tmp3[idx] = ptr_tmp3[idx] - ptr_Gamma[nb+ng+idx]*tmp1;
		}
	
	AXPY(nb+ng,  1.0, tmp_nbg+1, 0, tmp_nbg+0, 0, tmp_nbg+0, 0);
	AXPY(nb+ng, -1.0, tmp_nbg+3, 0, tmp_nbg+2, 0, tmp_nbg+1, 0);

	return;

	}



static void COND_SLACKS_SOLVE(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_WS *ws)
	{

	int ii, idx;

	int nv = qp->dim->nv;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;
	int ns = qp->dim->ns;

	int *idxs = qp->idxs;

//	struct STRVEC *dv = ws->sol_step->v;
	struct STRVEC *dv = qp_sol->v;

//	struct STRVEC *res_g = ws->res->res_g;
	struct STRVEC *res_g = qp->gz;

	struct STRVEC *Gamma = ws->Gamma;
	struct STRVEC *gamma = ws->gamma;
	struct STRVEC *Zs_inv = ws->Zs_inv;
	struct STRVEC *tmp_nbg = ws->tmp_nbg;

	REAL *ptr_Gamma = Gamma->pa;
	REAL *ptr_gamma = gamma->pa;
	REAL *ptr_Zs_inv = Zs_inv->pa;
	REAL *ptr_dv = dv->pa;
	REAL *ptr_res_g = res_g->pa;
	REAL *ptr_tmp2 = (tmp_nbg+2)->pa;
	REAL *ptr_tmp3 = (tmp_nbg+3)->pa;

	REAL tmp0, tmp1;

	VECCP(nb+ng, gamma, 0, tmp_nbg+2, 0);
	VECCP(nb+ng, gamma, nb+ng, tmp_nbg+3, 0);

	for(ii=0; ii<ns; ii++)
		{
		idx = idxs[ii];
		ptr_dv[nv+ii]     = ptr_res_g[nv+ii]    + ptr_gamma[0+idx]     + ptr_gamma[2*nb+2*ng+ii];
		ptr_dv[nv+ns+ii]  = ptr_res_g[nv+ns+ii] + ptr_gamma[nb+ng+idx] + ptr_gamma[2*nb+2*ng+ns+ii];
		tmp0 = ptr_dv[nv+ii]*ptr_Zs_inv[0+ii];
		tmp1 = ptr_dv[nv+ns+ii]*ptr_Zs_inv[ns+ii];
		ptr_tmp2[idx] = ptr_tmp2[idx] - ptr_Gamma[0+idx]*tmp0;
		ptr_tmp3[idx] = ptr_tmp3[idx] - ptr_Gamma[nb+ng+idx]*tmp1;
		}
	
	AXPY(nb+ng, -1.0, tmp_nbg+3, 0, tmp_nbg+2, 0, tmp_nbg+1, 0);

	return;

	}



static void EXPAND_SLACKS(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_WS *ws)
	{

	int ii, idx;

	int nv = qp->dim->nv;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;
	int ns = qp->dim->ns;

	int *idxs = qp->idxs;

	struct STRVEC *dv = qp_sol->v;
	struct STRVEC *dt = qp_sol->t;

	struct STRVEC *Gamma = ws->Gamma;
	struct STRVEC *Zs_inv = ws->Zs_inv;

	REAL *ptr_Gamma = Gamma->pa;
	REAL *ptr_dv = dv->pa;
	REAL *ptr_dt = dt->pa;
	REAL *ptr_Zs_inv = Zs_inv->pa;

	for(ii=0; ii<ns; ii++)
		{
		idx = idxs[ii];
		ptr_dv[nv+ii]    = - ptr_Zs_inv[0+ii]  * (ptr_dv[nv+ii]    + ptr_dt[idx]*ptr_Gamma[idx]);
		ptr_dv[nv+ns+ii] = - ptr_Zs_inv[ns+ii] * (ptr_dv[nv+ns+ii] + ptr_dt[nb+ng+idx]*ptr_Gamma[nb+ng+idx]);
		ptr_dt[2*nb+2*ng+ii]    = ptr_dv[nv+ii];
		ptr_dt[2*nb+2*ng+ns+ii] = ptr_dv[nv+ns+ii];
		ptr_dt[0+idx]     = ptr_dt[0+idx]     + ptr_dv[nv+ii];
		ptr_dt[nb+ng+idx] = ptr_dt[nb+ng+idx] + ptr_dv[nv+ns+ii];
		}

	return;

	}



// range-space (Schur complement) method
void FACT_SOLVE_KKT_STEP_DENSE_QP(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{

	int ii, jj;

	int nv = qp->dim->nv;
	int ne = qp->dim->ne;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;
	int ns = qp->dim->ns;

	struct STRMAT *Hg = qp->Hv;
	struct STRMAT *A = qp->A;
	struct STRMAT *Ct = qp->Ct;
	int *idxb = qp->idxb;

	struct STRVEC *res_g = qp->gz;
	struct STRVEC *res_b = qp->b;

	struct STRVEC *dv = qp_sol->v;
	struct STRVEC *dpi = qp_sol->pi;
	struct STRVEC *dt = qp_sol->t;

	struct STRMAT *Lv = ws->Lv;
	struct STRMAT *Le = ws->Le;
	struct STRMAT *Ctx = ws->Ctx;
	struct STRMAT *AL = ws->AL;
	struct STRVEC *lv = ws->lv;
	struct STRVEC *sv = ws->sv;
	struct STRVEC *se = ws->se;
	struct STRVEC *Gamma = ws->Gamma;
	struct STRVEC *gamma = ws->gamma;
	struct STRVEC *tmp_nbg = ws->tmp_nbg;

	// null space
	struct STRMAT *A_LQ = ws->A_LQ;
	struct STRMAT *A_Q = ws->A_Q;
	struct STRMAT *Zt = ws->Zt;
	struct STRMAT *ZtH = ws->ZtH;
	struct STRMAT *ZtHZ = ws->ZtHZ;
	struct STRVEC *xy = ws->xy;
	struct STRVEC *Yxy = ws->Yxy;
	struct STRVEC *xz = ws->xz;
	struct STRVEC *tmp_nv = ws->tmp_nv;
	void *lq_work_null = ws->lq_work_null;
	void *orglq_work_null = ws->orglq_work_null;

	REAL tmp;

	struct CORE_QP_IPM_WORKSPACE *cws = ws->core_workspace;

	if(nb+ng>0)
		{
		COMPUTE_GAMMA_GAMMA_QP(qp->d->pa, qp->m->pa, cws);
		}

	if(ne>0)
		{

		if(arg->kkt_fact_alg==0) // null space method
			{

			if(ws->use_A_fact==0)
				{
				GELQF(ne, nv, A, 0, 0, A_LQ, 0, 0, lq_work_null);

				// TODO cache dA containing tau into another vector !!!!!

				// TODO change dorglq API to pass tau explicitly as a vector !!!!!
				// TODO allocate its dedicated workspace !!!!!
				ORGLQ(nv, nv, ne, A_LQ, 0, 0, A_Q, 0, 0, orglq_work_null);

				GECP(nv-ne, nv, A_Q, ne, 0, Zt, 0, 0);

#if 0
printf("\nA\n");
blasfeo_print_dmat(ne, nv, A, 0, 0);
printf("\nA_LQ\n");
blasfeo_print_dmat(ne, nv, A_LQ, 0, 0);
printf("\nA_Q\n");
blasfeo_print_dmat(nv, nv, A_Q, 0, 0);

for(ii=0; ii<ne; ii++)
	for(jj=ii+1; jj<nv; jj++)
		BLASFEO_DMATEL(A_LQ, ii, jj) = 0;
blasfeo_dgemm_nn(ne, nv, nv, 1.0, A_LQ, 0, 0, A_Q, 0, 0, -1.0, A, 0, 0, AL, 0, 0);
double max_err = 0.0;
double tmp;
for(ii=0; ii<ne; ii++)
	for(jj=ii+1; jj<nv; jj++)
		{
		tmp = BLASFEO_DMATEL(AL, ii, jj);
		max_err = fabs(tmp)>max_err ? fabs(tmp) : max_err;
		}
printf("\nA_LQ * A_Q - A max err %e\n", max_err);
//blasfeo_print_exp_dmat(ne, nv, AL, 0, 0);
//exit(1);
#endif
				ws->use_A_fact=1;
				}


			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
//			GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);
//			DIARE(nv, arg->reg_prim, Lv, 0, 0);

			VECCP(nv, res_g, 0, lv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
				}
			if(ng>0)
				{
				GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				SYRK_LN(nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
				}

			TRTR_L(nv, Lv, 0, 0, Lv, 0, 0);

			GEMM_NT(nv-ne, nv, nv, 1.0, Zt, 0, 0, Lv, 0, 0, 0.0, ZtH, 0, 0, ZtH, 0, 0);
			SYRK_LN(nv-ne, nv, 1.0, ZtH, 0, 0, Zt, 0, 0, 0.0, ZtHZ, 0, 0, ZtHZ, 0, 0);
			DIARE(nv-ne, arg->reg_prim, ZtHZ, 0, 0);
			POTRF_L(nv-ne, ZtHZ, 0, 0, ZtHZ, 0, 0);

			TRSV_LNN(ne, A_LQ, 0, 0, res_b, 0, xy, 0);
//printf("\nxy\n");
//blasfeo_print_tran_dvec(ne, xy, 0);
			GEMV_T(ne, nv, 1.0, A_Q, 0, 0, xy, 0, 0.0, Yxy, 0, Yxy, 0);

			GEMV_N(nv-ne, nv, -1.0, ZtH, 0, 0, Yxy, 0, 0.0, xz, 0, xz, 0);
			GEMV_N(nv-ne, nv, -1.0, Zt, 0, 0, lv, 0, 1.0, xz, 0, xz, 0);
			TRSV_LNN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);
			TRSV_LTN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);

			GEMV_T(nv-ne, nv, 1.0, Zt, 0, 0, xz, 0, 1.0, Yxy, 0, dv, 0);

			SYMV_L(nv, nv, 1.0, Lv, 0, 0, dv, 0, 1.0, lv, 0, tmp_nv, 0);
			GEMV_N(ne, nv, 1.0, A_Q, 0, 0, tmp_nv, 0, 0.0, dpi, 0, dpi, 0);
			TRSV_LTN(ne, A_LQ, 0, 0, dpi, 0, dpi, 0);

			}
		else // schur-complement method
			{

			if(arg->scale)
				{

	//			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
				GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);

				VECCP(nv, res_g, 0, lv, 0);

				if(ns>0)
					{
					COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
					}
				else if(nb+ng>0)
					{
					AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
					AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
					}
				if(nb>0)
					{
					DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
					VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
					}
				if(ng>0)
					{
					GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
					GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
					SYRK_LN(nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
					}

				DIAEX(nv, 1.0, Lv, 0, 0, sv, 0);
				for(ii=0; ii<nv; ii++)
					{
					tmp = sqrt(sv->pa[ii]);
	//				tmp = sqrt(tmp);
	//				tmp = sqrt(sv->pa[ii]+tmp);
	//				tmp = 1.0;
					sv->pa[ii] = tmp==0 ? 1.0 : 1.0/tmp;
					}

				GEMM_L_DIAG(nv, nv, 1.0, sv, 0, Lv, 0, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
				GEMM_R_DIAG(nv, nv, 1.0, Lv, 0, 0, sv, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
				DIARE(nv, arg->reg_prim, Lv, 0, 0);
				POTRF_L(nv, Lv, 0, 0, Lv, 0, 0);

				GEMV_DIAG(nv, 1.0, sv, 0, lv, 0, 0.0, lv, 0, lv, 0);
				VECCP(nv, lv, 0, dv, 0);

				GECP(ne, nv, A, 0, 0, AL, 0, 0);
				GEMM_R_DIAG(ne, nv, 1.0, AL, 0, 0, sv, 0, 0.0, AL, 0, 0, AL, 0, 0);
				TRSM_RLTN(ne, nv, 1.0, Lv, 0, 0, AL, 0, 0, AL, 0, 0);

				TRSV_LNN(nv, Lv, 0, 0, lv, 0, lv, 0);

				GESE(ne, ne, 0.0, Le, 0, 0);
				SYRK_LN(ne, nv, 1.0, AL, 0, 0, AL, 0, 0, 1.0, Le, 0, 0, Le, 0, 0);

				DIAEX(ne, 1.0, Le, 0, 0, se, 0);
				for(ii=0; ii<ne; ii++)
					{
					tmp = sqrt(se->pa[ii]);
	//				tmp = sqrt(tmp);
	//				tmp = sqrt(se->pa[ii]+tmp);
	//				tmp = 1.0;
					se->pa[ii] = tmp==0 ? 1.0 : 1.0/tmp;
					}

				GEMM_L_DIAG(ne, ne, 1.0, se, 0, Le, 0, 0, 0.0, Le, 0, 0, Le, 0, 0);
				GEMM_R_DIAG(ne, ne, 1.0, Le, 0, 0, se, 0, 0.0, Le, 0, 0, Le, 0, 0);
				DIARE(ne, arg->reg_prim, Le, 0, 0);
				POTRF_L(ne, Le, 0, 0, Le, 0, 0);

				GEMV_N(ne, nv, 1.0, AL, 0, 0, lv, 0, 1.0, res_b, 0, dpi, 0);

				GEMV_DIAG(ne, 1.0, se, 0, dpi, 0, 0.0, dpi, 0, dpi, 0);
				TRSV_LNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
				TRSV_LTN(ne, Le, 0, 0, dpi, 0, dpi, 0);
				GEMV_DIAG(ne, 1.0, se, 0, dpi, 0, 0.0, dpi, 0, dpi, 0);

				GEMV_T(ne, nv, 1.0, A, 0, 0, dpi, 0, 0.0, lv, 0, lv, 0);
				GEMV_DIAG(nv, 1.0, sv, 0, lv, 0, -1.0, dv, 0, dv, 0);

				TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
				TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);
				GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);

				}
			else // no scale
				{

	//			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
				GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);

				VECCP(nv, res_g, 0, lv, 0);

				if(ns>0)
					{
					COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
					}
				else if(nb+ng>0)
					{
					AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
					AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
					}
				if(nb>0)
					{
					DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
					VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
					}
				if(ng>0)
					{
					GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
					GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
					SYRK_POTRF_LN(nv, ng, Ctx, 0, 0, Ct, 0, 0, Lv, 0, 0, Lv, 0, 0);
					}
				else
					{
					POTRF_L(nv, Lv, 0, 0, Lv, 0, 0);
					}
	//int pd = 1;
	//for(ii=0; ii<nv; ii++)
	//	if(Lv->dA[ii]==0.0)
	//		pd = 0;
	//printf(" chol pd %d\n", pd);

				VECCP(nv, lv, 0, dv, 0);

				TRSM_RLTN(ne, nv, 1.0, Lv, 0, 0, A, 0, 0, AL, 0, 0);

				TRSV_LNN(nv, Lv, 0, 0, lv, 0, lv, 0);

				GEMV_N(ne, nv, 1.0, AL, 0, 0, lv, 0, 1.0, res_b, 0, dpi, 0);

				GESE(ne, ne, 0.0, Le, 0, 0);
				SYRK_POTRF_LN(ne, nv, AL, 0, 0, AL, 0, 0, Le, 0, 0, Le, 0, 0);

				TRSV_LNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
				TRSV_LTN(ne, Le, 0, 0, dpi, 0, dpi, 0);

				GEMV_T(ne, nv, 1.0, A, 0, 0, dpi, 0, -1.0, dv, 0, dv, 0);

				TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
				TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);


				} // scale

			}

		}
	else // ne==0
		{

		if(arg->scale)
			{

			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
			VECCP(nv, res_g, 0, lv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
				}
			if(ng>0)
				{
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
				SYRK_LN(nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
				}

			DIAEX(nv, 1.0, Lv, 0, 0, sv, 0);
			for(ii=0; ii<nv; ii++)
				{
				tmp = sqrt(sv->pa[ii]);
//				tmp = sqrt(tmp);
//				tmp = sqrt(sv->pa[ii]+tmp);
//				tmp = 1.0;
				sv->pa[ii] = tmp==0 ? 1.0 : 1.0/tmp;
				}

			GEMM_L_DIAG(nv, nv, 1.0, sv, 0, Lv, 0, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
			GEMM_R_DIAG(nv, nv, 1.0, Lv, 0, 0, sv, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
			DIARE(nv, arg->reg_prim, Lv, 0, 0);
			POTRF_L_MN(nv, nv, Lv, 0, 0, Lv, 0, 0);

			VECCP(nv, lv, 0, dv, 0);
			VECSC(nv, -1.0, dv, 0);

			GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);
			TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);
			GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);
			}
		else // no scale
			{
	//		TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
			GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);
			ROWIN(nv, 1.0, res_g, 0, Lv, nv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				ROWAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, Lv, nv, 0);
				}
			if(ng>0)
				{
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				ROWIN(ng, 1.0, tmp_nbg+1, nb, Ctx, nv, 0);
				SYRK_POTRF_LN_MN(nv+1, nv, ng, Ctx, 0, 0, Ct, 0, 0, Lv, 0, 0, Lv, 0, 0);
				}
			else
				{
				POTRF_L_MN(nv+1, nv, Lv, 0, 0, Lv, 0, 0);
				}

			ROWEX(nv, -1.0, Lv, nv, 0, dv, 0);
			TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);

			} // scale

		} // ne>0

//blasfeo_print_tran_dvec(nv, dv, 0);
//blasfeo_print_tran_dvec(ne, dpi, 0);
//exit(1);
	if(nb+ng>0)
		{
		if(nb>0)
			VECEX_SP(nb, 1.0, idxb, dv, 0, dt, 0);

		if(ng>0)
			GEMV_T(nv, ng, 1.0, Ct, 0, 0, dv, 0, 0.0, dt, nb, dt, nb);

		VECCP(nb+ng, dt, 0, dt, nb+ng);
		VECSC(nb+ng, -1.0, dt, nb+ng);

		if(ns>0)
			EXPAND_SLACKS(qp, qp_sol, ws);

		COMPUTE_LAM_T_QP(qp->d->pa, qp->m->pa, qp_sol->lam->pa, qp_sol->t->pa, cws);
		}

	return;

	}



void FACT_LQ_SOLVE_KKT_STEP_DENSE_QP(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{

	int ii;

	int nv = qp->dim->nv;
	int ne = qp->dim->ne;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;
	int ns = qp->dim->ns;

	struct STRMAT *Hg = qp->Hv;
	struct STRMAT *A = qp->A;
	struct STRMAT *Ct = qp->Ct;
	int *idxb = qp->idxb;
	struct STRVEC *res_g = qp->gz;
	struct STRVEC *res_b = qp->b;

	struct STRVEC *dv = qp_sol->v;
	struct STRVEC *dpi = qp_sol->pi;
	struct STRVEC *dt = qp_sol->t;

	struct STRMAT *Lv = ws->Lv;
	struct STRMAT *Le = ws->Le;
	struct STRMAT *Ctx = ws->Ctx;
	struct STRMAT *AL = ws->AL;
	struct STRVEC *lv = ws->lv;
	struct STRVEC *sv = ws->sv;
	struct STRVEC *se = ws->se;
	struct STRVEC *Gamma = ws->Gamma;
	struct STRVEC *gamma = ws->gamma;
	struct STRVEC *tmp_nbg = ws->tmp_nbg;
	void *lq_work0 = ws->lq_work0;
	void *lq_work1 = ws->lq_work1;
	struct STRMAT *lq0 = ws->lq0;
	struct STRMAT *lq1 = ws->lq1;

	// null space
	struct STRMAT *A_LQ = ws->A_LQ;
	struct STRMAT *A_Q = ws->A_Q;
	struct STRMAT *Zt = ws->Zt;
	struct STRMAT *ZtH = ws->ZtH;
	struct STRMAT *ZtHZ = ws->ZtHZ;
	struct STRVEC *xy = ws->xy;
	struct STRVEC *Yxy = ws->Yxy;
	struct STRVEC *xz = ws->xz;
	struct STRVEC *tmp_nv = ws->tmp_nv;
	void *lq_work_null = ws->lq_work_null;
	void *orglq_work_null = ws->orglq_work_null;

	REAL tmp;

	struct CORE_QP_IPM_WORKSPACE *cws = ws->core_workspace;

	ws->scale = 0;

	if(nb+ng>0)
		{
		COMPUTE_GAMMA_GAMMA_QP(qp->d->pa, qp->m->pa, cws);
		}

	if(ne>0)
		{

		if(arg->kkt_fact_alg==0) // null space method
			{

			if(ws->use_A_fact==0)
				{
				GELQF(ne, nv, A, 0, 0, A_LQ, 0, 0, lq_work_null);

				// TODO cache dA containing tau into another vector !!!!!

				// TODO change dorglq API to pass tau explicitly as a vector !!!!!
				// TODO allocate its dedicated workspace !!!!!
				ORGLQ(nv, nv, ne, A_LQ, 0, 0, A_Q, 0, 0, orglq_work_null);

				GECP(nv-ne, nv, A_Q, ne, 0, Zt, 0, 0);

				ws->use_A_fact=1;
				}


			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
//			GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);

			VECCP(nv, res_g, 0, lv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
				}
			if(ng>0)
				{
				GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				SYRK_LN(nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
				}

			TRTR_L(nv, Lv, 0, 0, Lv, 0, 0);

			GEMM_NT(nv-ne, nv, nv, 1.0, Zt, 0, 0, Lv, 0, 0, 0.0, ZtH, 0, 0, ZtH, 0, 0);
			SYRK_LN(nv-ne, nv, 1.0, ZtH, 0, 0, Zt, 0, 0, 0.0, ZtHZ, 0, 0, ZtHZ, 0, 0);
			POTRF_L(nv-ne, ZtHZ, 0, 0, ZtHZ, 0, 0);

			TRSV_LNN(ne, A_LQ, 0, 0, res_b, 0, xy, 0);
			GEMV_T(ne, nv, 1.0, A_Q, 0, 0, xy, 0, 0.0, Yxy, 0, Yxy, 0);

			GEMV_N(nv-ne, nv, -1.0, ZtH, 0, 0, Yxy, 0, 0.0, xz, 0, xz, 0);
			GEMV_N(nv-ne, nv, -1.0, Zt, 0, 0, lv, 0, 1.0, xz, 0, xz, 0);
			TRSV_LNN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);
			TRSV_LTN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);

			GEMV_T(nv-ne, nv, 1.0, Zt, 0, 0, xz, 0, 1.0, Yxy, 0, dv, 0);

			SYMV_L(nv, nv, 1.0, Lv, 0, 0, dv, 0, 1.0, lv, 0, tmp_nv, 0);
			GEMV_N(ne, nv, 1.0, A_Q, 0, 0, tmp_nv, 0, 0.0, dpi, 0, dpi, 0);
			TRSV_LTN(ne, A_LQ, 0, 0, dpi, 0, dpi, 0);

			}
		else // schur-complement method
			{

			// XXX needed ???
			GESE(nv, nv+nv+ng, 0.0, lq1, 0, 0); // TODO not the first part for HP and RF

			if(ws->use_hess_fact==0)
				{
				POTRF_L(nv, Hg, 0, 0, Lv+1, 0, 0);
				ws->use_hess_fact=1;
				}
	//int pd = 1;
	//for(ii=0; ii<nv; ii++)
	//	if((Lv+1)->dA[ii]==0.0)
	//		pd = 0;
	//printf(" lq pd %d\n", pd);

			VECCP(nv, res_g, 0, lv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				for(ii=0; ii<nb; ii++)
					{
					tmp = BLASFEO_DVECEL(tmp_nbg+0, ii);
					tmp = tmp>=0.0 ? tmp : 0.0;
					tmp = sqrt( tmp );
					BLASFEO_DMATEL(lq1, idxb[ii], nv+idxb[ii]) = tmp>0.0 ? tmp : 0.0;
					}
				VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
				}
			if(ng>0)
				{
				for(ii=0; ii<ng; ii++)
					{
					tmp = BLASFEO_DVECEL(tmp_nbg+0, nb+ii);
					tmp = tmp>=0.0 ? tmp : 0.0;
					tmp = sqrt( tmp );
					BLASFEO_DVECEL(tmp_nbg+0, nb+ii) = tmp;
					}
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, lq1, 0, nv+nv, lq1, 0, nv+nv);
				GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
				}

			DIARE(nv, arg->reg_prim, lq1, 0, nv);

	//blasfeo_print_dmat(nv, nv, lq1, 0, 0);
	//blasfeo_print_dmat(nv, nv, lq1, 0, nv);
	//blasfeo_print_dmat(nv, ng, lq1, 0, nv+ng);

#if defined(LA_HIGH_PERFORMANCE) | defined(LA_REFERENCE)
	//		TRCP_L(nv, Lv+1, 0, 0, lq1, 0, 0);
	//		GELQF_PD(nv, nv+nv+ng, lq1, 0, 0, lq1, 0, 0, lq_work1);
	//		GELQF_PD_LA(nv, nv+ng, lq1, 0, 0, lq1, 0, nv, lq_work1);
	//		GELQF_PD_LLA(nv, ng, lq1, 0, 0, lq1, 0, nv, lq1, 0, 2*nv, lq_work1);
	//		TRCP_L(nv, lq1, 0, 0, Lv, 0, 0);
			TRCP_L(nv, Lv+1, 0, 0, Lv, 0, 0);
			GELQF_PD_LLA(nv, ng, Lv, 0, 0, lq1, 0, nv, lq1, 0, 2*nv, lq_work1); // TODO reduce lq1 size !!!
#else // LA_BLAS_WRAPPER
			TRCP_L(nv, Lv+1, 0, 0, lq1, 0, 0);
			GELQF(nv, nv+nv+ng, lq1, 0, 0, lq1, 0, 0, lq_work1);
			TRCP_L(nv, lq1, 0, 0, Lv, 0, 0);
			for(ii=0; ii<nv; ii++)
				if(BLASFEO_DMATEL(Lv, ii, ii) < 0)
					COLSC(nv-ii, -1.0, Lv, ii, ii);
#endif

	//blasfeo_print_dmat(nv, nv, Lv, 0, 0);

			VECCP(nv, lv, 0, dv, 0);

			TRSM_RLTN(ne, nv, 1.0, Lv, 0, 0, A, 0, 0, AL, 0, 0);

			TRSV_LNN(nv, Lv, 0, 0, lv, 0, lv, 0);

			GEMV_N(ne, nv, 1.0, AL, 0, 0, lv, 0, 1.0, res_b, 0, dpi, 0);

			GECP(ne, nv, AL, 0, 0, lq0, 0, ne);

#if defined(LA_HIGH_PERFORMANCE)
	//		GESE(ne, ne, 0.0, lq0, 0, 0);
	//		DIARE(ne, arg->reg_dual, lq0, 0, 0);
	//		GELQF_PD(ne, ne+nv, lq0, 0, 0, lq0, 0, 0, lq_work0);
	//		GELQF_PD_LA(ne, nv, lq0, 0, 0, lq0, 0, ne, lq_work0);
	//		TRCP_L(ne, lq0, 0, 0, Le, 0, 0);
			GESE(ne, ne, 0.0, Le, 0, 0);
			DIARE(ne, arg->reg_dual, Le, 0, 0);
			GELQF_PD_LA(ne, nv, Le, 0, 0, lq0, 0, ne, lq_work0); // TODO reduce lq0 size !!!
#elif defined(LA_REFERENCE)
	//		GESE(ne, ne, 0.0, lq0, 0, 0);
	//		DIARE(ne, arg->reg_dual, lq0, 0, 0);
	//		GELQF_PD(ne, ne+nv, lq0, 0, 0, lq0, 0, 0, lq_work0);
	//		GELQF_PD_LA(ne, nv, lq0, 0, 0, lq0, 0, ne, lq_work0);
	//		TRCP_L(ne, lq0, 0, 0, Le, 0, 0);
			GESE(ne, ne, 0.0, Le, 0, 0);
			DIARE(ne, arg->reg_dual, Le, 0, 0);
			GELQF_PD_LA(ne, nv, Le, 0, 0, lq0, 0, ne, lq_work0); // TODO reduce lq0 size !!!
#else // LA_BLAS_WRAPPER
			GESE(ne, ne, 0.0, lq0, 0, 0);
			DIARE(ne, arg->reg_dual, lq0, 0, 0);
			GELQF(ne, ne+nv, lq0, 0, 0, lq0, 0, 0, lq_work0);
			TRCP_L(ne, lq0, 0, 0, Le, 0, 0);
			for(ii=0; ii<ne; ii++)
				if(BLASFEO_DMATEL(Le, ii, ii) < 0)
					COLSC(ne-ii, -1.0, Le, ii, ii);
#endif

	//		blasfeo_print_dmat(ne, ne, Le, 0, 0);

			TRSV_LNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
			TRSV_LTN(ne, Le, 0, 0, dpi, 0, dpi, 0);

			GEMV_T(ne, nv, 1.0, A, 0, 0, dpi, 0, -1.0, dv, 0, dv, 0);

			TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);

			} // schur-complement method

		}
	else // ne==0
		{

		// XXX needed ???
		GESE(nv, nv+nv+ng, 0.0, lq1, 0, 0); // TODO not the first part for HP and RF

		if(ws->use_hess_fact==0)
			{
			POTRF_L(nv, Hg, 0, 0, Lv+1, 0, 0);
			ws->use_hess_fact=1;
			}

		VECCP(nv, res_g, 0, lv, 0);

		if(ns>0)
			{
			COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
			}
		else if(nb+ng>0)
			{
			AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
			AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
			}
		if(nb>0)
			{
			for(ii=0; ii<nb; ii++)
				{
				tmp = BLASFEO_DVECEL(tmp_nbg+0, ii);
				tmp = tmp>=0.0 ? tmp : 0.0;
				tmp = sqrt( tmp );
				BLASFEO_DMATEL(lq1, idxb[ii], nv+idxb[ii]) = tmp>0.0 ? tmp : 0.0;
				}
			VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
			}
		if(ng>0)
			{
			for(ii=0; ii<ng; ii++)
				{
				tmp = BLASFEO_DVECEL(tmp_nbg+0, nb+ii);
				tmp = tmp>=0.0 ? tmp : 0.0;
				tmp = sqrt( tmp );
				BLASFEO_DVECEL(tmp_nbg+0, nb+ii) = tmp;
				}
			GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, lq1, 0, nv+nv, lq1, 0, nv+nv);
			GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
			}

		DIARE(nv, arg->reg_prim, lq1, 0, nv);

#if defined(LA_HIGH_PERFORMANCE)
//		TRCP_L(nv, Lv+1, 0, 0, lq1, 0, 0);
//		GELQF_PD(nv, nv+nv+ng, lq1, 0, 0, lq1, 0, 0, lq_work1);
//		GELQF_PD_LA(nv, nv+ng, lq1, 0, 0, lq1, 0, nv, lq_work1);
//		GELQF_PD_LLA(nv, ng, lq1, 0, 0, lq1, 0, nv, lq1, 0, 2*nv, lq_work1);
//		TRCP_L(nv, lq1, 0, 0, Lv, 0, 0);
		TRCP_L(nv, Lv+1, 0, 0, Lv, 0, 0);
		GELQF_PD_LLA(nv, ng, Lv, 0, 0, lq1, 0, nv, lq1, 0, 2*nv, lq_work1); // TODO reduce lq1 size !!!
#elif defined(LA_REFERENCE)
//		TRCP_L(nv, Lv+1, 0, 0, lq1, 0, 0);
//		GELQF_PD(nv, nv+nv+ng, lq1, 0, 0, lq1, 0, 0, lq_work1);
//		GELQF_PD_LA(nv, nv+ng, lq1, 0, 0, lq1, 0, nv, lq_work1);
//		GELQF_PD_LLA(nv, ng, lq1, 0, 0, lq1, 0, nv, lq1, 0, 2*nv, lq_work1);
//		TRCP_L(nv, lq1, 0, 0, Lv, 0, 0);
		TRCP_L(nv, Lv+1, 0, 0, Lv, 0, 0);
		GELQF_PD_LLA(nv, ng, Lv, 0, 0, lq1, 0, nv, lq1, 0, 2*nv, lq_work1); // TODO reduce lq1 size !!!
#else // LA_BLAS_WRAPPER
		TRCP_L(nv, Lv+1, 0, 0, lq1, 0, 0);
		GELQF(nv, nv+nv+ng, lq1, 0, 0, lq1, 0, 0, lq_work1);
		TRCP_L(nv, lq1, 0, 0, Lv, 0, 0);
		for(ii=0; ii<nv; ii++)
			if(BLASFEO_DMATEL(Lv, ii, ii) < 0)
				COLSC(nv-ii, -1.0, Lv, ii, ii);
#endif

#if 0
if(nv<30)
{
//	blasfeo_print_dmat(nv, nv+nb+ng, lq1, 0, 0);
blasfeo_print_dmat(nv, nv, Lv, 0, 0);
exit(1);
}
#endif

		VECCP(nv, lv, 0, dv, 0);
		VECSC(nv, -1.0, dv, 0);

		TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
		TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);

		} // ne>0
	
	if(nb+ng>0)
		{
		if(nb>0)
			VECEX_SP(nb, 1.0, idxb, dv, 0, dt, 0);

		VECSE(ng, 0.0, dt, nb);
		if(ng>0)
			GEMV_T(nv, ng, 1.0, Ct, 0, 0, dv, 0, 0.0, dt, nb, dt, nb);

		VECCP(nb+ng, dt, 0, dt, nb+ng);
		VECSC(nb+ng, -1.0, dt, nb+ng);

		if(ns>0)
			EXPAND_SLACKS(qp, qp_sol, ws);

		COMPUTE_LAM_T_QP(qp->d->pa, qp->m->pa, qp_sol->lam->pa, qp_sol->t->pa, cws);
		}

	return;

	}



#if 0
void FACT_SOLVE_LU_KKT_STEP_DENSE_QP(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{

	int ii;

	int nv = qp->dim->nv;
	int ne = qp->dim->ne;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;
	int ns = qp->dim->ns;

	struct STRMAT *Hg = qp->Hv;
	struct STRMAT *A = qp->A;
	struct STRMAT *Ct = qp->Ct;
	int *idxb = qp->idxb;

	struct STRVEC *res_g = qp->gz;
	struct STRVEC *res_b = qp->b;

	struct STRVEC *dv = qp_sol->v;
	struct STRVEC *dpi = qp_sol->pi;
	struct STRVEC *dt = qp_sol->t;

	struct STRMAT *Lv = ws->Lv;
	struct STRMAT *Le = ws->Le;
	struct STRMAT *Ctx = ws->Ctx;
	struct STRMAT *AL = ws->AL;
	struct STRVEC *lv = ws->lv;
	struct STRVEC *sv = ws->sv;
	struct STRVEC *se = ws->se;
	struct STRVEC *Gamma = ws->Gamma;
	struct STRVEC *gamma = ws->gamma;
	struct STRVEC *tmp_nbg = ws->tmp_nbg;
	int *ipiv_v = ws->ipiv_v;
	int *ipiv_e = ws->ipiv_e;

	REAL tmp;

	struct CORE_QP_IPM_WORKSPACE *cws = ws->core_workspace;

	if(nb+ng>0)
		{
		COMPUTE_GAMMA_GAMMA_QP(qp->d->pa, qp->m->pa, cws);
		}

	if(ne>0)
		{

		if(arg->scale)
			{

//			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
			GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);

			VECCP(nv, res_g, 0, lv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
				}
			if(ng>0)
				{
				GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				SYRK_LN(nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
				}

			DIAEX(nv, 1.0, Lv, 0, 0, sv, 0);
			for(ii=0; ii<nv; ii++)
				{
				tmp = sqrt(sv->pa[ii]);
//				tmp = sqrt(tmp);
//				tmp = sqrt(sv->pa[ii]+tmp);
//				tmp = 1.0;
				sv->pa[ii] = tmp==0 ? 1.0 : 1.0/tmp;
				}

			GEMM_L_DIAG(nv, nv, 1.0, sv, 0, Lv, 0, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
			GEMM_R_DIAG(nv, nv, 1.0, Lv, 0, 0, sv, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
			DIARE(nv, arg->reg_prim, Lv, 0, 0);
			POTRF_L(nv, Lv, 0, 0, Lv, 0, 0);

			GEMV_DIAG(nv, 1.0, sv, 0, lv, 0, 0.0, lv, 0, lv, 0);
			VECCP(nv, lv, 0, dv, 0);

			GECP(ne, nv, A, 0, 0, AL, 0, 0);
			GEMM_R_DIAG(ne, nv, 1.0, AL, 0, 0, sv, 0, 0.0, AL, 0, 0, AL, 0, 0);
			TRSM_RLTN(ne, nv, 1.0, Lv, 0, 0, AL, 0, 0, AL, 0, 0);

			TRSV_LNN(nv, Lv, 0, 0, lv, 0, lv, 0);

			GESE(ne, ne, 0.0, Le, 0, 0);
			SYRK_LN(ne, nv, 1.0, AL, 0, 0, AL, 0, 0, 1.0, Le, 0, 0, Le, 0, 0);

			DIAEX(ne, 1.0, Le, 0, 0, se, 0);
			for(ii=0; ii<ne; ii++)
				{
				tmp = sqrt(se->pa[ii]);
//				tmp = sqrt(tmp);
//				tmp = sqrt(se->pa[ii]+tmp);
//				tmp = 1.0;
				se->pa[ii] = tmp==0 ? 1.0 : 1.0/tmp;
				}

			GEMM_L_DIAG(ne, ne, 1.0, se, 0, Le, 0, 0, 0.0, Le, 0, 0, Le, 0, 0);
			GEMM_R_DIAG(ne, ne, 1.0, Le, 0, 0, se, 0, 0.0, Le, 0, 0, Le, 0, 0);
			DIARE(ne, arg->reg_prim, Le, 0, 0);
			POTRF_L(ne, Le, 0, 0, Le, 0, 0);

			GEMV_N(ne, nv, 1.0, AL, 0, 0, lv, 0, 1.0, res_b, 0, dpi, 0);

			GEMV_DIAG(ne, 1.0, se, 0, dpi, 0, 0.0, dpi, 0, dpi, 0);
			TRSV_LNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
			TRSV_LTN(ne, Le, 0, 0, dpi, 0, dpi, 0);
			GEMV_DIAG(ne, 1.0, se, 0, dpi, 0, 0.0, dpi, 0, dpi, 0);

			GEMV_T(ne, nv, 1.0, A, 0, 0, dpi, 0, 0.0, lv, 0, lv, 0);
			GEMV_DIAG(nv, 1.0, sv, 0, lv, 0, -1.0, dv, 0, dv, 0);

			TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);
			GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);

			}
		else // no scale
			{

//			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
			GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);

			VECCP(nv, res_g, 0, lv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
				}
			if(ng>0)
				{
				GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				SYRK_LN(nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
				}

			TRTR_L(nv, Lv, 0, 0, Lv, 0, 0);
			GETRF(nv, nv, Lv, 0, 0, Lv, 0, 0, ipiv_v);

			VECCP(nv, lv, 0, dv, 0);
			VECPE(nv, ipiv_v, lv, 0);
			TRSV_LNU(nv, Lv, 0, 0, lv, 0, lv, 0);

			GECP(ne, nv, A, 0, 0, AL+1, 0, 0);
			COLPE(nv, ipiv_v, AL+1);
			TRSM_RLTU(ne, nv, 1.0, Lv, 0, 0, AL+1, 0, 0, AL+1, 0, 0);
//			TRSM_RUNN(ne, nv, 1.0, Lv, 0, 0, A, 0, 0, AL+0, 0, 0);
			TRTR_U(nv, Lv, 0, 0, Lv+1, 0, 0);
			TRSM_RLTN(ne, nv, 1.0, Lv+1, 0, 0, A, 0, 0, AL+0, 0, 0);

			GEMV_N(ne, nv, 1.0, AL+0, 0, 0, lv, 0, 1.0, res_b, 0, dpi, 0);

			SYRK_LN(ne, nv, 1.0, AL+0, 0, 0, AL+1, 0, 0, 0.0, Le, 0, 0, Le, 0, 0);

#if 0
			POTRF_L(ne, Le, 0, 0, Le, 0, 0);

			TRSV_LNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
			TRSV_LTN(ne, Le, 0, 0, dpi, 0, dpi, 0);
#else
			TRTR_L(ne, Le, 0, 0, Le, 0, 0);
			GETRF(ne, ne, Le, 0, 0, Le, 0, 0, ipiv_e);

			VECPE(ne, ipiv_e, dpi, 0);
			TRSV_LNU(ne, Le, 0, 0, dpi, 0, dpi, 0);
			TRSV_UNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
#endif

			GEMV_T(ne, nv, 1.0, A, 0, 0, dpi, 0, -1.0, dv, 0, dv, 0);

			VECPE(nv, ipiv_v, dv, 0);
			TRSV_LNU(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_UNN(nv, Lv, 0, 0, dv, 0, dv, 0);

			} // scale

		}
	else // ne==0
		{

		if(arg->scale)
			{

			TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
			VECCP(nv, res_g, 0, lv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
				}
			if(ng>0)
				{
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
				SYRK_LN(nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
				}

			DIAEX(nv, 1.0, Lv, 0, 0, sv, 0);
			for(ii=0; ii<nv; ii++)
				{
				tmp = sqrt(sv->pa[ii]);
//				tmp = sqrt(tmp);
//				tmp = sqrt(sv->pa[ii]+tmp);
//				tmp = 1.0;
				sv->pa[ii] = tmp==0 ? 1.0 : 1.0/tmp;
				}

			GEMM_L_DIAG(nv, nv, 1.0, sv, 0, Lv, 0, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
			GEMM_R_DIAG(nv, nv, 1.0, Lv, 0, 0, sv, 0, 0.0, Lv, 0, 0, Lv, 0, 0);
			DIARE(nv, arg->reg_prim, Lv, 0, 0);
			POTRF_L_MN(nv, nv, Lv, 0, 0, Lv, 0, 0);

			VECCP(nv, lv, 0, dv, 0);
			VECSC(nv, -1.0, dv, 0);

			GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);
			TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);
			GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);
			}
		else // no scale
			{
	//		TRCP_L(nv, Hg, 0, 0, Lv, 0, 0);
			GECP(nv, nv, Hg, 0, 0, Lv, 0, 0);
			ROWIN(nv, 1.0, res_g, 0, Lv, nv, 0);

			if(ns>0)
				{
				COND_SLACKS_FACT_SOLVE(qp, qp_sol, arg, ws);
				}
			else if(nb+ng>0)
				{
				AXPY(nb+ng,  1.0, Gamma, nb+ng, Gamma, 0, tmp_nbg+0, 0);
				AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
				}
			if(nb>0)
				{
				DIAAD_SP(nb, 1.0, tmp_nbg+0, 0, idxb, Lv, 0, 0);
				ROWAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, Lv, nv, 0);
				}
			if(ng>0)
				{
				GEMM_R_DIAG(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+0, nb, 0.0, Ctx, 0, 0, Ctx, 0, 0);
				ROWIN(ng, 1.0, tmp_nbg+1, nb, Ctx, nv, 0);
				SYRK_LN_MN(nv+1, nv, ng, 1.0, Ctx, 0, 0, Ct, 0, 0, 1.0, Lv, 0, 0, Lv, 0, 0);
				}

			ROWEX(nv, -1.0, Lv, nv, 0, dv, 0);

			TRTR_L(nv, Lv, 0, 0, Lv, 0, 0);
			GETRF(nv, nv, Lv, 0, 0, Lv, 0, 0, ipiv_v);

			VECPE(nv, ipiv_v, dv, 0);
			TRSV_LNU(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_UNN(nv, Lv, 0, 0, dv, 0, dv, 0);

			} // scale

		} // ne>0

	if(nb+ng>0)
		{
		if(nb>0)
			VECEX_SP(nb, 1.0, idxb, dv, 0, dt, 0);

		if(ng>0)
			GEMV_T(nv, ng, 1.0, Ct, 0, 0, dv, 0, 0.0, dt, nb, dt, nb);

		VECCP(nb+ng, dt, 0, dt, nb+ng);
		VECSC(nb+ng, -1.0, dt, nb+ng);

		if(ns>0)
			EXPAND_SLACKS(qp, qp_sol, ws);

		COMPUTE_LAM_T_QP(qp->d->pa, qp->m->pa, qp_sol->lam->pa, qp_sol->t->pa, cws);
		}

	return;

	}
#endif



// range-space (Schur complement) method
void SOLVE_KKT_STEP_DENSE_QP(struct DENSE_QP *qp, struct DENSE_QP_SOL *qp_sol, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{

	int nv = qp->dim->nv;
	int ne = qp->dim->ne;
	int nb = qp->dim->nb;
	int ng = qp->dim->ng;
	int ns = qp->dim->ns;

	struct STRMAT *A = qp->A;
	struct STRMAT *Ct = qp->Ct;
	int *idxb = qp->idxb;
//	struct STRVEC *res_g = ws->res->res_g;
//	struct STRVEC *res_b = ws->res->res_b;
	struct STRVEC *res_g = qp->gz;
	struct STRVEC *res_b = qp->b;

	struct STRVEC *dv = qp_sol->v;
	struct STRVEC *dpi = qp_sol->pi;
	struct STRVEC *dt = qp_sol->t;

	struct STRMAT *Lv = ws->Lv;
	struct STRMAT *Le = ws->Le;
	struct STRMAT *Ctx = ws->Ctx;
	struct STRMAT *AL = ws->AL;
	struct STRVEC *lv = ws->lv;
	struct STRVEC *sv = ws->sv;
	struct STRVEC *se = ws->se;
	struct STRVEC *gamma = ws->gamma;
	struct STRVEC *tmp_nbg = ws->tmp_nbg;

	// null space
	struct STRMAT *A_LQ = ws->A_LQ;
	struct STRMAT *A_Q = ws->A_Q;
	struct STRMAT *Zt = ws->Zt;
	struct STRMAT *ZtH = ws->ZtH;
	struct STRMAT *ZtHZ = ws->ZtHZ;
	struct STRVEC *xy = ws->xy;
	struct STRVEC *Yxy = ws->Yxy;
	struct STRVEC *xz = ws->xz;
	struct STRVEC *tmp_nv = ws->tmp_nv;
	void *lq_work = ws->lq_work_null;

	struct CORE_QP_IPM_WORKSPACE *cws = ws->core_workspace;

	if(nb>0 | ng>0)
		{
		COMPUTE_GAMMA_QP(qp->d->pa, qp->m->pa, cws);
		}

	VECCP(nv, res_g, 0, lv, 0);

	if(ns>0)
		{
		COND_SLACKS_SOLVE(qp, qp_sol, ws);
		}
	else if(nb+ng>0)
		{
		AXPY(nb+ng, -1.0, gamma, nb+ng, gamma, 0, tmp_nbg+1, 0);
		}
	if(nb>0)
		{
		VECAD_SP(nb, 1.0, tmp_nbg+1, 0, idxb, lv, 0);
		}
	if(ng>0)
		{
		GEMV_N(nv, ng, 1.0, Ct, 0, 0, tmp_nbg+1, nb, 1.0, lv, 0, lv, 0);
		}

	if(ne>0)
		{

		if(arg->kkt_fact_alg==0) // null space method
			{

			TRSV_LNN(ne, A_LQ, 0, 0, res_b, 0, xy, 0);
			GEMV_T(ne, nv, 1.0, A_Q, 0, 0, xy, 0, 0.0, Yxy, 0, Yxy, 0);

			GEMV_N(nv-ne, nv, -1.0, ZtH, 0, 0, Yxy, 0, 0.0, xz, 0, xz, 0);
			GEMV_N(nv-ne, nv, -1.0, Zt, 0, 0, lv, 0, 1.0, xz, 0, xz, 0);
			TRSV_LNN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);
			TRSV_LTN(nv-ne, ZtHZ, 0, 0, xz, 0, xz, 0);

			GEMV_T(nv-ne, nv, 1.0, Zt, 0, 0, xz, 0, 1.0, Yxy, 0, dv, 0);

			SYMV_L(nv, nv, 1.0, Lv, 0, 0, dv, 0, 1.0, lv, 0, tmp_nv, 0);
			GEMV_N(ne, nv, 1.0, A_Q, 0, 0, tmp_nv, 0, 0.0, dpi, 0, dpi, 0);
			TRSV_LTN(ne, A_LQ, 0, 0, dpi, 0, dpi, 0);

			}
		else // schur-complement method
			{

			if(ws->scale)
				{

				GEMV_DIAG(nv, 1.0, sv, 0, lv, 0, 0.0, lv, 0, lv, 0);
				VECCP(nv, lv, 0, dv, 0);

				TRSV_LNN(nv, Lv, 0, 0, lv, 0, lv, 0);

				GEMV_N(ne, nv, 1.0, AL, 0, 0, lv, 0, 1.0, res_b, 0, dpi, 0);

				GEMV_DIAG(ne, 1.0, se, 0, dpi, 0, 0.0, dpi, 0, dpi, 0);
				TRSV_LNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
				TRSV_LTN(ne, Le, 0, 0, dpi, 0, dpi, 0);
				GEMV_DIAG(ne, 1.0, se, 0, dpi, 0, 0.0, dpi, 0, dpi, 0);

				GEMV_T(ne, nv, 1.0, A, 0, 0, dpi, 0, 0.0, lv, 0, lv, 0);
				GEMV_DIAG(nv, 1.0, sv, 0, lv, 0, -1.0, dv, 0, dv, 0);

				TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
				TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);
				GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);

				}
			else // no scale
				{

				VECCP(nv, lv, 0, dv, 0);

				TRSV_LNN(nv, Lv, 0, 0, lv, 0, lv, 0);

				GEMV_N(ne, nv, 1.0, AL, 0, 0, lv, 0, 1.0, res_b, 0, dpi, 0);

				TRSV_LNN(ne, Le, 0, 0, dpi, 0, dpi, 0);
				TRSV_LTN(ne, Le, 0, 0, dpi, 0, dpi, 0);

				GEMV_T(ne, nv, 1.0, A, 0, 0, dpi, 0, -1.0, dv, 0, dv, 0);

				TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
				TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);

				} // scale

			} // schur-complemen method

		}
	else // ne==0
		{

		if(ws->scale)
			{

			VECCP(nv, lv, 0, dv, 0);
			VECSC(nv, -1.0, dv, 0);

			GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);
			TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);
			GEMV_DIAG(nv, 1.0, sv, 0, dv, 0, 0.0, dv, 0, dv, 0);

			}
		else // no scale
			{

			VECCP(nv, lv, 0, dv, 0);
			VECSC(nv, -1.0, dv, 0);

			TRSV_LNN(nv, Lv, 0, 0, dv, 0, dv, 0);
			TRSV_LTN(nv, Lv, 0, 0, dv, 0, dv, 0);

			} // scale

		} // ne>0

	if(nb+ng>0)
		{
		if(nb>0)
			VECEX_SP(nb, 1.0, idxb, dv, 0, dt, 0);

		if(ng>0)
			GEMV_T(nv, ng, 1.0, Ct, 0, 0, dv, 0, 0.0, dt, nb, dt, nb);

		VECCP(nb+ng, dt, 0, dt, nb+ng);
		VECSC(nb+ng, -1.0, dt, nb+ng);

		if(ns>0)
			EXPAND_SLACKS(qp, qp_sol, ws);

		COMPUTE_LAM_T_QP(qp->d->pa, qp->m->pa, qp_sol->lam->pa, qp_sol->t->pa, cws);
		}

	return;

	}



void DENSE_QP_REMOVE_LIN_DEP_EQ(struct DENSE_QP *qp, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{
//printf("\nstart\n");

	int ii, jj, ll;
	int stop_jj;

	int nv = qp->dim->nv;
	int ne = qp->dim->ne;

	struct STRMAT *A = qp->A;
	struct STRVEC *b = qp->b;

	struct STRMAT *A_li = ws->A_li;
	struct STRVEC *b_li = ws->b_li;
	struct STRMAT *AL = ws->AL;
	struct STRMAT *At_LU = ws->At_LU;
	struct STRMAT *Ab_LU = ws->Ab_LU;
	struct STRMAT *A_LQ = ws->A_LQ;
	void *lq_work_null = ws->lq_work_null;
	int *ipiv_v = ws->ipiv_v;
	int *ipiv_e = ws->ipiv_e;
	int *ipiv_e1 = ws->ipiv_e1;

	int ne_li = 0;

	// TODO tuning, single precision
	REAL thr = 1e-14;

	REAL tmp_diag, pivot, tmp, tmp_b, tmp_max;
	int idx0_max, idx1_max, tmp_int;

	ws->status = SUCCESS;

//	printf("\nsize %d %d\n", ne, nv);

//printf("\nA\n");
//blasfeo_print_dmat(ne, nv, A, 0, 0);
	if(ne>0)
		{
		// augment A with b
		GECP(ne, nv, A, 0, 0, Ab_LU, 0, 0);
		COLIN(ne, b, 0, Ab_LU, 0, nv);
//		printf("\nAb\n");
//		blasfeo_print_dmat(ne, nv+1, Ab_LU, 0, 0);

		// row-pivot LU factorization
		GETRF_RP(ne, nv+1, Ab_LU, 0, 0, Ab_LU, 0, 0, ipiv_e);
//		printf("\nAb_LU\n");
//		blasfeo_print_dmat(ne, nv+1, Ab_LU, 0, 0);
//		int_print_mat(1, ne, ipiv_e, 1);

		// get pivot in absolute form
		for(ll=0; ll<ne; ll++)
			{
			ipiv_e1[ll] = ll;
			}
		for(ll=0; ll<ne; ll++)
			{
			tmp_int = ipiv_e1[ll];
			ipiv_e1[ll] = ipiv_e1[ipiv_e[ll]];
			ipiv_e1[ipiv_e[ll]] = tmp_int;
			}
//		printf("\nipiv_e1\n");
//		int_print_mat(1, ne, ipiv_e1, 1);

		for(ii=0; ii<ne; ii++)
			{
//			printf("\nii %d\n", ii);
			pivot = BLASFEO_DMATEL(Ab_LU, ii, ii);
//			printf("\npivot %e\n", pivot);
			if(fabs(pivot)<=thr)
				{
				jj = ii+1;
				stop_jj = 0;
				while(stop_jj==0 & jj<nv)
					{
					tmp_max = thr;
					idx0_max = -1;
					idx1_max = -1;
					for(ll=ii; ll<ne & ll<=jj; ll++)
						{
//						printf("\nii jj ll %d %d %d\n", ii, jj, ll);
						tmp = fabs(BLASFEO_DMATEL(Ab_LU, ll, jj));
						if(tmp>tmp_max)
							{
							tmp_max = tmp;
							idx0_max = ll;
							idx1_max = jj;
							stop_jj = 1;
							}
						}
					jj++;
					}
//				printf("\nstop %d\n", stop_jj);
				if(stop_jj==1)
					{
					// swap rows
//					printf("\n%d %e %d %d\n", stop_jj, tmp_max, idx0_max, idx1_max);
					if(tmp_max>thr & idx0_max!=ii)
						{
//						printf("\nswap!\n");
						ROWSW(nv+1-idx1_max, Ab_LU, ii, idx1_max, Ab_LU, idx0_max, idx1_max);
						tmp_int = ipiv_e1[ii];
						ipiv_e1[ii] = ipiv_e1[idx0_max];
						ipiv_e1[idx0_max] = tmp_int;
//						printf("\nipiv_e1 new\n");
//						int_print_mat(1, ne, ipiv_e1, 1);
						}
					// copy li eq
//					printf("\nli %d %d %d\n", ii, ipiv_e1[ii], ne_li);
					GECP(1, nv, A, ipiv_e1[ii], 0, A_li, ne_li, 0);
					VECCP(1, b, ipiv_e1[ii], b_li, ne_li);
					ne_li++;
					// pivot
					pivot = BLASFEO_DMATEL(Ab_LU, ii, idx1_max);
//					printf("\npivot to clear %f\n", pivot);
					// clear below TODO implement using level 2 BLAS !!!
					for(ll=ii+1; ll<=idx1_max; ll++)
						{
						tmp = fabs(BLASFEO_DMATEL(Ab_LU, ll, idx1_max));
						if(tmp!=0.0)
							{
							tmp = -tmp/pivot;
							GEAD(1, nv+1-idx1_max, tmp, Ab_LU, ii, idx1_max, Ab_LU, ll, idx1_max);
							}
						}
					}
				else
					{
					// all remaining matrix is zero: check b and return
					for(ll=ii; ll<ne; ll++)
						{
						tmp = fabs(BLASFEO_DMATEL(Ab_LU, ll, nv));
						if(tmp>thr)
							{
							ws->status = INCONS_EQ;
//							printf("\nproblem unfeasible!\n");
							return;
							}
						}
					//return; TODO uncomment this !!!
					}
				}
			else
				{
				// copy li eq
//				printf("\nli pivot\n");
				GECP(1, nv, A, ipiv_e1[ii], 0, A_li, ne_li, 0);
				VECCP(1, b, ipiv_e1[ii], b_li, ne_li);
				ne_li++;
				}
//			blasfeo_print_dmat(ne, nv+1, Ab_LU, 0, 0);
//			exit(1);
			}

//printf("\nA_li\n");
//blasfeo_print_dmat(ne_li, nv, A_li, 0, 0);
//printf("\nb_li\n");
//blasfeo_print_tran_dvec(ne_li, b_li, 0);

//		exit(1);

#if 0
		if(1)
			{
			GETR(ne, nv, A, 0, 0, At_LU, 0, 0);
			GETRF_RP(nv, ne, At_LU, 0, 0, At_LU, 0, 0, ipiv_v);
			GETR(nv, ne, At_LU, 0, 0, A_LQ, 0, 0);
			}
		else //if(arg->kkt_fact_alg==0) // null space method
			{
			GELQF(ne, nv, A, 0, 0, A_LQ, 0, 0, lq_work_null);
			}
//printf("\nA_LQ\n");
//blasfeo_print_dmat(ne, nv, A_LQ, 0, 0);
		GESE(ne, nv, 0.0, AL, 0, 0);
		for(ii=0; ii<ne; ii++)
			{
			tmp_diag = BLASFEO_DMATEL(A_LQ, ii, ii);
			if(fabs(tmp_diag)<thr)
				{
//				printf("%e zero!\n", tmp_diag);
				GECP(1, ii, A_LQ, ii, 0, AL, ii, 0);
				tmp_b = BLASFEO_DVECEL(b, ii);
				for(jj=ii-1; jj>=0; jj--)
					{
					pivot = BLASFEO_DMATEL(A_LQ, jj, jj);
					if(fabs(pivot)>=thr)
						{
						tmp = BLASFEO_DMATEL(AL, ii, jj)/pivot; 
						BLASFEO_DMATEL(AL, ii, jj) = tmp;
//							printf("\ntmp %e %d\n", tmp, jj);
//							blasfeo_print_dmat(1, jj, A_LQ, jj, 0);
//							blasfeo_print_dmat(1, jj, AL, ii, 0);
						GEAD(1, jj, -tmp, A_LQ, jj, 0, AL, ii, 0);
//							blasfeo_print_dmat(1, jj, AL, ii, 0);
//							if(ii==2) exit(1);
						tmp_b -= tmp * BLASFEO_DVECEL(b, jj);
						}
					}
//					printf("\ntmp_b %e\n", tmp_b);
				if(fabs(tmp_b)>=thr)
					{
					printf("\nA = \n");
					blasfeo_print_dmat(ne, nv, A, 0, 0);
					printf("\nb = \n");
					blasfeo_print_tran_dvec(ne, b, 0);
					exit(1);
					printf("\ntmp_b %e\n", tmp_b);
					printf("\nA_LQ = \n");
					blasfeo_print_dmat(1, ii, A_LQ, ii, 0);
					printf("\nA = \n");
					blasfeo_print_dmat(1, nv, A, ii, 0);
					printf("\nb = \n");
					blasfeo_print_tran_dvec(1, b, ii);
					ws->status = INCONS_EQ;
//						printf("\nproblem unfeasible!\n");
					}
				}
			else
				{
//				printf("%e\n", tmp_diag);
				BLASFEO_DMATEL(AL, ii, ii) = 1.0;
				GECP(1, nv, A, ii, 0, A_li, ne_li, 0);
				VECCP(1, b, ii, b_li, ne_li);
				ne_li++;
				}
			}
//printf("\nAL\n");
//blasfeo_print_dmat(ne, nv, AL, 0, 0);

#endif
		if(ne_li<ne)
			{
//			printf("\nne %d, ne_li %d\n", ne, ne_li);
			ws->ne_bkp = qp->dim->ne;
			qp->dim->ne = ne_li;
			ws->A_bkp = qp->A;
			qp->A = A_li;
			ws->b_bkp = qp->b;
			qp->b = b_li;
			}
	
		}

//printf("\nA_li\n");
//blasfeo_print_dmat(ne_li, nv, A_li, 0, 0);
//printf("\nb_li\n");
//blasfeo_print_tran_dvec(ne_li, b_li, 0);

//printf("\nne %d ne_li %d\n", ne, ne_li);

//printf("\nend\n");

	return;

	}
					


void DENSE_QP_RESTORE_LIN_DEP_EQ(struct DENSE_QP *qp, struct DENSE_QP_IPM_ARG *arg, struct DENSE_QP_IPM_WS *ws)
	{

	int ii, jj;

	int nv = qp->dim->nv;
	int ne = qp->dim->ne;

	struct STRMAT *A = qp->A;
	struct STRVEC *b = qp->b;

	struct STRMAT *A_li = ws->A_li;
	struct STRVEC *b_li = ws->b_li;
	struct STRMAT *AL = ws->AL;
	struct STRMAT *At_LU = ws->At_LU;
	struct STRMAT *A_LQ = ws->A_LQ;
	void *lq_work_null = ws->lq_work_null;
	int *ipiv_v = ws->ipiv_v;

	if(ne>0)
		{
		if(ne<ws->ne_bkp)
			{
//			printf("\nne %d, ne_li %d\n", ne, ne_li);
			qp->dim->ne = ws->ne_bkp;
			qp->A = ws->A_bkp;
			qp->b = ws->b_bkp;
			}
		}

//printf("\nA\n");
//blasfeo_print_dmat(ne, nv, A, 0, 0);
//printf("\nb\n");
//blasfeo_print_tran_dvec(ne, b, 0);

	return;

	}
					



