#include<iostream>
#include "dmatrix.h"
#include "io/files.h"

int main(int argc, char *argv[])
{
	// validate command line arguments
	DMOptions dm;
	if( 0>dmOptions(argc,argv,&dm) )
	{
		return -1;
	}

	int err=0;
	float *dmatrix=NULL;
	IOFile *features=NULL;
	IOFile *labels=NULL;

	try
	{
		// open the features file and prepare for reading
		features=FileOpen(dm.fpath.c_str(),dm.fname.c_str(),IOSource);
		if( NULL==features )
		{
			throw std::string("cannot identify the features file format");
		}
		
		// open the labels file and prepare for reading
		labels=FileOpen(dm.lpath.c_str(),dm.lname.c_str(),IOSource);
		if( NULL==labels )
		{
			throw std::string("cannot identify the labels file format");
		}

		if( features->getRows()!=labels->getRows() )
		{
			throw std::string("number of features and labels elements mismatch");
		}

		if( 1!=labels->getCols() )
		{
			throw std::string("multi-dimensional labels are not supported");
		}

		// create and initialize the features cuda array and free host resources
		DMCudaArray *dm_features=dmCudaInitArray(features);

		delete features;
		features=NULL;

		// create and initialize the labels cuda array and free host resources
		DMCudaArray *dm_labels=dmCudaInitArray(labels);

		delete labels;
		labels=NULL;

		// create and initialize the indexes cuda array
		DMCudaArray *dm_indexes=dmCudaSortArray(dm_labels,dm.bsize);

		// count unique labels (identities)
		DMCudaArray *dm_count=dmCudaCountKeys(dm_labels);

		// release labels array
		dmCudaFree(dm_labels);

		// generate distance matrix
		std::cout << "here-1?\n";

		DMCudaArray *dm_matrix=dmCudaDistanceMatrix(dm_features,dm_indexes,dm_count,dm.bsize);

		std::cout << "here-2?\n";

		// release cuda arrays
		dmCudaFree(dm_features);

		std::cout << "here-3?\n";

		dmCudaFree(dm_indexes);


		std::cout << "here-4?\n";

		dmCudaFree(dm_count);

		std::cout << "here-5?\n";

		dmCudaFree(dm_matrix);
	}
	catch( std::string& error )
	{
		std::cerr << "(EE) " << argv[0] << ": " << error << std::endl;
		err=-1;
	}
	catch( ... )
	{
		std::cerr << "(EE) " << argv[0] << ": " << "unexpected exception during data conversion" << std::endl;
		err=-1;
	}

	if( NULL!=dmatrix )
	{
		delete dmatrix;
	}

	if( NULL!=features )
	{
		delete features;
	}

	if( NULL!=labels )
	{
		delete labels;
	}

	return err;
}