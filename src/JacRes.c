/*@ ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 **
 **    Copyright (c) 2011-2015, JGU Mainz, Anton Popov, Boris Kaus
 **    All rights reserved.
 **
 **    This software was developed at:
 **
 **         Institute of Geosciences
 **         Johannes-Gutenberg University, Mainz
 **         Johann-Joachim-Becherweg 21
 **         55128 Mainz, Germany
 **
 **    project:    LaMEM
 **    filename:   JacRes.c
 **
 **    LaMEM is free software: you can redistribute it and/or modify
 **    it under the terms of the GNU General Public License as published
 **    by the Free Software Foundation, version 3 of the License.
 **
 **    LaMEM is distributed in the hope that it will be useful,
 **    but WITHOUT ANY WARRANTY; without even the implied warranty of
 **    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 **    See the GNU General Public License for more details.
 **
 **    You should have received a copy of the GNU General Public License
 **    along with LaMEM. If not, see <http://www.gnu.org/licenses/>.
 **
 **
 **    Contact:
 **        Boris Kaus       [kaus@uni-mainz.de]
 **        Anton Popov      [popov@uni-mainz.de]
 **
 **
 **    Main development team:
 **         Anton Popov      [popov@uni-mainz.de]
 **         Boris Kaus       [kaus@uni-mainz.de]
 **         Tobias Baumann
 **         Adina Pusok
 **         Arthur Bauville
 **
 ** ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ @*/
//---------------------------------------------------------------------------
//...................   FDSTAG JACOBIAN AND RESIDUAL  .......................
//---------------------------------------------------------------------------
#include "LaMEM.h"
#include "JacRes.h"
#include "parsing.h"
#include "tssolve.h"
#include "scaling.h"
#include "fdstag.h"
#include "surf.h"
#include "bc.h"
#include "phase.h"
#include "constEq.h"
#include "tools.h"

//---------------------------------------------------------------------------
//...................   Runtime parameters and controls .....................
//---------------------------------------------------------------------------
PetscErrorCode ControlsRead(Controls *ctrl, FB *fb)
{
	Scaling     *scal;
	PetscScalar input_eta_max;
	char        gwtype [_STR_LEN_];

	PetscErrorCode ierr;
	PetscFunctionBegin;

	// access context
	scal = ctrl->scal;

	// set defaults
	ctrl->tauUlt       = DBL_MAX;
	ctrl->gwLevel      = DBL_MAX;
	ctrl->FSSA         = 1.0;
	ctrl->shearHeatEff = 1.0;
	ctrl->pShiftAct    = 1;
	ctrl->pLithoVisc   = 1;
	ctrl->initGuess    = 1;

	if(scal->utype != _NONE_)
	{
		ctrl->Rugc      = 8.3144621;
		ctrl->rho_fluid = 1040.0;
	}

	// read from options
	ierr = getScalarParam(fb, _OPTIONAL_, "gravity",         ctrl->grav,         3, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "FSSA",           &ctrl->FSSA,         1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "shear_heat_eff", &ctrl->shearHeatEff, 1, 1.0); CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "act_temp_diff",  &ctrl->pShiftAct,    1, 1);   CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "act_p_shift",    &ctrl->pShiftAct,    1, 1);   CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "init_guess",     &ctrl->initGuess,    1, 1);   CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "p_litho_visc",   &ctrl->pLithoVisc,   1, 1);   CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "p_litho_plast",  &ctrl->pLithoPlast,  1, 1);   CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "p_lim_plast",    &ctrl->pLimPlast,    1, 1);   CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "jac_mat_free",   &ctrl->jac_mat_free, 1, 1);   CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "eta_min",        &ctrl->eta_min,      1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "eta_max",        &input_eta_max,      1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "eta_ref",        &ctrl->eta_ref,      1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "T_ref",          &ctrl->TRef,         1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "RUGC",           &ctrl->Rugc,         1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "DII_ref",        &ctrl->DII_ref,      1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "min_cohes",      &ctrl->minCh,        1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "min_fric",       &ctrl->minFr,        1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "tau_ult",        &ctrl->tauUlt,       1, 1.0); CHKERRQ(ierr);
	ierr = getIntParam   (fb, _OPTIONAL_, "quasi_harm_avg", &ctrl->quasiHarmAvg, 1, 1);   CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "cf_eta_min",     &ctrl->cf_eta_min,   1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "n_pw",           &ctrl->n_pw,         1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "rho_fluid",      &ctrl->rho_fluid,    1, 1.0); CHKERRQ(ierr);
	ierr = getStringParam(fb, _OPTIONAL_, "gw_level_type",  gwtype,              "none"); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "gw_level",      &ctrl->gwLevel,       1, 1.0); CHKERRQ(ierr);
	ierr = getScalarParam(fb, _OPTIONAL_, "biot",          &ctrl->biot,          1, 1.0); CHKERRQ(ierr);

	if     (!strcmp(gwtype, "none"))  ctrl->gwType = _GW_NONE_;
	else if(!strcmp(gwtype, "top"))   ctrl->gwType = _GW_TOP_;
	else if(!strcmp(gwtype, "surf"))  ctrl->gwType = _GW_SURF_;
	else if(!strcmp(gwtype, "level")) ctrl->gwType = _GW_LEVEL_;
	else SETERRQ1(PETSC_COMM_WORLD, PETSC_ERR_USER, "Incorrect ground water level type: %s", gwtype);

	if(ctrl->gwType == _GW_LEVEL_ && ctrl->gwLevel == DBL_MAX)
	{
		SETERRQ(PETSC_COMM_WORLD, PETSC_ERR_USER, "Ground water level must be specified (gw_level_type, gw_level)");
	}

	// set default Biot pressure parameter
	if(ctrl->gwType != _GW_NONE_ && !ctrl->biot)
	{
		ctrl->biot = 1.0;
	}

	// scale parameters
	// NOTE: scale gas constant with characteristic temperature
	ctrl->eta_min     /=  scal->viscosity;
	input_eta_max     /=  scal->viscosity;
	ctrl->eta_ref     /=  scal->viscosity;
	ctrl->TRef         = (ctrl->TRef + scal->Tshift)/scal->temperature;
	ctrl->Rugc        *= scal->temperature;
	ctrl->DII_ref     /= scal->strain_rate;
	ctrl->minCh       /= scal->stress_si;
	ctrl->minFr       /= scal->angle;
	ctrl->tauUlt      /= scal->stress_si;
	ctrl->rho_fluid   /= scal->density;
	ctrl->gwLevel     /= scal->length;

	// set inverse of maximum viscosity
	if(input_eta_max) ctrl->inv_eta_max = 1.0/input_eta_max;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResCreateData"
PetscErrorCode JacResCreateData(JacRes *jr)
{
	FDSTAG         *fs;
	DOFIndex       *dof;
	PetscScalar    *svBuff;
	const PetscInt *lx, *ly;
	PetscInt        i, n, svBuffSz, numPhases;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs        =  jr->fs;
	dof       = &fs->dof;
	numPhases =  jr->dbm->numPhases;

	//========================
	// create solution vectors
	//========================

	// coupled solution vectors
	ierr = VecCreateMPI(PETSC_COMM_WORLD, dof->ln, PETSC_DETERMINE, &jr->gsol); CHKERRQ(ierr);
	ierr = VecCreateMPI(PETSC_COMM_WORLD, dof->ln, PETSC_DETERMINE, &jr->gres); CHKERRQ(ierr);

	// velocity components
	ierr = DMCreateGlobalVector(fs->DA_X, &jr->gvx); CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(fs->DA_Y, &jr->gvy); CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(fs->DA_Z, &jr->gvz); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_X, &jr->lvx); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_Y, &jr->lvy); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_Z, &jr->lvz); CHKERRQ(ierr);

	// momentum residual components
	ierr = DMCreateGlobalVector(fs->DA_X, &jr->gfx); CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(fs->DA_Y, &jr->gfy); CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(fs->DA_Z, &jr->gfz); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_X, &jr->lfx); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_Y, &jr->lfy); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_Z, &jr->lfz); CHKERRQ(ierr);

	// strain-rate components (also used as buffer vectors)
	ierr = DMCreateLocalVector (fs->DA_CEN, &jr->ldxx); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_CEN, &jr->ldyy); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_CEN, &jr->ldzz); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_XY,  &jr->ldxy); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_XZ,  &jr->ldxz); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_YZ,  &jr->ldyz); CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(fs->DA_XY,  &jr->gdxy); CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(fs->DA_XZ,  &jr->gdxz); CHKERRQ(ierr);
	ierr = DMCreateGlobalVector(fs->DA_YZ,  &jr->gdyz); CHKERRQ(ierr);

	// pressure
	ierr = DMCreateGlobalVector(fs->DA_CEN, &jr->gp);        CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_CEN, &jr->lp);        CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_CEN, &jr->lp_lithos); CHKERRQ(ierr);
	ierr = DMCreateLocalVector (fs->DA_CEN, &jr->lp_pore);   CHKERRQ(ierr);


	// continuity residual
	ierr = DMCreateGlobalVector(fs->DA_CEN, &jr->gc);  CHKERRQ(ierr);

	// corner buffer
	ierr = DMCreateLocalVector(fs->DA_COR,  &jr->lbcor); CHKERRQ(ierr);

	//======================================
	// allocate space for solution variables
	//======================================

	ierr = PetscMalloc(sizeof(SolVarCell)*(size_t)fs->nCells, &jr->svCell);   CHKERRQ(ierr);
	ierr = PetscMalloc(sizeof(SolVarEdge)*(size_t)fs->nXYEdg, &jr->svXYEdge); CHKERRQ(ierr);
	ierr = PetscMalloc(sizeof(SolVarEdge)*(size_t)fs->nXZEdg, &jr->svXZEdge); CHKERRQ(ierr);
	ierr = PetscMalloc(sizeof(SolVarEdge)*(size_t)fs->nYZEdg, &jr->svYZEdge); CHKERRQ(ierr);

	ierr = PetscMemzero(jr->svCell,   sizeof(SolVarCell)*(size_t)fs->nCells); CHKERRQ(ierr);
	ierr = PetscMemzero(jr->svXYEdge, sizeof(SolVarEdge)*(size_t)fs->nXYEdg); CHKERRQ(ierr);
	ierr = PetscMemzero(jr->svXZEdge, sizeof(SolVarEdge)*(size_t)fs->nXZEdg); CHKERRQ(ierr);
	ierr = PetscMemzero(jr->svYZEdge, sizeof(SolVarEdge)*(size_t)fs->nYZEdg); CHKERRQ(ierr);

	// compute total size per processor of the solution variables storage buffer
	svBuffSz = numPhases*(fs->nCells + fs->nXYEdg + fs->nXZEdg + fs->nYZEdg);

	// allocate buffer for solution variables (phRat)
	ierr = makeScalArray(&jr->svBuff, NULL, svBuffSz);

	// setup pointers
	svBuff = jr->svBuff;

	n = fs->nCells;
	for(i = 0; i < n; i++) { jr->svCell[i].phRat   = svBuff; svBuff += numPhases; }

	n = fs->nXYEdg;
	for(i = 0; i < n; i++) { jr->svXYEdge[i].phRat = svBuff; svBuff += numPhases; }

	n = fs->nXZEdg;
	for(i = 0; i < n; i++) { jr->svXZEdge[i].phRat = svBuff; svBuff += numPhases; }

	n = fs->nYZEdg;
	for(i = 0; i < n; i++) { jr->svYZEdge[i].phRat = svBuff; svBuff += numPhases; }

	// setup temperature parameters
	ierr = JacResCreateTempParam(jr); CHKERRQ(ierr);

	//==========================
	// 2D integration primitives
	//==========================

	// get grid partitioning in X & Y directions
	ierr = DMDAGetOwnershipRanges(fs->DA_CEN, &lx, &ly, NULL); CHKERRQ(ierr);

	// create 2D cell center grid
	ierr = DMDACreate3dSetUp(PETSC_COMM_WORLD,
		DM_BOUNDARY_NONE, DM_BOUNDARY_NONE, DM_BOUNDARY_NONE,
		DMDA_STENCIL_BOX,
		fs->dsx.tcels, fs->dsy.tcels, fs->dsz.nproc,
		fs->dsx.nproc, fs->dsy.nproc, fs->dsz.nproc,
		1, 1, lx, ly, NULL, &jr->DA_CELL_2D); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResReadRestart"
PetscErrorCode JacResReadRestart(JacRes *jr, FILE *fp)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = JacResCreateData(jr); CHKERRQ(ierr);

	// read solution vectors
	ierr = VecReadRestart(jr->gsol, fp); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResWriteRestart"
PetscErrorCode JacResWriteRestart(JacRes *jr, FILE *fp)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	// write solution vectors
	ierr = VecWriteRestart(jr->gsol, fp); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResDestroy"
PetscErrorCode JacResDestroy(JacRes *jr)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	// solution vectors
	ierr = VecDestroy(&jr->gsol);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->gres);      CHKERRQ(ierr);

	ierr = VecDestroy(&jr->gvx);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->gvy);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->gvz);       CHKERRQ(ierr);

	ierr = VecDestroy(&jr->lvx);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->lvy);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->lvz);       CHKERRQ(ierr);

	ierr = VecDestroy(&jr->gfx);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->gfy);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->gfz);       CHKERRQ(ierr);

	ierr = VecDestroy(&jr->lfx);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->lfy);       CHKERRQ(ierr);
	ierr = VecDestroy(&jr->lfz);       CHKERRQ(ierr);

	ierr = VecDestroy(&jr->ldxx);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->ldyy);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->ldzz);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->ldxy);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->ldxz);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->ldyz);      CHKERRQ(ierr);

	ierr = VecDestroy(&jr->gdxy);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->gdxz);      CHKERRQ(ierr);
	ierr = VecDestroy(&jr->gdyz);      CHKERRQ(ierr);

	ierr = VecDestroy(&jr->gp);        CHKERRQ(ierr);
	ierr = VecDestroy(&jr->lp);        CHKERRQ(ierr);
	ierr = VecDestroy(&jr->lp_lithos); CHKERRQ(ierr);
	ierr = VecDestroy(&jr->lp_pore);   CHKERRQ(ierr);

	ierr = VecDestroy(&jr->gc);        CHKERRQ(ierr);

	ierr = VecDestroy(&jr->lbcor);     CHKERRQ(ierr);

	// solution variables
	ierr = PetscFree(jr->svCell);      CHKERRQ(ierr);
	ierr = PetscFree(jr->svXYEdge);    CHKERRQ(ierr);
	ierr = PetscFree(jr->svXZEdge);    CHKERRQ(ierr);
	ierr = PetscFree(jr->svYZEdge);    CHKERRQ(ierr);
	ierr = PetscFree(jr->svBuff);      CHKERRQ(ierr);

	// temperature parameters
	ierr = JacResDestroyTempParam(jr); CHKERRQ(ierr);

	// 2D integration primitives
	ierr = DMDestroy(&jr->DA_CELL_2D); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResGetI2Gdt"
PetscErrorCode JacResGetI2Gdt(JacRes *jr)
{
	// compute average inverse elastic viscosity in the integration points
	// WARNING! this should be replaced by the effective elastic strain rates

	FDSTAG     *fs;
	SolVarCell *svCell;
	SolVarEdge *svEdge;
	Material_t *phases;
	PetscScalar dt;
	PetscInt    i, n, numPhases;

	PetscFunctionBegin;

	fs        = jr->fs;
	dt        = jr->ts->dt;
	numPhases = jr->dbm->numPhases;
	phases    = jr->dbm->phases;

	//=============
	// cell centers
	//=============
	n = fs->nCells;
	for(i = 0; i < n; i++)
	{	// access solution variables
		svCell = &jr->svCell[i];
		// compute & store inverse viscosity
		svCell->svDev.I2Gdt = GetI2Gdt(numPhases, phases, svCell->phRat, dt);
	}
	//===========
	// xy - edges
	//===========
	n = fs->nXYEdg;
	for(i = 0; i < n; i++)
	{	// access solution variables
		svEdge = &jr->svXYEdge[i];
		// compute & store inverse viscosity
		svEdge->svDev.I2Gdt = GetI2Gdt(numPhases, phases, svEdge->phRat, dt);
	}
	//===========
	// xz - edges
	//===========
	n = fs->nXZEdg;
	for(i = 0; i < n; i++)
	{	// access solution variables
		svEdge = &jr->svXZEdge[i];
		// compute & store inverse viscosity
		svEdge->svDev.I2Gdt = GetI2Gdt(numPhases, phases, svEdge->phRat, dt);
	}
	//===========
	// yz - edges
	//===========
	n = fs->nYZEdg;
	for(i = 0; i < n; i++)
	{	// access solution variables
		svEdge = &jr->svYZEdge[i];
		// compute & store inverse viscosity
		svEdge->svDev.I2Gdt = GetI2Gdt(numPhases, phases, svEdge->phRat, dt);
	}
	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResGetPressShift"
PetscErrorCode JacResGetPressShift(JacRes *jr)
{
	// get average pressure near the top surface

	FDSTAG      *fs;
	PetscScalar ***p;
	PetscScalar lpShift, gpShift;
	PetscInt    i, j, k, nx, ny, nz, sx, sy, sz, mcz;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	// check if requested
	if(!jr->ctrl->pShiftAct) PetscFunctionReturn(0);

	fs      = jr->fs;
	mcz     = fs->dsz.tcels - 1;
	lpShift = 0.0;

	ierr = DMDAVecGetArray(fs->DA_CEN, jr->gp, &p);  CHKERRQ(ierr);

	ierr = DMDAGetCorners(fs->DA_CEN, &sx, &sy, &sz, &nx, &ny, &nz); CHKERRQ(ierr);

	START_STD_LOOP
	{
		if(k == mcz) lpShift += p[k][j][i];
	}
	END_STD_LOOP

	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->gp, &p);  CHKERRQ(ierr);

	// synchronize
	if(ISParallel(PETSC_COMM_WORLD))
	{
		ierr = MPI_Allreduce(&lpShift, &gpShift, 1, MPIU_SCALAR, MPI_SUM, PETSC_COMM_WORLD); CHKERRQ(ierr);
	}
	else
	{
		gpShift = lpShift;
	}

	// store pressure shift
	jr->ctrl->pShift = gpShift/(PetscScalar)(fs->dsx.tcels*fs->dsy.tcels);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResGetEffStrainRate"
PetscErrorCode JacResGetEffStrainRate(JacRes *jr)
{

	FDSTAG     *fs;
	SolVarCell *svCell;
	SolVarEdge *svEdge;
	SolVarDev  *svDev;
	SolVarBulk *svBulk;

	PetscInt    i, j, k, nx, ny, nz, sx, sy, sz, iter;
	PetscScalar dvxdy, dvydx, dvxdz, dvzdx, dvydz, dvzdy;
	PetscScalar dx, dy, dz, xx, yy, zz, xy, xz, yz, theta, tr;
	PetscScalar ***vx,  ***vy,  ***vz;
	PetscScalar ***dxx, ***dyy, ***dzz, ***dxy, ***dxz, ***dyz;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs = jr->fs;

	// access local (ghosted) velocity components
	ierr = DMDAVecGetArray(fs->DA_X,   jr->lvx,  &vx);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y,   jr->lvy,  &vy);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Z,   jr->lvz,  &vz);  CHKERRQ(ierr);

	// access global strain-rate components
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->ldxx, &dxx); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->ldyy, &dyy); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->ldzz, &dzz); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_XY,  jr->ldxy, &dxy); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_XZ,  jr->ldxz, &dxz); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_YZ,  jr->ldyz, &dyz); CHKERRQ(ierr);

	//-------------------------------
	// central points (dxx, dyy, dzz)
	//-------------------------------
	iter = 0;
	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svCell = &jr->svCell[iter++];
		svDev  = &svCell->svDev;
		svBulk = &svCell->svBulk;

		// get mesh steps
		dx = SIZE_CELL(i, sx, fs->dsx);
		dy = SIZE_CELL(j, sy, fs->dsy);
		dz = SIZE_CELL(k, sz, fs->dsz);

		// compute velocity gradients
		xx = (vx[k][j][i+1] - vx[k][j][i])/dx;
		yy = (vy[k][j+1][i] - vy[k][j][i])/dy;
		zz = (vz[k+1][j][i] - vz[k][j][i])/dz;

		// compute & store volumetric strain rate
		theta = xx + yy + zz;
		svBulk->theta = theta;

		// compute & store total deviatoric strain rates

		tr  = theta/3.0;
		xx -= tr;
		yy -= tr;
		zz -= tr;

		svCell->dxx = xx;
		svCell->dyy = yy;
		svCell->dzz = zz;

		// compute & store effective deviatoric strain rates
		dxx[k][j][i] = xx + svCell->hxx*svDev->I2Gdt;
		dyy[k][j][i] = yy + svCell->hyy*svDev->I2Gdt;
		dzz[k][j][i] = zz + svCell->hzz*svDev->I2Gdt;

	}
	END_STD_LOOP

	//-------------------------------
	// xy edge points (dxy)
	//-------------------------------
	iter = 0;
	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svEdge = &jr->svXYEdge[iter++];
		svDev  = &svEdge->svDev;

		// get mesh steps
		dx = SIZE_NODE(i, sx, fs->dsx);
		dy = SIZE_NODE(j, sy, fs->dsy);

		// compute velocity gradients
		dvxdy = (vx[k][j][i] - vx[k][j-1][i])/dy;
		dvydx = (vy[k][j][i] - vy[k][j][i-1])/dx;

		// compute & store total strain rate
		xy = 0.5*(dvxdy + dvydx);
		svEdge->d = xy;

		// compute & store effective deviatoric strain rate
		dxy[k][j][i] = xy + svEdge->h*svDev->I2Gdt;

	}
	END_STD_LOOP

	//-------------------------------
	// xz edge points (dxz)
	//-------------------------------
	iter = 0;
	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_NODE_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svEdge = &jr->svXZEdge[iter++];
		svDev  = &svEdge->svDev;

		// get mesh steps
		dx = SIZE_NODE(i, sx, fs->dsx);
		dz = SIZE_NODE(k, sz, fs->dsz);

		// compute velocity gradients
		dvxdz = (vx[k][j][i] - vx[k-1][j][i])/dz;
		dvzdx = (vz[k][j][i] - vz[k][j][i-1])/dx;

		// compute & store total strain rate
        xz = 0.5*(dvxdz + dvzdx);
        svEdge->d = xz;

		// compute & store effective deviatoric strain rate
		dxz[k][j][i] = xz + svEdge->h*svDev->I2Gdt;

	}
	END_STD_LOOP

	//-------------------------------
	// yz edge points (dyz)
	//-------------------------------
	iter = 0;
	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_NODE_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svEdge = &jr->svYZEdge[iter++];
		svDev  = &svEdge->svDev;

		// get mesh steps
		dy = SIZE_NODE(j, sy, fs->dsy);
		dz = SIZE_NODE(k, sz, fs->dsz);

		// compute velocity gradients
		dvydz = (vy[k][j][i] - vy[k-1][j][i])/dz;
		dvzdy = (vz[k][j][i] - vz[k][j-1][i])/dy;

		// compute & store total strain rate
		yz = 0.5*(dvydz + dvzdy);
		svEdge->d = yz;

		// compute & store effective deviatoric strain rate
		dyz[k][j][i] = yz + svEdge->h*svDev->I2Gdt;

	}
	END_STD_LOOP

	// restore vectors
	ierr = DMDAVecRestoreArray(fs->DA_X,   jr->lvx,  &vx);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y,   jr->lvy,  &vy);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Z,   jr->lvz,  &vz);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->ldxx, &dxx); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->ldyy, &dyy); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->ldzz, &dzz); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_XY,  jr->ldxy, &dxy); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_XZ,  jr->ldxz, &dxz); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_YZ,  jr->ldyz, &dyz); CHKERRQ(ierr);

	// communicate boundary strain-rate values
	LOCAL_TO_LOCAL(fs->DA_CEN, jr->ldxx);
	LOCAL_TO_LOCAL(fs->DA_CEN, jr->ldyy);
	LOCAL_TO_LOCAL(fs->DA_CEN, jr->ldzz);
	LOCAL_TO_LOCAL(fs->DA_XY,  jr->ldxy);
	LOCAL_TO_LOCAL(fs->DA_XZ,  jr->ldxz);
	LOCAL_TO_LOCAL(fs->DA_YZ,  jr->ldyz);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResGetVorticity"
PetscErrorCode JacResGetVorticity(JacRes *jr)
{
	// Compute components of the vorticity pseudo-vector
	// (instantaneous rotation rates around three coordinate axis).
	// Take care of rotation direction and sign convention.
	// Throughout LaMEM, right-handed coordinate system is assumed!

	FDSTAG     *fs;
	PetscInt    i, j, k, nx, ny, nz, sx, sy, sz;
	PetscScalar dvxdy, dvydx, dvxdz, dvzdx, dvydz, dvzdy;
	PetscScalar ***lvx, ***lvy, ***lvz;
	PetscScalar ***gwx, ***gwy, ***gwz;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs = jr->fs;

	// access vectors
	ierr = DMDAVecGetArray(fs->DA_X,  jr->lvx,  &lvx);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y,  jr->lvy,  &lvy);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Z,  jr->lvz,  &lvz);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_XY, jr->ldxy, &gwz);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_XZ, jr->ldxz, &gwy);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_YZ, jr->ldyz, &gwx);  CHKERRQ(ierr);

	//-------------------------------
	// xy edge points (wz)
	//-------------------------------

	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		dvxdy = (lvx[k][j][i] - lvx[k][j-1][i])/SIZE_NODE(j, sy, fs->dsy);
		dvydx = (lvy[k][j][i] - lvy[k][j][i-1])/SIZE_NODE(i, sx, fs->dsx);

		// positive (counter-clockwise) rotation around Z axis X -> Y
		gwz[k][j][i] = dvydx - dvxdy;
	}
	END_STD_LOOP

	//-------------------------------
	// xz edge points (wy)
	//-------------------------------

	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_NODE_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		dvxdz = (lvx[k][j][i] - lvx[k-1][j][i])/SIZE_NODE(k, sz, fs->dsz);
		dvzdx = (lvz[k][j][i] - lvz[k][j][i-1])/SIZE_NODE(i, sx, fs->dsx);

		// positive (counter-clockwise) rotation around Y axis Z -> X
		gwy[k][j][i] = dvxdz - dvzdx;
	}
	END_STD_LOOP

	//-------------------------------
	// yz edge points (wx)
	//-------------------------------

	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_NODE_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		dvydz = (lvy[k][j][i] - lvy[k-1][j][i])/SIZE_NODE(k, sz, fs->dsz);
		dvzdy = (lvz[k][j][i] - lvz[k][j-1][i])/SIZE_NODE(j, sy, fs->dsy);

		// positive (counter-clockwise) rotation around X axis Y -> Z
		gwx[k][j][i] = dvzdy - dvydz;
	}
	END_STD_LOOP

	// restore velocity & strain rate component vectors
	ierr = DMDAVecRestoreArray(fs->DA_X,  jr->lvx,  &lvx);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y,  jr->lvy,  &lvy);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Z,  jr->lvz,  &lvz);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_XY, jr->ldxy, &gwz);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_XZ, jr->ldxz, &gwy);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_YZ, jr->ldyz, &gwx);  CHKERRQ(ierr);

	// communicate boundary values
	LOCAL_TO_LOCAL(fs->DA_XY, jr->ldxy);
	LOCAL_TO_LOCAL(fs->DA_XZ, jr->ldxz);
	LOCAL_TO_LOCAL(fs->DA_YZ, jr->ldyz);

	PetscFunctionReturn(0);
}
//-----------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResGetResidual"
PetscErrorCode JacResGetResidual(JacRes *jr)
{
	// Compute residual of nonlinear momentum and mass conservation
	// equations, based on pre-computed components of effective
	// strain-rate tensor, current values of pressure and temperature.
	// Missing components of the second invariant of the effective strain-rate
	// tensor (squares of the corresponding strain rate components) are averaged
	// form the hosting nodes using arithmetic mean.
	// DII = (0.5*D_ij*D_ij)^0.5
	// NOTE: we interpolate and average D_ij*D_ij terms instead of D_ij

	FDSTAG     *fs;
	SolVarCell *svCell;
	SolVarEdge *svEdge;
	SolVarDev  *svDev;
	SolVarBulk *svBulk;
	Material_t *phases;
	Controls   *ctrl;
	PetscInt    iter, numPhases, AirPhase;
	PetscInt    I1, I2, J1, J2, K1, K2;
	PetscInt    i, j, k, nx, ny, nz, sx, sy, sz, mx, my, mz;
	PetscScalar XX, XX1, XX2, XX3, XX4;
	PetscScalar YY, YY1, YY2, YY3, YY4;
	PetscScalar ZZ, ZZ1, ZZ2, ZZ3, ZZ4;
	PetscScalar XY, XY1, XY2, XY3, XY4;
	PetscScalar XZ, XZ1, XZ2, XZ3, XZ4;
	PetscScalar YZ, YZ1, YZ2, YZ3, YZ4;
	PetscScalar bdx, fdx, bdy, fdy, bdz, fdz;
	PetscScalar gx, gy, gz, tx, ty, tz, sxx, syy, szz, sxy, sxz, syz;
	PetscScalar J2Inv, theta, rho, IKdt, Tc, pc, pShift, pn, dt, fssa, *grav;
	PetscScalar ***fx,  ***fy,  ***fz, ***vx,  ***vy,  ***vz, ***gc;
	PetscScalar ***dxx, ***dyy, ***dzz, ***dxy, ***dxz, ***dyz, ***p, ***T, ***p_lithos, ***p_pore;
	PetscScalar eta_creep, eta_vp;
	PetscScalar depth, pc_lithos, pc_pore, biot, ptotal, avg_topo;
//	PetscScalar alpha, Tn,

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs = jr->fs;

//	PetscInt mcz = fs->dsz.tcels - 1;

	// initialize maximum node index in all directions
	mx = fs->dsx.tnods - 1;
	my = fs->dsy.tnods - 1;
	mz = fs->dsz.tnods - 1;

	// access residual context variables
	numPhases = jr->dbm->numPhases; // number phases
	phases    = jr->dbm->phases;    // phase parameters
	ctrl      = jr->ctrl;           // control parameters
	dt        = jr->ts->dt;         // time step
	fssa      = ctrl->FSSA;         // density gradient penalty parameter
	grav      = ctrl->grav;         // gravity acceleration
	pShift    = ctrl->pShift;       // pressure shift
	biot      = ctrl->biot;         // Biot pressure parameter
	AirPhase  = jr->surf->AirPhase; // sticky air phase number
	avg_topo  = jr->surf->avg_topo; // average surface topography

	// clear local residual vectors
	ierr = VecZeroEntries(jr->lfx); CHKERRQ(ierr);
	ierr = VecZeroEntries(jr->lfy); CHKERRQ(ierr);
	ierr = VecZeroEntries(jr->lfz); CHKERRQ(ierr);
	ierr = VecZeroEntries(jr->gc);  CHKERRQ(ierr);

	// access work vectors
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->gc,   &gc);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->lp,   &p);   CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->lT,   &T);   CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->ldxx, &dxx); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->ldyy, &dyy); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->ldzz, &dzz); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_XY,  jr->ldxy, &dxy); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_XZ,  jr->ldxz, &dxz); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_YZ,  jr->ldyz, &dyz); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_X,   jr->lfx,  &fx);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y,   jr->lfy,  &fy);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Z,   jr->lfz,  &fz);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_X,   jr->lvx,  &vx);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y,   jr->lvy,  &vy);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Z,   jr->lvz,  &vz);  CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->lp_lithos, &p_lithos); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->lp_pore,   &p_pore);   CHKERRQ(ierr);

	// compute lithostatic pressure
	ierr = JacResGetLithoStaticPressure(jr); CHKERRQ(ierr);

	//-------------------------------
	// central points
	//-------------------------------
	iter = 0;
	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svCell = &jr->svCell[iter++];
		svDev  = &svCell->svDev;
		svBulk = &svCell->svBulk;

		//=================
		// SECOND INVARIANT
		//=================

		// access strain rates
		XX = dxx[k][j][i];
		YY = dyy[k][j][i];
		ZZ = dzz[k][j][i];

		// x-y plane, i-j indices
		XY1 = dxy[k][j][i];
		XY2 = dxy[k][j+1][i];
		XY3 = dxy[k][j][i+1];
		XY4 = dxy[k][j+1][i+1];

		// x-z plane, i-k indices
		XZ1 = dxz[k][j][i];
		XZ2 = dxz[k+1][j][i];
		XZ3 = dxz[k][j][i+1];
		XZ4 = dxz[k+1][j][i+1];

		// y-z plane, j-k indices
		YZ1 = dyz[k][j][i];
		YZ2 = dyz[k+1][j][i];
		YZ3 = dyz[k][j+1][i];
		YZ4 = dyz[k+1][j+1][i];

		// compute second invariant
		J2Inv = 0.5*(XX*XX + YY*YY + ZZ*ZZ) +
		0.25*(XY1*XY1 + XY2*XY2 + XY3*XY3 + XY4*XY4) +
		0.25*(XZ1*XZ1 + XZ2*XZ2 + XZ3*XZ3 + XZ4*XZ4) +
		0.25*(YZ1*YZ1 + YZ2*YZ2 + YZ3*YZ3 + YZ4*YZ4);

		// store square root of second invariant
		svDev->DII = sqrt(J2Inv);

		//=======================
		// CONSTITUTIVE EQUATIONS
		//=======================

		// access current pressure
		pc = p[k][j][i];

		// current temperature
		Tc = T[k][j][i];

		// access current lithostatic pressure
		pc_lithos = p_lithos[k][j][i];

		// access current pore pressure (zero if deactivated)
		pc_pore = p_pore[k][j][i];

		// compute depth below the free surface
		if(AirPhase != -1) depth = avg_topo - COORD_CELL(k, sz, fs->dsz);
		else               depth = 0.0;
		if(depth < 0.0)    depth = 0.0;

		// evaluate deviatoric constitutive equations
		ierr = DevConstEq(svDev, &eta_creep, &eta_vp, numPhases, phases, svCell->phRat, ctrl, pc_lithos, pc_pore, dt, pc-pShift, Tc); CHKERRQ(ierr);

		// store creep viscosity
		svCell->eta_creep = eta_creep;
		svCell->eta_vp    = eta_vp;

		// compute stress, plastic strain rate and shear heating term on cell
		ierr = GetStressCell(svCell, ctrl, XX, YY, ZZ); CHKERRQ(ierr);

		// get total pressure (effective pressure, computed by LaMEM, plus pore pressure)
		ptotal = pc + biot*pc_pore;

		// compute total Cauchy stresses
		sxx = svCell->sxx - ptotal;
		syy = svCell->syy - ptotal;
		szz = svCell->szz - ptotal;

		// evaluate volumetric constitutive equations
		ierr = VolConstEq(svBulk, numPhases, phases, svCell->phRat, ctrl, depth, dt, pc-pShift , Tc); CHKERRQ(ierr);

		// access
		theta = svBulk->theta; // volumetric strain rate
		rho   = svBulk->rho;   // effective density
		IKdt  = svBulk->IKdt;  // inverse bulk viscosity
//		alpha = svBulk->alpha; // effective thermal expansion
		pn    = svBulk->pn;    // pressure history
//		Tn    = svBulk->Tn;    // temperature history

		// compute gravity terms
		gx = rho*grav[0];
		gy = rho*grav[1];
		gz = rho*grav[2];

		// compute stabilization terms (lumped approximation)
		tx = -fssa*dt*gx;
		ty = -fssa*dt*gy;
		tz = -fssa*dt*gz;

		//=========
		// RESIDUAL
		//=========

		// get mesh steps for the backward and forward derivatives
		bdx = SIZE_NODE(i, sx, fs->dsx);   fdx = SIZE_NODE(i+1, sx, fs->dsx);
		bdy = SIZE_NODE(j, sy, fs->dsy);   fdy = SIZE_NODE(j+1, sy, fs->dsy);
		bdz = SIZE_NODE(k, sz, fs->dsz);   fdz = SIZE_NODE(k+1, sz, fs->dsz);

		// momentum
		fx[k][j][i] -= (sxx + vx[k][j][i]*tx)/bdx + gx/2.0;   fx[k][j][i+1] += (sxx + vx[k][j][i+1]*tx)/fdx - gx/2.0;
		fy[k][j][i] -= (syy + vy[k][j][i]*ty)/bdy + gy/2.0;   fy[k][j+1][i] += (syy + vy[k][j+1][i]*ty)/fdy - gy/2.0;
		fz[k][j][i] -= (szz + vz[k][j][i]*tz)/bdz + gz/2.0;   fz[k+1][j][i] += (szz + vz[k+1][j][i]*tz)/fdz - gz/2.0;

//****************************************
// ADHOC (HARD-CODED PRESSURE CONSTRAINTS)
//****************************************

//		if(k == 0)   fz[k][j][i]   += -p[k-1][j][i]/bdz;
//		if(k == mcz) fz[k+1][j][i] -= -p[k+1][j][i]/fdz;

		// mass - currently T-dependency is deactivated
//		gc[k][j][i] = -IKdt*(pc - pn) - theta + alpha*(Tc - Tn)/dt;
        
        gc[k][j][i] = -IKdt*(pc - pn) - theta ;
        
	}
	END_STD_LOOP

	//-------------------------------
	// xy edge points
	//-------------------------------
	iter = 0;
	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svEdge = &jr->svXYEdge[iter++];
		svDev  = &svEdge->svDev;

		//=================
		// SECOND INVARIANT
		//=================

		// check index bounds
		I1 = i;   if(I1 == mx) I1--;
		I2 = i-1; if(I2 == -1) I2++;
		J1 = j;   if(J1 == my) J1--;
		J2 = j-1; if(J2 == -1) J2++;

		// access strain rates
		XY = dxy[k][j][i];

		// x-y plane, i-j indices (i & j - bounded)
		XX1 = dxx[k][J1][I1];
		XX2 = dxx[k][J1][I2];
		XX3 = dxx[k][J2][I1];
		XX4 = dxx[k][J2][I2];

		// x-y plane, i-j indices (i & j - bounded)
		YY1 = dyy[k][J1][I1];
		YY2 = dyy[k][J1][I2];
		YY3 = dyy[k][J2][I1];
		YY4 = dyy[k][J2][I2];

		// x-y plane, i-j indices (i & j - bounded)
		ZZ1 = dzz[k][J1][I1];
		ZZ2 = dzz[k][J1][I2];
		ZZ3 = dzz[k][J2][I1];
		ZZ4 = dzz[k][J2][I2];

		// y-z plane j-k indices (j - bounded)
		XZ1 = dxz[k][J1][i];
		XZ2 = dxz[k+1][J1][i];
		XZ3 = dxz[k][J2][i];
		XZ4 = dxz[k+1][J2][i];

		// x-z plane i-k indices (i - bounded)
		YZ1 = dyz[k][j][I1];
		YZ2 = dyz[k+1][j][I1];
		YZ3 = dyz[k][j][I2];
		YZ4 = dyz[k+1][j][I2];

		// compute second invariant
		J2Inv = XY*XY +
		0.125*(XX1*XX1 + XX2*XX2 + XX3*XX3 + XX4*XX4) +
		0.125*(YY1*YY1 + YY2*YY2 + YY3*YY3 + YY4*YY4) +
		0.125*(ZZ1*ZZ1 + ZZ2*ZZ2 + ZZ3*ZZ3 + ZZ4*ZZ4) +
		0.25 *(XZ1*XZ1 + XZ2*XZ2 + XZ3*XZ3 + XZ4*XZ4) +
		0.25 *(YZ1*YZ1 + YZ2*YZ2 + YZ3*YZ3 + YZ4*YZ4);

		// store square root of second invariant
		svDev->DII = sqrt(J2Inv);

		//=======================
		// CONSTITUTIVE EQUATIONS
		//=======================

		// access current pressure (x-y plane, i-j indices)
		pc = 0.25*(p[k][j][i] + p[k][j][i-1] + p[k][j-1][i] + p[k][j-1][i-1]);

		// current temperature (x-y plane, i-j indices)
		Tc = 0.25*(T[k][j][i] + T[k][j][i-1] + T[k][j-1][i] + T[k][j-1][i-1]);

		// access current lithostatic pressure (x-y plane, i-j indices)
		pc_lithos = 0.25*(p_lithos[k][j][i] + p_lithos[k][j][i-1] + p_lithos[k][j-1][i] + p_lithos[k][j-1][i-1]);

		// access current pore pressure (x-y plane, i-j indices)
		pc_pore = 0.25*(p_pore[k][j][i] + p_pore[k][j][i-1] + p_pore[k][j-1][i] + p_pore[k][j-1][i-1]);

		// evaluate deviatoric constitutive equations
		ierr = DevConstEq(svDev, &eta_creep, &eta_vp, numPhases, phases, svEdge->phRat, ctrl, pc_lithos, pc_pore, dt, pc-pShift, Tc); CHKERRQ(ierr);

		// compute stress, plastic strain rate and shear heating term on edge
		ierr = GetStressEdge(svEdge, ctrl, XY); CHKERRQ(ierr);

		// access xy component of the Cauchy stress
		sxy = svEdge->s;

		//=========
		// RESIDUAL
		//=========

		// get mesh steps for the backward and forward derivatives
		bdx = SIZE_CELL(i-1, sx, fs->dsx);   fdx = SIZE_CELL(i, sx, fs->dsx);
		bdy = SIZE_CELL(j-1, sy, fs->dsy);   fdy = SIZE_CELL(j, sy, fs->dsy);

		// momentum
		fx[k][j-1][i] -= sxy/bdy;   fx[k][j][i] += sxy/fdy;
		fy[k][j][i-1] -= sxy/bdx;   fy[k][j][i] += sxy/fdx;

	}
	END_STD_LOOP

	//-------------------------------
	// xz edge points
	//-------------------------------
	iter = 0;
	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_NODE_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svEdge = &jr->svXZEdge[iter++];
		svDev  = &svEdge->svDev;

		//=================
		// SECOND INVARIANT
		//=================

		// check index bounds
		I1 = i;   if(I1 == mx) I1--;
		I2 = i-1; if(I2 == -1) I2++;
		K1 = k;   if(K1 == mz) K1--;
		K2 = k-1; if(K2 == -1) K2++;

		// access strain rates
		XZ = dxz[k][j][i];

		// x-z plane, i-k indices (i & k - bounded)
		XX1 = dxx[K1][j][I1];
		XX2 = dxx[K1][j][I2];
		XX3 = dxx[K2][j][I1];
		XX4 = dxx[K2][j][I2];

		// x-z plane, i-k indices (i & k - bounded)
		YY1 = dyy[K1][j][I1];
		YY2 = dyy[K1][j][I2];
		YY3 = dyy[K2][j][I1];
		YY4 = dyy[K2][j][I2];

		// x-z plane, i-k indices (i & k - bounded)
		ZZ1 = dzz[K1][j][I1];
		ZZ2 = dzz[K1][j][I2];
		ZZ3 = dzz[K2][j][I1];
		ZZ4 = dzz[K2][j][I2];

		// y-z plane, j-k indices (k - bounded)
		XY1 = dxy[K1][j][i];
		XY2 = dxy[K1][j+1][i];
		XY3 = dxy[K2][j][i];
		XY4 = dxy[K2][j+1][i];

		// xy plane, i-j indices (i - bounded)
		YZ1 = dyz[k][j][I1];
		YZ2 = dyz[k][j+1][I1];
		YZ3 = dyz[k][j][I2];
		YZ4 = dyz[k][j+1][I2];

		// compute second invariant
		J2Inv = XZ*XZ +
		0.125*(XX1*XX1 + XX2*XX2 + XX3*XX3 + XX4*XX4) +
		0.125*(YY1*YY1 + YY2*YY2 + YY3*YY3 + YY4*YY4) +
		0.125*(ZZ1*ZZ1 + ZZ2*ZZ2 + ZZ3*ZZ3 + ZZ4*ZZ4) +
		0.25 *(XY1*XY1 + XY2*XY2 + XY3*XY3 + XY4*XY4) +
		0.25 *(YZ1*YZ1 + YZ2*YZ2 + YZ3*YZ3 + YZ4*YZ4);

		// store square root of second invariant
		svDev->DII = sqrt(J2Inv);

		//=======================
		// CONSTITUTIVE EQUATIONS
		//=======================

		// access current pressure (x-z plane, i-k indices)
		pc = 0.25*(p[k][j][i] + p[k][j][i-1] + p[k-1][j][i] + p[k-1][j][i-1]);

		// current temperature (x-z plane, i-k indices)
		Tc = 0.25*(T[k][j][i] + T[k][j][i-1] + T[k-1][j][i] + T[k-1][j][i-1]);

		// access current lithostatic pressure (x-z plane, i-k indices)
		pc_lithos = 0.25*(p_lithos[k][j][i] + p_lithos[k][j][i-1] + p_lithos[k-1][j][i] + p_lithos[k-1][j][i-1]);

		// access current pore pressure (x-z plane, i-k indices)
		pc_pore = 0.25*(p_pore[k][j][i] + p_pore[k][j][i-1] + p_pore[k-1][j][i] + p_pore[k-1][j][i-1]);

		// evaluate deviatoric constitutive equations
		ierr = DevConstEq(svDev, &eta_creep, &eta_vp, numPhases, phases, svEdge->phRat, ctrl, pc_lithos, pc_pore, dt, pc-pShift, Tc); CHKERRQ(ierr);

		// compute stress, plastic strain rate and shear heating term on edge
		ierr = GetStressEdge(svEdge, ctrl, XZ); CHKERRQ(ierr);

		// access xz component of the Cauchy stress
		sxz = svEdge->s;

		//=========
		// RESIDUAL
		//=========

		// get mesh steps for the backward and forward derivatives
		bdx = SIZE_CELL(i-1, sx, fs->dsx);   fdx = SIZE_CELL(i, sx, fs->dsx);
		bdz = SIZE_CELL(k-1, sz, fs->dsz);   fdz = SIZE_CELL(k, sz, fs->dsz);

		// momentum
		fx[k-1][j][i] -= sxz/bdz;   fx[k][j][i] += sxz/fdz;
		fz[k][j][i-1] -= sxz/bdx;   fz[k][j][i] += sxz/fdx;

	}
	END_STD_LOOP

	//-------------------------------
	// yz edge points
	//-------------------------------
	iter = 0;
	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_NODE_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// access solution variables
		svEdge = &jr->svYZEdge[iter++];
		svDev  = &svEdge->svDev;

		//=================
		// SECOND INVARIANT
		//=================

		// check index bounds
		J1 = j;   if(J1 == my) J1--;
		J2 = j-1; if(J2 == -1) J2++;
		K1 = k;   if(K1 == mz) K1--;
		K2 = k-1; if(K2 == -1) K2++;

		// access strain rates
		YZ = dyz[k][j][i];

		// y-z plane, j-k indices (j & k - bounded)
		XX1 = dxx[K1][J1][i];
		XX2 = dxx[K1][J2][i];
		XX3 = dxx[K2][J1][i];
		XX4 = dxx[K2][J2][i];

		// y-z plane, j-k indices (j & k - bounded)
		YY1 = dyy[K1][J1][i];
		YY2 = dyy[K1][J2][i];
		YY3 = dyy[K2][J1][i];
		YY4 = dyy[K2][J2][i];

		// y-z plane, j-k indices (j & k - bounded)
		ZZ1 = dzz[K1][J1][i];
		ZZ2 = dzz[K1][J2][i];
		ZZ3 = dzz[K2][J1][i];
		ZZ4 = dzz[K2][J2][i];

		// x-z plane, i-k indices (k -bounded)
		XY1 = dxy[K1][j][i];
		XY2 = dxy[K1][j][i+1];
		XY3 = dxy[K2][j][i];
		XY4 = dxy[K2][j][i+1];

		// x-y plane, i-j indices (j - bounded)
		XZ1 = dxz[k][J1][i];
		XZ2 = dxz[k][J1][i+1];
		XZ3 = dxz[k][J2][i];
		XZ4 = dxz[k][J2][i+1];

		// compute second invariant
		J2Inv = YZ*YZ +
		0.125*(XX1*XX1 + XX2*XX2 + XX3*XX3 + XX4*XX4) +
		0.125*(YY1*YY1 + YY2*YY2 + YY3*YY3 + YY4*YY4) +
		0.125*(ZZ1*ZZ1 + ZZ2*ZZ2 + ZZ3*ZZ3 + ZZ4*ZZ4) +
		0.25 *(XY1*XY1 + XY2*XY2 + XY3*XY3 + XY4*XY4) +
		0.25 *(XZ1*XZ1 + XZ2*XZ2 + XZ3*XZ3 + XZ4*XZ4);

		// store square root of second invariant
		svDev->DII = sqrt(J2Inv);

		//=======================
		// CONSTITUTIVE EQUATIONS
		//=======================

		// access current pressure (y-z plane, j-k indices)
		pc = 0.25*(p[k][j][i] + p[k][j-1][i] + p[k-1][j][i] + p[k-1][j-1][i]);

		// current temperature (y-z plane, j-k indices)
		Tc = 0.25*(T[k][j][i] + T[k][j-1][i] + T[k-1][j][i] + T[k-1][j-1][i]);

		// access current lithostatic pressure (y-z plane, j-k indices)
		pc_lithos = 0.25*(p_lithos[k][j][i] + p_lithos[k][j-1][i] + p_lithos[k-1][j][i] + p_lithos[k-1][j-1][i]);

		// access current pore pressure (y-z plane, j-k indices)
		pc_pore = 0.25*(p_pore[k][j][i] + p_pore[k][j-1][i] + p_pore[k-1][j][i] + p_pore[k-1][j-1][i]);

		// evaluate deviatoric constitutive equations
		ierr = DevConstEq(svDev, &eta_creep, &eta_vp, numPhases, phases, svEdge->phRat, ctrl, pc_lithos, pc_pore, dt, pc-pShift, Tc); CHKERRQ(ierr);

		// compute stress, plastic strain rate and shear heating term on edge
		ierr = GetStressEdge(svEdge, ctrl, YZ); CHKERRQ(ierr);

		// access yz component of the Cauchy stress
		syz = svEdge->s;

		//=========
		// RESIDUAL
		//=========

		// get mesh steps for the backward and forward derivatives
		bdy = SIZE_CELL(j-1, sy, fs->dsy);   fdy = SIZE_CELL(j, sy, fs->dsy);
		bdz = SIZE_CELL(k-1, sz, fs->dsz);   fdz = SIZE_CELL(k, sz, fs->dsz);

		// update momentum residuals
		fy[k-1][j][i] -= syz/bdz;   fy[k][j][i] += syz/fdz;
		fz[k][j-1][i] -= syz/bdy;   fz[k][j][i] += syz/fdy;

	}
	END_STD_LOOP

	// restore vectors
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->gc,   &gc);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->lp,   &p);   CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->lT,   &T);   CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->ldxx, &dxx); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->ldyy, &dyy); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->ldzz, &dzz); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_XY,  jr->ldxy, &dxy); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_XZ,  jr->ldxz, &dxz); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_YZ,  jr->ldyz, &dyz); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_X,   jr->lfx,  &fx);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y,   jr->lfy,  &fy);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Z,   jr->lfz,  &fz);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_X,   jr->lvx,  &vx);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y,   jr->lvy,  &vy);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Z,   jr->lvz,  &vz);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->lp_lithos, &p_lithos); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->lp_pore,   &p_pore);   CHKERRQ(ierr);

	// assemble global residuals from local contributions
	LOCAL_TO_GLOBAL(fs->DA_X, jr->lfx, jr->gfx)
	LOCAL_TO_GLOBAL(fs->DA_Y, jr->lfy, jr->gfy)
	LOCAL_TO_GLOBAL(fs->DA_Z, jr->lfz, jr->gfz)

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResCopySol"
PetscErrorCode JacResCopySol(JacRes *jr, Vec x)
{
	// copy solution from global to local vectors, enforce boundary constraints

	PetscErrorCode ierr;
	PetscFunctionBegin;

	ierr = JacResCopyVel (jr, x); CHKERRQ(ierr);

	ierr = JacResCopyPres(jr, x); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResCopyVel"
PetscErrorCode JacResCopyVel(JacRes *jr, Vec x)
{
	// copy velocity from global to local vectors, enforce boundary constraints

	FDSTAG           *fs;
	BCCtx            *bc;
	PetscInt          mcx, mcy, mcz;
	PetscInt          I, J, K, fi, fj, fk;
	PetscInt          i, j, k, nx, ny, nz, sx, sy, sz;
	PetscScalar       ***bcvx,  ***bcvy,  ***bcvz;
	PetscScalar       ***lvx, ***lvy, ***lvz;
	PetscScalar       *vx, *vy, *vz, pmdof;
	const PetscScalar *sol, *iter;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs  =  jr->fs;
	bc  =  jr->bc;

	// initialize maximal index in all directions
	mcx = fs->dsx.tcels - 1;
	mcy = fs->dsy.tcels - 1;
	mcz = fs->dsz.tcels - 1;

	// access vectors
	ierr = VecGetArray    (jr->gvx, &vx);  CHKERRQ(ierr);
	ierr = VecGetArray    (jr->gvy, &vy);  CHKERRQ(ierr);
	ierr = VecGetArray    (jr->gvz, &vz);  CHKERRQ(ierr);
	ierr = VecGetArrayRead(x,       &sol); CHKERRQ(ierr);

	// copy vectors component-wise
	iter = sol;

	ierr  = PetscMemcpy(vx, iter, (size_t)fs->nXFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nXFace;

	ierr  = PetscMemcpy(vy, iter, (size_t)fs->nYFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nYFace;

	ierr  = PetscMemcpy(vz, iter, (size_t)fs->nZFace*sizeof(PetscScalar)); CHKERRQ(ierr);

	// restore access
	ierr = VecRestoreArray    (jr->gvx, &vx);  CHKERRQ(ierr);
	ierr = VecRestoreArray    (jr->gvy, &vy);  CHKERRQ(ierr);
	ierr = VecRestoreArray    (jr->gvz, &vz);  CHKERRQ(ierr);
	ierr = VecRestoreArrayRead(x,       &sol); CHKERRQ(ierr);

	// fill local (ghosted) version of solution vectors
	GLOBAL_TO_LOCAL(fs->DA_X,   jr->gvx, jr->lvx)
	GLOBAL_TO_LOCAL(fs->DA_Y,   jr->gvy, jr->lvy)
	GLOBAL_TO_LOCAL(fs->DA_Z,   jr->gvz, jr->lvz)

	// access local solution vectors
	ierr = DMDAVecGetArray(fs->DA_X,   jr->lvx, &lvx); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y,   jr->lvy, &lvy); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Z,   jr->lvz, &lvz); CHKERRQ(ierr);

	// access boundary constraints vectors
	ierr = DMDAVecGetArray(fs->DA_X,   bc->bcvx, &bcvx); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y,   bc->bcvy, &bcvy); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Z,   bc->bcvz, &bcvz); CHKERRQ(ierr);

	//==============================
	// enforce two-point constraints
	//==============================

	//---------
	// X points
	//---------

	GET_NODE_RANGE_GHOST_INT(nx, sx, fs->dsx)
	GET_CELL_RANGE_GHOST_INT(ny, sy, fs->dsy)
	GET_CELL_RANGE_GHOST_INT(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		pmdof = lvx[k][j][i];

		J = j; fj = 0;
		K = k; fk = 0;

		if(j == 0)   { fj = 1; J = j-1; SET_TPC(bcvx, lvx, k, J, i, pmdof) }
		if(j == mcy) { fj = 1; J = j+1; SET_TPC(bcvx, lvx, k, J, i, pmdof) }
		if(k == 0)   { fk = 1; K = k-1; SET_TPC(bcvx, lvx, K, j, i, pmdof) }
		if(k == mcz) { fk = 1; K = k+1; SET_TPC(bcvx, lvx, K, j, i, pmdof) }

		if(fj*fk) SET_EDGE_CORNER(n, lvx, K, J, i, k, j, i, pmdof)
	}
	END_STD_LOOP

	//---------
	// Y points
	//---------
	GET_CELL_RANGE_GHOST_INT(nx, sx, fs->dsx)
	GET_NODE_RANGE_GHOST_INT(ny, sy, fs->dsy)
	GET_CELL_RANGE_GHOST_INT(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		pmdof = lvy[k][j][i];

		I = i; fi = 0;
		K = k; fk = 0;

		if(i == 0)   { fi = 1; I = i-1; SET_TPC(bcvy, lvy, k, j, I, pmdof) }
		if(i == mcx) { fi = 1; I = i+1; SET_TPC(bcvy, lvy, k, j, I, pmdof) }
		if(k == 0)   { fk = 1; K = k-1; SET_TPC(bcvy, lvy, K, j, i, pmdof) }
		if(k == mcz) { fk = 1; K = k+1; SET_TPC(bcvy, lvy, K, j, i, pmdof) }

		if(fi*fk) SET_EDGE_CORNER(n, lvy, K, j, I, k, j, i, pmdof)
	}
	END_STD_LOOP


	//---------
	// Z points
	//---------
	GET_CELL_RANGE_GHOST_INT(nx, sx, fs->dsx)
	GET_CELL_RANGE_GHOST_INT(ny, sy, fs->dsy)
	GET_NODE_RANGE_GHOST_INT(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		pmdof = lvz[k][j][i];

		I = i; fi = 0;
		J = j; fj = 0;

		if(i == 0)   { fi = 1; I = i-1; SET_TPC(bcvz, lvz, k, j, I, pmdof) }
		if(i == mcx) { fi = 1; I = i+1; SET_TPC(bcvz, lvz, k, j, I, pmdof) }
		if(j == 0)   { fj = 1; J = j-1; SET_TPC(bcvz, lvz, k, J, i, pmdof) }
		if(j == mcy) { fj = 1; J = j+1; SET_TPC(bcvz, lvz, k, J, i, pmdof) }

		if(fi*fj) SET_EDGE_CORNER(n, lvz, k, J, I, k, j, i, pmdof)
	}
	END_STD_LOOP

	// restore access
	ierr = DMDAVecRestoreArray(fs->DA_X,   jr->lvx,  &lvx);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y,   jr->lvy,  &lvy);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Z,   jr->lvz,  &lvz);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_X,   bc->bcvx, &bcvx); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y,   bc->bcvy, &bcvy); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Z,   bc->bcvz, &bcvz); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResCopyPres"
PetscErrorCode JacResCopyPres(JacRes *jr, Vec x)
{
	// copy pressure from global to local vectors, enforce boundary constraints

	FDSTAG            *fs;
	BCCtx             *bc;
	PetscInt          mcx, mcy, mcz;
	PetscInt          I, J, K, fi, fj, fk;
	PetscInt          i, j, k, nx, ny, nz, sx, sy, sz;
	PetscScalar       ***bcp;
	PetscScalar       ***lp;
	PetscScalar       *p, pmdof;
	const PetscScalar *sol, *iter;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs  =  jr->fs;
	bc  =  jr->bc;

	// initialize maximal index in all directions
	mcx = fs->dsx.tcels - 1;
	mcy = fs->dsy.tcels - 1;
	mcz = fs->dsz.tcels - 1;

	// access vectors
	ierr = VecGetArray    (jr->gp, &p);   CHKERRQ(ierr);
	ierr = VecGetArrayRead(x,      &sol); CHKERRQ(ierr);

	// copy vectors component-wise
	iter = sol + fs->nXFace + fs->nYFace + fs->nZFace;

	ierr = PetscMemcpy(p, iter, (size_t)fs->nCells*sizeof(PetscScalar)); CHKERRQ(ierr);

	// restore access
	ierr = VecRestoreArray    (jr->gp, &p);   CHKERRQ(ierr);
	ierr = VecRestoreArrayRead(x,      &sol); CHKERRQ(ierr);

	// fill local (ghosted) version of solution vectors
	GLOBAL_TO_LOCAL(fs->DA_CEN, jr->gp, jr->lp)

	// access local solution vectors
	ierr = DMDAVecGetArray(fs->DA_CEN, jr->lp, &lp);  CHKERRQ(ierr);

	// access boundary constraints vectors
	ierr = DMDAVecGetArray(fs->DA_CEN, bc->bcp, &bcp); CHKERRQ(ierr);

	//==============================
	// enforce two-point constraints
	//==============================

	//--------------------------
	// central points (pressure)
	//--------------------------
	GET_CELL_RANGE_GHOST_INT(nx, sx, fs->dsx)
	GET_CELL_RANGE_GHOST_INT(ny, sy, fs->dsy)
	GET_CELL_RANGE_GHOST_INT(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		pmdof = lp[k][j][i];

		I = i; fi = 0;
		J = j; fj = 0;
		K = k; fk = 0;

		if(i == 0)   { fi = 1; I = i-1; SET_TPC(bcp, lp, k, j, I, pmdof) }
		if(i == mcx) { fi = 1; I = i+1; SET_TPC(bcp, lp, k, j, I, pmdof) }
		if(j == 0)   { fj = 1; J = j-1; SET_TPC(bcp, lp, k, J, i, pmdof) }
		if(j == mcy) { fj = 1; J = j+1; SET_TPC(bcp, lp, k, J, i, pmdof) }
		if(k == 0)   { fk = 1; K = k-1; SET_TPC(bcp, lp, K, j, i, pmdof) }
		if(k == mcz) { fk = 1; K = k+1; SET_TPC(bcp, lp, K, j, i, pmdof) }

		if(fi*fj)    SET_EDGE_CORNER(n, lp, k, J, I, k, j, i, pmdof)
		if(fi*fk)    SET_EDGE_CORNER(n, lp, K, j, I, k, j, i, pmdof)
		if(fj*fk)    SET_EDGE_CORNER(n, lp, K, J, i, k, j, i, pmdof)
		if(fi*fj*fk) SET_EDGE_CORNER(n, lp, K, J, I, k, j, i, pmdof)
	}
	END_STD_LOOP

	// restore access
	ierr = DMDAVecRestoreArray(fs->DA_CEN, jr->lp,  &lp);  CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, bc->bcp, &bcp); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResCopyRes"
PetscErrorCode JacResCopyRes(JacRes *jr, Vec f)
{
	// copy residuals from local to global vectors, enforce boundary constraints

	FDSTAG      *fs;
	BCCtx       *bc;
	PetscInt    i, num, *list;
	PetscScalar *fx, *fy, *fz, *c, *res, *iter;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs  = jr->fs;
	bc  = jr->bc;

	// access vectors
	ierr = VecGetArray(jr->gfx, &fx); CHKERRQ(ierr);
	ierr = VecGetArray(jr->gfy, &fy); CHKERRQ(ierr);
	ierr = VecGetArray(jr->gfz, &fz); CHKERRQ(ierr);
	ierr = VecGetArray(jr->gc,  &c);  CHKERRQ(ierr);
	ierr = VecGetArray(f, &res);      CHKERRQ(ierr);

	// copy vectors component-wise
	iter = res;

	ierr  = PetscMemcpy(iter, fx, (size_t)fs->nXFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nXFace;

	ierr  = PetscMemcpy(iter, fy, (size_t)fs->nYFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nYFace;

	ierr  = PetscMemcpy(iter, fz, (size_t)fs->nZFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nZFace;

	ierr  = PetscMemcpy(iter, c,  (size_t)fs->nCells*sizeof(PetscScalar)); CHKERRQ(ierr);

	// zero out constrained residuals (velocity)
	num   = bc->vNumSPC;
	list  = bc->vSPCList;

	for(i = 0; i < num; i++) res[list[i]] = 0.0;

	// zero out constrained residuals (pressure)
	num   = bc->pNumSPC;
	list  = bc->pSPCList;

	for(i = 0; i < num; i++) res[list[i]] = 0.0;

	// restore access
	ierr = VecRestoreArray(jr->gfx,  &fx); CHKERRQ(ierr);
	ierr = VecRestoreArray(jr->gfy,  &fy); CHKERRQ(ierr);
	ierr = VecRestoreArray(jr->gfz,  &fz); CHKERRQ(ierr);
	ierr = VecRestoreArray(jr->gc,   &c);  CHKERRQ(ierr);
	ierr = VecRestoreArray(f, &res);       CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResCopyMomentumRes"
PetscErrorCode JacResCopyMomentumRes(JacRes *jr, Vec f)
{
	// copy momentum residuals from global to local vectors for output

	FDSTAG      *fs;
	PetscScalar *fx, *fy, *fz, *res, *iter;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs  = jr->fs;

	// access vectors
	ierr = VecGetArray(jr->gfx, &fx); CHKERRQ(ierr);
	ierr = VecGetArray(jr->gfy, &fy); CHKERRQ(ierr);
	ierr = VecGetArray(jr->gfz, &fz); CHKERRQ(ierr);
	ierr = VecGetArray(f, &res);      CHKERRQ(ierr);

	// copy vectors component-wise
	iter = res;

	ierr  = PetscMemcpy(fx, iter, (size_t)fs->nXFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nXFace;

	ierr  = PetscMemcpy(fy, iter, (size_t)fs->nYFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nYFace;

	ierr  = PetscMemcpy(fz, iter, (size_t)fs->nZFace*sizeof(PetscScalar)); CHKERRQ(ierr);
	iter += fs->nZFace;

	// restore access
	ierr = VecRestoreArray(jr->gfx,  &fx); CHKERRQ(ierr);
	ierr = VecRestoreArray(jr->gfy,  &fy); CHKERRQ(ierr);
	ierr = VecRestoreArray(jr->gfz,  &fz); CHKERRQ(ierr);
	ierr = VecRestoreArray(f, &res);       CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResCopyContinuityRes"
PetscErrorCode JacResCopyContinuityRes(JacRes *jr, Vec f)
{
	// copy continuity residuals from global to local vectors for output

	FDSTAG      *fs;
	PetscScalar *c, *res, *iter;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs  = jr->fs;

	// access vectors
	ierr = VecGetArray(jr->gc,  &c);  CHKERRQ(ierr);
	ierr = VecGetArray(f, &res);      CHKERRQ(ierr);

	// copy vectors component-wise
	iter = res + fs->dof.lnv;

	ierr = PetscMemcpy(c,  iter, (size_t)fs->nCells*sizeof(PetscScalar)); CHKERRQ(ierr);

	// restore access
	ierr = VecRestoreArray(jr->gc,   &c);  CHKERRQ(ierr);
	ierr = VecRestoreArray(f, &res);       CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResViewRes"
PetscErrorCode JacResViewRes(JacRes *jr)
{
	// show assembled residual with boundary constraints
	// WARNING! rewrite this function using coupled residual vector directly

	PetscScalar dmin, dmax, d2, e2, fx, fy, fz, f2, div_tol;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	// get constrained residual vectors
	ierr = JacResCopyMomentumRes  (jr, jr->gres); CHKERRQ(ierr);
	ierr = JacResCopyContinuityRes(jr, jr->gres); CHKERRQ(ierr);

	// compute norms
	ierr = VecMin (jr->gc,  NULL,   &dmin); CHKERRQ(ierr);
	ierr = VecMax (jr->gc,  NULL,   &dmax); CHKERRQ(ierr);
	ierr = VecNorm(jr->gc,  NORM_2, &d2);   CHKERRQ(ierr);

	ierr = VecNorm(jr->gfx, NORM_2, &fx);   CHKERRQ(ierr);
	ierr = VecNorm(jr->gfy, NORM_2, &fy);   CHKERRQ(ierr);
	ierr = VecNorm(jr->gfz, NORM_2, &fz);   CHKERRQ(ierr);

	f2 = sqrt(fx*fx + fy*fy + fz*fz);

	if(jr->ctrl->actTemp)
	{
		ierr = JacResGetTempRes(jr);         CHKERRQ(ierr);
		ierr = VecNorm(jr->ge, NORM_2, &e2); CHKERRQ(ierr);
	}

	// print
	PetscPrintf(PETSC_COMM_WORLD, "------------------------------------------\n");
	PetscPrintf(PETSC_COMM_WORLD, "Residual summary: \n");
	PetscPrintf(PETSC_COMM_WORLD, "  Continuity: \n");
	PetscPrintf(PETSC_COMM_WORLD, "    Div_min  = %12.12e \n", dmin);
	PetscPrintf(PETSC_COMM_WORLD, "    Div_max  = %12.12e \n", dmax);
	PetscPrintf(PETSC_COMM_WORLD, "    |Div|_2  = %12.12e \n", d2);
	PetscPrintf(PETSC_COMM_WORLD, "  Momentum: \n" );
	PetscPrintf(PETSC_COMM_WORLD, "    |mRes|_2 = %12.12e \n", f2);

	if(jr->ctrl->actTemp)
	{
		PetscPrintf(PETSC_COMM_WORLD, "  Energy: \n" );
		PetscPrintf(PETSC_COMM_WORLD, "    |eRes|_2 = %12.12e \n", e2);
	}

	PetscPrintf(PETSC_COMM_WORLD, "------------------------------------------\n");

	// stop if divergence more than tolerance
	div_tol = 0.0;
	ierr = PetscOptionsGetScalar(NULL, NULL, "-div_tol",  &div_tol,  NULL); CHKERRQ(ierr);

	if ((div_tol) && (( dmax > div_tol ) || (f2 > div_tol)))
	{
		SETERRQ(PETSC_COMM_SELF, PETSC_ERR_USER, " *** Emergency stop! Maximum divergence or momentum residual is too large; solver did not converge! *** \n");
	}

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
PetscScalar JacResGetTime(JacRes *jr)
{
	return	jr->ts->time*jr->scal->time;
}
//---------------------------------------------------------------------------
PetscInt JacResGetStep(JacRes *jr)
{
	return	jr->ts->istep;
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "JacResGetCourantStep"
PetscErrorCode JacResGetCourantStep(JacRes *jr)
{
	//-------------------------------------
	// compute length of the next time step
	//-------------------------------------
/*
	FDSTAG      *fs;
	TSSol       *ts;
	PetscScalar dt, lidtmax, gidtmax;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	fs =  jr->fs;
	ts =  jr->ts;

	lidtmax = 0.0;

	// determine maximum local inverse time step
	ierr = getMaxInvStep1DLocal(&fs->dsx, fs->DA_X, jr->gvx, 0, &lidtmax); CHKERRQ(ierr);
	ierr = getMaxInvStep1DLocal(&fs->dsy, fs->DA_Y, jr->gvy, 1, &lidtmax); CHKERRQ(ierr);
	ierr = getMaxInvStep1DLocal(&fs->dsz, fs->DA_Z, jr->gvz, 2, &lidtmax); CHKERRQ(ierr);

	// synchronize
	if(ISParallel(PETSC_COMM_WORLD))
	{
		ierr = MPI_Allreduce(&lidtmax, &gidtmax, 1, MPIU_SCALAR, MPI_MAX, PETSC_COMM_WORLD); CHKERRQ(ierr);
	}
	else
	{
		gidtmax = lidtmax;
	}


	ACHTUNG!!!

	// compute time step
	gidtmax /= ts->Cmax;
    
    dt = (ts->dt)*1.1;                          // slightly increase timestep
    if (dt > 1.0/gidtmax)   dt = 1.0/gidtmax;   // if dt larger than dt_courant, use courant
    if (dt > ts->dtmax)     dt = ts->dtmax;     // if dt larger than maximum dt use maximum dt
    
	// store new time step
	ts->pdt = ts->dt;
	ts->dt  = dt;
*/

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
/*
#undef __FUNCT__
#define __FUNCT__ "getMaxInvStep1DLocal"
PetscErrorCode getMaxInvStep1DLocal(Discret1D *ds, DM da, Vec gv, PetscInt dir, PetscScalar *_idtmax)
{
	PetscScalar v, h, vmax, idt, idtmax;
	PetscInt    i, j, k, nx, ny, nz, sx, sy, sz, idx, ijk[3], jj, ln;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	// initialize
	idtmax = (*_idtmax);

	if(!ds->uniform)
	{
		// compute time step on variable spacing grid
		PetscScalar ***va;

		ierr = DMDAGetCorners(da, &sx, &sy, &sz, &nx, &ny, &nz); CHKERRQ(ierr);
		ierr = DMDAVecGetArray(da, gv, &va);                     CHKERRQ(ierr);

		START_STD_LOOP
		{
			// get velocity
			v = va[k][j][i];

			// prepare node index buffer
			ijk[0] = i-sx;
			ijk[1] = j-sy;
			ijk[2] = k-sz;

			// anisotropic direction-dependent criterion
			if(v >= 0.0)  idx = ijk[dir];
			else          idx = ijk[dir]-1;

			// get mesh step
			h = ds->ncoor[idx+1] - ds->ncoor[idx];

			// get inverse time step (safe to compute)
			idt = v/h;

			// update maximum inverse time step
			if(idt > idtmax) idtmax = idt;
		}
		END_STD_LOOP

		ierr = DMDAVecRestoreArray(da, gv, &va); CHKERRQ(ierr);
	}
	else
	{
		// compute time step on uniform spacing grid
		PetscScalar *va;

		// get maximum local velocity
		ierr = VecGetLocalSize(gv, &ln); CHKERRQ(ierr);
		ierr = VecGetArray(gv, &va);     CHKERRQ(ierr);

		vmax = 0.0;
		for(jj = 0; jj < ln; jj++) { v = PetscAbsScalar(va[jj]); if(v > vmax) vmax = v;	}

		ierr = VecRestoreArray(gv, &va); CHKERRQ(ierr);

		// get uniform mesh step
		h = (ds->crdend - ds->crdbeg)/(PetscScalar)ds->tcels;

		// get inverse time step
		idt = vmax/h;

		// update maximum inverse time step
		if(idt > idtmax) idtmax = idt;
	}

	// return result
	(*_idtmax) = idtmax;

	PetscFunctionReturn(0);
}
*/
//---------------------------------------------------------------------------
