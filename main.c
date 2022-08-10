#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <mpi.h>
#include "functions.h"
#include "cuda.h"


int main(int argc, char *argv[]) {
	
	//general variables
	MPI_Status status;
	int my_rank;
	int num_procs;
	int finish; //will send if with tag STOP to stop sending/reciving data
	int count = 0; //will use for count pair of picture\object
	int totalCount = 0; //will contain all "count" after reducing
	
	//for measuring time
	double startTime;
	double finishTime;
	
	double matchingValue; //matching value --> we will use it to calc abs sum
	int numOfPictures; //number of picture in file
	int numOfObjects; //number of objects in file
	int pictureToSend; //picture to send for each process
	
	
	Element** ranksObjects; //will contain objects that process was recived
	Element** ranksPicture; //will contain pictures that process was recived
	Element* pictures; //for reading pictures
	Element** objects; //for reading objects

	Pair* foundObject; //for saving results
	Pair* finalResult; //final resault array that will use to print the data of pair picture/object
	
	//MPI INIT
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
	
	if(my_rank == ROOT) //PROCESS 0
	{
	
		//starting timer
		startTime = MPI_Wtime();
		
		//reading from file
		pictures = readPicturesFromFile(FILE_NAME,&matchingValue,&numOfPictures);
		objects =  readObjectsFromFile(FILE_NAME,&numOfObjects);
	
		//calc how many pictures to send to each process
		if(num_procs < numOfPictures)
			pictureToSend = numOfPictures/num_procs;
		else
			pictureToSend = 1;

		//SEND TO OTHER PROCESS
		sendDataToSlaves(num_procs,numOfPictures, matchingValue, numOfObjects, pictures, objects, pictureToSend);
		
	}
	
	else // OTHER PROCESSES
	{	
		MPI_Recv(&finish,0,MPI_INT,ROOT,MPI_ANY_TAG,MPI_COMM_WORLD,&status); //checks if the tag is WORK or STOP for knowing when to stop reciving data
		
		if(status.MPI_TAG == WORK){
			recvDataFromMaster(num_procs,&ranksPicture,&ranksObjects,&numOfObjects,&numOfPictures, &matchingValue, &pictureToSend); // reciving data
		
			//for saving data when object was found
			foundObject = (Pair*)malloc(pictureToSend*sizeof(Pair));
			if (foundObject == NULL) {
				printf("Problem to allocate memory\n");
				exit(0);
			}
				
			if(my_rank % 2 == 0)
			{
				for(int i=0; i < pictureToSend; i++)
				{
					findPair(ranksPicture[i], ranksObjects, matchingValue,&foundObject[i],&count,numOfObjects); //checks one picture with all objects if there is a pair

					MPI_Send(&foundObject[i].pictureID ,1 ,MPI_INT,ROOT,0,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].objectID ,1 ,MPI_INT,ROOT,1,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].i,1 ,MPI_INT,ROOT,2,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].j,1 ,MPI_INT,ROOT,3,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].isPair,1 ,MPI_INT,ROOT,4,MPI_COMM_WORLD);
						
				}
			}
			else
			{
				for(int i=0; i < pictureToSend; i++)
				{
					findPairOnGPU(ranksPicture[i], ranksObjects, matchingValue,&foundObject[i],&count,numOfObjects); //checks one picture with all objects if there is a pair

					MPI_Send(&foundObject[i].pictureID ,1 ,MPI_INT,ROOT,0,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].objectID ,1 ,MPI_INT,ROOT,1,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].i,1 ,MPI_INT,ROOT,2,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].j,1 ,MPI_INT,ROOT,3,MPI_COMM_WORLD);
					MPI_Send(&foundObject[i].isPair,1 ,MPI_INT,ROOT,4,MPI_COMM_WORLD);
						
				}	
			}
			
			//free all allocations
			free(ranksObjects);
			free(ranksPicture);
			free(foundObject);
		}	
							
		}	
	
	
	MPI_Reduce(&count, &totalCount, 1, MPI_INT, MPI_SUM, ROOT,MPI_COMM_WORLD); // all procceses do it to count how many pictures was checked
	
	if(my_rank == ROOT) // PROCESS 0
	{
		
		//for saving results ---> now for procces 0.
		foundObject = (Pair*)malloc(numOfPictures*sizeof(Pair));
		if (foundObject == NULL){
				printf("Problem to allocate memory\n");
				exit(0);
		}
		
		//final result array that will use to print the data of pair picture/object
		finalResult = (Pair*)malloc(numOfPictures*sizeof(Pair));
		if (finalResult == NULL) {
			printf("Problem to allocate memory\n");
			exit(0);
		}
			
		for(int i=0; i < totalCount; i++) //recving data of found pairs/not pairs --> pay attintion for diffrent tags
		{
			MPI_Recv(&finalResult[i].pictureID,1,MPI_INT,MPI_ANY_SOURCE,0,MPI_COMM_WORLD,&status);
			MPI_Recv(&finalResult[i].objectID,1 ,MPI_INT,MPI_ANY_SOURCE,1,MPI_COMM_WORLD,&status);	
			MPI_Recv(&finalResult[i].i,1 ,MPI_INT,MPI_ANY_SOURCE,2,MPI_COMM_WORLD,&status);
			MPI_Recv(&finalResult[i].j,1 ,MPI_INT,MPI_ANY_SOURCE,3,MPI_COMM_WORLD,&status);
			MPI_Recv(&finalResult[i].isPair,1 ,MPI_INT,MPI_ANY_SOURCE,4,MPI_COMM_WORLD,&status);
			
		}	
			
		int rest = pictureToSend*(num_procs-1); //if there is picture that didnt checked --> prosses 0 will check them
		//printf("rest is %d\n",rest);
		if(rest < numOfPictures)
		{
			for(int i=rest; i < numOfPictures; i++) // rank 0 will do the rest pictures
			{				
				findPair(&pictures[i], objects, matchingValue,&foundObject[i],&count,numOfObjects);
				
				finalResult[totalCount].pictureID = foundObject[i].pictureID;
				finalResult[totalCount].objectID = foundObject[i].objectID;
				finalResult[totalCount].i = foundObject[i].i;
				finalResult[totalCount].j = foundObject[i].j;
				finalResult[totalCount].isPair = foundObject[i].isPair;
				totalCount++; //for knowing how many data to print							
			}
		}
		
		//prints result to output.txt
		printResult(totalCount, finalResult, pictures);
		
		//shutting down timer
		finishTime = MPI_Wtime();
		printf("Execution Time: %f\n", finishTime - startTime);
		
		//free all allocations
		free(pictures);
		free(objects);
		free(finalResult);
		free(foundObject);
		
	}
	
	MPI_Finalize();
	return 0;
}
