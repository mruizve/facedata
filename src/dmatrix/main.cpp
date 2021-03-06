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

		// validate data records
		if( features->getRows()!=labels->getRows() )
		{
			throw std::string("number of features and labels elements mismatch");
		}

		if( 1!=labels->getCols() )
		{
			throw std::string("multi-dimensional labels are not supported");
		}

		// create and initialize the features cuda array and free host resources
		DMArray *dm_features=dmInitArray(features);

		delete features;
		features=NULL;

		// create and initialize the labels cuda array and free host resources
		DMArray *dm_labels=dmInitArray(labels);

		delete labels;
		labels=NULL;

		// create and initialize the indexes cuda array
		DMArray *dm_indexes=dmSortArray(dm_labels,dm.bsize);

		// count unique labels (identities)
		std::vector<int> count=dmCountUniqueKeys(dm_labels);

		#ifdef DEBUGGING
			std::cout << "count:" << std::endl;
			for( size_t i=0; count.size()>i; i++ )
			{
				std::cout << count.at(i) << ", ";
			}
			std::cout << "size=" << count.size() << std::endl << std::endl;
		#endif

		// release labels array
		dmFree(dm_labels);

		// compute indexes offsets
		std::vector<int> offsets;
		offsets.push_back(0);
		for( size_t i=0; count.size()>i; i++ )
		{
			offsets.push_back(offsets.back()+count.at(i));
		}

		#ifdef DEBUGGING
			std::cout << "offsets:" << std::endl;
			for( size_t i=0; offsets.size()>i; i++ )
			{
				std::cout << offsets[i] << ", ";
			}
			std::cout << "size=" << offsets.size() << std::endl << std::endl;
		#endif

		// compute distance matrix
		cv::Mat matrix=dmDistanceMatrix(dm_features,dm_indexes,offsets,dm.bsize);

		// release cuda arrays
		dmFree(dm_features);
		dmFree(dm_indexes);

		// export the distances matrices
		std::vector<cv::Mat> channels;
		cv::split(matrix,channels);
		cv::FileStorage file;
		file.open("dmatrix.xml",cv::FileStorage::WRITE);
		file << "mean" << channels[0];
		file << "var" << channels[1];
		file << "max" << channels[2];
		file << "min" << channels[3];
		file.release();

		#ifdef DEBUGGING
			std::cout << std::endl << "distance matrix:" << std::endl;
			std::cout << channels[0] << std::endl << std::endl;
			std::cout << channels[1] << std::endl << std::endl;
			std::cout << channels[2] << std::endl << std::endl;
			std::cout << channels[3] << std::endl << std::endl;
		#endif

		// convert to images
		// (it is assumed that min equals zero)
		double min,max,scale;
		cv::minMaxLoc(channels[0],&min,&max); scale=255.0/(max-min);
		cv::convertScaleAbs(channels[0],channels[0],scale,-scale*min);
		cv::imwrite("dmatrix-mean.png",channels[0]);

		cv::minMaxLoc(channels[1],&min,&max); scale=255.0/(max-min);
		cv::convertScaleAbs(channels[1],channels[1],scale,-scale*min);
		cv::imwrite("dmatrix-var.png",channels[1]);

		cv::minMaxLoc(channels[2],&min,&max); scale=255.0/(max-min);
		cv::convertScaleAbs(channels[2],channels[2],scale,-scale*min);
		cv::imwrite("dmatrix-max.png",channels[2]);

		cv::minMaxLoc(channels[3],&min,&max); scale=255.0/(max-min);
		cv::convertScaleAbs(channels[3],channels[3],scale,-scale*min);
		cv::imwrite("dmatrix-min.png",channels[3]);
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
