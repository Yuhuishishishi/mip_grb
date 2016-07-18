#include "solver.h"
#include "gurobi_c.h"
#include "data_structure.h"
#include "json_read.h"


#define usevehicle(v)			v
#define assigntest(t,v)			numVehicles + numTests*(t) + v
#define start(t)				numVehicles + numTests*numVehicles + t
#define before(t1,t2)			numVehicles + numTests*numVehicles + numTests + numTests*t1 + t2


static 
void buildModel(
	int				numTests,
	int				numVehicles,
	TEST*			testArr,
	VEHICLE*		vehicleArr,
	int**			rehits,
	GRBenv*			env,
	GRBmodel*		model)
{

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

	testArr = malloc(numTests * sizeof(TEST));
	vehicleArr = malloc(numVehicles * sizeof(VEHICLE));
	rehitRules = malloc(numTests * sizeof(int*));
	for (int i = 0; i < numTests; ++i)
	{
		rehitRules[i] = malloc(numTests * sizeof(int));
	}

	GRBenv* env;
	GRBmodel* model;



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
