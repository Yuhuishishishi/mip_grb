#include "solver.h"
#include "gurobi_c.h"
#include "data_structure.h"
#include "json_read.h"
#include <stdlib.h>
#include <stdio.h>

#define usevehicle(v)			v
#define assigntest(t,v)			numVehicles + numTests*(t) + v
#define start(t)				numVehicles + numTests*numVehicles + t
#define tardiness(t)			numVehicles + numTests*numVehicles + numTests + t
#define before(t1,t2)			numVehicles + numTests*numVehicles + numTests + numTests + numTests*t1 + t2

#define MAX_STR_LEN				256

static 
void buildModel(
	int				numTests,
	int				numVehicles,
	TEST*			testArr,
	VEHICLE*		vehicleArr,
	int**			rehits,
	GRBenv**			env,
	GRBmodel**		model)
{
	GRBloadenv(env, "mipmodel.log");

	int totalNumVar = numVehicles + numTests*numVehicles + numTests + numTests + numTests*numTests;

	GRBnewmodel(*env, model, "tp3s_mip", totalNumVar,
				NULL, NULL, NULL, NULL, NULL);

	/* initialize variables for use vehicle variables */
	for (int v=0; v<numVehicles; ++v)
	{
		int col = usevehicle(v);
		GRBsetcharattrelement(*model, "VType", col, GRB_BINARY);
		GRBsetdblattrelement(*model, "Obj", col, 50.0);
		char vname[MAX_STR_LEN];
		sprintf(vname, "Use vehicle %i", v);
		GRBsetstrattrelement(*model, "VarName", col, vname);
	}

	/* initialize variales for test-vehicle assignment */
	for (int t=0; t<numTests; ++t)
	{
		for (int v=0; v<numVehicles; ++v)
		{
			int col = assigntest(t,v);
			GRBsetcharattrelement(*model, "VType", col, GRB_BINARY);
			GRBsetdblattrelement(*model, "Obj", col, 0);
			char vname[MAX_STR_LEN];
			sprintf(vname, "Assign test %i to vehicle %i", t, v);
			GRBsetstrattrelement(*model, "VarName", col, vname);
		}
	}

	/* initialize variables for starting times of tests */
	for (int t=0; t<numTests; ++t)
	{
		int col = start(t);
		GRBsetcharattrelement(*model, "VType", col, GRB_CONTINUOUS);
		GRBsetdblattrelement(*model, "Obj", col, 0);
		GRBsetdblattrelement(*model, "LB", col, 0);
		char vname[MAX_STR_LEN];
		sprintf(vname, "Start time %i", t);
		GRBsetstrattrelement(*model, "VarName", col, vname);		
	}	

	/* initialize variables for tardiness of tests */
	for (int t=0; t<numTests; ++t)
	{
		int col = start(t);
		GRBsetcharattrelement(*model, "VType", col, GRB_CONTINUOUS);
		GRBsetdblattrelement(*model, "Obj", col, 1);
		GRBsetdblattrelement(*model, "LB", col, 0);
		char vname[MAX_STR_LEN];
		sprintf(vname, "Tardiness %i", t);
		GRBsetstrattrelement(*model, "VarName", col, vname);		
	}

	/* initialize variables for before indicators */
	for (int t1=0; t1<numTests; ++t1)
	{
		for (int t2=0; t2<numTests; ++t2)
		{
			if (t1==t2)
				continue;

			int col = before(t1,t2);
			GRBsetcharattrelement(*model, "VType", col, GRB_BINARY);
			GRBsetdblattrelement(*model, "Obj", col, 0);
			char vname[MAX_STR_LEN];
			sprintf(vname, "%i before %i", t1,t2);
			GRBsetstrattrelement(*model, "VarName", col, vname);	
		}
	}

	/* model sense */
	GRBsetintattr(*model, "ModelSense", 1);

	/* lazy update */
	GRBupdatemodel(*model);


	int *ind, *val;
	char sense;
	double rhs;
	char cname[MAX_STR_LEN];
	int rwcnt = numVehicles > numTests ? numVehicles : numTests;
	int nonzero;

	ind = malloc(sizeof(int) * rwcnt * rwcnt);
	val = malloc(sizeof(double) * rwcnt * rwcnt);


	/* for each test, it needs to be assigned to exactly one vehicle */
	for (int t=0; t<numTests; ++t)
	{
		nonzero = 0;

		sense = GRB_EQAUL;
		for (int v = 0; v < numVehicles; ++v)
		{
			ind[nonzero] = assigntest(t,v);
			val[nonzero++] = 1.0;
		}
		rhs = 1.0;
		sprintf(cname, "Cover test %i", t);
		GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);
	}

	/* for  each vehicle, if it is not used, 
		then no tests can be assigned to it */
	for (int v = 0; v < numVehicles; ++v)
	{
		nonzero = 0;
		sense = GRB_LESS_EQUAL;
		rhs = 0.0;
		sprintf(cname, "Use vehicle %i", v);
		for (int t = 0; t < numTests; ++t)
		{
			ind[nonzero] = assigntest(t,v);
			val[nonzero++] = 1.0;
		}
		ind[nonzero] = usevehicle(v);
		val[nonzero++] = -1.0;
		GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);
	}

	/* for each test, start after it is released */
	for (int t = 0; t < numTests; ++t)
	{
		nonzero = 0;
		sense = GRB_GREATER_EQUAL;
		rhs = testArr[t].release;
		sprintf(cname, "start after release %i", t);
		ind[nonzero] = start(t);
		val[nonzero++] = 1.0;
		GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);

	}

	/* for each test, start after the vehicle release */
	for (int t = 0; t < numTests; ++t)
	{
		nonzero = 0;
		sense = GRB_GREATER_EQUAL;
		rhs = 0;
		sprintf(cname, "start after vehicle release %i", t);
		ind[nonzero] = start(t);
		val[nonzero++] = 1.0;
		for (int v = 0; v < numVehicles; ++v)
		{
			ind[nonzero] = usevehicle(v);
			val[nonzero++] = -1.0*vehicleArr[v].release;
		}
		GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);
	}

	/* for each test, end before deadline or there will be tardiness */
	for (int t = 0; t < numTests; ++t)
	{
		nonzero = 0;
		sense = GRB_LESS_EQUAL;
		rhs = testArr[t].deadline - testArr[t].dur;
		sprintf(cname, "tardiness %i", t);
		ind[nonzero] = start(t);
		val[nonzero++] = 1.0;
		ind[nonzero] = tardiness(t);
		val[nonzero++] = -1.0;
		GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);		
	}

	/* for each pair of tests,
		either t1 before t2 or t2 before t1 */
	for (int t1 = 0; t1 < numTests; ++t1)
	{
		for (int t2 = 0; t2 < t1; ++t2)
		{
			nonzero = 0;
			sense = GRB_LESS_EQUAL;
			rhs = 1.0;
			sprintf(cname, "either before %i %i", t1, t2);
			ind[nonzero] = before(t1,t2);
			val[nonzero++] = 1.0;
			ind[nonzero] = before(t2,t1);
			val[nonzero++] = 1.0;
			GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);		
		}
	}

	/* for each pair of tests, if they are assigned to 
		same vehicle, then an order needs to be decided
		*/
	for (int t1 = 0; t1 < numTests; ++t1)
	{
		for (int t2 = 0; t2 < t1; ++t2)
		{
			nonzero = 0;
			sense = GRB_LESS_EQUAL;
			rhs = 1.0;
			sprintf(cname, "same vehicle %i %i", t1,t2);
			ind[nonzero] = assigntest(t1);
			val[nonzero++] = 1.0;
			ind[nonzero] = assigntest(t2);
			val[nonzero++] = 1.0;
			ind[nonzero] = before(t1,t2);
			val[nonzero++] = -1.0;
			ind[nonzero] = before(t2,t1);
			val[nonzero++] = -1.0;
			GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);		
		}
	}

	const double bigM = 500.0;
	/* for each pair of tests, ends before */
	for (int t1 = 0; t1 < numTests; ++t1)
	{
		for (int t2 = 0; t2 < t1; ++t2)
		{
			nonzero = 0;
			sense = GRB_LESS_EQUAL;
			rhs = bigM - testArr[t1].dur;
			sprintf(cname, "%i before %i", t1, t2);
			ind[nonzero] = start(t1);
			val[nonzero++] = 1.0;
			ind[nonzero] = start(t2);
			val[nonzero++] = -1.0;
			ind[nonzero] = before(t1,t2);
			val[nonzero++] = bigM;

			GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);		

			/* the other way */
			nonzero = 0;
			sense = GRB_LESS_EQUAL;
			rhs = bigM - testArr[t2].dur;
			sprintf(cname, "%i before %i", t2, t1);
			ind[nonzero] = start(t2);
			val[nonzero++] = 1.0;
			ind[nonzero] = start(t1);
			val[nonzero++] = -1.0;
			ind[nonzero] = before(t2,t1);
			val[nonzero++] = bigM;

			GRBaddconstr(*model, nonzero, ind, val, sense, rhs, cname);		
		}
	}

	/* solve the model */
	

	free(ind);
	free(val);

}

void solve(const char* filepath)
{
	/* read in data */
	int numTests, numVehicles;
	TEST* testArr;
	VEHICLE* vehicleArr;
	int** rehitRules;

	numTests = get_tests_size(filepath);
	numVehicles = get_vehicle_size(filepath);

	printf("num tests: %d\n", numTests);
	printf("num vehicles: %d\n", numVehicles);	

	testArr = malloc(numTests * sizeof(TEST));
	vehicleArr = malloc(numVehicles * sizeof(VEHICLE));
	rehitRules = malloc(numTests * sizeof(int*));
	for (int i = 0; i < numTests; ++i)
	{
		rehitRules[i] = malloc(numTests * sizeof(int));
	}

	GRBenv* env;
	GRBmodel* model;

	buildModel(numTests, numVehicles, testArr, vehicleArr, rehitRules, &env, &model);


	/* clear the pointers */

	free(testArr);
	free(vehicleArr);
	for (int i = 0; i < numTests; ++i)
	{
		free(rehitRules[i]);
	}
	free(rehitRules);

	GRBfreemodel(model);
	GRBfreeenv(env);
}
