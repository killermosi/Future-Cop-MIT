#include "../../Utilities/Grid2D.h"
#include "Grid2D.h"

int main() {
    // This checks if the grid 2d base functions.
    int problem = test_Grid2DBase<Utilities::Grid2D<int>>();
    
    return problem;
}