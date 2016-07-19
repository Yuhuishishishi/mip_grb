#include "solver.h"
#include "gurobi_c.h"
#include "data_structure.h"
#include "json_read.h"
#include <stdlib.h>
#include <stdio.h>

#define usevehicle(v)			v
#define assigntest(t,v)			numVehicles + numTests*(t) + v
#define start(t)				numVehicles + numTests*numVehicles + t
#define before(t1,t2)			numVehicles + numTests*numVehicles + numTests + numTests*t1 + t2

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

	int totalNumVar = numVehicles + numTests*numVehicles + numTests + numTests*numTests;

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
